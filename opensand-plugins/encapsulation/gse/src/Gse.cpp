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
 * @file Gse.cpp
 * @brief GSE encapsulation plugin implementation
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
 */


#include "Gse.h"
#include "GseEncapCtx.h"
#include <NetPacket.h>
#include <NetBurst.h>
#include <OpenSandModelConf.h>

#include <opensand_output/Output.h>

#include <utility>


constexpr uint16_t GSE_MIN_ETHER_TYPE = 1536;
constexpr uint8_t MAX_QOS_NBR = 0xFF;
constexpr std::size_t MAX_CNI_EXT_LEN = 6;
constexpr std::size_t GSE_MANDATORY_FIELDS_LENGTH = 2;
constexpr std::size_t GSE_FRAG_ID_LENGTH = 1;
constexpr std::size_t GSE_TOTAL_LENGTH_LENGTH = 2;


static int encodeHeaderCniExtensions(unsigned char *ext,
                                     size_t *length,  
                                     uint16_t *protocol_type, 
                                     uint16_t extension_type,
                                     void *opaque)
{
	uint32_t *cni = (uint32_t*)opaque;
	extension_type = htons(extension_type);

	memcpy(ext, cni, sizeof(uint32_t));
	*length = sizeof(uint32_t);
	
	// add the protocol type in extension
	memcpy(ext + *length, &extension_type, sizeof(uint16_t));
	*length += sizeof(uint16_t);
	
	// 0x0300 is necessary to indicate the extension size (6 bytes) 
	*protocol_type = to_underlying(NET_PROTO::GSE_EXTENSION_CNI) | 0x0300;
	
	return 0;
}

static int deencodeHeaderCniExtensions(unsigned char *ext,
                                       size_t *UNUSED(length),
                                       uint16_t *UNUSED(protocol_type), 
                                       uint16_t extension_type,
                                       void *opaque)
{
	uint16_t current_type;
	current_type = extension_type & 0xFF;
	
	//check current type
	if(current_type != to_underlying(NET_PROTO::GSE_EXTENSION_CNI))
	{
		DFLTLOG(LEVEL_ERROR, "GSE header extension is not a CNI extension\n");
		return -1;
	}

	memcpy(opaque, ext, sizeof(uint32_t));
	
	return 0;
}

static int gse_ext_check_cb(unsigned char *ext,
                            size_t *length,  
                            uint16_t *protocol_type, 
                            uint16_t extension_type,
                            void *UNUSED(opaque))
{
	gse_status_t status = GSE_STATUS_OK;
	
	status = gse_check_header_extension_validity(ext, length,
	                                             extension_type, 
	                                             protocol_type);
	
	if( status != GSE_STATUS_OK)
	{
		return -1;
	}
	
	return 0;
}


Gse::Gse():
	 OpenSandPlugin(), EncapPlugin(NET_PROTO::GSE)
{
	this->upper.push_back("ROHC");
	this->upper.push_back("Ethernet");
}


Gse::~Gse()
{
}


Gse::Context::Context(EncapPlugin &plugin):
	EncapPlugin::EncapContext(plugin), contexts()
{
	this->vfrag_pkt = nullptr;
	this->vfrag_gse = nullptr;
	this->buf = nullptr;
}


void Gse::generateConfiguration(const std::string &, const std::string &, const std::string &)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();
	auto conf = Conf->getOrCreateComponent("encap", "Encapsulation", "The Encapsulation Plugins Configuration");
	auto gse = conf->addComponent("gse_C", "GSE", "The GSE Plugin Configuration");
	conf->setAdvanced(true);
	auto gse_enum = std::dynamic_pointer_cast<OpenSANDConf::MetaEnumType>(types->getType("GSE_library_type"));
	if (gse_enum)
	{
		gse_enum->getMutableValues().push_back("C");
	}
	else
	{
		types->addEnumType("GSE_library_type", "GSE protocol libraries types", {"C"});
		conf->addParameter("GSE_library", "the GSE protocol library used", types->getType("GSE_library_type"));
	}

	auto lib_type = conf->getParameter("GSE_library");

	gse->addParameter("packing_threshold", "Packing Threshold", types->getType("int"));

	Conf->setProfileReference(gse, lib_type, "C");
}


bool Gse::Context::init(void)
{

	if(!EncapPlugin::EncapContext::init())
	{
		return false;
	}
	gse_status_t status;

	auto gse = OpenSandModelConf::Get()->getProfileData()->getComponent("encap")->getComponent("gse_C");

	// Retrieving the packing threshold
	int threshold;
	if(!OpenSandModelConf::extractParameterData(gse->getParameter("packing_threshold"), threshold))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Section GSE, missing packing threshold parameter\n");
		goto error;
	}
	this->packing_threshold = threshold;
	LOG(this->log, LEVEL_NOTICE,
	    "packing threshold: %lu\n", this->packing_threshold);

	// Initialize encapsulation and deencapsulation contexts
	// Since we use a "custom" frag_id based on QoS value and the source tal_id,
	// set the qos_nbr in GSE library to its max value.
	status = gse_encap_init(MAX_QOS_NBR, 1, &this->encap);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot init GSE encapsulation context (%s)\n",
		    gse_get_status(status));
		goto error;
	}
	status = gse_deencap_init(MAX_QOS_NBR, &this->deencap);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot init GSE deencapsulation context (%s)\n",
		    gse_get_status(status));
		goto release_encap;
	}
	
	// we need to set a callback else GSE won't be able to deencapsulate
	// packets with extension
	gse_deencap_set_extension_callback(this->deencap,
	                                   gse_ext_check_cb,
	                                   //gse_check_header_extension_validity,
	                                   nullptr);

	// create vfrags and buf for storing virtual fragments
	// cannot create vfrag_pkt because don't know if we have to allocate
	// a vbuf too.
	status = gse_allocate_vfrag(&this->vfrag_gse, 0);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate vfrag_gse at init\n");
		goto release_encap;
	}
	return true;

release_encap:
	status = gse_encap_release(this->encap);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot release GSE encapsulation context (%s)\n",
		    gse_get_status(status));
	}

error:
	this->encap = nullptr;
	this->deencap = nullptr;
	return false;
}

Gse::Context::~Context()
{
	gse_status_t status;

	// free the vfrags and buffer if created
	if(this->vfrag_pkt != nullptr)
	{
		if(this->current_upper->getFixedLength() > 0)
		{
			status = gse_free_vfrag_no_alloc(&this->vfrag_pkt, 0, 0);
		}
		else
		{
			status = gse_free_vfrag_no_alloc(&this->vfrag_pkt, 0, 1);
		}
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot free vfrag in GSE encapsulation context (%s)\n",
			    gse_get_status(status));
		}
	}
	if(this->vfrag_gse != nullptr)
	{
		status = gse_free_vfrag_no_alloc(&this->vfrag_gse, 0, 0);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot free vfrag in GSE encapsulation context (%s)\n",
			    gse_get_status(status));
		}
	}
	if(this->buf != nullptr)
	{
		delete[] this->buf;
	}

	// release GSE encapsulation and deencapsulation contexts if created
	if(this->encap != nullptr)
	{
		status = gse_encap_release(this->encap);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot release GSE encapsulation context (%s)\n",
			    gse_get_status(status));
		}
	}
	if(this->deencap != nullptr)
	{
		status = gse_deencap_release(this->deencap);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot release GSE deencapsulation context (%s)\n",
			    gse_get_status(status));
		}
	}

	this->contexts.clear();
}

Rt::Ptr<NetBurst> Gse::Context::encapsulate(Rt::Ptr<NetBurst> burst,
                                            std::map<long, int> &time_contexts)
{
	Rt::Ptr<NetBurst> gse_packets = Rt::make_ptr<NetBurst>(nullptr);

	// create an empty burst of GSE packets
	try
	{
		gse_packets = Rt::make_ptr<NetBurst>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of GSE packets\n");
		return gse_packets;
	}

	for(auto&& packet : *burst)
	{
		// packet must be valid
		if(!packet)
		{
			LOG(this->log, LEVEL_ERROR,
			    "packet is not valid, drop the packet\n");
			continue;
		}

		long time = 0;
		uint32_t context_id = ((packet->getSrcTalId() & 0x1f) << 8) |
		                      ((packet->getDstTalId() & 0x1f) << 3) |
		                      (packet->getQos() & 0x07);

		LOG(this->log, LEVEL_INFO,
		    "encapsulate a %zu-byte packet of type 0x%04x "
		    "with SRC TAL Id = %u, DST TAL Id = %u, QoS = %u\n",
		    packet->getTotalLength(), packet->getType(),
		    packet->getSrcTalId(), packet->getDstTalId(),
		    packet->getQos());

		// the GSE encapsulation context must exist
		if(this->encap == nullptr)
		{
			LOG(this->log, LEVEL_ERROR,
			    "GSE encapsulation context unexisting, drop packet\n");
			continue;
		}

		LOG(this->log, LEVEL_INFO,
		    "received a packet with type 0x%.4x\n",
		    packet->getType());

		// if packet size is fixed, more than one packet can be encapsulated in
		// one GSE packet, we need to handle the context
		if(this->current_upper->getFixedLength() > 0)
		{
			if(!this->encapFixedLength(std::move(packet), *gse_packets, time))
			{
				continue;
			}
		}
		// if packet size is variable, the whole packet and only it is
		// encapsulated in the GSE packet
		else if(!this->encapVariableLength(std::move(packet), *gse_packets))
		{
			continue;
		}
		time_contexts.emplace(time, context_id);
	}

	return gse_packets;
}


/**
 *  @brief encap packet with fixed length, they can be packet in on GSE packet
 *
 *  @param packet      The packet from upper layer
 *  @param gse_packets The burst of GSE packets
 *  @return true on success, false otherwise
 */
bool Gse::Context::encapFixedLength(Rt::Ptr<NetPacket> packet,
                                    NetBurst &gse_packets,
                                    long &time)
{
	std::map<GseIdentifier, GseEncapCtx, ltGseIdentifier >::iterator context_it;
	gse_status_t status;
	// keep the destination spot
	uint16_t dest_spot = packet->getSpot();

	if(packet->getTotalLength() != this->current_upper->getFixedLength())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Bad packet length (%zu), drop packet\n",
		    this->current_upper->getFixedLength());
		return false;
	}

	GseIdentifier identifier{packet->getSrcTalId(),
	                         packet->getDstTalId(),
	                         packet->getQos()};
	LOG(this->log, LEVEL_INFO,
	    "check if encapsulation context exists\n");
	context_it = this->contexts.find(identifier);
	if(context_it == this->contexts.end())
	{
		LOG(this->log, LEVEL_INFO,
		    "encapsulation context does not exist yet\n");
		context_it = this->contexts.emplace(identifier, GseEncapCtx(identifier, dest_spot)).first;
		LOG(this->log, LEVEL_INFO,
		    "new encapsulation context created, "
		    "Src TAL Id = %u, Dst TAL Id = %u, QoS = %u\n",
		    context_it->second.getSrcTalId(),
		    context_it->second.getDstTalId(),
		    context_it->second.getQos());
	}
	else
	{
		LOG(this->log, LEVEL_INFO,
		    "find an encapsulation context containing %zu bytes of data\n",
		    context_it->second.length());
	}

	// set the destination spot ID
	packet->setSpot(dest_spot);
	// add the packet in context
	status = context_it->second.add(*packet);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Error when adding packet in context (%s), drop packet\n",
		    gse_get_status(status));
		return false;
	}

	LOG(this->log, LEVEL_INFO,
	    "Packet now entirely packed into GSE context, "
	    "context contains %zu bytes\n",
		context_it->second.length());

	// if there is enough space in buffer for another MPEG/ATM packet or if
	// packing_threshold is not 0 keep data in the virtual buffer
	if((!context_it->second.isFull()) && this->packing_threshold != 0)
	{
		LOG(this->log, LEVEL_INFO,
		    "enough unused space in virtual buffer for packing "
		    "=> keep the packets %lu ms\n",
		    this->packing_threshold);

		time = this->packing_threshold;

		return true;
	}

	// Allocate vfrag_pkt if it hasn't been created yet
	if(this->vfrag_pkt == nullptr) 
	{
		// no need to allocate another buffer since vfrag uses
		// the one of EncapCtx
		status = gse_allocate_vfrag(&this->vfrag_pkt, 0);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot allocate vfrag_pkt\n");
			return false;
		}
	}

	// Duplicate context virtual fragment before giving it to GSE library
	// (otherwise the GSE library will destroy it after use) and delete context.
	// Context shall be deleted otherwise there will be two accesses in
	// the virtual buffer (vfrag_pkt and context->vfrag), thus get_packet
	// could not be called (indeed, there can't be more than two accesses
	// in the same virtual buffer to avoid data modifications in other
	// packets). Another solution could have been to call get_packet_copy
	// but it is less efficient.
	// Initialize vfrags and buffer for GSE virtual fragments
	status = gse_duplicate_vfrag_no_alloc(&this->vfrag_pkt,
	                                      context_it->second.data(),
	                                      context_it->second.length());
	// do not delete the context, we're going to reuse it. set flag
	// to_delete in order to clean it during next add. do not reset 
	// now because the buffer is still being used (until the gse packet
	// is created)
	context_it->second.setReset();

	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Fail to duplicated context data (%s), drop packet\n",
		    gse_get_status(status));
		return false;
	}

	return this->encapPacket(std::move(packet), gse_packets);
}

/**
 *  @brief encap packet with variable length
 *
 *  @param packet      The packet from upper layer
 *  @param gse_packets The burst of GSE packets
 *  @return true on success, false otherwise
 */
bool Gse::Context::encapVariableLength(Rt::Ptr<NetPacket> packet, NetBurst &gse_packets)
{
	gse_status_t status;
	if(this->vfrag_pkt == nullptr)
	{
		status = gse_allocate_vfrag(&this->vfrag_pkt, 1);
		this->buf = new uint8_t[GSE_MAX_PACKET_LENGTH +
		                        GSE_MAX_HEADER_LENGTH +
		                        GSE_MAX_TRAILER_LENGTH];
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			        "cannot allocate vfrag_pkt\n");
			return false;
		}
	}

	// Affect buffer to vfrag
	status = gse_affect_buf_vfrag(this->vfrag_pkt, this->buf,
	                              GSE_MAX_HEADER_LENGTH,
	                              GSE_MAX_TRAILER_LENGTH,
	                              packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot affect buf to vfrag\n");
		return false;
	}

	// Copy data to buffer
	status = gse_copy_data(this->vfrag_pkt, packet->getRawData(),
	                       packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Virtual fragment data copy failed (%s), drop packet\n",
		    gse_get_status(status));
		return false;
	}
	return this->encapPacket(std::move(packet), gse_packets);
}

/**
 *  @brief encapsulate the data into GSE packets
 *
 *  @param packet       The packet from upper layer
 *  @param gse_packets  The burst of GSE packets
 *  @return true on success, false otherwise
 */
bool Gse::Context::encapPacket(Rt::Ptr<NetPacket> packet, NetBurst &gse_packets)
{
	gse_status_t status;
	unsigned int counter;
	uint8_t label[6];
	uint8_t frag_id;
	// keep the destination spot
	uint16_t dest_spot = packet->getSpot();
	// keep the QoS
	uint8_t qos = packet->getQos();
	
	// keep the source/destination tal_id
	uint8_t src_tal_id = packet->getSrcTalId();
	uint8_t dst_tal_id = packet->getDstTalId();

	// Common part for all packet types
	if((packet->getSrcTalId() & 0x1f) != packet->getSrcTalId())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a source TAL ID greater than 0x1f,"
		    " it will be truncated for GSE packet creation!!!\n");
	}
	if((packet->getDstTalId() & 0x1f) != packet->getDstTalId())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a destination TAL ID greater than 0x1f,"
		    " it will be truncated for GSE packet creation!!!\n");
	}
	if((packet->getQos() & 0x7) != packet->getQos())
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a QoS greater than 0x7,"
		    " it will be truncated for GSE packet creation!!!\n");
	}

	// Set packet label
	if(!Gse::setLabel(*packet, label))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Cannot set label for GSE packet\n");
		goto drop;
	}

	// Get the frag Id
	frag_id = Gse::getFragId(*packet);

	// Store the IP packet in the encapsulation context thanks
	// to the GSE library
	status = gse_encap_receive_pdu(this->vfrag_pkt, this->encap, label, 0,
	                               to_underlying(packet->getType()), frag_id);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Fail to store packet in GSE encapsulation context (%s), "
		    "drop packet\n", gse_get_status(status));
		goto drop;
	}

	counter = 0;
	do
	{
		counter++;
		status = gse_encap_get_packet_no_alloc(&this->vfrag_gse,
		                                       this->encap, GSE_MAX_PACKET_LENGTH,
		                                       frag_id);
		if(status != GSE_STATUS_OK && status != GSE_STATUS_FIFO_EMPTY)
		{
			LOG(this->log, LEVEL_ERROR,
			    "Fail to get GSE packet #%u in encapsulation context "
			    "(%s), drop packet\n", counter, gse_get_status(status));
			goto clean;
		}

		if(status == GSE_STATUS_OK)
		{
			Rt::Ptr<NetPacket> gse = Rt::make_ptr<NetPacket>(nullptr);
			Rt::Data gse_packet(gse_get_vfrag_start(this->vfrag_gse),
			                    gse_get_vfrag_length(this->vfrag_gse));
			try
			{
				// create a GSE packet from fragments computed by the GSE library
				gse = this->createPacket(gse_packet,
				                         gse_get_vfrag_length(this->vfrag_gse),
				                         qos, src_tal_id, dst_tal_id);
			}
			catch (const std::bad_alloc&)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create GSE packet, drop the network "
				    "packet\n");
				goto clean;
			}


			// set the destination spot ID
			gse->setSpot(dest_spot);
			LOG(this->log, LEVEL_INFO,
			    "%zu-byte GSE packet added to burst\n",
			    gse->getTotalLength());
			// add GSE packet to burst
			gse_packets.add(std::move(gse));

			status = gse_free_vfrag_no_alloc(&this->vfrag_gse, 1, 0);
			if(status != GSE_STATUS_OK)
			{
				LOG(this->log, LEVEL_ERROR,
				    "Fail to free GSE fragment #%u (%s), "
				    "drop packet\n", counter,
				    gse_get_status(status));
				goto clean;
			}
		}
	}
	while(status != GSE_STATUS_FIFO_EMPTY && !gse_packets.isFull());
	LOG(this->log, LEVEL_INFO,
	    "%zu-byte %s packet/frame => %u GSE packets\n",
	    packet->getTotalLength(), packet->getName().c_str(),
	    counter - 1);

	return true;

clean:
	if(this->vfrag_gse != nullptr)
	{
		status = gse_free_vfrag_no_alloc(&this->vfrag_gse, 1, 0);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to free GSE virtual fragment (%s)\n",
			    gse_get_status(status));
		}
	}
drop:
	return false;
}

Rt::Ptr<NetBurst> Gse::Context::deencapsulate(Rt::Ptr<NetBurst> burst)
{
	Rt::Ptr<NetBurst> net_packets = Rt::make_ptr<NetBurst>(nullptr);
	gse_vfrag_t *vfrag_gse;
	gse_status_t status;

	// create an empty burst of network packets
	try
	{
		net_packets = Rt::make_ptr<NetBurst>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of network packets\n");
		return net_packets;
	}

	for(auto &&packet : *burst)
	{
		uint8_t dst_tal_id;

		// packet must be valid
		if(!packet)
		{
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not valid, drop the packet\n");
			continue;
		}

		// Filter if packet is for this ST
		dst_tal_id = packet->getDstTalId();
		if (dst_tal_id != this->dst_tal_id &&
		    this->dst_tal_id != BROADCAST_TAL_ID &&
		    dst_tal_id != BROADCAST_TAL_ID)
		{
			LOG(this->log, LEVEL_INFO,
			    "encapsulation packet is for ST#%u. Drop\n",
			    packet->getDstTalId());
			continue;
		}


		// packet must be a GSE packet
		if(packet->getType() != this->getEtherType())
		{
			LOG(this->log, LEVEL_ERROR,
			    "encapsulation packet is not a GSE packet "
			    "(type = 0x%04x), drop the packet\n",
			    packet->getType());
			continue;
		}

		// the GSE deencapsulation context must exist
		if(this->deencap == nullptr)
		{
			LOG(this->log, LEVEL_ERROR,
			    "GSE deencapsulation context does not exist, "
			    "drop packet\n");
			continue;
		}

		// Create a virtual fragment containing the GSE packet
		// TODO : this function could be optimized (preallocating vfrag_gse), but
		// gse_deencap_packet call below frees vfrag struct (need to change that
		// function in order to be no_alloc compatible).
		status = gse_create_vfrag_with_data(&vfrag_gse, packet->getTotalLength(),
		                                    0, 0,
		                                    packet->getRawData(),
		                                    packet->getTotalLength());
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "Virtual fragment creation failed (%s), drop packet\n",
			    gse_get_status(status));
			continue;
		}
		LOG(this->log, LEVEL_INFO,
		    "Create a virtual fragment for GSE library "
		    "(length = %zu)\n", packet->getTotalLength());

		if(!this->deencapPacket(vfrag_gse, packet->getSpot(), *net_packets))
		{
			continue;
		}
	}

	return net_packets;
}

bool Gse::Context::deencapPacket(gse_vfrag_t *vfrag_gse,
                                 uint16_t dest_spot,
                                 NetBurst &net_packets)
{
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
			LOG(this->log, LEVEL_INFO,
			    "GSE packet deencapsulated, Gse packet length = %u; "
			    "PDU is not complete\n", packet_length);
			break;

		case GSE_STATUS_DATA_OVERWRITTEN:
			LOG(this->log, LEVEL_NOTICE,
			    "GSE packet deencapsulated, GSE Length = %u (%s); "
			    "PDU is not complete, a context was erased\n",
			    packet_length, gse_get_status(status));
			break;

		case GSE_STATUS_PADDING_DETECTED:
			LOG(this->log, LEVEL_INFO,
			    "%s\n", gse_get_status(status));
			break;

		case GSE_STATUS_PDU_RECEIVED:
			LOG(this->log, LEVEL_INFO,
			    "received a packet with type 0x%.4x\n", protocol);
			if(this->current_upper->getFixedLength() > 0)
			{
				LOG(this->log, LEVEL_INFO,
				    "Inner packet has a fixed length (%zu)\n",
				    this->current_upper->getFixedLength());
				return this->deencapFixedLength(vfrag_pdu ,dest_spot,
				                                label, net_packets);
			}
			else
			{
				LOG(this->log, LEVEL_INFO,
				    "Inner packet has a variable length\n");
				return this->deencapVariableLength(vfrag_pdu, dest_spot,
				                                   label, net_packets);
			}
			break;

		case GSE_STATUS_CTX_NOT_INIT:
			LOG(this->log, LEVEL_INFO,
			    "GSE deencapsulation failed (%s), drop packet "
			    "(probably not an error, this happens when we receive a "
			    "fragment that is not for us)\n",
			    gse_get_status(status));
			break;

		case GSE_STATUS_BUFF_LENGTH_NULL:
			LOG(this->log, LEVEL_INFO,
			    "GSE deencapsulation success even if %s ",
			    gse_get_status(status));
			break;

		default:
			LOG(this->log, LEVEL_ERROR,
			    "GSE deencapsulation failed (%s), drop packet\n",
			    gse_get_status(status));
			return false;

	}
	return true;
}


bool Gse::Context::deencapFixedLength(gse_vfrag_t *vfrag_pdu,
                                      uint16_t dest_spot,
                                      uint8_t label[6],
                                      NetBurst &net_packets)
{
	gse_status_t status;
	uint8_t src_tal_id, dst_tal_id;
	uint8_t qos;
	unsigned int pkt_nbr = 0;

	src_tal_id = Gse::getSrcTalIdFromLabel(label);
	dst_tal_id = Gse::getDstTalIdFromLabel(label);
	qos = Gse::getQosFromLabel(label);

	if(gse_get_vfrag_length(vfrag_pdu) %
	   this->current_upper->getFixedLength() != 0)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Number of packets in GSE payload is not an integer,"
		    " drop packets\n");
		gse_free_vfrag(&vfrag_pdu);
		return false;
	}
	while(gse_get_vfrag_length(vfrag_pdu) > 0)
	{
		Rt::Ptr<NetPacket> packet = Rt::make_ptr<NetPacket>(nullptr);
		Rt::Data pdu_frag(gse_get_vfrag_start(vfrag_pdu),
		                  this->current_upper->getFixedLength());
		try
		{
			packet = this->current_upper->build(pdu_frag,
			                                    this->current_upper->getFixedLength(),
			                                    qos, src_tal_id, dst_tal_id);
		}
		catch (const std::bad_alloc&)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot build a %s packet, drop the GSE packet\n",
			    this->current_upper->getName().c_str());
			gse_free_vfrag(&vfrag_pdu);
			// move the data pointer after the current packet
			status = gse_shift_vfrag(vfrag_pdu,
			                         this->current_upper->getFixedLength(), 0);
			if(status != GSE_STATUS_OK)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot shift virtual fragment (%s), drop the "
				    "GSE packet\n", gse_get_status(status));
				gse_free_vfrag(&vfrag_pdu);
				return false;
			}
			continue;
		}

		// set the destination spot ID
		packet->setSpot(dest_spot);
		// add network packet to burst
		net_packets.add(std::move(packet));
		pkt_nbr++;

		// move the data pointer after the current packet
		status = gse_shift_vfrag(vfrag_pdu,
		                         this->current_upper->getFixedLength(), 0);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot shift virtual fragment (%s), drop the "
			    "GSE packet\n", gse_get_status(status));
			gse_free_vfrag(&vfrag_pdu);
			return false;
		}
		pkt_nbr++;
	}

	LOG(this->log, LEVEL_INFO,
	    "Complete PDU received, got %u GSE packet(s)/frame "
	    "(GSE packet length = %zu, Src TAL id = %u, Dst TAL id = %u, qos = %u)\n",
	    pkt_nbr, gse_get_vfrag_length(vfrag_pdu), src_tal_id, dst_tal_id, qos);

	gse_free_vfrag(&vfrag_pdu);

	return true;
}

bool Gse::Context::deencapVariableLength(gse_vfrag_t *vfrag_pdu,
                                         uint16_t dest_spot,
                                         uint8_t label[6],
                                         NetBurst &net_packets)
{
	uint8_t src_tal_id, dst_tal_id;
	uint8_t qos;
	unsigned int pkt_nbr = 0;
	Rt::Data pdu_frag(gse_get_vfrag_start(vfrag_pdu),
	                  gse_get_vfrag_length(vfrag_pdu));

	src_tal_id = Gse::getSrcTalIdFromLabel(label);
	dst_tal_id = Gse::getDstTalIdFromLabel(label);
	qos = Gse::getQosFromLabel(label);

	Rt::Ptr<NetPacket> packet = Rt::make_ptr<NetPacket>(nullptr);
	try
	{
		packet = this->current_upper->build(pdu_frag,
		                                    gse_get_vfrag_length(vfrag_pdu),
		                                    qos, src_tal_id, dst_tal_id);
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR, 
		    "cannot build a %s packet, drop the GSE packet\n",
		    this->current_upper->getName().c_str());
		gse_free_vfrag(&vfrag_pdu);
		return false;
	}

	// set the destination spot ID
	packet->setSpot(dest_spot);
	pkt_nbr++;

	LOG(this->log, LEVEL_INFO,
	    "Complete PDU received, got %u %zu-byte %s packet(s)/frame "
	    "(GSE packet length = %zu, Src TAL id = %u, Dst TAL id = %u, qos = %u)\n",
	    pkt_nbr, packet->getTotalLength(), packet->getName().c_str(),
	    gse_get_vfrag_length(vfrag_pdu), src_tal_id, dst_tal_id, qos);

	// add network packet to burst
	net_packets.add(std::move(packet));

	gse_free_vfrag(&vfrag_pdu);

	return true;
}

Rt::Ptr<NetBurst> Gse::Context::flush(int context_id)
{
	gse_status_t status;
	std::map<GseIdentifier, GseEncapCtx, ltGseIdentifier >::iterator context_it;
	uint8_t label[6];
	std::string packet_name;
	uint16_t protocol;
	size_t ctx_length;
	uint8_t src_tal_id, dst_tal_id;
	uint8_t qos;
	uint8_t frag_id;
	uint16_t dest_spot;
	unsigned int counter;
	Rt::Ptr<NetBurst> gse_packets = Rt::make_ptr<NetBurst>(nullptr);

	// create an empty burst of GSE packets
	try
	{
		gse_packets = Rt::make_ptr<NetBurst>();
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot allocate memory for burst of GSE packets\n");
		return Rt::make_ptr<NetBurst>(nullptr);
	}

	LOG(this->log, LEVEL_INFO,
	    "search for encapsulation context (id = %d) to flush...\n",
	    context_id);

	{
		GseIdentifier identifier{static_cast<uint8_t>((context_id >> 8) & 0x1f),
		                         static_cast<uint8_t>((context_id >> 3) & 0x1f),
		                         static_cast<uint8_t>(context_id & 0x07)};
		LOG(this->log, LEVEL_INFO,
		    "Associated identifier: Src TAL Id = %u, Dst TAL Id = %u, QoS = %u\n",
		    identifier.getSrcTalId(),
		    identifier.getDstTalId(),
		    identifier.getQos());

		context_it = this->contexts.find(identifier);
	}

	if(context_it == this->contexts.end())
	{
		LOG(this->log, LEVEL_ERROR,
		    "encapsulation context does not exist\n");
		return Rt::make_ptr<NetBurst>(nullptr);
	}
	else
	{
		LOG(this->log, LEVEL_INFO,
		    "find an encapsulation context containing %zu bytes of data\n",
		    context_it->second.length());
	}

	// Duplicate context virtual fragment before giving it to GSE library
	// (otherwise the GSE library will destroy it after use) and delete context.
	// Context shall be deleted otherwise there will be two accesses in
	// the virtual buffer (vfrag_pkt and context->vfrag), thus get_packet
	// could not be called (indeed, there can't be more than two accesses
	// in the same virtual buffer to avoid data modifications in other
	// packets). Another solution could have been to call get_packet_copy
	// but it is less efficient.
	packet_name = context_it->second.getPacketName();
	protocol = context_it->second.getProtocol();
	ctx_length = context_it->second.length();
	src_tal_id = context_it->second.getSrcTalId();
	dst_tal_id = context_it->second.getDstTalId();
	qos = context_it->second.getQos();
	// Allocate vfrag_pkt if it hasn't been created yet
	if(this->vfrag_pkt == nullptr) 
	{
		// no need to allocate another buffer since vfrag uses
		// the one of EncapCtx
		status = gse_allocate_vfrag(&this->vfrag_pkt, 0);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR, "cannot allocate vfrag_pkt\n");
			return Rt::make_ptr<NetBurst>(nullptr);
		}
	}
	status = gse_duplicate_vfrag_no_alloc(&this->vfrag_pkt,
	                                      context_it->second.data(),
	                                      context_it->second.length());
	// keep the destination spot
	dest_spot = context_it->second.getDestSpot();
	// Set packet label
	if(!Gse::setLabel(context_it->second, label))
	{
		LOG(this->log, LEVEL_ERROR,
		    "Cannot set label for GSE packet\n");
		return Rt::make_ptr<NetBurst>(nullptr);
	}
	// Get the frag Id
	frag_id = Gse::getFragId(context_it->second);
	
	// Set context to reset
	context_it->second.setReset();
	
	// now context is release check status
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Fail to duplicated context data (%s), drop packets\n",
		    gse_get_status(status));
		return Rt::make_ptr<NetBurst>(nullptr);
	}

	if((src_tal_id & 0x1f) != src_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a source TAL ID greater than 0x1f,"
		    " it will be truncated for GSE packet creation!!!\n");
	}
	if((dst_tal_id & 0x1f) != dst_tal_id)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a destination TAL ID greater than 0x1f,"
		    " it will be truncated for GSE packet creation!!!\n");
	}
	if((qos & 0x7) != qos)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Be careful, you have set a QoS greater than 0x7,"
		    " it will be truncated for GSE packet creation!!!\n");
	}

	// Store the IP packet in the encapsulation context thanks to the GSE library
	status = gse_encap_receive_pdu(this->vfrag_pkt, this->encap, label, 0,
	                               protocol, frag_id);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Fail to store packet in GSE encapsulation context (%s), "
		    "drop packet\n", gse_get_status(status));
		return Rt::make_ptr<NetBurst>(nullptr);
	}

	counter = 0;
	do
	{
		counter++;
		status = gse_encap_get_packet_no_alloc(&this->vfrag_gse, this->encap,
		                                       GSE_MAX_PACKET_LENGTH, frag_id);
		if(status != GSE_STATUS_OK && status != GSE_STATUS_FIFO_EMPTY)
		{
			LOG(this->log, LEVEL_ERROR,
			    "Fail to get GSE packet #%d in encapsulation context "
			    "(%s), drop packet\n",
			    counter, gse_get_status(status));
			goto clean;
		}

		if(status == GSE_STATUS_OK)
		{
			Rt::Ptr<NetPacket> gse = Rt::make_ptr<NetPacket>(nullptr);
			Rt::Data gse_frag(gse_get_vfrag_start(this->vfrag_gse),
			                  gse_get_vfrag_length(this->vfrag_gse));
			try
			{
			// create a GSE packet from fragments computed by the GSE library
				gse = this->createPacket(gse_frag,
				                         gse_get_vfrag_length(this->vfrag_gse),
				                         qos, src_tal_id, dst_tal_id);
			}
			catch (const std::bad_alloc&)
			{
				LOG(this->log, LEVEL_ERROR,
				    "cannot create GSE packet, drop the network packet\n");
				goto clean;
			}

			// set the destination spot ID
			gse->setSpot(dest_spot);
			LOG(this->log, LEVEL_INFO,
			    "%zu-byte GSE packet added to burst\n",
			    gse->getTotalLength());
			// add GSE packet to burst
			gse_packets->add(std::move(gse));

			status = gse_free_vfrag_no_alloc(&this->vfrag_gse, 1, 0);
			if(status != GSE_STATUS_OK)
			{
				LOG(this->log, LEVEL_ERROR,
				    "Fail to free GSE fragment #%u (%s), drop packet\n", counter,
				    gse_get_status(status));
				goto clean;
			}
		}
	}
	while(status != GSE_STATUS_FIFO_EMPTY && !gse_packets->isFull());
	LOG(this->log, LEVEL_INFO,
	    "%zu-byte %s packet/frame => %u GSE packets\n",
	    ctx_length, packet_name.c_str(), counter - 1);

	return gse_packets;

clean:
	if(vfrag_gse != nullptr)
	{
		status = gse_free_vfrag_no_alloc(&this->vfrag_gse, 1, 0);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to free GSE virtual fragment (%s)\n",
			    gse_get_status(status));
		}
	}
	return Rt::make_ptr<NetBurst>(nullptr);
}

Rt::Ptr<NetBurst> Gse::Context::flushAll()
{
	//TODO
	return Rt::make_ptr<NetBurst>(nullptr);
}

Rt::Ptr<NetPacket> Gse::PacketHandler::build(const Rt::Data &data,
                                             size_t data_length,
                                             uint8_t qos,
                                             uint8_t src_tal_id,
                                             uint8_t dst_tal_id)
{
	gse_status_t status;
	uint8_t s;
	uint8_t e;
	uint8_t label_length = 6;

	uint16_t header_length = 0;
	unsigned char *packet = const_cast<unsigned char *>(data.data());

	status = gse_get_start_indicator(packet, &s);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get start indicator (%s)\n",
		    gse_get_status(status));
		return Rt::make_ptr<NetPacket>(nullptr);
	}

	status = gse_get_end_indicator(packet, &e);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get end indicator (%s)\n",
		    gse_get_status(status));
		return Rt::make_ptr<NetPacket>(nullptr);
	}

	// subsequent fragment
	if(s == 0)
	{
		LOG(this->log, LEVEL_DEBUG,
		    "build a subsequent fragment "
		    "SRC TAL Id = %u, QoS = %u, DST TAL Id=  %u\n",
		    src_tal_id, qos, dst_tal_id);
		header_length = GSE_MANDATORY_FIELDS_LENGTH +
		                GSE_FRAG_ID_LENGTH +
		                label_length;
	}
	// complete or first fragment
	else
	{
		// first fragment
		if(e == 0)
		{
			LOG(this->log, LEVEL_DEBUG,
			    "build a first fragment\n");
			header_length = GSE_MANDATORY_FIELDS_LENGTH +
			                GSE_FRAG_ID_LENGTH +
			                GSE_TOTAL_LENGTH_LENGTH +
			                label_length;
		}
		// complete
		else
		{
			LOG(this->log, LEVEL_DEBUG,
			    "build a complete packet\n");
			header_length = GSE_MANDATORY_FIELDS_LENGTH +
			                label_length;
		}
		LOG(this->log, LEVEL_INFO,
		    "build a new %zu-bytes GSE packet: QoS = %u, Src Tal ID = %u, "
		    "Dst TAL ID = %u, header length = %u\n", data_length,
		    qos, src_tal_id, dst_tal_id, header_length);
	}

	return Rt::make_ptr<NetPacket>(data, data_length,
	                               this->getName(), this->getEtherType(),
	                               qos, src_tal_id, dst_tal_id, header_length);
}


size_t Gse::PacketHandler::getLength(const unsigned char *data) const
{
	uint16_t length;
	gse_status_t status;

	status = gse_get_gse_length(const_cast<unsigned char *>(data), &length);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get length (%s)\n",
		    gse_get_status(status));
		return 0;
	}

	// Add 2 bits for S, E and LT fields
	return length + GSE_MANDATORY_FIELDS_LENGTH;
}


bool Gse::PacketHandler::getChunk(Rt::Ptr<NetPacket> packet,
                                  std::size_t remaining_length,
                                  Rt::Ptr<NetPacket>& data,
                                  Rt::Ptr<NetPacket>& remaining_data)
{
	gse_vfrag_t *first_frag;
	gse_vfrag_t *second_frag;
	gse_status_t status;
	uint8_t frag_id;

	frag_id = Gse::getFragId(*packet);

	LOG(this->log, LEVEL_DEBUG,
	    "Create a virtual fragment with GSE packet to "
	    "refragment it\n");
	// TODO : this vfrag creation could be optimized with no_alloc
	status = gse_create_vfrag_with_data(&first_frag,
	                                    packet->getTotalLength(),
	                                    GSE_MAX_REFRAG_HEAD_OFFSET, 0,
	                                    packet->getRawData(),
	                                    packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "Failed to create a virtual fragment for the GSE packet "
		    "refragmentation (%s)\n", gse_get_status(status));
		goto error;
	}

	LOG(this->log, LEVEL_DEBUG,
	    "Refragment the GSE packet to fit the BB frame "
	    "(length = %zu)\n", remaining_length);
	status = gse_refrag_packet(first_frag, &second_frag, 0, 0,
	                           frag_id,
	                           MIN(remaining_length, GSE_MAX_PACKET_LENGTH));
	if(status == GSE_STATUS_LENGTH_TOO_SMALL)
	{
		// there is not enough space to create a GSE fragment,

		LOG(this->log, LEVEL_INFO,
		    "Unable to refragment GSE packet (%s)\n",
		    gse_get_status(status));
		// the packet cannot be encapsulated, copy it in
		// remaining data and return true (use case 3)
		remaining_data = std::move(packet);
		goto success;
	}
	else if(status == GSE_STATUS_REFRAG_UNNECESSARY)
	{
		LOG(this->log, LEVEL_DEBUG,
		    "no need to refragment, the whole packet can be "
		    "encapsulated\n");
		// the whole packet can be encapsulated, copy it in
		// data and return true (use case 1)
		data = std::move(packet);
		goto success;
	}
	else if(status == GSE_STATUS_OK)
	{
		// the packet has been fragmented in order to be encapsulated partially
		// (use case 2)
		Rt::Data gse_first(gse_get_vfrag_start(first_frag),
		                   gse_get_vfrag_length(first_frag));
		Rt::Data gse_second(gse_get_vfrag_start(second_frag),
		                    gse_get_vfrag_length(second_frag));

		LOG(this->log, LEVEL_INFO,
		    "packet has been refragmented, first fragment is "
		    "%zu bytes long, second fragment is %zu bytes long\n",
		    gse_get_vfrag_length(first_frag), gse_get_vfrag_length(second_frag));
		// add the first fragment to the BB frame
		try
		{
			data = this->build(gse_first,
			                   gse_get_vfrag_length(first_frag),
			                   packet->getQos(),
			                   packet->getSrcTalId(),
			                   packet->getDstTalId());
		}
		catch (const std::bad_alloc&)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to create the first fragment\n");
			goto error;
		}

		// create a new NetPacket containing the second fragment
		try
		{
			remaining_data = this->build(gse_second,
			                             gse_get_vfrag_length(second_frag),
			                             packet->getQos(),
			                             packet->getSrcTalId(),
			                             packet->getDstTalId());
		}
		catch (const std::bad_alloc&)
		{
			LOG(this->log, LEVEL_ERROR,
			    "failed to create the second fragment\n");
			goto error;
		}
	}
	else
	{
		LOG(this->log, LEVEL_ERROR,
		    "Failed to refragment GSE packet (%s)\n",
		    gse_get_status(status));
		goto error;
	}

success:
	if(second_frag != nullptr)
	{
		gse_free_vfrag(&second_frag);
	}
	gse_free_vfrag(&first_frag);
	return true;

error:
	data = Rt::make_ptr<NetPacket>(nullptr);

	if (second_frag != nullptr)
	{
		gse_free_vfrag(&second_frag);
	}
	if (first_frag != nullptr)
	{
		gse_free_vfrag(&first_frag);
	}

	return false;
}


Gse::PacketHandler::PacketHandler(EncapPlugin &plugin):
	EncapPlugin::EncapPacketHandler(plugin)
{
	this->encap_callback["encodeCniExt"] = encodeHeaderCniExtensions;
	this->deencap_callback["deencodeCniExt"] = deencodeHeaderCniExtensions;
	this->callback_name.push_back("encodeCniExt");
	this->callback_name.push_back("deencodeCniExt");
}

bool Gse::PacketHandler::getSrc(const Rt::Data &data, tal_id_t &tal_id) const
{
	gse_status_t status;
	uint8_t s;
	uint8_t e;

	unsigned char *packet = const_cast<unsigned char *>(data.data());

	status = gse_get_start_indicator(packet, &s);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get start indicator (%s)\n",
		    gse_get_status(status));
		return false;
	}

	status = gse_get_end_indicator(packet, &e);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get end indicator (%s)\n",
		    gse_get_status(status));
		return false;
	}

	// subsequent fragment
	if(s == 0)
	{
		uint8_t frag_id;
		status = gse_get_frag_id(packet, &frag_id);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get frag ID (%s)\n",
			    gse_get_status(status));
			return false;
		}
		tal_id = Gse::getSrcTalIdFromFragId(frag_id);
	}
	// complete or first fragment
	else
	{
		uint8_t label[6];
		status = gse_get_label(packet, label);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get label (%s)\n",
			    gse_get_status(status));
			return false;
		}
		tal_id = Gse::getSrcTalIdFromLabel(label);
	}

	return true;
}

bool Gse::PacketHandler::getDst(const Rt::Data &data, tal_id_t &tal_id) const
{
	gse_status_t status;
	uint8_t s;
	uint8_t e;

	unsigned char *packet = const_cast<unsigned char *>(data.data());

	status = gse_get_start_indicator(packet, &s);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get start indicator (%s)\n",
		    gse_get_status(status));
		return false;
	}

	status = gse_get_end_indicator(packet, &e);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR,
		    "cannot get end indicator (%s)\n",
		    gse_get_status(status));
		return false;
	}

	// subsequent fragment
	if(s == 0)
	{
		uint8_t frag_id;
		status = gse_get_frag_id(packet, &frag_id);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get frag ID (%s)\n",
			    gse_get_status(status));
			return false;
		}
		tal_id = Gse::getDstTalIdFromFragId(frag_id);
	}
	// complete or first fragment
	else
	{
		uint8_t label[6];
		status = gse_get_label(packet, label);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR,
			    "cannot get label (%s)\n",
			    gse_get_status(status));
			return false;
		}
		tal_id = Gse::getDstTalIdFromLabel(label);
	}

	return true;
}

bool Gse::PacketHandler::getQos(const Rt::Data &data, qos_t &qos) const
{
	gse_status_t status;
	uint8_t s;
	uint8_t e;

	unsigned char *packet = const_cast<unsigned char *>(data.data());

	status = gse_get_start_indicator(packet, &s);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR, "cannot get start indicator (%s)\n",
		    gse_get_status(status));
		return false;
	}

	status = gse_get_end_indicator(packet, &e);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR, "cannot get end indicator (%s)\n",
		    gse_get_status(status));
		return false;
	}

	// subsequent fragment
	if(s == 0)
	{
		uint8_t frag_id;
		status = gse_get_frag_id(packet, &frag_id);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR, "cannot get frag ID (%s)\n",
			    gse_get_status(status));
			return false;
		}
		qos = Gse::getQosFromFragId(frag_id);
	}
	// complete or first fragment
	else
	{
		uint8_t label[6];
		status = gse_get_label(packet, label);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR, "cannot get label (%s)\n",
			    gse_get_status(status));
			return false;
		}
		qos = Gse::getQosFromLabel(label);

	}

	return true;
}


bool Gse::PacketHandler::checkPacketForHeaderExtensions(Rt::Ptr<NetPacket> &packet)
{
	// Search for a non-fragmented GSE packet, since extension cannot be
	// added to a fragment.
	uint8_t indicator;
	uint16_t protocol_type;

	unsigned char *packet_data = packet->getRawData();

	auto status = gse_get_start_indicator(packet_data, &indicator);
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR, "cannot get start indicator (%s)\n",
		    gse_get_status(status));
		packet.reset();
		return false;
	}

	if(indicator != 0)
	{
		LOG(this->log, LEVEL_DEBUG, "non-fragmented GSE packet found\n");

		status = gse_get_protocol_type(packet_data, &protocol_type);
		if(status != GSE_STATUS_OK)
		{
			LOG(this->log, LEVEL_ERROR, 
			    "cannot get protocol type of the GSE packet (%s)\n",
			    gse_get_status(status));
			packet.reset();
			return false;
		}

		// Test if packet already has extensions
		// (case protocol_type < GSE_MIN_ETHER_TYPE)
		if(protocol_type >= GSE_MIN_ETHER_TYPE)
		{
			return true;
		}
		else
		{
			LOG(this->log, LEVEL_DEBUG, "packet already has extensions\n");
		}
	}

	packet.reset();  // TODO: check this is the right thing to do here
	return true;
}


bool Gse::PacketHandler::setHeaderExtensions(Rt::Ptr<NetPacket> packet,
                                             Rt::Ptr<NetPacket>& new_packet,
                                             tal_id_t tal_id_src,
                                             tal_id_t tal_id_dst,
                                             std::string callback_name,
                                             void *opaque)
{

	gse_status_t status;
	gse_vfrag_t *vfrag;
	gse_vfrag_t *vfrag2;
	uint32_t crc;

	// Empty GSE packet
	// TODO macro for sizes
	// TODO endianess !!
	static unsigned char empty_gse[7] = 
	{
		0xd0, /* LT = 01 (three bytes label) */
		0x05, /* length */
		to_underlying(NET_PROTO::IPV4) >> 8 & 0xff,
		to_underlying(NET_PROTO::IPV4) & 0xff,
		(unsigned char)tal_id_src,
		(unsigned char)tal_id_dst,
		0x00 /* highest priority fifo (eg. NM FIFO) */
	};

	if(!packet)
	{
		LOG(this->log, LEVEL_INFO, 
		    "no packet, create empty one\n");
		packet = Rt::make_ptr<NetPacket>(empty_gse, 7);
	}

	// TODO : this could be optimized using no_alloc
	status = gse_create_vfrag_with_data(&vfrag, GSE_MAX_PACKET_LENGTH,
	                                    MAX_CNI_EXT_LEN, 0,
	                                    packet->getRawData(),
	                                    packet->getTotalLength());

	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR, "cannot create virtual fragment (%s)",
		    gse_get_status(status));
		return false;
	}

	// TODO: once packet refragmentation will be handled, set QoS to actual
	// value (see NOTE #2).
	status = gse_encap_add_header_ext(vfrag, &vfrag2, &crc,
	                                  this->encap_callback[callback_name],
	                                  GSE_MAX_PACKET_LENGTH, 0, 0,
	                                  /* qos */ 0,
	                                  opaque);
	if(status == GSE_STATUS_EXTENSION_UNAVAILABLE)
	{
		// NOTE #1 this should never happen except if PDU are greater than max
		//      GSE packet PDU length (this is not the case for the moment)
		LOG(this->log, LEVEL_ERROR, 
		   "cannot add extension in the next GSE packet\n");
		gse_free_vfrag(&vfrag);
		if(vfrag2)
			gse_free_vfrag(&vfrag2);
		return false;
	}
	else if(status == GSE_STATUS_PARTIAL_CRC || vfrag2)
	{
		// NOTE #2 this should never happend except if PDU + ext are greater than max
		//      GSE packet PDU length (this is not the case for the moment)
		LOG(this->log, LEVEL_ERROR, "packet has been refragmented\n");
		gse_free_vfrag(&vfrag);
		if(vfrag2)
			gse_free_vfrag(&vfrag2);
		return false;
	}
	else if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR, 
		   "cannot add header extension in packet (%s)",
		   gse_get_status(status));
		gse_free_vfrag(&vfrag);
		if(vfrag2)
			gse_free_vfrag(&vfrag2);
		return false;
	}

	Rt::Data gse_frag(gse_get_vfrag_start(vfrag),
	                  gse_get_vfrag_length(vfrag));
	try
	{
		new_packet = this->build(gse_frag,
		                         gse_get_vfrag_length(vfrag),
		                         packet->getQos(),
		                         tal_id_src, tal_id_dst);
	}
	catch (const std::bad_alloc&)
	{
		LOG(this->log, LEVEL_ERROR, 
		    "failed to create the GSE packet with extensions\n");
		return false;
	}

	gse_free_vfrag(&vfrag);
	if(vfrag2)
		gse_free_vfrag(&vfrag2);

	return true;
}


bool Gse::PacketHandler::getHeaderExtensions(const Rt::Ptr<NetPacket>& packet,
                                             std::string callback_name,
                                             void *opaque)
{
	gse_vfrag_t *gse_data;
	gse_status_t status;

	status = gse_create_vfrag_with_data(&gse_data,
	                                    packet->getTotalLength(),
	                                    0, 0,
	                                    packet->getRawData(),
	                                    packet->getTotalLength());
	if(status != GSE_STATUS_OK)
	{
		LOG(this->log, LEVEL_ERROR, "cannot create virtual fragment (%s)",
		    gse_get_status(status));
		return false;
	}
	
	// Get the in-band extension
	status = gse_deencap_get_header_ext(packet->getRawData(),
	                                    this->deencap_callback[callback_name],
	                                    opaque);
	if(status != GSE_STATUS_OK && status != GSE_STATUS_EXTENSION_UNAVAILABLE)
	{
		LOG(this->log, LEVEL_ERROR, 
		    "cannot deencapsulate header extension (%s)",
		    gse_get_status(status));
		return false;
	}

	return true;
}


// Static methods

bool Gse::setLabel(const NetPacket &packet, uint8_t label[])
{
	tal_id_t src_tal_id = packet.getSrcTalId();
	tal_id_t dst_tal_id = packet.getDstTalId();
	qos_t qos = packet.getQos();

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
	label[3] = 0;
	label[4] = 0;
	label[5] = 0;

	return true;
}

bool Gse::setLabel(const GseEncapCtx &context, uint8_t label[])
{
	uint8_t src_tal_id = context.getSrcTalId();
	uint8_t dst_tal_id = context.getDstTalId();
	uint8_t qos = context.getQos();

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
	label[3] = 0;
	label[4] = 0;
	label[5] = 0;

	return true;
}

uint8_t Gse::getSrcTalIdFromLabel(const uint8_t label[])
{
	return label[0] & 0x1F;
}

uint8_t Gse::getDstTalIdFromLabel(const uint8_t label[])
{
	return label[1] & 0x1F;
}

uint8_t Gse::getQosFromLabel(const uint8_t label[])
{
	return label[2] & 0x07;
}

// TODO add const here once NetPacket getter will be const
uint8_t Gse::getFragId(const NetPacket &packet)
{
	uint8_t src_tal_id = packet.getSrcTalId();
	uint8_t qos = packet.getQos();

	return ((src_tal_id & 0x1F) << 3 | ((qos & 0x07)));
}

uint8_t Gse::getFragId(const GseEncapCtx &context)
{
	uint8_t src_tal_id = context.getSrcTalId();
	uint8_t qos = context.getQos();

	return ((src_tal_id & 0x1F) << 3 | ((qos & 0x07)));
}

uint8_t Gse::getSrcTalIdFromFragId(const uint8_t frag_id)
{

	return (frag_id >> 3) & 0x1F;
}

uint8_t Gse::getDstTalIdFromFragId(const uint8_t UNUSED(frag_id))
{
	// Not encoded in frag_id TODO
	return 0x1F;
}

uint8_t Gse::getQosFromFragId(const uint8_t frag_id)
{
	return frag_id & 0x07;
}


