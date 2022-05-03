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
 * @file BlockMesh.cpp
 * @brief Block that handles mesh or star architecture on satellites
 * @author Yohan Simard <yohan.simard@viveris.com>
 */

#include "BlockMesh.h"
#include "NetBurst.h"
#include "NetPacket.h"
#include "NetPacketSerializer.h"
#include <iterator>

BlockMesh::BlockMesh(const std::string &name, tal_id_t entity_id):
    Block(name), entity_id{entity_id} {}

bool BlockMesh::onInit()
{
	auto conf = OpenSandModelConf::Get();
	auto downward = dynamic_cast<Downward *>(this->downward);
	auto upward = dynamic_cast<Upward *>(this->upward);

	bool mesh_arch = conf->isMeshArchitecture();
	upward->mesh_architecture = mesh_arch;
	downward->mesh_architecture = mesh_arch;
	LOG(log_init, LEVEL_INFO, "Architecture: %s", mesh_arch ? "mesh" : "star");

	if (!conf->getInterSatLinkCarriers(entity_id, downward->isl_in, upward->isl_out))
	{
		return false;
	}

	auto handled_entities = conf->getEntitiesHandledBySat(entity_id);
	upward->handled_entities = handled_entities;
	downward->handled_entities = handled_entities;

	std::stringstream ss;
	std::copy(handled_entities.begin(), handled_entities.end(), std::ostream_iterator<tal_id_t>{ss, " "});
	LOG(log_init, LEVEL_INFO, "Handled entities: %s", ss.str().c_str());

	tal_id_t default_entity;
	if (!conf->getDefaultEntityForSat(entity_id, default_entity))
	{
		return false;
	}
	upward->default_entity = default_entity;
	downward->default_entity = default_entity;
	LOG(log_init, LEVEL_INFO, "Default entity: %d", default_entity);
	
	return true;
}

/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockMesh::Upward::Upward(const std::string &name, tal_id_t UNUSED(sat_id)):
    RtUpwardMux(name) {}

bool BlockMesh::Upward::onInit()
{
	// Open the inter-satellite out channel
	std::string local_ip_addr;
	if (!OpenSandModelConf::Get()->getSatInfrastructure(local_ip_addr))
	{
		return false;
	};
	std::string isl_name = getName() + "_isl_out";
	LOG(log_init, LEVEL_INFO, "Creating ISL output channel bound to %s, sending to %s:%d",
	    local_ip_addr.c_str(),
	    isl_out.address.c_str(),
	    isl_out.port);
	isl_out_channel = std::unique_ptr<UdpChannel>{
	    new UdpChannel{isl_name,
	                   0, // unused (spot id)
	                   isl_out.id,
	                   false, // input
	                   true,  // output
	                   isl_out.port,
	                   isl_out.is_multicast,
	                   local_ip_addr,
	                   isl_out.address,
	                   isl_out.udp_stack,
	                   isl_out.udp_rmem,
	                   isl_out.udp_wmem}};
	if (addNetSocketEvent(isl_name, isl_out_channel->getChannelFd()) == -1)
	{
		return false;
	}
	return true;
}

bool BlockMesh::Upward::onEvent(const RtEvent *const event)
{
	if (event->getType() != event_type_t::evt_message)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
		    event->getName().c_str());
		return false;
	}

	auto msg_event = static_cast<const MessageEvent *>(event);

	if (msg_event->getMessageType() != msg_data)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected message received: %s",
		    msg_event->getName().c_str());
		return false;
	}

	auto burst = static_cast<const NetBurst *>(msg_event->getData());
	return handleNetBurst(std::unique_ptr<const NetBurst>(burst));
	// TODO: make event a unique_ptr? should we delete it or not?
}

bool BlockMesh::Upward::handleNetBurst(std::unique_ptr<const NetBurst> burst)
{
	if (burst->empty())
		return true;

	NetPacket &msg = *burst->front();

	// TODO: check that getDstTalId() returns the final destination
	tal_id_t dest_entity = msg.getDstTalId();

	LOG(log_receive, LEVEL_INFO, "Handling a NetBurst from entity %d to entity %d",
	    msg.getSrcTalId(), dest_entity);

	auto conf = OpenSandModelConf::Get();
	component_t default_entity_type = conf->getEntityType(default_entity);

	if (mesh_architecture &&
	    handled_entities.find(dest_entity) == handled_entities.end() &&
	    default_entity_type == satellite)
	{
		return sendViaIsl(std::move(burst));
	}
	else
	{
		return sendToOppositeChannel(std::move(burst));
	}
	// unreachable
	return false;
}

bool BlockMesh::Upward::sendToOppositeChannel(std::unique_ptr<const NetBurst> burst)
{
	LOG(log_send, LEVEL_INFO, "Sending a NetBurst to the opposite channel");

	auto burst_ptr = burst.release();
	bool ok = shareMessage((void **)&burst_ptr, sizeof(NetBurst), msg_data);
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to transmit message to the opposite channel");
		delete burst_ptr;
		return false;
	}
	return true;
}

bool BlockMesh::Upward::sendViaIsl(std::unique_ptr<const NetBurst> burst)
{
	LOG(log_send, LEVEL_INFO, "Sending a NetBurst via ISL");

	for (auto &&pkt: *burst)
	{
		net_packet_buffer_t buf{pkt};
		if (!isl_out_channel->send((uint8_t *)&buf, sizeof(net_packet_buffer_t)))
		{
			LOG(this->log_send, LEVEL_ERROR,
			    "Failed to transmit message via ISL");
			return false;
		}
	}
	return true;
}

/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/

BlockMesh::Downward::Downward(const std::string &name, tal_id_t UNUSED(sat_id)):
    RtDownwardDemux<component_t>(name) {}

bool BlockMesh::Downward::onInit()
{
	std::string local_ip_addr;
	if (!OpenSandModelConf::Get()->getSatInfrastructure(local_ip_addr))
	{
		return false;
	};
	std::string isl_name = getName() + "_isl_in";
	LOG(log_init, LEVEL_INFO, "Creating ISL input channel listening on %s:%d",
	    local_ip_addr.c_str(),
	    isl_in.port);
	isl_in_channel = std::unique_ptr<UdpChannel>{
	    new UdpChannel{isl_name,
	                   0, // unused (spot id)
	                   isl_in.id,
	                   true,  // input
	                   false, // output
	                   isl_in.port,
	                   isl_in.is_multicast,
	                   local_ip_addr,
	                   isl_in.address,  // unused for now (dest IP), but may be used if we switch to multicast for ISL
	                   isl_in.udp_stack,
	                   isl_in.udp_rmem,
	                   isl_in.udp_wmem}};
	if (addNetSocketEvent(isl_name, isl_in_channel->getChannelFd()) == -1)
	{
		return false;
	}
	return true;
}

bool BlockMesh::Downward::onEvent(const RtEvent *const event)
{
	switch (event->getType())
	{
		case evt_message:
			return handleMessageEvent(static_cast<const MessageEvent *>(event));
		case evt_net_socket:
			return handleNetSocketEvent((NetSocketEvent *)event);
		default:
			LOG(log_receive, LEVEL_ERROR, "Unexpected event received: %s",
			    event->getName().c_str());
			return false;
	}
	// TODO: make event a unique_ptr? should we delete it or not?
}

bool BlockMesh::Downward::handleMessageEvent(const MessageEvent *event)
{
	if (event->getMessageType() != msg_data)
	{
		LOG(log_receive, LEVEL_ERROR, "Unexpected message received: %s",
		    event->getName().c_str());
		return false;
	}
	LOG(log_receive, LEVEL_INFO, "Received a NetBurst MessageEvent");

	auto burst = static_cast<const NetBurst *>(event->getData());
	return handleNetBurst(std::unique_ptr<const NetBurst>(burst));
}

bool BlockMesh::Downward::handleNetSocketEvent(NetSocketEvent *event)
{
	LOG(log_receive, LEVEL_INFO, "Received a NetSocketEvent");

	std::size_t length;
	net_packet_buffer_t *buf;

	// TODO: remove this 
	if (!NetBurst::log_net_burst)
		NetBurst::log_net_burst = Output::Get()->registerLog(LEVEL_WARNING, "NetBurst");

	std::unique_ptr<NetBurst> burst{new NetBurst{}};
	int ret;
	do
	{
		ret = isl_in_channel->receive(event, reinterpret_cast<uint8_t **>(&buf), length);
		if (ret < 0 || length <= 0 || buf == nullptr)
		{
			LOG(this->log_receive, LEVEL_ERROR, "Error while receiving an ISL packet");
			return false;
		}
		burst->add(buf->deserialize().release());
		delete buf;
	} while (ret == 1 && !burst->isFull());

	return handleNetBurst(std::move(burst));
}

bool BlockMesh::Downward::handleNetBurst(std::unique_ptr<const NetBurst> burst)
{
	if (burst->empty())
		return true;

	auto conf = OpenSandModelConf::Get();

	NetPacket &first_pkt = *burst->front();

	LOG(log_receive, LEVEL_INFO, "Handling a NetBurst from entity %d to entity %d",
	    first_pkt.getSrcTalId(), first_pkt.getDstTalId());

	if (mesh_architecture) // Mesh architecture -> packet are routed according to their destination
	{
		// TODO: check that getDstTalId() returns the final destination
		tal_id_t dest_entity = first_pkt.getDstTalId();

		if (handled_entities.find(dest_entity) != handled_entities.end())
		{
			component_t dest_type = conf->getEntityType(dest_entity);
			if (dest_type == terminal)
			{
				return sendToLowerBlock(terminal, std::move(burst));
			}
			else if (dest_type == gateway)
			{
				return sendToLowerBlock(gateway, std::move(burst));
			}
			else
			{
				LOG(log_receive, LEVEL_ERROR, "Destination of the packet is neither a terminal nor a gateway");
				return false;
			}
		}
		else // destination not handled by this satellite -> transmit to default entity
		{
			component_t default_entity_type = conf->getEntityType(default_entity);
			if (default_entity_type == satellite)
			{
				return sendToOppositeChannel(std::move(burst));
			}
			else if (default_entity_type == gateway)
			{
				return sendToLowerBlock(gateway, std::move(burst));
			}
			else
			{
				LOG(log_receive, LEVEL_ERROR, "Default entity is neither a satellite nor a gateway");
				return false;
			}
		}
	}
	else // Star architecture -> packet are routed according to their source
	{
		// TODO: check that getSrcTalId() returns the actual source
		tal_id_t src_entity = first_pkt.getSrcTalId();
		component_t src_type = conf->getEntityType(src_entity);

		if (src_type == terminal)
		{
			return sendToLowerBlock(gateway, std::move(burst));
		}
		else if (src_type == gateway)
		{
			return sendToLowerBlock(terminal, std::move(burst));
		}
		else
		{
			LOG(log_receive, LEVEL_ERROR, "Source of the packet is neither a terminal nor a gateway");
			return false;
		}
	}
	// unreachable
	return false;
}

bool BlockMesh::Downward::sendToLowerBlock(component_t dest, std::unique_ptr<const NetBurst> burst)
{
	LOG(log_send, LEVEL_INFO, "Sending a NetBurst to the lower block, %s side", dest == gateway ? "GW" : "ST");
	auto burst_ptr = burst.release();
	bool ok = enqueueMessage(dest, (void **)&burst_ptr, sizeof(NetBurst), msg_data);
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to transmit message to the opposite channel");
		delete burst_ptr;
		return false;
	}
	return true;
}

bool BlockMesh::Downward::sendToOppositeChannel(std::unique_ptr<const NetBurst> burst)
{
	LOG(log_send, LEVEL_INFO, "Sending a NetBurst to the opposite channel");

	auto burst_ptr = burst.release();
	bool ok = shareMessage((void **)&burst_ptr, sizeof(NetBurst), msg_data);
	if (!ok)
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to transmit message to the opposite channel");
		delete burst_ptr;
		return false;
	}
	return true;
}
