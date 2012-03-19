/**
 * @file MpegCtx.cpp
 * @brief MPEG2-TS encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "MpegCtx.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"


MpegCtx::MpegCtx(unsigned int packing_min_len,
                 unsigned long packing_threshold,
                 unsigned int (*sndu_length)(Data *data, unsigned int offset),
                 NetPacket * (*create_sndu)(Data data)):
	EncapCtx(), encap_contexts(), desencap_contexts()
{
	this->packing_min_len = packing_min_len;
	this->packing_threshold = packing_threshold;
	this->sndu_length = sndu_length;
	this->create_sndu = create_sndu;
}

MpegCtx::~MpegCtx()
{
	std::map < int, MpegEncapCtx * >::iterator encap_it;
	std::map < int, MpegDesencapCtx * >::iterator desencap_it;

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

NetBurst *MpegCtx::encapsulate(NetPacket *packet,
                               int &context_id,
                               long &time)
{
	const char *FUNCNAME = "[MpegCtx::encapsulate]";
	NetBurst *mpeg_packets = NULL;

	uint16_t pid;
	MpegEncapCtx *context;

	unsigned int packet_len;
	unsigned int packet_off;
	Data packet_data;
	unsigned int length;
	MpegPacket *mpeg_packet;

	time = 0;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	// PID (13 bits) = MAC id (7 bits) + TAL id (3 bits) + QoS (3 bits)
	if((packet->macId() & 0x7f) != packet->macId())
	{
		UTI_ERROR("Be careful, you have set a MAC ID greater than 0x7f"
		          " this can not stand in 7 bits of PID field of MPEG2-TS"
		          " packets!!!\n");
	}
	if((packet->talId() & 0x07) != packet->talId())
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 0x07"
		          " this can not stand in 3 bits of PID field of MPEG2-TS"
		          " packets!!!\n");
	}
	if((packet->qos() & 0x07) != packet->qos())
	{
		UTI_ERROR("Be careful, you have set a QoS priority greater than 0x07"
		          " this can not stand in 3 bits of PID field of MPEG2-TS"
		          " packets!!!\n");
	}

	pid = ((packet->macId() & 0x7f) << 6) +
	      ((packet->talId() & 0x07) << 3) +
	      (packet->qos() & 0x07);

	// find the encapsulation context for the network packet
	UTI_DEBUG("%s network packet belongs to the encapsulation context "
	          "identified by PID = %u\n", FUNCNAME, pid);
	context = this->find_encap_context(pid);
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

	// create an empty burst of MPEG packets
	mpeg_packets = new NetBurst();
	if(mpeg_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst "
		          "of MPEG packets\n", FUNCNAME);
		goto drop;
	}

	packet_data = packet->data();
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
				mpeg_packets->push_back(mpeg_packet);
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
	//  - there is less than <packing_min_len> byte(s) of unused payload in the
	//    frame
	//  - there is exactly <packing_min_len> byte(s) of unused payload in the
	//    frame and the PUSI bit is not set
	if(this->packing_threshold == 0 ||
	   context->left() < this->packing_min_len ||
	   (context->left() == this->packing_min_len && !context->pusi()))
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
			mpeg_packet->addTrace(HERE());
			UTI_DEBUG("%s one MPEG packet created\n", FUNCNAME);
			mpeg_packets->push_back(mpeg_packet);
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

drop:
	return mpeg_packets;
}

NetBurst *MpegCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[MpegCtx::desencapsulate]";
	NetBurst *net_packets = NULL;
	NetBurst::iterator it;
	MpegPacket *mpeg_packet;
	uint16_t pid;
	std::map < int, MpegDesencapCtx * >::iterator context_it;
	MpegDesencapCtx *context;

	Data payload;
	unsigned int sndu_offset;
	bool pp_used;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// packet must be a MPEG packet
	if(packet->type() != NET_PROTO_MPEG)
	{
		UTI_ERROR("%s encapsulation packet is not an MPEG packet, "
		          "drop the packet\n", FUNCNAME);
		goto drop;
	}

	// cast from a generic packet to a MPEG packet
	mpeg_packet = dynamic_cast<MpegPacket *>(packet);
	if(mpeg_packet == NULL)
	{
		UTI_ERROR("%s bad cast from NetPacket to MpegPacket!\n",
		          FUNCNAME);
		goto drop;
	}

	// filter packet when desencapsulating if filter is set
#if 0 // TODO: is it possible to filter packets against PID without breaking transparent mode ?
	if(this->talId() != -1 && this->talId() != mpeg_packet->talId())
	{
		UTI_DEBUG("MPEG packet with TAL ID = %ld (PID = %ld) filtered (packet "
		          "is not for TAL ID = %ld)\n", mpeg_packet->talId(),
		          mpeg_packet->pid(), this->talId());

		// packet is filtered, this is not an error, so we must return an empty
		// but not NULL burst
		net_packets = new NetBurst();
		if(net_packets == NULL)
		{
			UTI_ERROR("%s cannot allocate memory for burst of MPEG packets\n",
			          FUNCNAME);
			goto drop;
		}

		goto drop;
	}
#endif

	// get the PID number for the MPEG packet to desencapsulate
	pid = mpeg_packet->pid();
	UTI_DEBUG("%s MPEG packet belongs to the encapsulation context "
	          "identified by PID = %u\n", FUNCNAME, pid);

	// find the desencapsulation context for the MPEG packet
	context_it = this->desencap_contexts.find(pid);
	if(context_it == this->desencap_contexts.end())
	{
		UTI_DEBUG("%s desencapsulation context does not exist yet\n", FUNCNAME);

		MpegDesencapCtx *new_context = new MpegDesencapCtx(pid);
		std::pair < std::map < int, MpegDesencapCtx * >::iterator, bool > infos;
		infos = this->desencap_contexts.insert(std::make_pair(pid, new_context));

		if(!infos.second)
		{
			UTI_ERROR("%s cannot create a new desencapsulation context, "
			          "drop the packet\n", FUNCNAME);
			delete new_context;
			goto drop;
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
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of MPEG packets\n",
		          FUNCNAME);
		goto drop;
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
	   (unsigned int) (TS_DATASIZE - 1 - mpeg_packet->pp()) < this->packing_min_len)
	{
		UTI_ERROR("%s too few bytes (%d < %d) after Payload Pointer to contain "
		          "a SNDU fragment, reset context, sync on PUSI\n", FUNCNAME,
		          TS_DATASIZE - 1 - mpeg_packet->pp(), this->packing_min_len);
		context->reset();
		context->set_need_pusi(true);
		// PUSI bit set in current MPEG frame, but PP is not valid, so we cannot
		// synchronize with current frame, drop current frame
		goto drop;
	}

	pp_used = false;
	payload = mpeg_packet->payload();

	// desencapsulate SNDUs from the MPEG2-TS frame
	while(TS_DATASIZE - sndu_offset >= this->packing_min_len)
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
			len = this->sndu_length(&payload, sndu_offset);
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
		          context->sndu_len() - context->length(), TS_DATASIZE - sndu_offset);

		// add SNDU fragment to context
		context->add((unsigned char *) mpeg_packet->payload().c_str() + sndu_offset, max_len);
		sndu_offset += max_len;

		if(context->length() == context->sndu_len())
		{
			// SNDU completed, add it to the list

			NetPacket *net_packet;

			UTI_DEBUG("%s SNDU completed (%u bytes)\n", FUNCNAME, context->length());

			net_packet = this->create_sndu(context->data());
			if(net_packet == NULL)
			{
				UTI_ERROR("%s cannot create a new SNDU, drop it\n", FUNCNAME);
			}
			else
			{
				net_packet->addTrace(HERE());
				// copy some parameters
				net_packet->setMacId(packet->macId());
				net_packet->setTalId(packet->talId());
				net_packet->setQos(packet->qos());

				// add the network packet to the list
				net_packets->push_back(net_packet);

				UTI_DEBUG("%s SNDU created and added to the list\n", FUNCNAME);
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
				for(it = net_packets->begin(); it != net_packets->end(); it++)
				{
					delete *it;
				}
				net_packets->clear();
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
			if((unsigned char) mpeg_packet->payload().at(i) != 0xff)
			{
				UTI_ERROR("%s bad padding byte (0x%02x) at offset %u, reset "
				          "context, delete SNDUs, sync on PUSI\n", FUNCNAME,
				          mpeg_packet->payload().at(i), i);
				context->reset();
				for(it = net_packets->begin(); it != net_packets->end(); it++)
				{
					delete *it;
				}
				net_packets->clear();
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
		for(it = net_packets->begin(); it != net_packets->end(); it++)
		{
			delete *it;
		}
		net_packets->clear();
		context->set_need_pusi(true);
		// MPEG frame is completely analyzed, we cannot synchronize with
		// current frame
		goto drop;
	}

	UTI_DEBUG("%s MPEG packet is now desencapsulated (context data = %u "
				 "bytes)\n", FUNCNAME, context->length());

drop:
	return net_packets;
}

std::string MpegCtx::type()
{
	return std::string("MPEG2-TS");
}

NetBurst *MpegCtx::flush(int context_id)
{
	const char *FUNCNAME = "[MpegCtx::flush]";
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
			UTI_DEBUG("%s one MPEG packet created\n", FUNCNAME);
			mpeg_packets->push_back(mpeg_packet);
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

NetBurst *MpegCtx::flushAll()
{
	const char *FUNCNAME = "[MpegCtx::flushAll]";
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
				mpeg_packets->push_back(mpeg_packet);
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

MpegEncapCtx *MpegCtx::find_encap_context(uint16_t pid)
{
	const char *FUNCNAME = "[MpegCtx::find_encap_context]";
	std::map < int, MpegEncapCtx * >::iterator context_it;
	MpegEncapCtx *context;

	context_it = this->encap_contexts.find(pid);

	if(context_it == this->encap_contexts.end())
	{
		MpegEncapCtx *new_context;
		std::pair < std::map < int, MpegEncapCtx * >::iterator, bool > infos;

		UTI_DEBUG("%s encapsulation context does not exist yet\n", FUNCNAME);

		new_context = new MpegEncapCtx(pid);

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

