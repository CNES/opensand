/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file Gse.cpp
 * @brief GSE encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "Gse.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include <platine_conf/uti_debug.h>
#include <platine_conf/ConfigurationFile.h>
#include <vector>
#include <map>

#define PACKING_THRESHOLD "packing_threshold"
#define GSE_SECTION "gse"
#define CONF_GSE_FILE "/etc/platine/plugins/gse.conf"

#define MAX_QOS_NBR 0xFF

Gse::Gse():
	EncapPlugin()
{
	this->ether_type = NET_PROTO_GSE;
	this->encap_name = "GSE";

	this->upper[TRANSPARENT].push_back("ROHC");
	this->upper[TRANSPARENT].push_back("IP");
	this->upper[REGENERATIVE].push_back("ATM/AAL5");
	this->upper[REGENERATIVE].push_back("MPEG2-TS");
}


Gse::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin), contexts()
{
	const char *FUNCNAME = "[Gse::Context::Context]";
	gse_status_t status;
	ConfigurationFile config;

	if(config.loadConfig(CONF_GSE_FILE) < 0)
	{   
		UTI_ERROR("%s failed to load config file '%s'",
		          FUNCNAME, CONF_GSE_FILE);
		goto error;
	}   

	// Retrieving the packing threshold
	if(!config.getValue(GSE_SECTION,
	                    PACKING_THRESHOLD, this->packing_threshold))
	{   
		UTI_ERROR("%s missing %s parameter\n", FUNCNAME, PACKING_THRESHOLD);
		goto unload;
	}   
	UTI_DEBUG("%s packing thershold: %lu\n", FUNCNAME, this->packing_threshold);

	// Initialize encapsulation and deencapsulation contexts
	// Since we use a "custom" frag_id based on QoS value and the source tal_id,
    // set the qos_nbr in GSE library to its max value.
	status = gse_encap_init(MAX_QOS_NBR, 1, &this->encap);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot init GSE encapsulation context (%s)\n",
		          FUNCNAME, gse_get_status(status));
		goto unload;
	}
	status = gse_deencap_init(MAX_QOS_NBR, &this->deencap);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot init GSE deencapsulation context (%s)\n",
		          FUNCNAME, gse_get_status(status));
		goto release_encap;
	}

	config.unloadConfig();

	return;

release_encap:
	status = gse_encap_release(this->encap);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot release GSE encapsulation context (%s)\n",
		          FUNCNAME, gse_get_status(status));
	}
unload:
	config.unloadConfig();
error:
	this->encap = NULL;
	this->deencap = NULL;
}

Gse::Context::~Context()
{
	const char *FUNCNAME = "[Gse::Context::~Context]";

	gse_status_t status;
	std::map <GseIdentifier *, GseEncapCtx *, ltGseIdentifier>::iterator it;

	// release GSE encapsulation and deencapsulation contexts if created
	if(this->encap != NULL)
	{
		status = gse_encap_release(this->encap);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s cannot release GSE encapsulation context (%s)\n",
			          FUNCNAME, gse_get_status(status));
		}
	}
	if(this->deencap != NULL)
	{
		status = gse_deencap_release(this->deencap);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s cannot release GSE deencapsulation context (%s)\n",
			          FUNCNAME, gse_get_status(status));
		}
	}

	for(it = this->contexts.begin(); it != this->contexts.end(); it++)
	{
		if((*it).second != NULL)
			delete (*it).second;
	}
}

NetBurst *Gse::Context::encapsulate(NetBurst *burst,
                                    map<long, int> &time_contexts)
{
	const char *FUNCNAME = "[Gse::Context::encapsulate]";
	NetBurst *gse_packets = NULL;

	NetBurst::iterator packet;

	// create an empty burst of GSE packets
	gse_packets = new NetBurst();
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of GSE packets\n",
		          FUNCNAME);
		delete burst;
		return NULL;
	}

	for(packet = burst->begin(); packet != burst->end(); packet++)
	{
		int context_id;
		long time = 0;
		
		// packet must be valid
		if(*packet == NULL)
		{
			UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
			continue;
		}

		context_id = (((*packet)->getSrcTalId()	& 0x7f) << 6) |
		             (((*packet)->getDstTalId() & 0x07) << 3) |
		             ((*packet)->getQos() & 0x07);

		UTI_DEBUG("%s encapsulate a %d-byte packet of type 0x%04x "
		          "with SRC TAL Id = %u, DST TAL Id = %u, QoS = %u\n",
		          FUNCNAME,
		          (*packet)->getTotalLength(),
		          (*packet)->getType(),
		          (*packet)->getSrcTalId(),
		          (*packet)->getDstTalId(),
		          (*packet)->getQos());

		// the GSE encapsulation context must exist
		if(this->encap == NULL)
		{
			UTI_ERROR("%s GSE encapsulation context unexisting, drop packet\n",
			          FUNCNAME);
			continue;
		}

		if((*packet)->getType() != this->current_upper->getEtherType())
		{
			// check if this is an IP packet (current_upper do not know the type)
			if(((*packet)->getType() == NET_PROTO_IPV4 ||
			    (*packet)->getType() == NET_PROTO_IPV6) &&
			   this->current_upper->getName() != "IP")
			{
				UTI_ERROR("%s wrong packet type (%u instead of %u)\n", FUNCNAME,
				          (*packet)->getType(),
				          this->current_upper->getEtherType());
				continue;
			}
		}

		// if packet size is fixed, more than one packet can be encapsulated in
		// one GSE packet, we need to handle the context
		if(this->current_upper->getFixedLength() > 0)
		{
			if(!this->encapFixedLength(*packet, gse_packets, time))
			{
				continue;
			}
		}
		// if packet size is variable, the whole packet and only it is
		// encapsulated in the GSE packet
		else if(!this->encapVariableLength(*packet, gse_packets))
		{
			continue;
		}
		time_contexts.insert(make_pair(time, context_id));
	}

	// delete the burst and all packets in it
	delete burst;
	return gse_packets;
}


/**
 *  @brief encap packet with fixed length, they can be packet in on GSE packet
 *
 *  @param packet      The packet from upper layer
 *  @param gse_packets The burst of GSE packets
 *  @return true on success, false otherwise
 */
bool Gse::Context::encapFixedLength(NetPacket *packet, NetBurst *gse_packets,
                                    long &time)
{
	const char *FUNCNAME = "[Gse::Context::encapFixedLength]";
	std::map <GseIdentifier *, GseEncapCtx *,
	          ltGseIdentifier >::iterator context_it;
	GseEncapCtx *context = NULL;
	gse_vfrag_t *vfrag_pkt;
	gse_status_t status;
	GseIdentifier *identifier;
	GseIdentifier *ctx_id;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();

	if(packet->getTotalLength() != this->current_upper->getFixedLength())
	{
		UTI_ERROR("%s Bad packet length (%d), drop packet\n",
		          FUNCNAME, this->current_upper->getFixedLength());
		return false;
	}

	identifier = new GseIdentifier(packet->getSrcTalId(),
	                               packet->getDstTalId(),
	                               packet->getQos());
	UTI_DEBUG("%s check if encapsulation context exists\n", FUNCNAME);
	context_it = this->contexts.find(identifier);
	if(context_it == this->contexts.end())
	{
		UTI_DEBUG("%s encapsulation context does not exist yet\n", FUNCNAME);
		context = new GseEncapCtx(identifier, dest_spot);
		this->contexts.insert(std::pair <GseIdentifier *, GseEncapCtx *>
		                      (identifier, context));
		UTI_DEBUG("%s new encapsulation context created, "
		          "Src TAL Id = %u, Dst TAL Id = %u, QoS = %u\n",
		          FUNCNAME,
		          context->getSrcTalId(), context->getDstTalId(), context->getQos());
	}
	else
	{
		context = (*context_it).second;
		UTI_DEBUG("%s find an encapsulation context containing %u "
		          "bytes of data\n", FUNCNAME, context->length());
		delete identifier;
	}

	// set the destination spot ID
	packet->setDstSpot(dest_spot);
	// add the packet in context
	status = context->add(packet);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Error when adding packet in context (%s), drop packet\n",
		          FUNCNAME, gse_get_status(status));
		return false;
	}

	UTI_DEBUG("%s Packet now entirely packed into GSE context, "
	          "context contains %d bytes\n", FUNCNAME, context->length());

	// if there is enough space in buffer for another MPEG/ATM packet or if
	// packing_threshold is not 0 keep data in the virtual buffer
	if((!context->isFull()) && this->packing_threshold != 0)
	{
		UTI_DEBUG("%s enough unused space in virtual buffer for packing "
		          "=> keep the packets %lu ms\n",
		          FUNCNAME, this->packing_threshold);

		time = this->packing_threshold;

		return true;
	}

	// Duplicate context virtual fragment before giving it to GSE library
	// (otherwise the GSE library will destroy it after use) and delete context.
	// Context shall be deleted otherwise there will be two accesses in
	// the virtual buffer (vfrag_pkt and context->vfrag), thus get_packet
	// could not be called (indeed, there can't be more than two accesses
	// in the same virtual buffer to avoid data modifications in other
	// packets). Another solution could have been to call get_packet_copy
	// but it is less efficient.
	status = gse_duplicate_vfrag(&vfrag_pkt, context->data(),
	                             context->length());
	delete context;
	ctx_id = (*context_it).first;
	this->contexts.erase((*context_it).first);
	delete ctx_id;
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Fail to duplicated context data (%s), drop packet\n",
		          FUNCNAME, gse_get_status(status));
		return false;
	}

	return this->encapPacket(packet, vfrag_pkt, gse_packets);
}

/**
 *  @brief encap packet with variable length
 *
 *  @param packet      The packet from upper layer
 *  @param gse_packets The burst of GSE packets
 *  @return true on success, false otherwise
 */
bool Gse::Context::encapVariableLength(NetPacket *packet, NetBurst *gse_packets)
{
	const char *FUNCNAME = "[Gse::Context::encapVariableLength]";
	gse_status_t status;
	gse_vfrag_t *vfrag_pkt;

	// Create a virtual fragment containing the packet
	status = gse_create_vfrag_with_data(&vfrag_pkt, packet->getTotalLength(),
	                                    GSE_MAX_HEADER_LENGTH,
	                                    GSE_MAX_TRAILER_LENGTH,
	                                    (unsigned char *)packet->getData().c_str(),
                                        packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Virtual fragment creation failed (%s), drop packet\n",
		          FUNCNAME, gse_get_status(status));
		return false;
	}
	return this->encapPacket(packet, vfrag_pkt, gse_packets);
}

/**
 *  @brief encapsulate the data into GSE packets
 *
 *  @param packet       The packet from upper layer
 *  @paramvfrag_ packet The current GSE packet
 *  @param gse_packets  The burst of GSE packets
 *  @return true on success, false otherwise
 */
bool Gse::Context::encapPacket(NetPacket *packet,
                               gse_vfrag_t *vfrag_pkt,
                               NetBurst *gse_packets)
{
	const char *FUNCNAME = "[Gse::Context::encapPacket]";
	gse_status_t status;
	gse_vfrag_t *vfrag_gse;
	unsigned int counter;
	uint8_t label[6];
	uint8_t frag_id;
	// keep the destination spot
	uint16_t dest_spot = packet->getDstSpot();
	// keep the QoS
	uint8_t qos = packet->getQos();
	// keep the source/destination tal_id
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t dst_tal_id = packet->getDstTalId();

	// Common part for all packet types
	if((packet->getSrcTalId() & 0x1f) != packet->getSrcTalId())
	{
		UTI_ERROR("Be careful, you have set a source TAL ID greater than 0x1f,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	if((packet->getDstTalId() & 0x1f) != packet->getDstTalId())
	{
		UTI_ERROR("Be careful, you have set a destination TAL ID greater than 0x1f,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	if((packet->getQos() & 0x7) != packet->getQos())
	{
		UTI_ERROR("Be careful, you have set a QoS greater than 0x7,"
		          " it will be truncated for GSE packet creation!!!\n");
	}

	// Set packet label
	if(!Gse::setLabel(packet, label))
	{
		UTI_ERROR("%s Cannot set label for GSE packet\n", FUNCNAME);
		goto drop;
	}

	// Get the frag Id
	frag_id = Gse::getFragId(packet);

	// Store the IP packet in the encapsulation context thanks
	// to the GSE library
	status = gse_encap_receive_pdu(vfrag_pkt, this->encap, label, 0,
	                               packet->getType(), frag_id);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Fail to store packet in GSE encapsulation context (%s), "
		          "drop packet\n", FUNCNAME, gse_get_status(status));
		goto drop;
	}

	counter = 0;
	do
	{
		counter++;
		status = gse_encap_get_packet(&vfrag_gse, this->encap,
		                              GSE_MAX_PACKET_LENGTH, frag_id);
		if(status != GSE_STATUS_OK && status != GSE_STATUS_FIFO_EMPTY)
		{
			UTI_ERROR("%s Fail to get GSE packet #%u in encapsulation context "
			          "(%s), drop packet\n", FUNCNAME, counter,
			          gse_get_status(status));
			goto clean;
		}

		if(status == GSE_STATUS_OK)
		{
			NetPacket *gse;
			gse = this->createPacket(gse_get_vfrag_start(vfrag_gse),
			                         gse_get_vfrag_length(vfrag_gse),
			                         qos, src_tal_id, dst_tal_id);
			// create a GSE packet from fragments computed by the GSE library
			if(gse == NULL)
			{
				UTI_ERROR("%s cannot create GSE packet, drop the network "
				          "packet\n", FUNCNAME);
				goto clean;
			}


			// set the destination spot ID
			gse->setDstSpot(dest_spot);
			// add GSE packet to burst
			gse_packets->add(gse);
			UTI_DEBUG("%s %d-byte GSE packet added to burst\n", FUNCNAME,
			          gse->getTotalLength());

			status = gse_free_vfrag(&vfrag_gse);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Fail to free GSE fragment #%u (%s), "
				          "drop packet\n", FUNCNAME, counter,
				          gse_get_status(status));
				goto clean;
			}
		}
	}
	while(status != GSE_STATUS_FIFO_EMPTY && !gse_packets->isFull());
	UTI_DEBUG("%s %d-byte %s packet/frame => %u GSE packets\n",
	          FUNCNAME, packet->getTotalLength(), packet->getName().c_str(),
	          counter);

	return true;

clean:
	if(vfrag_gse != NULL)
	{
		status = gse_free_vfrag(&vfrag_gse);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s failed to free GSE virtual fragment (%s)\n",
			          FUNCNAME, gse_get_status(status));
		}
	}
drop:
	return false;
}

NetBurst *Gse::Context::deencapsulate(NetBurst *burst)
{
	const char *FUNCNAME = "[Gse::Context::deencapsulate]";
	NetBurst *net_packets;
	gse_vfrag_t *vfrag_gse;
	gse_status_t status;

	NetBurst::iterator packet;

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		gse_free_vfrag(&vfrag_gse);
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

		// Filter if packet is for this ST
		dst_tal_id = (*packet)->getDstTalId();
		if((dst_tal_id != this->dst_tal_id)
			&& (dst_tal_id != BROADCAST_TAL_ID))
		{
			UTI_DEBUG("%s encapsulation packet is for ST#%u. Drop\n",
			          FUNCNAME, (*packet)->getDstTalId());
			continue;
		}


		// packet must be a GSE packet
		if((*packet)->getType() != this->getEtherType())
		{
			UTI_ERROR("%s encapsulation packet is not a GSE packet "
			          "(type = 0x%04x), drop the packet\n",
			          FUNCNAME, (*packet)->getType());
			continue;
		}

		// the GSE deencapsulation context must exist
		if(this->deencap == NULL)
		{
			UTI_ERROR("%s GSE deencapsulation context does not exist, "
			          "drop packet\n", FUNCNAME);
			continue;
		}

		// Create a virtual fragment containing the GSE packet
		status = gse_create_vfrag_with_data(&vfrag_gse, (*packet)->getTotalLength(),
		                                    0, 0, (unsigned char *)
		                                    (*packet)->getData().c_str(),
		                                    (*packet)->getTotalLength());
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s Virtual fragment creation failed (%s), drop packet\n",
			          FUNCNAME, gse_get_status(status));
			continue;
		}
		UTI_DEBUG("%s Create a virtual fragment for GSE library "
		          "(length = %d)\n", FUNCNAME, (*packet)->getTotalLength());

		if(!this->deencapPacket(vfrag_gse, (*packet)->getDstSpot(), net_packets))
		{
			continue;
		}
	}

	// delete the burst and all packets in it
	delete burst;
	return net_packets;
}

bool Gse::Context::deencapPacket(gse_vfrag_t *vfrag_gse,
                                 uint16_t dest_spot, NetBurst *net_packets)
{
	const char *FUNCNAME = "[Gse::Context::deencapPacket]";
	gse_vfrag_t *vfrag_pdu;
	gse_status_t status;
	uint8_t label_type;
	uint8_t label[6];
	uint16_t protocol;
	uint16_t packet_length;

	// deencapsulate the GSE packet thanks to the GSE library
	status = gse_deencap_packet(vfrag_gse, this->deencap, &label_type, label,
	                            &protocol, &vfrag_pdu, &packet_length);
	switch(status)
	{
		case GSE_STATUS_OK:
			UTI_DEBUG("%s GSE packet deencapsulated, Gse packet length = %u;"
			          "PDU is not complete\n", FUNCNAME, packet_length);
			break;

		case GSE_STATUS_DATA_OVERWRITTEN:
			UTI_INFO("%s GSE packet deencapsulated, GSE Length = %u (%s);"
			         "PDU is not complete, a context was erased\n",
			         FUNCNAME, packet_length, gse_get_status(status));
			break;

		case GSE_STATUS_PADDING_DETECTED:
			UTI_DEBUG("%s %s\n", FUNCNAME, gse_get_status(status));
			break;

		case GSE_STATUS_PDU_RECEIVED:
			if(protocol != this->current_upper->getEtherType())
			{
				// check if this is an IP packet (current_upper do not know the type)
				if((protocol == NET_PROTO_IPV4 ||
				    protocol == NET_PROTO_IPV6) &&
				   this->current_upper->getName() != "IP")
				{
					UTI_ERROR("%s wrong packet type received (%u instead of %u)\n",
					          FUNCNAME, protocol, this->current_upper->getEtherType());
					gse_free_vfrag(&vfrag_pdu);
					return false;
				}
			}
			if(this->current_upper->getFixedLength() > 0)
			{
				UTI_DEBUG("%s Inner packet has a fixed length (%u)\n",
				          FUNCNAME, this->current_upper->getFixedLength());
				return this->deencapFixedLength(vfrag_pdu ,dest_spot,
				                                label, net_packets);
			}
			else
			{
				UTI_DEBUG("%s Inner packet has a variable length\n", FUNCNAME);
				return this->deencapVariableLength(vfrag_pdu, dest_spot,
				                                   label, net_packets);
			}
			break;

		case GSE_STATUS_CTX_NOT_INIT:
			UTI_DEBUG("%s GSE deencapsulation failed (%s), drop packet "
			          "(probably not an error, this happens when we receive a "
			          "fragment that is not for us)\n",
			          FUNCNAME, gse_get_status(status));
			break;

		default:
			UTI_ERROR("%s GSE deencapsulation failed (%s), drop packet\n",
			          FUNCNAME, gse_get_status(status));
			return false;

	}
	//TODO FREE !
	return true;
}


bool Gse::Context::deencapFixedLength(gse_vfrag_t *vfrag_pdu,
                                      uint16_t dest_spot,
                                      uint8_t label[6],
                                      NetBurst *net_packets)
{
	const char *FUNCNAME = "[Gse::Context::deencapFixedLength]";
	gse_status_t status;
	NetPacket *packet = NULL;
	uint8_t src_tal_id, dst_tal_id;
	uint8_t qos;
	unsigned int pkt_nbr = 0;

	src_tal_id = Gse::getSrcTalIdFromLabel(label);
	dst_tal_id = Gse::getDstTalIdFromLabel(label);
	qos = Gse::getQosFromLabel(label);

	if(gse_get_vfrag_length(vfrag_pdu) %
	   this->current_upper->getFixedLength() != 0)
	{
		UTI_ERROR("%s Number of packets in GSE payload is not an integer,"
		          " drop packets\n", FUNCNAME);
		gse_free_vfrag(&vfrag_pdu);
		return false;
	}
	while(gse_get_vfrag_length(vfrag_pdu) > 0)
	{
		packet = this->current_upper->build(gse_get_vfrag_start(vfrag_pdu),
		                                    this->current_upper->getFixedLength(),
		                                    qos, src_tal_id, dst_tal_id);
		if(packet == NULL)
		{
			UTI_ERROR("%s cannot build a %s packet, drop the GSE packet\n",
			          FUNCNAME, this->current_upper->getName().c_str());
			gse_free_vfrag(&vfrag_pdu);
			// move the data pointer after the current packet
			status = gse_shift_vfrag(vfrag_pdu,
			                         this->current_upper->getFixedLength(), 0);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s cannot shift virtual fragment (%s), drop the "
				          "GSE packet\n", FUNCNAME, gse_get_status(status));
				gse_free_vfrag(&vfrag_pdu);
				return false;
			}
			continue;
		}

		// set the destination spot ID
		packet->setDstSpot(dest_spot);
		// add network packet to burst
		net_packets->add(packet);
		pkt_nbr++;

		// move the data pointer after the current packet
		status = gse_shift_vfrag(vfrag_pdu,
		                         this->current_upper->getFixedLength(), 0);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s cannot shift virtual fragment (%s), drop the "
			          "GSE packet\n", FUNCNAME, gse_get_status(status));
			gse_free_vfrag(&vfrag_pdu);
			return false;
		}
		pkt_nbr++;
	}

	UTI_DEBUG("%s Complete PDU received, got %u %d-byte %s packet(s)/frame "
	          "(GSE packet length = %d, Src TAL id = %u, Dst TAL id = %u, qos = %u)\n",
	          FUNCNAME, pkt_nbr, packet->getTotalLength(),
	          packet->getName().c_str(),
	          gse_get_vfrag_length(vfrag_pdu),
	          src_tal_id, dst_tal_id, qos);

	gse_free_vfrag(&vfrag_pdu);

	return true;
}

bool Gse::Context::deencapVariableLength(gse_vfrag_t *vfrag_pdu,
                                         uint16_t dest_spot,
                                         uint8_t label[6],
                                         NetBurst *net_packets)
{
	const char *FUNCNAME = "[Gse::Context::deencapVariableLength]";
	NetPacket *packet = NULL;
	uint8_t src_tal_id, dst_tal_id;
	uint8_t qos;
	unsigned int pkt_nbr = 0;

	src_tal_id = Gse::getSrcTalIdFromLabel(label);
	dst_tal_id = Gse::getDstTalIdFromLabel(label);
	qos = Gse::getQosFromLabel(label);

	packet = this->current_upper->build(gse_get_vfrag_start(vfrag_pdu),
	                                    gse_get_vfrag_length(vfrag_pdu),
	                                    qos, src_tal_id, dst_tal_id);
	if(packet == NULL)
	{
		UTI_ERROR("%s cannot build a %s packet, drop the GSE packet\n",
		          FUNCNAME, this->current_upper->getName().c_str());
		gse_free_vfrag(&vfrag_pdu);
		return false;
	}

	// set the destination spot ID
	packet->setDstSpot(dest_spot);
	// add network packet to burst
	net_packets->add(packet);
	pkt_nbr++;

	UTI_DEBUG("%s Complete PDU received, got %u %d-byte %s packet(s)/frame "
	          "(GSE packet length = %d, Src TAL id = %u, Dst TAL id = %u, qos = %u)\n",
	          FUNCNAME, pkt_nbr, packet->getTotalLength(),
	          packet->getName().c_str(),
	          gse_get_vfrag_length(vfrag_pdu),
	          src_tal_id, dst_tal_id, qos);

	gse_free_vfrag(&vfrag_pdu);

	return true;
}

NetBurst *Gse::Context::flush(int context_id)
{
	const char *FUNCNAME = "[Gse::Context::flush]";

	gse_status_t status;
	NetBurst *gse_packets;
	GseEncapCtx *context;
	GseIdentifier *identifier;
	GseIdentifier *ctx_id;
	std::map <GseIdentifier *, GseEncapCtx *,
	          ltGseIdentifier >::iterator context_it;
	gse_vfrag_t *vfrag_pkt;
	gse_vfrag_t *vfrag_gse;
	uint8_t label[6];
	std::string packet_name;
	uint16_t protocol;
	size_t ctx_length;
	uint8_t src_tal_id, dst_tal_id;
	uint8_t qos;
	uint8_t frag_id;
	uint16_t dest_spot;
	unsigned int counter;

	// create an empty burst of GSE packets
	gse_packets = new NetBurst();
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of GSE packets\n",
		          FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s search for encapsulation context (id = %d) to flush...\n",
			  FUNCNAME, context_id);
	identifier = new GseIdentifier((context_id >> 6) & 0x7f,
	                               (context_id >> 3) & 0x07,
	                               context_id & 0x07);
	UTI_DEBUG("%s Associated identifier: Src TAL Id = %u, Dst TAL Id = %u, QoS = %u\n",
	          FUNCNAME, identifier->getSrcTalId(), identifier->getDstTalId(),
	          identifier->getQos());
	context_it = this->contexts.find(identifier);
	if(context_it == this->contexts.end())
	{
		UTI_ERROR("%s encapsulation context does not exist\n", FUNCNAME);
		delete identifier;
		goto erase_burst;
	}
	else
	{
		context = (*context_it).second;
		UTI_DEBUG("%s find an encapsulation context containing %u "
		          "bytes of data\n", FUNCNAME, context->length());
		delete identifier;
	}

	// Duplicate context virtual fragment before giving it to GSE library
	// (otherwise the GSE library will destroy it after use) and delete context.
	// Context shall be deleted otherwise there will be two accesses in
	// the virtual buffer (vfrag_pkt and context->vfrag), thus get_packet
	// could not be called (indeed, there can't be more than two accesses
	// in the same virtual buffer to avoid data modifications in other
	// packets). Another solution could have been to call get_packet_copy
	// but it is less efficient.
	packet_name = context->getPacketName();
	protocol = context->getProtocol();
	ctx_length = context->length();
	src_tal_id = context->getSrcTalId();
	dst_tal_id = context->getDstTalId();
	qos = context->getQos();
	status = gse_duplicate_vfrag(&vfrag_pkt, context->data(),
	                             context->length());
	// keep the destination spot
	dest_spot = context->getDestSpot(); 
	delete context;
	ctx_id = (*context_it).first;
	this->contexts.erase((*context_it).first);
	delete ctx_id;
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Fail to duplicated context data (%s), drop packets\n",
		          FUNCNAME, gse_get_status(status));
		goto erase_burst;
	}

	if((src_tal_id & 0x1f) != src_tal_id)
	{
		UTI_ERROR("Be careful, you have set a source TAL ID greater than 0x1f,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	if((dst_tal_id & 0x1f) != dst_tal_id)
	{
		UTI_ERROR("Be careful, you have set a destination TAL ID greater than 0x1f,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	if((qos & 0x7) != qos)
	{
		UTI_ERROR("Be careful, you have set a QoS greater than 0x7,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	// Set packet label
	if(!Gse::setLabel(context, label))
	{
		UTI_ERROR("%s Cannot set label for GSE packet\n", FUNCNAME);
		goto drop;
	}

	// Get the frag Id
	frag_id = Gse::getFragId(context);

	// Store the IP packet in the encapsulation context thanks to the GSE library
	status = gse_encap_receive_pdu(vfrag_pkt, this->encap, label, 0,
	                               protocol, frag_id);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Fail to store packet in GSE encapsulation context (%s), "
		          "drop packet\n", FUNCNAME, gse_get_status(status));
		goto erase_burst;
	}

	counter = 0;
	do
	{
		counter++;
		status = gse_encap_get_packet_copy(&vfrag_gse, this->encap,
		                                   GSE_MAX_PACKET_LENGTH, frag_id);
		if(status != GSE_STATUS_OK && status != GSE_STATUS_FIFO_EMPTY)
		{
			UTI_ERROR("%s Fail to get GSE packet #%d in encapsulation context "
			          "(%s), drop packet\n",
			          FUNCNAME, counter, gse_get_status(status));
			goto clean;
		}

		if(status == GSE_STATUS_OK)
		{
			NetPacket *gse;
			gse = this->createPacket(gse_get_vfrag_start(vfrag_gse),
			                         gse_get_vfrag_length(vfrag_gse),
			                         qos, src_tal_id, dst_tal_id);
			// create a GSE packet from fragments computed by the GSE library
			if(gse == NULL)
			{
				UTI_ERROR("%s cannot create GSE packet, drop the network "
				          "packet\n", FUNCNAME);
				goto clean;
			}

			// set the destination spot ID
			gse->setDstSpot(dest_spot);
			// add GSE packet to burst
			gse_packets->add(gse);
			UTI_DEBUG("%s %d-byte GSE packet added to burst\n", FUNCNAME,
			          gse->getTotalLength());

			status = gse_free_vfrag(&vfrag_gse);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Fail to free GSE fragment #%u (%s), "
				          "drop packet\n", FUNCNAME, counter,
				          gse_get_status(status));
				goto clean;
			}
		}
	}
	while(status != GSE_STATUS_FIFO_EMPTY && !gse_packets->isFull());
	UTI_DEBUG("%s %d-byte %s packet/frame => %u GSE packets\n",
	          FUNCNAME, ctx_length, packet_name.c_str(),
	          counter);

	return gse_packets;

clean:
	if(vfrag_gse != NULL)
	{
		status = gse_free_vfrag(&vfrag_gse);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s failed to free GSE virtual fragment (%s)\n",
			          FUNCNAME, gse_get_status(status));
		}
	}
erase_burst:
	delete gse_packets;
drop:
	return NULL;
}

NetBurst *Gse::Context::flushAll()
{
	UTI_DEBUG("[Gse::Context::flushAll]");
	//TODO
	return NULL;
}

NetPacket *Gse::PacketHandler::build(unsigned char *data, size_t data_length,
                                     uint8_t UNUSED(_qos),
                                     uint8_t UNUSED(_src_tal_id), uint8_t UNUSED(_dst_tal_id))
{
	const char *FUNCNAME = "[Gse::PacketHandler::build]";
	gse_status_t status;
	uint8_t label[6];
	uint8_t s;
	uint8_t e;
	uint8_t label_length = 6;

	uint8_t qos;
	uint8_t src_tal_id = BROADCAST_TAL_ID;
	uint8_t dst_tal_id = BROADCAST_TAL_ID;
	uint8_t frag_id;
	uint16_t header_length = 0;

	status = gse_get_start_indicator(data, &s);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot get start indicator (%s)\n", FUNCNAME,
		          gse_get_status(status));
		return NULL;
	}

	status = gse_get_end_indicator(data, &e);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot get end indicator (%s)\n", FUNCNAME,
		          gse_get_status(status));
		return NULL;
	}

	// subsequent fragment
	if(s == 0)
	{
		status = gse_get_frag_id(data, &frag_id);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s cannot get frag ID (%s)\n", FUNCNAME,
			          gse_get_status(status));
			return NULL;
		}
		qos = Gse::getQosFromFragId(frag_id);
		src_tal_id = Gse::getSrcTalIdFromFragId(frag_id);
		UTI_DEBUG_L3("%s build a subsequent fragment "
		             "SRC TAL Id = %u, QoS = %u\n",
		             FUNCNAME,
		             src_tal_id, qos);
		header_length = 2 + //GSE_MANDATORY_FIELDS_LENGTH +
		                1 + //GSE_FRAG_ID_LENGTH +
		                label_length;
	}
	// complete or first fragment
	else
	{
		status = gse_get_label(data, label);
		if(status != GSE_STATUS_OK)
		{
			UTI_ERROR("%s cannot get label (%s)\n", FUNCNAME,
			          gse_get_status(status));
			return NULL;
		}
		qos = Gse::getQosFromLabel(label);
		src_tal_id = Gse::getSrcTalIdFromLabel(label);
		dst_tal_id = Gse::getDstTalIdFromLabel(label);

		// first fragment
		if(e == 0)
		{
			UTI_DEBUG_L3("%s build a first fragment\n", FUNCNAME);
			header_length = 2 + //GSE_MANDATORY_FIELDS_LENGTH
			                1 + //GSE_FRAG_ID_LENGTH +
			                2 + //GSE_TOTAL_LENGTH_LENGTH +
			                label_length;
		}
		// complete
		else
		{
			UTI_DEBUG_L3("%s build a complete packet\n", FUNCNAME);
			header_length = 2 + //GSE_MANDATORY_FIELDS_LENGTH +
			                label_length;
		}
		UTI_DEBUG("%s build a new %u-bytes GSE packet: QoS = %u, Src Tal ID = %u, "
		          "Dst TAL ID = %u, header length = %u\n", FUNCNAME, data_length,
		          qos, src_tal_id, dst_tal_id, header_length);
	}

	return new NetPacket(data, data_length,
	                     this->getName(), this->getEtherType(),
	                     qos, src_tal_id, dst_tal_id, header_length);
}

size_t Gse::PacketHandler::getLength(const unsigned char *data)
{
	const char *FUNCNAME = "[Gse::PacketHandler::getLength]";
	uint16_t length;
	gse_status_t status;

	status = gse_get_gse_length((unsigned char *)data, &length);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot get length (%s)\n", FUNCNAME,
		          gse_get_status(status));
		return 0;
	}
	// Add 2 bits for S, E and LT fields
	length += 2; //GSE_MANDATORY_FIELDS_LENGTH;
	return length;
}

bool Gse::PacketHandler::getChunk(NetPacket *packet, size_t remaining_length,
                                  NetPacket **data, NetPacket **remaining_data)
{
	const char *FUNCNAME = "[Gse::PacketHandler::getChunk]";
	gse_vfrag_t *first_frag;
	gse_vfrag_t *second_frag;
	gse_status_t status;
	uint8_t frag_id;

	// initialize data and remaining_data
	*data = NULL;
	*remaining_data = NULL;

	frag_id = Gse::getFragId(packet);

	UTI_DEBUG_L3("%s Create a virtual fragment with GSE packet to "
	             "refragment it\n", FUNCNAME);
	status = gse_create_vfrag_with_data(&first_frag,
	                                    packet->getTotalLength(),
	                                    GSE_MAX_REFRAG_HEAD_OFFSET, 0,
	                                    (unsigned char *)packet->getData().c_str(),
	                                    packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Failed to create a virtual fragment for the GSE packet "
		          "refragmentation (%s)\n", FUNCNAME, gse_get_status(status));
		goto error;
	}

	UTI_DEBUG_L3("%s Refragment the GSE packet to fit the BB frame "
	             "(length = %d)\n", FUNCNAME, remaining_length);
	status = gse_refrag_packet(first_frag, &second_frag, 0, 0,
	                           frag_id,
	                           MIN(remaining_length, GSE_MAX_PACKET_LENGTH));
	if(status == GSE_STATUS_LENGTH_TOO_SMALL)
	{
		// there is not enough space to create a GSE fragment,

		UTI_DEBUG("%s Unable to refragment GSE packet (%s)\n",
		          FUNCNAME, gse_get_status(status));
		// the packet cannot be encapsulated, copy it on data but return false
		// (use case 3)
		*remaining_data = packet;
		goto keep;
	}
	else if(status == GSE_STATUS_REFRAG_UNNECESSARY)
	{
		UTI_DEBUG_L3("%s no need to refragment, the whole packet can be "
		             "encapsulated\n", FUNCNAME);
		// the whole packet can be encapsulated, copy it in data and return true
		// (use case 1)
		*data = packet;
		goto keep;
	}
	else if(status == GSE_STATUS_OK)
	{
		// the packet has been fragmented in order to be encapsulated partially
		// (use case 2)

		UTI_DEBUG("%s packet has been refragmented, first fragment is "
		          "%u bytes long, second fragment is %u bytes long\n",
		          FUNCNAME, gse_get_vfrag_length(first_frag),
		          gse_get_vfrag_length(second_frag));
		// add the first fragment to the BB frame
		*data = this->build(gse_get_vfrag_start(first_frag),
		                    gse_get_vfrag_length(first_frag),
		                    packet->getQos(),
		                    packet->getSrcTalId(), packet->getDstTalId());
		if(*data == NULL)
		{
			UTI_ERROR("%s failed to create the first fragment\n", FUNCNAME);
			goto free;
		}
		
		// create a new NetPacket containing the second fragment
		*remaining_data = this->build(gse_get_vfrag_start(second_frag),
		                              gse_get_vfrag_length(second_frag),
		                              packet->getQos(),
		                              packet->getSrcTalId(), packet->getDstTalId());
		if(*remaining_data == NULL)
		{
			UTI_ERROR("%s failed to create the second fragment\n", FUNCNAME);
			goto free_data;
		}
	}
	else
	{
		UTI_ERROR("%s Failed to refragment GSE packet (%s)\n",
		          FUNCNAME, gse_get_status(status));
		goto error;
	}

	if(second_frag != NULL)
	{
		gse_free_vfrag(&second_frag);
	}
	gse_free_vfrag(&first_frag);
	delete packet;
	return true;

keep:
	if(second_frag != NULL)
	{
		gse_free_vfrag(&second_frag);
	}
	gse_free_vfrag(&first_frag);
	return true;

free_data:
	delete *data;
free:
	gse_free_vfrag(&first_frag);
	gse_free_vfrag(&second_frag);
error:
	delete packet;
	return false;
}

Gse::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
}

// Static methods

bool Gse::setLabel(NetPacket *packet, uint8_t label[])
{
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t dst_tal_id = packet->getDstTalId();
	uint8_t qos = packet->getQos();

	if(((src_tal_id & 0x1F) != src_tal_id)
	   || ((dst_tal_id & 0x1F) != dst_tal_id)
	   || ((qos & 0x07) != qos))
	{
		// Value to big to be encoded in label
		return false;
	}

	label[0] = src_tal_id & 0x1F;
	label[1] = dst_tal_id & 0x1F;
	label[2] = qos & 0x07;

	return true;
}

bool Gse::setLabel(GseEncapCtx *context, uint8_t label[])
{
	uint8_t src_tal_id = context->getSrcTalId();
	uint8_t dst_tal_id = context->getDstTalId();
	uint8_t qos = context->getQos();

	if(((src_tal_id & 0x1F) != src_tal_id)
	   || ((dst_tal_id & 0x1F) != dst_tal_id)
	   || ((qos & 0x07) != qos))
	{
		// Value to big to be encoded in label
		return false;
	}

	label[0] = src_tal_id & 0x1F;
	label[1] = dst_tal_id & 0x1F;
	label[2] = qos & 0x07;

	return true;
}

uint8_t Gse::getSrcTalIdFromLabel(uint8_t label[])
{
	return label[0] & 0x1F;
}

uint8_t Gse::getDstTalIdFromLabel(uint8_t label[])
{
	return label[1] & 0x1F;
}

uint8_t Gse::getQosFromLabel(uint8_t label[])
{
	return label[2] & 0x07;
}

uint8_t Gse::getFragId(NetPacket *packet)
{
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t qos = packet->getQos();

	return ((src_tal_id & 0x1F) << 3 | ((qos & 0x07)));
}

uint8_t Gse::getFragId(GseEncapCtx *context)
{
	uint8_t src_tal_id = context->getSrcTalId();
	uint8_t qos = context->getQos();

	return ((src_tal_id & 0x1F) << 3 | ((qos & 0x07)));
}

uint8_t Gse::getSrcTalIdFromFragId(uint8_t frag_id)
{
	
	return (frag_id >> 3) & 0x1F;
}

uint8_t Gse::getDstTalIdFromFragId(uint8_t UNUSED(frag_id))
{
	// Not encoded in frag_id
	return 0x1F;
}

uint8_t Gse::getQosFromFragId(uint8_t frag_id)
{
	return frag_id & 0x07;
}
