/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file Mpeg.cpp
 * @brief MPEG encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Mpeg.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include <opensand_conf/uti_debug.h>
#include <opensand_conf/ConfigurationFile.h>
#include <vector>
#include <map>

#define PACKING_THRESHOLD "packing_threshold"
#define MPEG_SECTION "mpeg"
#define CONF_MPEG_FILE "/etc/opensand/plugins/mpeg.conf"


Mpeg::Mpeg():
	EncapPlugin(NET_PROTO_MPEG)
{
	this->upper[TRANSPARENT].push_back("ULE");
	this->upper[REGENERATIVE].push_back("AAL5/ATM");
	this->upper[REGENERATIVE].push_back("ULE");
}


Mpeg::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin), encap_contexts(), desencap_contexts()
{
	const char *FUNCNAME = "[Mpeg::Context::Context]";
	ConfigurationFile config;

	if(config.loadConfig(CONF_MPEG_FILE) < 0)
	{
		UTI_ERROR("%s failed to load config file '%s'",
		          FUNCNAME, CONF_MPEG_FILE);
		goto error;
	}
	// Retrieving the packing threshold
	if(!config.getValue(MPEG_SECTION,
	                    PACKING_THRESHOLD, this->packing_threshold))
	{
		UTI_ERROR("%s missing %s parameter\n", FUNCNAME, PACKING_THRESHOLD);
		goto unload;
	}
	UTI_DEBUG("%s packing thershold: %lu\n", FUNCNAME, this->packing_threshold);


unload:
	config.unloadConfig();
error:
	return;
}

Mpeg::Context::~Context()
{
	std::map < int, MpegEncapCtx * >::iterator encap_it;
	std::map < int, MpegDeencapCtx * >::iterator desencap_it;

	for(encap_it = this->encap_contexts.begin();
		 encap_it != this->encap_contexts.end(); encap_it++)
	{
		if((*encap_it).second != NULL)
			delete(*encap_it).second;
	}

	for(desencap_it = this->desencap_contexts.begin();
	    desencap_it != this->desencap_contexts.end();
	    desencap_it++)
	{
		if((*desencap_it).second != NULL)
			delete(*desencap_it).second;
	}
}

NetBurst *Mpeg::Context::encapsulate(NetBurst *burst,
                                     map<long, int> &time_contexts)
{
	const char *FUNCNAME = "[Mpeg::Context::encapsulate]";
	NetBurst *mpeg_packets = NULL;
	NetBurst::iterator packet;

	// create an empty burst of MPEG packets
	mpeg_packets = new NetBurst();
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of MPEG packets\n",
		          FUNCNAME);
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		int context_id;
		long time = 0;

		if(!this->encapMpeg(*packet, mpeg_packets, context_id, time))
		{
			UTI_ERROR("%s MPEG encapsulation failed, drop packet\n", FUNCNAME);
			continue;
		}
		time_contexts.insert(make_pair(time, context_id));
	}
	// delete the burst and all packets in it
	delete burst;

	return mpeg_packets;
}


NetBurst *Mpeg::Context::deencapsulate(NetBurst *burst)
{
	const char *FUNCNAME = "[Mpeg::Context::deencapsulate]";
	NetBurst *net_packets;

	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		delete burst;
		return false;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		uint8_t dst_tal_id;

		// packet must be valid
		if(*packet == NULL)
		{
			UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
			          FUNCNAME);
			continue;
		}

		// packet must be a MPEG packet
		if((*packet)->getType() != this->getEtherType())
		{
			UTI_ERROR("%s encapsulation packet is not a MPEG packet "
			          "(type = 0x%04x), drop the packet\n",
			          FUNCNAME, (*packet)->getType());
			continue;
		}

		// Filter if packet is for this ST
		dst_tal_id = (*packet)->getDstTalId();
		if((dst_tal_id != this->dst_tal_id)
			&& (dst_tal_id != BROADCAST_TAL_ID))
		{
			UTI_DEBUG("%s encapsulation packet is for ST#%u. Drop\n",
			          FUNCNAME, dst_tal_id);
			continue;
		}

		if(!this->deencapMpeg(*packet, net_packets))
		{
			UTI_ERROR("%s cannot create a burst of packets, drop packet\n", FUNCNAME);
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;

}

bool Mpeg::Context::encapMpeg(NetPacket *packet,
                              NetBurst *mpeg_packets,
                              int &context_id,
                              long &time)
{
	const char *FUNCNAME = "Mpeg::Context::encapMpeg";
	MpegPacket *mpeg_packet;
	uint16_t pid;
	MpegEncapCtx *context;

	unsigned int packet_len;
	unsigned int packet_off;
	Data packet_data;
	unsigned int length;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	time = 0;

	if((packet->getSrcTalId() & 0x1F) != packet->getSrcTalId())
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 0x1F"
		          " this can not stand in 5 bits of PID field of MPEG2-TS"
		          " packets!!!\n");
	}
	if((packet->getDstTalId() & 0x1F) != packet->getDstTalId())
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 0x1F"
		          " this can not stand in 5 bits of PID field of MPEG2-TS"
		          " packets!!!\n");
	}
	if((packet->getQos() & 0x07) != packet->getQos())
	{
		UTI_ERROR("Be careful, you have set a QoS priority greater than 0x07"
		          " this can not stand in 3 bits of PID field of MPEG2-TS"
		          " packets!!!\n");
	}

	pid = MpegPacket::getPidFromPacket(packet);

	// find the encapsulation context for the network packet
	UTI_DEBUG("%s network packet belongs to the encapsulation context "
	          "identified by PID = %u\n", FUNCNAME, pid);
	context = this->find_encap_context(pid, dest_spot);
	if(context == NULL)
		goto drop;

	// return a reference to the encapsulation context to encapsulation bloc
	context_id = context->pid();

	UTI_DEBUG("%s encapsulation context contains %u bytes of data\n",
	          FUNCNAME, context->length());

	// build MPEG packets
	UTI_DEBUG("%s Synchonisation Byte = 0x%02x\n", FUNCNAME,
	          context->sync());

	// set PUSI bit to 1 only if not already set. If not set, insert a
	// Payload Pointer just after the header
	if(!context->pusi())
	{
		UTI_DEBUG("%s PUSI (%d) not set, set PUSI = 1 and add Payload "
		          "Pointer (packet length = %u)\n", FUNCNAME,
		          context->pusi(), context->length());

		// set the PUSI bit
		context->setPusi();
		UTI_DEBUG("%s PUSI is now set to %d\n", FUNCNAME, context->pusi());

		// add the Payload Pointer field
		context->addPP();
		UTI_DEBUG("%s packet is now %u byte length\n",
		          FUNCNAME, context->length());
	}

	packet_data = packet->getData();
	packet_len = packet_data.length();
	packet_off = 0;

	while(packet_len > 0)
	{
		length = MIN(packet_len, context->left());
		context->add(&packet_data, packet_off, length);

		UTI_DEBUG("%s copy %u bytes of SNDU data into MPEG payload (SNDU data "
		          "= %u bytes, unused payload = %u bytes)\n", FUNCNAME, length,
		          packet_len, context->left());

		packet_len -= length;
		packet_off += length;

		if(context->left() == 0) // left() is unsigned and therefore cannot be false
		{
			// MPEG2-TS frame is full, add the frame to the list and build another
			// frame with remaining SNDU data
			mpeg_packet = new MpegPacket(*(context->frame()));

			if(mpeg_packet != NULL)
			{
				UTI_DEBUG("%s one MPEG packet created\n", FUNCNAME);
				// set the destination spot ID
				mpeg_packet->setDstSpot(dest_spot);

				mpeg_packets->add(mpeg_packet);
			}
			else
			{
				UTI_ERROR("%s failed to create MPEG packet\n", FUNCNAME);
			}

			// clear the encapsulation context
			context->reset();
		}
	}

	UTI_DEBUG("%s SNDU packet now entirely packed into MPEG packets\n",
	          FUNCNAME);
	UTI_DEBUG("%s unused space in MPEG payload = %u bytes\n",
	          FUNCNAME, context->left());

	// SNDU packet is now entirely packed, check for unused payload at the end
	// of the MPEG2-TS frame. Perhaps can we later pack another SNDU packet
	// in the MPEG2-TS frame

	// there is too few space for packing another SNDU packet in the MPEG2-TS
	// frame if:
	//  - there is less than upper packet minimum lentgh byte(s) of unused
	//    payload in the frame
	//  - there is exactly upper packet minimum lentgh byte(s) of unused payload
	//    in the frame and the PUSI bit is not set
	if(this->packing_threshold == 0 ||
	   context->left() < this->current_upper->getMinLength() ||
	   (context->left() == this->current_upper->getMinLength() && !context->pusi()))
	{
		// there is too few unused space in the frame for packing another SNDU
		// packet, add padding (0xff) in the unused payload bytes and add the
		// frame to the list

		UTI_DEBUG("%s too few unused space in the MPEG payload for packing "
		          "=> add padding to packet and send it\n", FUNCNAME);

		// add padding if necessary
		context->padding();

		// add frame to the list...
		mpeg_packet = new MpegPacket(*(context->frame()));

		if(mpeg_packet != NULL)
		{
			UTI_DEBUG("%s one MPEG packet created\n", FUNCNAME);
			// set the destination spot ID
			mpeg_packet->setDstSpot(dest_spot);
			mpeg_packets->add(mpeg_packet);
		}
		else
		{
			UTI_ERROR("%s failed to create MPEG packet\n", FUNCNAME);
		}

		// ... and clear the encapsulation context
		context->reset();
	}
	else
	{
		// there is enough unused payload bytes for packing another SNDU packet
		// in this MPEG2-TS frame, we can wait some time (Packing Threshold)
		// before sending the frame. Keep data in the encapsulation context
		// for further use

		UTI_DEBUG("%s enough unused space in the MPEG payload for packing "
		          "=> keep incomplete MPEG packet during %ld ms\n",
		          FUNCNAME, this->packing_threshold);

		time = this->packing_threshold;
	}

	return true;

drop:
	return false;
}

bool Mpeg::Context::deencapMpeg(NetPacket *packet, NetBurst *net_packets)
{
	const char *FUNCNAME = "[Mpeg::Context::deencapMpeg]";

	NetBurst *tmp_packets = NULL;
	NetBurst::iterator it;
	MpegPacket *mpeg_packet = NULL;
	uint16_t pid;
	std::map < int, MpegDeencapCtx * >::iterator context_it;
	MpegDeencapCtx *context;

	Data payload;
	unsigned int sndu_offset;
	bool pp_used;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	// packet must be a MPEG packet
	if(packet->getType() != NET_PROTO_MPEG)
	{
		UTI_ERROR("%s encapsulation packet is not an MPEG packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// cast from a generic packet to a MPEG packet
	mpeg_packet = new MpegPacket(packet->getData());
	if(mpeg_packet == NULL)
	{
		UTI_ERROR("%s cannot create a MpegPacket from NetPacket\n",
		          FUNCNAME);
		goto error;
	}

	// get the PID number for the MPEG packet to desencapsulate
	pid = mpeg_packet->getPid();
	UTI_DEBUG("%s MPEG packet belongs to the encapsulation context "
	          "identified by PID = %u\n", FUNCNAME, pid);

	// find the desencapsulation context for the MPEG packet
	context_it = this->desencap_contexts.find(pid);
	if(context_it == this->desencap_contexts.end())
	{
		UTI_DEBUG("%s desencapsulation context does not exist yet\n", FUNCNAME);

		MpegDeencapCtx *new_context = new MpegDeencapCtx(pid, dest_spot);
		std::pair < std::map < int, MpegDeencapCtx * >::iterator, bool > infos;
		infos = this->desencap_contexts.insert(std::make_pair(pid, new_context));

		if(!infos.second)
		{
			UTI_ERROR("%s cannot create a new desencapsulation context, "
			          "drop the packet\n", FUNCNAME);
			delete new_context;
			goto error;
		}

		UTI_INFO("%s new desencapsulation context created (PID = %u)\n",
		         FUNCNAME, pid);
		context = (*(infos.first)).second;
	}
	else
	{
		UTI_DEBUG("%s desencapsulation context already exists\n", FUNCNAME);
		context = (*context_it).second;
	}

	UTI_DEBUG("%s desencapsulation context contains %d bytes of data\n",
	          FUNCNAME, context->length());

	// create an empty burst of SNDU packets
	tmp_packets = new NetBurst();
	if(tmp_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of MPEG packets\n",
		          FUNCNAME);
		goto error;
	}

restart:

	UTI_DEBUG("%s MPEG frame has PUSI = %d\n", FUNCNAME,
	          mpeg_packet->pusi() ? 1 : 0);

	// synchronize on PUSI bit if necessary
	if(context->need_pusi())
	{
		UTI_DEBUG("%s PUSI synchronizing is needed\n", FUNCNAME);

		if(mpeg_packet->pusi())
		{
			UTI_DEBUG("%s sync on PUSI with MPEG frame CC = %d\n",
			          FUNCNAME, mpeg_packet->cc());
			// synchronize Continuity Counter
			context->setCc(mpeg_packet->cc());
			// find out the offset of the SNDU in MPEG payload
			sndu_offset = 1 + mpeg_packet->pp();
			// stop synchronizing on PUSI
			context->set_need_pusi(false);
		}
		else
		{
			// PUSI bit not set, drop MPEG frame
			UTI_ERROR("%s sync on PUSI needed, drop MPEG frame CC = %d "
			          "with no PUSI\n", FUNCNAME, mpeg_packet->cc());
			goto drop;
		}
	}
	else
	{
		// if synchronizing on PUSI is not necessary, check Continuity Counters
		// for lost frames

		UTI_DEBUG("%s PUSI synchronizing not needed, check CC\n", FUNCNAME);

		context->incCc();
		if(mpeg_packet->cc() != context->cc())
		{
			// Continuity Counters are different, some MPEG frames were lost

			UTI_ERROR("%s MPEG frame(s) lost (MPEG CC = %d, CTXT CC = %d), reset "
			          "context, sync on PUSI\n", FUNCNAME, mpeg_packet->cc(),
			          context->cc());

			// delete partially desencapsulated SNDUs
			context->reset();
			// ask for PUSI synchronizing
			context->set_need_pusi(true);

			// drop MPEG frame or synchronize CC with it
			if(mpeg_packet->pusi())
			{
				// the current MPEG frame has PUSI bit set, do not drop it,
				// but synchronize CC with it
				UTI_DEBUG("%s PUSI is set in current MPEG frame, "
				          "restart analysis...\n", FUNCNAME);
				goto restart;
			}
			else
			{
				// the current MPEG frame has no PUSI bit set, drop it and
				// synchronize on next MPEG frame with PUSI bit set
				UTI_ERROR("%s PUSI not set in current MPEG frame, drop it\n",
				          FUNCNAME);
				goto drop;
			}
		}
		else
		{
			// Continuity Counters are equal, no MPEG frame was lost

			UTI_DEBUG("%s MPEG frame with CC = %d received\n", FUNCNAME,
			          context->cc());

			if(mpeg_packet->pusi())
			{
				// PUSI bit set, skip Payload Pointer field
				sndu_offset = 1;
			}
			else
			{
				// PUSI bit not set, no Payload Pointer field to skip
				sndu_offset = 0;
			}
		}
	}

	UTI_DEBUG("%s SNDU starts at offset %u in MPEG payload\n",
	          FUNCNAME, sndu_offset);

	// check Payload Pointer validity: the number of packed bytes at the end of
	// the MPEG payload depends on SNDU type
	if(mpeg_packet->pusi() &&
	   mpeg_packet->pp() + 1 > TS_DATASIZE &&
	   (unsigned int)
	   (TS_DATASIZE - 1 - mpeg_packet->pp()) < this->current_upper->getMinLength())
	{
		UTI_ERROR("%s too few bytes (%d < %zu) after Payload Pointer to contain "
		          "a SNDU fragment, reset context, sync on PUSI\n", FUNCNAME,
		          TS_DATASIZE - 1 - mpeg_packet->pp(),
		          this->current_upper->getMinLength());
		context->reset();
		context->set_need_pusi(true);
		// PUSI bit set in current MPEG frame, but PP is not valid, so we cannot
		// synchronize with current frame, drop current frame
		goto drop;
	}

	pp_used = false;
	payload = mpeg_packet->getPayload();

	// desencapsulate SNDUs from the MPEG2-TS frame
	while(TS_DATASIZE - sndu_offset >= this->current_upper->getMinLength())
	{
		// desencapsulate one SNDU from the MPEG2-TS frame

		unsigned int max_len;

		// find out if current SNDU starts at offset specified by the MPEG2-TS
		// Payload Pointer
		pp_used = pp_used || (sndu_offset == (unsigned int) (mpeg_packet->pp() + 1));

		if(context->length() > 0)
		{
			// SNDU partially built, complete with data in current MPEG frame

			UTI_DEBUG("%s context not empty, complete partially built SNDU\n",
			          FUNCNAME);

			// check data length in context
			if(context->length() >= context->sndu_len())
			{
				UTI_ERROR("%s context contains too much data (%d bytes) for "
				          "one %d-byte SNDU, reset context, sync on PUSI\n",
				          FUNCNAME, context->length(), context->sndu_len());
				context->reset();
				context->set_need_pusi(true);
				// we can eventually synchronize with the current MPEG frame
				// if the SNDU specified by Payload Pointer was not read yet
				if(mpeg_packet->pusi() && !pp_used)
					goto restart;
				else
					goto drop;
			}
		}
		else
		{
			// context is empty, try to extract a new SNDU from the MPEG frame

			unsigned int len;

			if(payload.at(sndu_offset) == 0xff &&
			   payload.at(sndu_offset + 1) == 0xff)
			{
				// End Indicator
				UTI_DEBUG("%s End Indicator found at offset %u\n",
				          FUNCNAME, sndu_offset);
				goto padding;
			}

			// get SNDU length
			len = this->current_upper->getLength(payload.c_str()
			                                     + sndu_offset);
			if(len == 0)
			{
				UTI_DEBUG("%s 0-byte SNDU\n", FUNCNAME);
				goto drop;
			}

			context->set_sndu_len(len);
			UTI_DEBUG("%s context is empty, extract a new %u-byte SNDU\n",
			          FUNCNAME, context->sndu_len());
		}

		// find out how much SNDU data is available
		max_len = MIN(TS_DATASIZE - sndu_offset, context->sndu_len() - context->length());
		UTI_DEBUG("%s add %u bytes of data to SNDU (SNDU needs %u bytes, "
		          "MPEG frame owns %u bytes)\n", FUNCNAME, max_len,
		          context->sndu_len() - context->length(),
		          TS_DATASIZE - sndu_offset);

		// add SNDU fragment to context
		context->add((unsigned char *)mpeg_packet->getPayload().c_str() +
		             sndu_offset, max_len);
		sndu_offset += max_len;

		if(context->length() == context->sndu_len())
		{
			// SNDU completed, add it to the list

			NetPacket *net_packet;

			UTI_DEBUG("%s SNDU completed (%u bytes)\n", FUNCNAME, context->length());

			net_packet = this->current_upper->build(
					(unsigned char *)(context->data().c_str()),
					context->length(),
					packet->getQos(),
					packet->getSrcTalId(), packet->getDstTalId());
			if(net_packet == NULL)
			{
				UTI_ERROR("%s cannot create a new SNDU, drop it\n", FUNCNAME);
			}
			else
			{
				// set the destination spot ID
				net_packet->setDstSpot(dest_spot);
				// add the network packet to the list
				tmp_packets->add(net_packet);

				UTI_DEBUG("%s SNDU (%s) created and added to the list\n",
				          FUNCNAME, this->current_upper->getName().c_str());
			}

			// reset context
			context->reset();
		}
		else if(context->length() < context->sndu_len())
		{
			// SNDU incomplete, wait for next MPEG frame

			// there should be no remaining bytes in the MPEG payload
			if(sndu_offset < TS_DATASIZE)
			{
				UTI_ERROR("%s SNDU incomplete, but %u remaining bytes in MPEG "
				          "payload, reset context, sync on PUSI\n", FUNCNAME,
				          TS_DATASIZE - sndu_offset);
				context->reset();
				context->set_need_pusi(true);
				// we can eventually synchronize with the current MPEG frame
				// if the SNDU specified by Payload Pointer was not read yet
				if(mpeg_packet->pusi() && !pp_used)
					goto restart;
				else
					goto drop;
			}
			else if(sndu_offset > TS_DATASIZE)
			{
				UTI_ERROR("%s sndu_offset too big (offset = %u), reset context, "
				          "delete SNDUs, sync on PUSI\n", FUNCNAME, sndu_offset);
				context->reset();
				for(it = tmp_packets->begin(); it != tmp_packets->end(); it++)
				{
					delete *it;
				}
				tmp_packets->clear();
				context->set_need_pusi(true);
				// offset is beyond MPEG payload, so we cannot sync with current
				// MPEG frame
				goto drop;
			}
		}
		else // context->length() > context->sndu_len()
		{
			UTI_ERROR("%s context contains too much data (%d bytes) for "
			          "one %u-byte SNDU, reset context, sync on PUSI\n",
			          FUNCNAME, context->length(), context->sndu_len());
			context->reset();
			context->set_need_pusi(true);
			// we can eventually synchronize with the current MPEG frame
			// if the SNDU specified by Payload Pointer was not read yet
			if(mpeg_packet->pusi() && !pp_used)
				goto restart;
			else
				goto drop;
		}
	}

padding:

	// check padding
	if(sndu_offset < TS_DATASIZE)
	{
		unsigned int i;

		UTI_DEBUG("%s %u bytes of padding\n", FUNCNAME,
		          TS_DATASIZE - sndu_offset);

		for(i = sndu_offset; i < TS_DATASIZE; i++)
		{
			if((unsigned char) mpeg_packet->getPayload().at(i) != 0xff)
			{
				UTI_ERROR("%s bad padding byte (0x%02x) at offset %u, reset "
				          "context, delete SNDUs, sync on PUSI\n", FUNCNAME,
				          mpeg_packet->getPayload().at(i), i);
				context->reset();
				for(it = tmp_packets->begin(); it != tmp_packets->end(); it++)
				{
					delete *it;
				}
				tmp_packets->clear();
				context->set_need_pusi(true);
				// MPEG frame is completely analyzed, we cannot synchronize with
				// current frame
				goto drop;
			}
		}
	}
	else if(sndu_offset == TS_DATASIZE)
	{
		UTI_DEBUG("%s no padding\n", FUNCNAME);
	}
	else if(sndu_offset > TS_DATASIZE)
	{
		UTI_ERROR("%s sndu_offset too big (offset = %u), reset context, "
		          "delete SNDUs, sync on PUSI\n", FUNCNAME, sndu_offset);
		context->reset();
		delete tmp_packets;
		context->set_need_pusi(true);
		// MPEG frame is completely analyzed, we cannot synchronize with
		// current frame
		goto drop;
	}

	UTI_DEBUG("%s MPEG packet is now desencapsulated (context data = %u "
				 "bytes)\n", FUNCNAME, context->length());

	// add packets to the burst of network packets
	net_packets->insert(net_packets->end(),
	                    tmp_packets->begin(), tmp_packets->end());

	tmp_packets->clear();
	delete tmp_packets;
	delete mpeg_packet;
	return true;
drop:
	delete tmp_packets;
error:
	delete mpeg_packet;
	return false;
}

NetBurst *Mpeg::Context::flush(int context_id)
{
	const char *FUNCNAME = "[Mpeg::Context::flush]";
	std::map < int, MpegEncapCtx * >::iterator it;
	MpegEncapCtx *context;
	NetBurst *mpeg_packets = NULL;
	NetPacket *mpeg_packet;

	UTI_DEBUG("%s search for encapsulation context to flush...\n", FUNCNAME);

	it = this->encap_contexts.find(context_id);

	// if encapsulation context is found and if a MPEG2-TS
	// frame is under build in the context, send it
	if(it != this->encap_contexts.end() &&
		(context = (*it).second) && context->frame()->length() > 0)
	{
		UTI_DEBUG("%s context with PID = %d has to be flushed\n",
		           FUNCNAME, context->pid());

		// create an empty burst of MPEG packets
		mpeg_packets = new NetBurst();
		if(mpeg_packets == NULL)
		{
			UTI_ERROR("%s cannot allocate memory for burst of MPEG packets\n",
			          FUNCNAME);
			goto error;
		}

		// add padding if necessary
		context->padding();

		// build MPEG2-TS packet
		mpeg_packet = new MpegPacket(*(context->frame()));
		if(mpeg_packet != NULL)
		{
			UTI_DEBUG("%s one MPEG packet created "
			          "(SRC Tal Id = %u, DST Tal ID = %u, QoS = %u)\n",
			          FUNCNAME,
			          mpeg_packet->getSrcTalId(),
			          mpeg_packet->getDstTalId(),
			          mpeg_packet->getQos());
			// set the destination spot ID
			mpeg_packet->setDstSpot(context->getDstSpot());
			mpeg_packets->add(mpeg_packet);
		}
		else
		{
			UTI_ERROR("%s failed to create MPEG packet\n", FUNCNAME);
		}

		// clear the encapsulation context
		context->reset();
	}
	else
	{
		UTI_ERROR("%s encapsulation context to flush not found or empty\n",
		          FUNCNAME);
	}

error:
	return mpeg_packets;
}

NetBurst *Mpeg::Context::flushAll()
{
	const char *FUNCNAME = "[Mpeg::Context::flushAll]";
	NetBurst *mpeg_packets;
	NetPacket *mpeg_packet;
	std::map < int, MpegEncapCtx * >::iterator it;
	MpegEncapCtx *context;

	// create an empty burst of MPEG packets
	mpeg_packets = new NetBurst();
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of MPEG packets\n",
		          FUNCNAME);
		goto error;
	}

	// for each encapsulation context...
	for(it = this->encap_contexts.begin(); it != this->encap_contexts.end(); it++)
	{
		context = (*it).second;

		UTI_DEBUG("%s flush context with PID = %d\n", FUNCNAME, context->pid());

		if(context->length() > 0)
		{
			// add padding if necessary
			context->padding();

			// build MPEG2-TS packet
			mpeg_packet = new MpegPacket(*(context->frame()));

			if(mpeg_packet != NULL)
			{
				UTI_DEBUG("%s one MPEG packet created\n", FUNCNAME);
				// set the destination spot ID
				mpeg_packet->setDstSpot(context->getDstSpot());
				mpeg_packets->add(mpeg_packet);
			}
			else
			{
				UTI_ERROR("%s failed to create MPEG packet\n", FUNCNAME);
			}

			// clear the encapsulation context
			context->reset();
		}
	}

error:
	return mpeg_packets;
}

MpegEncapCtx *Mpeg::Context::find_encap_context(uint16_t pid, uint16_t spot_id)
{
	const char *FUNCNAME = "[Mpeg::Context::find_encap_context]";
	std::map < int, MpegEncapCtx * >::iterator context_it;
	MpegEncapCtx *context;

	context_it = this->encap_contexts.find(pid);

	if(context_it == this->encap_contexts.end())
	{
		MpegEncapCtx *new_context;
		std::pair < std::map < int, MpegEncapCtx * >::iterator, bool > infos;

		UTI_DEBUG("%s encapsulation context does not exist yet\n", FUNCNAME);

		new_context = new MpegEncapCtx(pid, spot_id);

		infos = this->encap_contexts.insert(std::make_pair(pid, new_context));

		if(!infos.second)
		{
			UTI_ERROR("%s cannot create a new encapsulation context, "
			          "drop the packet\n", FUNCNAME);
			delete new_context;
			goto error;
		}

		UTI_INFO("%s new encapsulation context created (PID = %u)\n",
		         FUNCNAME, pid);
		context = (*(infos.first)).second;
	}
	else
	{
		UTI_DEBUG("%s encapsulation context already exists\n", FUNCNAME);
		context = (*context_it).second;
	}

	return context;

error:
	return NULL;
}


NetPacket *Mpeg::PacketHandler::build(unsigned char *data, size_t data_length,
                                      uint8_t UNUSED(_qos),
                                      uint8_t UNUSED(_src_tal_id),
                                      uint8_t UNUSED(_dst_tal_id)) const
{
	const char *FUNCNAME = "[Mpeg::PacketHandler::build]";
	uint8_t qos;
	uint8_t src_tal_id, dst_tal_id;

	if(data_length != this->getFixedLength())
	{
		UTI_ERROR("%s bad data length (%zu) for MPEG packet\n",
		          FUNCNAME, data_length);
		return NULL;
	}

	MpegPacket packet(data, data_length);

	qos = packet.getQos();
	src_tal_id = packet.getSrcTalId();
	dst_tal_id = packet.getDstTalId();

	return new NetPacket(data, data_length,
	                     this->getName(), this->getEtherType(),
	                     qos, src_tal_id, dst_tal_id, TS_HEADERSIZE);
}

bool Mpeg::PacketHandler::getChunk(NetPacket *packet,
                                   size_t remaining_length,
                                   NetPacket **data,
                                   NetPacket **remaining_data) const
{
	*data = NULL;
	*remaining_data = NULL;
	if(remaining_length < this->getFixedLength())
	{
		*remaining_data = packet;
	}
	else
	{
		*data = packet;
	}

	return true;
}

Mpeg::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
}

