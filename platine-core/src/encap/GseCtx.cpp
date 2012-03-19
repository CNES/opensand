/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file GseCtx.cpp
 * @brief GSE encapsulation / deencapsulation context
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#include "GseCtx.h"
#include "RohcPacket.h"

#undef DBG_PACKAGE
#define DBG_PACKAGE PKG_ENCAP
#include "platine_conf/uti_debug.h"
#include <vector>
#include <map>
#include <assert.h>

GseCtx::GseCtx(int qos_nbr,
               unsigned int packing_threshold,
               unsigned int packet_length):
  EncapCtx(), contexts()
{
	const char *FUNCNAME = "[GseCtx::GseCtx]";

	gse_status_t status;

	this->packing_threshold = packing_threshold;
	this->packet_length = packet_length;

	// Initialize encapsulation and deencapsulation contexts
	status = gse_encap_init(qos_nbr, 1, &this->encap);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot init GSE encapsulation context (%s)\n",
		          FUNCNAME, gse_get_status(status));
		goto error;
	}
	status = gse_deencap_init(qos_nbr, &this->deencap);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot init GSE deencapsulation context (%s)\n",
		          FUNCNAME, gse_get_status(status));
		goto release_encap;
	}

	return;

release_encap:
	status = gse_encap_release(this->encap);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot release GSE encapsulation context (%s)\n",
		          FUNCNAME, gse_get_status(status));
	}
error:
	this->encap = NULL;
	this->deencap = NULL;
}

GseCtx::~GseCtx()
{
	const char *FUNCNAME = "[GseCtx::~GseCtx]";

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

NetBurst *GseCtx::encapsulate(NetPacket *packet,
                              int &context_id,
                              long &time)
{
	const char *FUNCNAME = "[GseCtx::encapsulate]";
	NetBurst *gse_packets;
	std::vector<GsePacket *> gse_packet_vec;
	std::map <GseIdentifier *, GseEncapCtx *,
	          ltGseIdentifier >::iterator context_it;
	GseEncapCtx *context = NULL;
	gse_vfrag_t *vfrag_pkt;
	gse_vfrag_t *vfrag_gse;
	gse_status_t status;
	uint8_t label[6];
	GseIdentifier *identifier;
	GseIdentifier *ctx_id;

	time = 0;
	//TODO return talId, macId and QoS instead of a PID (it has no sense for GSE)
	context_id = ((packet->macId() & 0x7f) << 6) +
	             ((packet->talId() & 0x07) << 3) +
	             (packet->qos() & 0x07);

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s packet is not valid, drop the packet\n", FUNCNAME);
		goto drop;
	}

	UTI_DEBUG("%s encapsulate a %d-byte packet of type 0x%04x with QoS %d\n", FUNCNAME,
	          packet->totalLength(), packet->type(), packet->qos());

	// the GSE encapsulation context must exist
	if(this->encap == NULL)
	{
		UTI_ERROR("%s GSE encapsulation context unexisting, drop packet\n", FUNCNAME);
		goto drop;
	}

	switch(packet->type())
	{
		case(NET_PROTO_MPEG):
		case(NET_PROTO_ATM):
			if(packet->totalLength() != this->packet_length)
			{
				UTI_ERROR("%s Bad packet length (%d), drop packet\n",
				          FUNCNAME, this->packet_length);
				goto drop;
			}

			identifier = new GseIdentifier(packet->talId(),
			                               packet->macId(),
			                               packet->qos());
			UTI_DEBUG("%s check if encapsulation context exists\n", FUNCNAME);
			context_it = this->contexts.find(identifier);
			if(context_it == this->contexts.end())
			{
				UTI_DEBUG("%s encapsulation context does not exist yet\n", FUNCNAME);
				context = new GseEncapCtx(identifier);
				this->contexts.insert(std::pair <GseIdentifier *, GseEncapCtx *> (identifier, context));
				UTI_DEBUG("%s new encapsulation context created, "
				         "MAC Id = %lu, TAL Id = %ld, QoS = %d\n",
				         FUNCNAME, context->macId(), context->talId(), context->qos());
			}
			else
			{
				context = (*context_it).second;
				UTI_DEBUG("%s find an encapsulation context containing %u bytes of data\n",
				          FUNCNAME, context->length());
				delete identifier;
			}

			// add the packet in context
			status = context->add(packet);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Error when adding packet in context (%s), drop packet\n",
				          FUNCNAME, gse_get_status(status));
				goto drop;
			}

			UTI_DEBUG("%s Packet now entirely packed into GSE context, "
			          "context contains %d bytes\n", FUNCNAME, context->length());

			// if there is enough space in buffer for another MPEG/ATM packet or if
			// packing_threshold is not 0 keep data in the virtual buffer
			if((!context->isFull()) && this->packing_threshold != 0)
			{
				UTI_DEBUG("%s enough unused space in virtual buffer for packing "
				          "=> keep the packets %ld ms\n",
				          FUNCNAME, this->packing_threshold);

				time = this->packing_threshold;

				goto wait;
			}

			// Duplicate context virtual fragment before giving it to GSE library
			// (otherwise the GSE library will destroy it after use) and delete context.
			// Context shall be deleted otherwise there will be two accesses in
			// the virtual buffer (vfrag_pkt and context->vfrag), thus get_packet
			// could not be called (indeed, there can't be more than two accesses
			// in the same virtual buffer to avoid data modifications in other
			// packets). Another solution could have been to call get_packet_copy
			// but it is less efficient.
			status = gse_duplicate_vfrag(&vfrag_pkt,
			                             context->data(),
			                             context->length());
			delete context;
			ctx_id = (*context_it).first;
			this->contexts.erase((*context_it).first);
			delete ctx_id;
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Fail to duplicated context data (%s), drop packet\n",
				          FUNCNAME, gse_get_status(status));
				goto drop;
			}
			break;

		case(NET_PROTO_IPV4):
		case(NET_PROTO_IPV6):
		case(NET_PROTO_ROHC):
			// Create a virtual fragment containing the packet
			status = gse_create_vfrag_with_data(&vfrag_pkt, packet->totalLength(),
			                                    GSE_MAX_HEADER_LENGTH,
			                                    GSE_MAX_TRAILER_LENGTH,
			                                    (unsigned char *)packet->data().c_str(),
			                                    packet->totalLength());
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Virtual fragment creation failed (%s), drop packet\n",
				          FUNCNAME, gse_get_status(status));
				goto drop;
			}
			break;

	default:
			// packet must be IPv4, IPv6, MPEG, ATM or ROHC
			UTI_ERROR("%s packet type (%d) is invalid, drop the packet\n",
			          FUNCNAME, packet->type());
			goto drop;
	}

	// Common part for all packet types
	if((packet->macId() & 0x00ff) != packet->macId())
	{
		UTI_ERROR("Be careful, you have set a MAC ID (satellite spot id) greater"
		           " than 0x00ff, it will be truncated for GSE packet creation!!!\n");
	}
	if((packet->talId() & 0x1fff) != packet->talId())
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 0x1fff,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	if((packet->qos() & 0x7) != packet->qos())
	{
		UTI_ERROR("Be careful, you have set a QoS greater than 0x7,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	// label (6 * 8 bits) = MAC id (8 bits) | QoS (3 bits) TAL id (5 MSB) |
	// TAL id (8 LSB) | 0 (3 bytes)
	label[0] = packet->macId();
	label[1] = ((packet->qos() & 0x7) << 5) | ((packet->talId() >> 8) & 0x1f);
	label[2] = packet->talId() & 0xff;

	// Store the IP packet in the encapsulation context thanks to the GSE library
	status = gse_encap_receive_pdu(vfrag_pkt, this->encap, label, 0,
	                               packet->type(), packet->qos());
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Fail to store packet in GSE encapsulation context (%s), "
		          "drop packet\n", FUNCNAME, gse_get_status(status));
		goto drop;
	}

	// create an empty burst of GSE packets
	gse_packets = new NetBurst();
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of GSE packets\n",
		          FUNCNAME);
		goto clean;
	}

	do
	{
		status = gse_encap_get_packet(&vfrag_gse, this->encap,
		                              GSE_MAX_PACKET_LENGTH, packet->qos());
		if(status != GSE_STATUS_OK && status != GSE_STATUS_FIFO_EMPTY)
		{
			UTI_ERROR("%s Fail to get GSE packet #%d in encapsulation context (%s), "
			          "drop packet\n",
			          FUNCNAME, gse_packet_vec.size(), gse_get_status(status));
			goto clean;
		}

		if(status == GSE_STATUS_OK)
		{
			// create a GSE packet from fragments computed by the GSE library
			gse_packet_vec.push_back(new GsePacket(gse_get_vfrag_start(vfrag_gse),
			                                       gse_get_vfrag_length(vfrag_gse)));
			if(gse_packet_vec.back() == NULL)
			{
				UTI_ERROR("%s cannot create GSE packet, drop the network packet\n",
				          FUNCNAME);
				goto clean;
			}

			// copy some parameters
			gse_packet_vec.back()->setMacId(packet->macId());
			gse_packet_vec.back()->setTalId(packet->talId());
			gse_packet_vec.back()->setQos(packet->qos());

			// add GSE packet to burst
			gse_packets->add(gse_packet_vec.back());
			UTI_DEBUG("%s %d-byte GSE packet added to burst\n", FUNCNAME,
			          (gse_packet_vec.back())->totalLength());

			status = gse_free_vfrag(&vfrag_gse);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Fail to free GSE fragment #%d (%s), drop packet\n",
				          FUNCNAME, gse_packet_vec.size(), gse_get_status(status));
				goto clean;
			}
		}
	}
	while(status != GSE_STATUS_FIFO_EMPTY && !gse_packets->isFull());
	UTI_DEBUG("%s %d-byte %s packet/frame => %d GSE packets\n",
	          FUNCNAME, packet->totalLength(), packet->name().c_str(),
	          gse_packet_vec.size());

	return gse_packets;

clean:
	gse_packet_vec.clear();
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
	return NULL;
wait:
	gse_packets = new NetBurst();
	if(gse_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of GSE packets\n",
		          FUNCNAME);
	}
	return gse_packets;
}

NetBurst *GseCtx::desencapsulate(NetPacket *packet)
{
	const char *FUNCNAME = "[GseCtx::desencapsulate]";
	NetPacket *net_packet = NULL;
	NetBurst *net_packets;
	gse_vfrag_t *vfrag_gse;
	gse_vfrag_t *vfrag_pdu;
	uint8_t label_type;
	uint8_t label[6];
	uint16_t protocol;
	uint16_t packet_length;
	unsigned int pkt_nbr = 0;
	gse_status_t status;

	// packet must be valid
	if(packet == NULL || !packet->isValid())
	{
		UTI_ERROR("%s encapsulation packet is not valid, drop the packet\n",
		          FUNCNAME);
		goto drop;
	}

	// packet must be a GSE packet
	if(packet->type() != NET_PROTO_GSE)
	{
		UTI_ERROR("%s encapsulation packet is not a GSE packet (type = 0x%04x), "
		          "drop the packet\n", FUNCNAME, packet->type());
		goto drop;
	}

	// the GSE deencapsulation context must exist
	if(this->deencap == NULL)
	{
		UTI_ERROR("%s GSE deencapsulation context does not exist, drop packet\n",
		          FUNCNAME);
		goto drop;
	}

	// Create a virtual fragment containing the GSE packet
	status = gse_create_vfrag_with_data(&vfrag_gse, packet->totalLength(),
	                                    0, 0,
	                                    (unsigned char *)packet->payload().c_str(),
	                                    packet->totalLength());
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Virtual fragment creation failed (%s), drop packet\n",
		          FUNCNAME, gse_get_status(status));
		goto drop;
	}
	else
	{
		UTI_DEBUG("%s Create a virtual fragment for GSE library (length = %d)\n",
		          FUNCNAME, packet->totalLength());
	}

	// create an empty burst of network packets
	net_packets = new NetBurst();
	if(net_packets == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for burst of network packets\n",
		          FUNCNAME);
		goto drop;
	}

	// deencapsulate the GSE packet thanks to the GSE library
	status = gse_deencap_packet(vfrag_gse, this->deencap, &label_type, label,
	                            &protocol, &vfrag_pdu, &packet_length);
	switch(status)
	{
		case GSE_STATUS_OK:
			UTI_DEBUG("%s GSE packet deencapsulated, Gse packet length = %u;"
			          "PDU is not complete\n",
			          FUNCNAME, packet_length);
			break;

		case GSE_STATUS_DATA_OVERWRITTEN:
			UTI_DEBUG("%s GSE packet deencapsulated, GSE Length = %u (%s);"
			          "PDU is not complete\n",
			          FUNCNAME, packet_length, gse_get_status(status));
			break;

		case GSE_STATUS_PADDING_DETECTED:
			UTI_DEBUG("%s %s\n", FUNCNAME, gse_get_status(status));
			break;

		case GSE_STATUS_PDU_RECEIVED:
		{
			int macId = label[0];
			int talId = ((label[1] & 0x1f) << 8) | (label[2] & 0xff);
			int qos = (label[1] >> 5) & 0x7;

			// create network packet according to type
			if(this->type().find("GSE/ROHC") != std::string::npos)
			{
				UTI_DEBUG("%s GSE payload is ROHC packet\n", FUNCNAME);
				net_packet = new RohcPacket(gse_get_vfrag_start(vfrag_pdu),
				                            gse_get_vfrag_length(vfrag_pdu));
				if(net_packet == NULL)
				{
					UTI_ERROR("%s cannot create ROHC packet, "
					          "drop the GSE packet\n", FUNCNAME);
					goto free_vfrag_pdu;
				}
				// copy some parameters
				net_packet->setMacId(macId);
				net_packet->setTalId(talId);
				net_packet->setQos(qos);

				// add network packet to burst
				net_packets->push_back(net_packet);
				pkt_nbr++;
			}
			else
			{
			switch(protocol)
			{
				case NET_PROTO_IPV4:
					if(IpPacket::version(gse_get_vfrag_start(vfrag_pdu),
					                     gse_get_vfrag_length(vfrag_pdu)) != 4)
					{
						UTI_ERROR("%s IP version (%d) does not correspond to protocol (%u),"
						          " drop packet\n", FUNCNAME,
						          IpPacket::version(gse_get_vfrag_start(vfrag_pdu),
						                            gse_get_vfrag_length(vfrag_pdu)),
						                            protocol);
						goto free_vfrag_pdu;
					}
					net_packet = new Ipv4Packet(gse_get_vfrag_start(vfrag_pdu),
					                            gse_get_vfrag_length(vfrag_pdu));
					if(net_packet == NULL)
					{
						UTI_ERROR("%s cannot create IPv4 packet, drop the GSE packet\n",
						          FUNCNAME);
						goto free_vfrag_pdu;
					}
					// copy some parameters
					net_packet->setMacId(macId);
					net_packet->setTalId(talId);
					net_packet->setQos(qos);

					// add network packet to burst
					net_packets->push_back(net_packet);
					pkt_nbr++;
					break;

				case NET_PROTO_IPV6:
					if(IpPacket::version(gse_get_vfrag_start(vfrag_pdu),
					                     gse_get_vfrag_length(vfrag_pdu)) != 6)
					{
						UTI_ERROR("%s IP version (%d) does not correspond to protocol (%u),"
						          " drop packet\n", FUNCNAME,
						          IpPacket::version(gse_get_vfrag_start(vfrag_pdu),
						                            gse_get_vfrag_length(vfrag_pdu)),
						          protocol);
						goto free_vfrag_pdu;
					}
					net_packet = new Ipv6Packet(gse_get_vfrag_start(vfrag_pdu),
					                            gse_get_vfrag_length(vfrag_pdu));
					if(net_packet == NULL)
					{
						UTI_ERROR("%s cannot create IPv6 packet, drop the GSE packet\n",
						          FUNCNAME);
						goto free_vfrag_pdu;
					}
					// copy some parameters
					net_packet->setMacId(macId);
					net_packet->setTalId(talId);
					net_packet->setQos(qos);

					// add network packet to burst
					net_packets->push_back(net_packet);
					pkt_nbr++;
					break;

/*				case NET_PROTO_ROHC:
					net_packet = new RohcPacket(gse_get_vfrag_start(vfrag_pdu),
					                            gse_get_vfrag_length(vfrag_pdu));
					if(net_packet == NULL)
					{
						UTI_ERROR("%s cannot create ROHC packet, "
						          "drop the GSE packet\n", FUNCNAME);
						goto free_vfrag_pdu;
					}
					// copy some parameters
					net_packet->setMacId(macId);
					net_packet->setTalId(talId);
					net_packet->setQos(qos);

					// add network packet to burst
					net_packets->push_back(net_packet);
					pkt_nbr++;
					break;*/


				case NET_PROTO_ATM:
				case NET_PROTO_MPEG:
					if(gse_get_vfrag_length(vfrag_pdu) % this->packet_length != 0)
					{
						UTI_ERROR("%s Number of packets in GSE payload is not an integer,"
						          " drop packets\n", FUNCNAME);
					}
					while(gse_get_vfrag_length(vfrag_pdu) > 0)
					{
						if(protocol == NET_PROTO_ATM)
						{
							net_packet = new AtmCell(gse_get_vfrag_start(vfrag_pdu),
							                         this->packet_length);
						}
						else if(protocol == NET_PROTO_MPEG)
						{
							net_packet = new MpegPacket(gse_get_vfrag_start(vfrag_pdu),
							                            this->packet_length);
						}
						else
						{
							UTI_ERROR("%s GSE payload type is not supported (0x%04x)\n",
							          FUNCNAME, protocol);
						}
						if(net_packet == NULL)
						{
							UTI_ERROR("%s cannot create ATM or MPEG packet, "
							          "drop the GSE packet\n",
							          FUNCNAME);
							goto free_vfrag_pdu;
						}
						// copy some parameters
						net_packet->setMacId(macId);
						net_packet->setTalId(talId);
						net_packet->setQos(qos);

						// add network packet to burst
						net_packets->push_back(net_packet);
						status = gse_shift_vfrag(vfrag_pdu, this->packet_length, 0);
						if(status != GSE_STATUS_OK)
						{
							UTI_ERROR("%s cannot shift virtual fragment (%s), drop the "
							          "GSE packet\n",
							          FUNCNAME, gse_get_status(status));
							goto free_vfrag_pdu;
						}
						pkt_nbr++;
					}
					break;

				default:
					UTI_ERROR("%s unknown protocol (%u), drop packet\n", FUNCNAME, protocol);
					goto free_vfrag_pdu;
			}
			}

			UTI_DEBUG("%s Complete PDU received, got %u %d-byte %s packet(s)/frame "
			          "(GSE packet length = %d, MAC id = %d, TAL id = %d, qos = %d)\n",
			          FUNCNAME, pkt_nbr, net_packet->totalLength(),
			          net_packet->name().c_str(),
			          packet_length, macId, talId, qos);

			// Free PDU virtual buffer
			status = gse_free_vfrag(&vfrag_pdu);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s cannot free pdu virtual fragment (%s)\n",
				          FUNCNAME, gse_get_status(status));
			}
		}
		break;

		default:
			UTI_ERROR("%s GSE deencapsulation failed (%s), drop packet\n",
					      FUNCNAME, gse_get_status(status));
			goto clean;
	}

	return net_packets;

free_vfrag_pdu:
	status = gse_free_vfrag(&vfrag_pdu);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s cannot free pdu virtual fragment (%s)\n",
		          FUNCNAME, gse_get_status(status));
	}
clean:
	delete net_packets;
drop:
	return NULL;
}

std::string GseCtx::type()
{
	return std::string("GSE");
}

NetBurst *GseCtx::flush(int context_id)
{
	const char *FUNCNAME = "[GseCtx::flush]";

	gse_status_t status;
	NetBurst *gse_packets;
	GseEncapCtx *context;
	std::vector<GsePacket *> gse_packet_vec;
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
	long tal_id;
	unsigned long mac_id;
	int qos;


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
	identifier = new GseIdentifier((context_id >> 3) & 0x07,
	                               (context_id >> 6) & 0x7f,
	                               context_id & 0x07);
	UTI_DEBUG("%s Associated identifier: MacId = %lu, TalId = %ld, QoS = %d\n",
	          FUNCNAME, identifier->macId(), identifier->talId(), identifier->qos());
	context_it = this->contexts.find(identifier);
	if(context_it == this->contexts.end())
	{
		UTI_DEBUG("%s encapsulation context does not exist\n", FUNCNAME);
		delete identifier;
		goto erase_burst;
	}
	else
	{
		context = (*context_it).second;
		UTI_DEBUG("%s find an encapsulation context containing %u bytes of data\n",
		          FUNCNAME, context->length());
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
	packet_name = context->packetName();
	protocol = context->protocol();
	ctx_length = context->length();
	tal_id = context->talId();
	mac_id = context->macId();
	qos = context->qos();
	status = gse_duplicate_vfrag(&vfrag_pkt,
	                             context->data(),
	                             context->length());
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

	if((mac_id & 0x00ff) != mac_id)
	{
		UTI_ERROR("Be careful, you have set a MAC ID (satellite spot id) greater"
		           " than 0x00ff, it will be truncated for GSE packet creation!!!\n");
	}
	if((tal_id & 0x1fff) != tal_id)
	{
		UTI_ERROR("Be careful, you have set a TAL ID greater than 0x1fff,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	if((qos & 0x7) != qos)
	{
		UTI_ERROR("Be careful, you have set a QoS greater than 0x7,"
		          " it will be truncated for GSE packet creation!!!\n");
	}
	// label (6 * 8 bits) = MAC id (8 bits) | QoS (3 bits) TAL id (5 MSB) |
	// TAL id (8 LSB) | 0 (3 bytes)
	label[0] = mac_id;
	label[1] = ((qos & 0x7) << 5) | ((tal_id >> 8) & 0x1f);
	label[2] = tal_id & 0xff;

	// Store the IP packet in the encapsulation context thanks to the GSE library
	status = gse_encap_receive_pdu(vfrag_pkt, this->encap, label, 0,
	                               protocol, qos);
	if(status != GSE_STATUS_OK)
	{
		UTI_ERROR("%s Fail to store packet in GSE encapsulation context (%s), drop packet\n",
		          FUNCNAME, gse_get_status(status));
		goto erase_burst;
	}

	do
	{
		status = gse_encap_get_packet_copy(&vfrag_gse, this->encap,
		                                   GSE_MAX_PACKET_LENGTH, qos);
		if(status != GSE_STATUS_OK && status != GSE_STATUS_FIFO_EMPTY)
		{
			UTI_ERROR("%s Fail to get GSE packet #%d in encapsulation context (%s), "
			          "drop packet\n",
			          FUNCNAME, gse_packet_vec.size(), gse_get_status(status));
			goto clean;
		}

		if(status == GSE_STATUS_OK)
		{
			// create a GSE packet from fragments computed by the GSE library
			gse_packet_vec.push_back(new GsePacket(gse_get_vfrag_start(vfrag_gse),
			                                       gse_get_vfrag_length(vfrag_gse)));
			if(gse_packet_vec.back() == NULL)
			{
				UTI_ERROR("%s cannot create GSE packet, drop the network packet\n",
				          FUNCNAME);
				goto clean;
			}

			// copy some parameters
			gse_packet_vec.back()->setMacId(mac_id);
			gse_packet_vec.back()->setTalId(tal_id);
			gse_packet_vec.back()->setQos(qos);

			// add GSE packet to burst
			gse_packets->add(gse_packet_vec.back());
			UTI_DEBUG("%s %d-byte GSE packet added to burst\n", FUNCNAME,
		            (gse_packet_vec.back())->totalLength());

			status = gse_free_vfrag(&vfrag_gse);
			if(status != GSE_STATUS_OK)
			{
				UTI_ERROR("%s Fail to free GSE fragment #%d (%s), drop packet\n",
				          FUNCNAME, gse_packet_vec.size(), gse_get_status(status));
				goto clean;
			}
		}
	}
	while(status != GSE_STATUS_FIFO_EMPTY && !gse_packets->isFull());
	UTI_DEBUG("%s %d-byte %s packet/frame => %d GSE packets\n",
	          FUNCNAME, ctx_length, packet_name.c_str(),
	          gse_packet_vec.size());

	return gse_packets;

clean:
	gse_packet_vec.clear();
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

NetBurst *GseCtx::flushAll()
{
	UTI_DEBUG("[GseCtx::flushAll]");
	//TODO
	return NULL;
}
