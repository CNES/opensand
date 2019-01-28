/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */

#include "Mpeg.h"
#include "MpegPacket.h"

#include <opensand_output/Output.h>
#include <opensand_conf/ConfigurationFile.h>

#include <vector>
#include <map>

#define PACKING_THRESHOLD "packing_threshold"
#define MPEG_SECTION "mpeg"
#define CONF_MPEG_FILENAME "mpeg.conf"


Mpeg::Mpeg():
	EncapPlugin(NET_PROTO_MPEG)
{
	this->upper[TRANSPARENT].push_back("ULE");
	this->upper[REGENERATIVE].push_back("AAL5/ATM");
	this->upper[REGENERATIVE].push_back("ULE");
	// register the static packet log
	MpegPacket::mpeg_log = Output::registerLog(LEVEL_WARNING, "Encap.Net.MPEG");
}


Mpeg::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin), encap_contexts(), desencap_contexts()
{
};

bool Mpeg::Context::init()
{
	if(!EncapPlugin::EncapContext::init())
	{
		return false;
	}
	ConfigurationFile config;
	map<string, ConfigurationList> config_section_map;
	string conf_mpeg_path;
	conf_mpeg_path = this->getConfPath() + string(CONF_MPEG_FILENAME);

	if(config.loadConfig(conf_mpeg_path.c_str()) < 0)
	{
		LOG(this->log, LEVEL_ERROR,
		    "failed to load config file '%s'",
		    conf_mpeg_path.c_str());
		goto error;
	}

	config.loadSectionMap(config_section_map);

	// Retrieving the packing threshold
	if(!config.getValue(config_section_map[MPEG_SECTION],
	                    PACKING_THRESHOLD, this->packing_threshold))
	{
		LOG(this->log, LEVEL_ERROR,
		    "missing %s parameter\n", PACKING_THRESHOLD);
		goto error;
	}
	LOG(this->log, LEVEL_INFO,
	    "packing thershold: %lu\n",
	    this->packing_threshold);
	
	return true;
error:
	return false;
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
	NetBurst *mpeg_packets = NULL;
	NetBurst::iterator packet;

	// create an empty burst of MPEG packets
	mpeg_packets = new NetBurst();
	if(mpeg_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of MPEG "
		    "packets\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		int context_id;
		long time = 0;

		if(!this->encapMpeg(*packet, mpeg_packets, context_id, time))
		{
			LOG(this->log, LEVEL_ERROR,
			    "MPEG encapsulation failed, drop packet\n");
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
	NetBurst *net_packets;

	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network "
		    "packets\n");
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); ++packet)
	{
		uint8_t dst_tal_id;

		// packet must be valid
		if(*packet == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not valid, drop "
			    "the packet\n");
			continue;
		}

		// packet must be a MPEG packet
		if((*packet)->getType() != this->getEtherType())
		{
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not a MPEG packet "
			    "(type = 0x%04x), drop the packet\n",
			    (*packet)->getType());
			continue;
		}

		// Filter if packet is for this ST
		dst_tal_id = (*packet)->getDstTalId();
		if((dst_tal_id != this->dst_tal_id)
			&& (dst_tal_id != BROADCAST_TAL_ID))
		{
			LOG(this->log, LEVEL_INFO,
			    "encapsulation packet is for ST#%u. Drop\n",
			    dst_tal_id);
			continue;
		}

		if(!this->deencapMpeg(*packet, net_packets))
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot create a burst of packets, "
			    "drop packet\n");
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
	MpegPacket *mpeg_packet;
	uint16_t pid;
	MpegEncapCtx *context;

	unsigned int packet_len;
	unsigned int packet_off;
	Data packet_data;
	unsigned int length;
	// keep the destination spot
	uint16_t dest_spot = packet->getSpot();

	time = 0;

	if((packet->getSrcTalId() & 0x1F) != packet->getSrcTalId())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a TAL ID greater than 0x1F"
		    " this can not stand in 5 bits of PID field of "
		    "MPEG2-TS packets!!!\n");
	}
	if((packet->getDstTalId() & 0x1F) != packet->getDstTalId())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a TAL ID greater than 0x1F"
		    " this can not stand in 5 bits of PID field of "
		    "MPEG2-TS packets!!!\n");
	}
	if((packet->getQos() & 0x07) != packet->getQos())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a QoS priority greater "
		    "than 0x07 this can not stand in 3 bits of PID field "
		    "of MPEG2-TS packets!!!\n");
	}

	pid = MpegPacket::getPidFromPacket(packet);

	// find the encapsulation context for the network packet
	LOG(this->log, LEVEL_INFO,
	    "network packet belongs to the encapsulation context "
	    "identified by PID = %u\n", pid);
	context = this->find_encap_context(pid, dest_spot);
	if(context == NULL)
		goto drop;

	// return a reference to the encapsulation context to encapsulation bloc
	context_id = context->pid();

	LOG(this->log, LEVEL_INFO,
	    "encapsulation context contains %u bytes of data\n",
	    context->length());

	// build MPEG packets
	LOG(this->log, LEVEL_INFO,
	    "Synchonisation Byte = 0x%02x\n",
	    context->sync());

	// set PUSI bit to 1 only if not already set. If not set, insert a
	// Payload Pointer just after the header
	if(!context->pusi())
	{
		LOG(this->log, LEVEL_INFO,
		    "PUSI (%d) not set, set PUSI = 1 and add Payload "
		    "Pointer (packet length = %u)\n",
		    context->pusi(), context->length());

		// set the PUSI bit
		context->setPusi();
		LOG(this->log, LEVEL_INFO,
		    "PUSI is now set to %d\n",
		    context->pusi());

		// add the Payload Pointer field
		context->addPP();
		LOG(this->log, LEVEL_INFO,
		    "packet is now %u byte length\n",
		    context->length());
	}

	packet_data = packet->getData();
	packet_len = packet_data.length();
	packet_off = 0;

	while(packet_len > 0)
	{
		length = MIN(packet_len, context->left());
		context->add(&packet_data, packet_off, length);

		LOG(this->log, LEVEL_INFO,
		    "copy %u bytes of SNDU data into MPEG payload "
		    "(SNDU data = %u bytes, unused payload = %u bytes)\n",
		    length,
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
				LOG(this->log, LEVEL_INFO,
				    "one MPEG packet created\n");
				// set the destination spot ID
				mpeg_packet->setSpot(dest_spot);

				mpeg_packets->add(mpeg_packet);
			}
			else
			{
				LOG(this->log, LEVEL_ERROR,
				    "failed to create MPEG packet\n");
			}

			// clear the encapsulation context
			context->reset();
		}
	}

	LOG(this->log, LEVEL_INFO,
	    "SNDU packet now entirely packed into MPEG packets\n");
	LOG(this->log, LEVEL_INFO,
	    "unused space in MPEG payload = %u bytes\n",
	    context->left());

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

		LOG(this->log, LEVEL_INFO,
		    "too few unused space in the MPEG payload for "
		    "packing => add padding to packet and send it\n");

		// add padding if necessary
		context->padding();

		// add frame to the list...
		mpeg_packet = new MpegPacket(*(context->frame()));

		if(mpeg_packet != NULL)
		{
			LOG(this->log, LEVEL_INFO,
			    "one MPEG packet created\n");
			// set the destination spot ID
			mpeg_packet->setSpot(dest_spot);
			mpeg_packets->add(mpeg_packet);
		}
		else
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to create MPEG packet\n");
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

		LOG(this->log, LEVEL_INFO,
		    "enough unused space in the MPEG payload for "
		    "packing => keep incomplete MPEG packet during %ld "
		    "ms\n", this->packing_threshold);

		time = this->packing_threshold;
	}

	return true;

drop:
	return false;
}

bool Mpeg::Context::deencapMpeg(NetPacket *packet, NetBurst *net_packets)
{
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
	uint16_t dest_spot = packet->getSpot();

	// packet must be a MPEG packet
	if(packet->getType() != NET_PROTO_MPEG)
	{
		LOG(this->log, LEVEL_ERROR,
		    "encapsulation packet is not an MPEG packet, "
		    "drop the packet\n");
		goto drop;
	}

	// cast from a generic packet to a MPEG packet
	mpeg_packet = new MpegPacket(packet->getData());
	if(mpeg_packet == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot create a MpegPacket from NetPacket\n");
		goto error;
	}

	// get the PID number for the MPEG packet to desencapsulate
	pid = mpeg_packet->getPid();
	LOG(this->log, LEVEL_INFO,
	    "MPEG packet belongs to the encapsulation context "
	    "identified by PID = %u\n", pid);

	// find the desencapsulation context for the MPEG packet
	context_it = this->desencap_contexts.find(pid);
	if(context_it == this->desencap_contexts.end())
	{
		LOG(this->log, LEVEL_INFO,
		    "desencapsulation context does not exist yet\n");

		MpegDeencapCtx *new_context = new MpegDeencapCtx(pid, dest_spot);
		std::pair < std::map < int, MpegDeencapCtx * >::iterator, bool > infos;
		infos = this->desencap_contexts.insert(std::make_pair(pid, new_context));

		if(!infos.second)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot create a new desencapsulation context, "
			    "drop the packet\n");
			delete new_context;
			goto error;
		}

		LOG(this->log, LEVEL_NOTICE,
		    "new desencapsulation context created (PID = %u)\n",
		    pid);
		context = (*(infos.first)).second;
	}
	else
	{
		LOG(this->log, LEVEL_INFO,
		    "desencapsulation context already exists\n");
		context = (*context_it).second;
	}

	LOG(this->log, LEVEL_INFO,
	    "desencapsulation context contains %d bytes of "
	    "data\n", context->length());

	// create an empty burst of SNDU packets
	tmp_packets = new NetBurst();
	if(tmp_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of MPEG "
		    "packets\n");
		goto error;
	}

restart:

	LOG(this->log, LEVEL_INFO,
	    "MPEG frame has PUSI = %d\n",
	    mpeg_packet->pusi() ? 1 : 0);

	// synchronize on PUSI bit if necessary
	if(context->need_pusi())
	{
		LOG(this->log, LEVEL_INFO,
		    "PUSI synchronizing is needed\n");

		if(mpeg_packet->pusi())
		{
			LOG(this->log, LEVEL_INFO,
			    "sync on PUSI with MPEG frame CC = %d\n",
			    mpeg_packet->cc());
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
			LOG(this->log, LEVEL_ERROR,
			    "sync on PUSI needed, drop MPEG frame CC = %d "
			    "with no PUSI\n", mpeg_packet->cc());
			goto drop;
		}
	}
	else
	{
		// if synchronizing on PUSI is not necessary, check Continuity Counters
		// for lost frames

		LOG(this->log, LEVEL_INFO,
		    "PUSI synchronizing not needed, check CC\n");

		context->incCc();
		if(mpeg_packet->cc() != context->cc())
		{
			// Continuity Counters are different, some MPEG frames were lost

			LOG(this->log, LEVEL_ERROR,
			    "MPEG frame(s) lost (MPEG CC = %d, CTXT CC "
			    "= %d), reset context, sync on PUSI\n",
			    mpeg_packet->cc(), context->cc());

			// delete partially desencapsulated SNDUs
			context->reset();
			// ask for PUSI synchronizing
			context->set_need_pusi(true);

			// drop MPEG frame or synchronize CC with it
			if(mpeg_packet->pusi())
			{
				// the current MPEG frame has PUSI bit set, do not drop it,
				// but synchronize CC with it
				LOG(this->log, LEVEL_INFO,
				    "PUSI is set in current MPEG frame, "
				    "restart analysis...\n");
				goto restart;
			}
			else
			{
				// the current MPEG frame has no PUSI bit set, drop it and
				// synchronize on next MPEG frame with PUSI bit set
				LOG(this->log, LEVEL_ERROR,
				    "PUSI not set in current MPEG frame, "
				    "drop it\n");
				goto drop;
			}
		}
		else
		{
			// Continuity Counters are equal, no MPEG frame was lost

			LOG(this->log, LEVEL_INFO,
			    "MPEG frame with CC = %d received\n",
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

	LOG(this->log, LEVEL_INFO,
	    "SNDU starts at offset %u in MPEG payload\n",
	    sndu_offset);

	// check Payload Pointer validity: the number of packed bytes at the end of
	// the MPEG payload depends on SNDU type
	if(mpeg_packet->pusi() &&
	   mpeg_packet->pp() + 1 > TS_DATASIZE &&
	   (unsigned int)
	   (TS_DATASIZE - 1 - mpeg_packet->pp()) < this->current_upper->getMinLength())
	{
		LOG(this->log, LEVEL_ERROR,
		    "too few bytes (%d < %zu) after Payload Pointer "
		    "to contain a SNDU fragment, reset context, sync on"
		    " PUSI\n",
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

			LOG(this->log, LEVEL_INFO,
			    "context not empty, complete partially built "
			    "SNDU\n");

			// check data length in context
			if(context->length() >= context->sndu_len())
			{
				LOG(this->log, LEVEL_ERROR,
				    "context contains too much data (%d bytes) "
				    "for one %d-byte SNDU, reset context, sync "
				    "on PUSI\n", context->length(),
				    context->sndu_len());
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
				LOG(this->log, LEVEL_INFO,
				    "End Indicator found at offset %u\n",
				    sndu_offset);
				goto padding;
			}

			// get SNDU length
			len = this->current_upper->getLength(payload.c_str()
			                                     + sndu_offset);
			if(len == 0)
			{
				LOG(this->log, LEVEL_INFO,
				    "0-byte SNDU\n");
				goto drop;
			}

			context->set_sndu_len(len);
			LOG(this->log, LEVEL_INFO,
			    "context is empty, extract a new %u-byte "
			    "SNDU\n", context->sndu_len());
		}

		// find out how much SNDU data is available
		max_len = MIN(TS_DATASIZE - sndu_offset, context->sndu_len() - context->length());
		LOG(this->log, LEVEL_INFO,
		    "add %u bytes of data to SNDU (SNDU needs %u "
		    "bytes, MPEG frame owns %u bytes)\n",
		    max_len, context->sndu_len() - context->length(),
		    TS_DATASIZE - sndu_offset);

		// add SNDU fragment to context
		context->add(mpeg_packet->getPayload(),
		             sndu_offset, max_len);
		sndu_offset += max_len;

		if(context->length() == context->sndu_len())
		{
			// SNDU completed, add it to the list

			NetPacket *net_packet;

			LOG(this->log, LEVEL_INFO,
			    "SNDU completed (%u bytes)\n",
			    context->length());

			net_packet = this->current_upper->build(
					context->data(),
					context->length(),
					packet->getQos(),
					packet->getSrcTalId(), packet->getDstTalId());
			if(net_packet == NULL)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create a new SNDU, drop it\n");
			}
			else
			{
				// set the destination spot ID
				net_packet->setSpot(dest_spot);
				// add the network packet to the list
				tmp_packets->add(net_packet);

				LOG(this->log, LEVEL_INFO,
				    "SNDU (%s) created and added to the list\n",
				    this->current_upper->getName().c_str());
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
				LOG(this->log, LEVEL_ERROR,
				    "SNDU incomplete, but %u remaining "
				    "bytes in MPEG payload, reset context, sync "
				    "on PUSI\n",
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
				LOG(this->log, LEVEL_ERROR,
				    "sndu_offset too big (offset = %u), reset "
				    "context, delete SNDUs, sync on PUSI\n",
				    sndu_offset);
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
			LOG(this->log, LEVEL_ERROR,
			    "context contains too much data (%d bytes) for "
			    "one %u-byte SNDU, reset context, sync on PUSI\n",
			    context->length(), context->sndu_len());
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

		LOG(this->log, LEVEL_INFO,
		    "%u bytes of padding\n",
		    TS_DATASIZE - sndu_offset);

		for(i = sndu_offset; i < TS_DATASIZE; i++)
		{
			if((unsigned char) mpeg_packet->getPayload().at(i) != 0xff)
			{
				LOG(this->log, LEVEL_ERROR,
				    "bad padding byte (0x%02x) at offset %u, "
				    "reset context, delete SNDUs, sync on PUSI\n",
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
		LOG(this->log, LEVEL_INFO,
		    "no padding\n");
	}
	else if(sndu_offset > TS_DATASIZE)
	{
		LOG(this->log, LEVEL_ERROR,
		    "sndu_offset too big (offset = %u), reset context, "
		    "delete SNDUs, sync on PUSI\n", sndu_offset);
		context->reset();
		delete tmp_packets;
		context->set_need_pusi(true);
		// MPEG frame is completely analyzed, we cannot synchronize with
		// current frame
		goto drop;
	}

	LOG(this->log, LEVEL_INFO,
	    "MPEG packet is now desencapsulated (context data = %u "
	    "bytes)\n", context->length());

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
	std::map < int, MpegEncapCtx * >::iterator it;
	MpegEncapCtx *context;
	NetBurst *mpeg_packets = NULL;
	NetPacket *mpeg_packet;

	LOG(this->log, LEVEL_INFO,
	    "search for encapsulation context to flush...\n");

	it = this->encap_contexts.find(context_id);

	// if encapsulation context is found and if a MPEG2-TS
	// frame is under build in the context, send it
	if(it != this->encap_contexts.end() &&
		(context = (*it).second) && context->frame()->length() > 0)
	{
		LOG(this->log, LEVEL_INFO,
		    "context with PID = %d has to be flushed\n",
		    context->pid());

		// create an empty burst of MPEG packets
		mpeg_packets = new NetBurst();
		if(mpeg_packets == NULL)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot allocate memory for burst of MPEG "
			    "packets\n");
			goto error;
		}

		// add padding if necessary
		context->padding();

		// build MPEG2-TS packet
		mpeg_packet = new MpegPacket(*(context->frame()));
		if(mpeg_packet != NULL)
		{
			LOG(this->log, LEVEL_INFO,
			    "one MPEG packet created "
			    "(SRC Tal Id = %u, DST Tal ID = %u, QoS = %u)\n",
			    mpeg_packet->getSrcTalId(),
			    mpeg_packet->getDstTalId(),
			    mpeg_packet->getQos());
			// set the destination spot ID
			mpeg_packet->setSpot(context->getDstSpot());
			mpeg_packets->add(mpeg_packet);
		}
		else
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to create MPEG packet\n");
		}

		// clear the encapsulation context
		context->reset();
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "encapsulation context to flush not found or "
		    "empty\n");
	}

error:
	return mpeg_packets;
}

NetBurst *Mpeg::Context::flushAll()
{
	NetBurst *mpeg_packets;
	NetPacket *mpeg_packet;
	std::map < int, MpegEncapCtx * >::iterator it;
	MpegEncapCtx *context;

	// create an empty burst of MPEG packets
	mpeg_packets = new NetBurst();
	if(mpeg_packets == NULL)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of MPEG "
		    "packets\n");
		goto error;
	}

	// for each encapsulation context...
	for(it = this->encap_contexts.begin(); it != this->encap_contexts.end(); it++)
	{
		context = (*it).second;

		LOG(this->log, LEVEL_INFO,
		    "flush context with PID = %d\n",
		    context->pid());

		if(context->length() > 0)
		{
			// add padding if necessary
			context->padding();

			// build MPEG2-TS packet
			mpeg_packet = new MpegPacket(*(context->frame()));

			if(mpeg_packet != NULL)
			{
				LOG(this->log, LEVEL_INFO,
				    "one MPEG packet created\n");
				// set the destination spot ID
				mpeg_packet->setSpot(context->getDstSpot());
				mpeg_packets->add(mpeg_packet);
			}
			else
			{
				LOG(this->log, LEVEL_ERROR,
				    "failed to create MPEG packet\n");
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
	std::map < int, MpegEncapCtx * >::iterator context_it;
	MpegEncapCtx *context;

	context_it = this->encap_contexts.find(pid);

	if(context_it == this->encap_contexts.end())
	{
		MpegEncapCtx *new_context;
		std::pair < std::map < int, MpegEncapCtx * >::iterator, bool > infos;

		LOG(this->log, LEVEL_INFO,
		    "encapsulation context does not exist yet\n");

		new_context = new MpegEncapCtx(pid, spot_id);

		infos = this->encap_contexts.insert(std::make_pair(pid, new_context));

		if(!infos.second)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot create a new encapsulation context, "
			    "drop the packet\n");
			delete new_context;
			goto error;
		}

		LOG(this->log, LEVEL_NOTICE,
		    "new encapsulation context created (PID = %u)\n",
		    pid);
		context = (*(infos.first)).second;
	}
	else
	{
		LOG(this->log, LEVEL_INFO,
		    "encapsulation context already exists\n");
		context = (*context_it).second;
	}

	return context;

error:
	return NULL;
}


NetPacket *Mpeg::PacketHandler::build(const Data &data, size_t data_length,
                                      uint8_t UNUSED(_qos),
                                      uint8_t UNUSED(_src_tal_id),
                                      uint8_t UNUSED(_dst_tal_id)) const
{
	uint8_t qos;
	uint8_t src_tal_id, dst_tal_id;

	if(data_length != this->getFixedLength())
	{
		LOG(this->log, LEVEL_ERROR,
		    "bad data length (%zu) for MPEG packet\n",
		    data_length);
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

bool Mpeg::PacketHandler::getSrc(const Data &data, tal_id_t &tal_id) const
{
	MpegPacket packet(data, this->getFixedLength());
	if(!packet.isValid())
	{
		return false;
	}
	tal_id = packet.getSrcTalId();
	return true;
}

bool Mpeg::PacketHandler::getQos(const Data &data, qos_t &qos) const
{
	MpegPacket packet(data, this->getFixedLength());
	if(!packet.isValid())
	{
		return false;
	}
	qos = packet.getQos();
	return true;
}
