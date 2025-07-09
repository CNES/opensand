/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file BlockLanAdaptation.cpp
 * @brief Interface between network interfaces and OpenSAND
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */


#include "BlockLanAdaptation.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandFrames.h"
#include "Ethernet.h"
#include "OpenSandModelConf.h"
#include "PacketSwitch.h"
#include "FifoElement.h"
#include "IslPlugin.h"

#include <opensand_output/Output.h>
#include <opensand_rt/FileEvent.h>
#include <opensand_rt/TimerEvent.h>
#include <opensand_rt/MessageEvent.h>

#include <cstdio>
#include <cstring>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>

#define TUNTAP_FLAGS_LEN 4 // Flags [2 bytes] + Proto [2 bytes]
#define TUNTAP_BUFSIZE MAX_ETHERNET_SIZE // ethernet header + mtu + options, crc not included


/**
 * constructor
 */
BlockLanAdaptation::BlockLanAdaptation(const std::string &name, la_specific specific):
	Rt::Block<BlockLanAdaptation, la_specific>{name, specific},
	tap_iface{specific.tap_iface}
{
}


Rt::DownwardChannel<BlockLanAdaptation>::DownwardChannel(const std::string &name, la_specific specific):
	Channels::Downward<DownwardChannel<BlockLanAdaptation>>{name},
	stats_period_ms{},
	contexts{},
	tal_id{specific.connected_satellite},
	state{specific.is_used_for_isl ? SatelliteLinkState::UP : SatelliteLinkState::DOWN},
	packet_switch{specific.packet_switch}
{
}
 

Rt::UpwardChannel<BlockLanAdaptation>::UpwardChannel(const std::string &name, la_specific specific):
	Channels::Upward<UpwardChannel<BlockLanAdaptation>>{name},
	sarp_table{},
	contexts{},
	tal_id{specific.connected_satellite},
	state{specific.is_used_for_isl ? SatelliteLinkState::UP : SatelliteLinkState::DOWN},
	packet_switch{specific.packet_switch},
	delay{specific.delay}
{
}

/**
 * initializers and setters
*/

void BlockLanAdaptation::generateConfiguration()
{
	Ethernet::generateConfiguration();
}

bool BlockLanAdaptation::onInit()
{
	LanAdaptationPlugin *plugin = Ethernet::constructPlugin();
	std::shared_ptr<LanAdaptationPlugin::LanAdaptationContext> context = plugin->getContext();
	if(!context->setUpperPacketHandler(nullptr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot use %s for packets read on the interface",
		    context->getName().c_str());
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "add lan adaptation: %s\n",
	    plugin->getName());

	// create TAP virtual interface
	int fd = -1;
	if(!this->allocTap(fd))
	{
		return false;
	}

	lan_contexts_t contexts;
	contexts.push_back(context);
	this->upward.setContexts(contexts);
	this->downward.setContexts(contexts);
	// we can share FD as one thread will write, the second will read
	this->upward.setFd(fd);
	this->downward.setFd(fd);

	return true;
}

bool Rt::DownwardChannel<BlockLanAdaptation>::onInit()
{
	// statistics timer
	if(!OpenSandModelConf::Get()->getStatisticsPeriod(this->stats_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'timers': missing parameter 'statistics'\n");
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "statistics_timer set to %f\n",
	    this->stats_period_ms);
	this->stats_timer = this->addTimerEvent("LanAdaptationStats",
	                                        ArgumentWrapper(this->stats_period_ms));
	return true;
}

bool Rt::UpwardChannel<BlockLanAdaptation>::onInit()
{
	if (this->state == SatelliteLinkState::UP)
	{
		// Initialize context here in ISL mode as we don't need the link up message to know our tal_id
		for(auto&& context : this->contexts)
		{
			if(!context->initLanAdaptationContext(this->tal_id, packet_switch))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "cannot initialize %s context\n",
				    context->getName());
				return false;
			}
		}
	}

	if (delay == nullptr)
	{
		// No need to poll, messages are sent directly
		return true;
	}

	time_ms_t polling_rate;
	if (!OpenSandModelConf::Get()->getDelayTimer(polling_rate))
	{
		LOG(log_init, LEVEL_ERROR, "Cannot get the polling rate for the delay timer");
		return false;
	}
	delay_timer = this->addTimerEvent(getName() + ".delay_timer",
	                                  ArgumentWrapper(polling_rate));
	return true;
}

void Rt::UpwardChannel<BlockLanAdaptation>::setContexts(const lan_contexts_t &contexts)
{
	this->contexts = contexts;
}

void Rt::DownwardChannel<BlockLanAdaptation>::setContexts(const lan_contexts_t &contexts)
{
	this->contexts = contexts;
}

void Rt::UpwardChannel<BlockLanAdaptation>::setFd(int fd)
{
	this->fd = fd;
}

void Rt::DownwardChannel<BlockLanAdaptation>::setFd(int fd)
{
	// add file descriptor for TAP interface
	this->addFileEvent("tap", fd, TUNTAP_BUFSIZE + 4);
}


bool Rt::DownwardChannel<BlockLanAdaptation>::onEvent(const Event& event)
{
	LOG(this->log_receive, LEVEL_ERROR,
			"unknown event received %s",
			event.getName().c_str());
	return false;
}

bool Rt::DownwardChannel<BlockLanAdaptation>::onEvent(const TimerEvent& event)
{
	if(event == this->stats_timer)
	{
		for(auto&& context : this->contexts)
		{
			context->updateStats(this->stats_period_ms);
		}
		return true;
	}

	LOG(this->log_receive, LEVEL_ERROR,
			"unknown timer event received %s\n",
			event.getName().c_str());
	return false;
}

bool Rt::DownwardChannel<BlockLanAdaptation>::onEvent(const MessageEvent& event)
{
	if(to_enum<InternalMessageType>(event.getMessageType()) == InternalMessageType::link_up)
	{
		// 'link is up' message advertised
		Ptr<T_LINK_UP> link_up_msg = event.getMessage<T_LINK_UP>();
		// save group id and TAL id sent by MAC layer
		this->tal_id = link_up_msg->tal_id;
		this->state = SatelliteLinkState::UP;
		return true;
	}

	// this is not a link up message, this should be a forward burst
	LOG(this->log_receive, LEVEL_DEBUG,
	    "Get a forward burst from opposite channel\n");
	Ptr<NetBurst> forward_burst = event.getMessage<NetBurst>();
	if (!this->enqueueMessage(std::move(forward_burst), to_underlying(InternalMessageType::decap_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to forward burst to lower layer\n");
		return false;
	}

	return true;
}

bool Rt::DownwardChannel<BlockLanAdaptation>::onEvent(const FileEvent& event)
{
	// read  data received on tap interface
	std::size_t length = event.getSize() - TUNTAP_FLAGS_LEN;
	Data read_data = event.getData();

	if(this->state != SatelliteLinkState::UP)
	{
		LOG(this->log_receive, LEVEL_NOTICE,
		    "packets received from TAP, but link is down "
		    "=> drop packets\n");
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "new %u-bytes packet received from network\n", length);
	Ptr<NetPacket> packet = make_ptr<NetPacket>(read_data.substr(TUNTAP_FLAGS_LEN), length);
	// Learn source_mac address
	packet->setSrcTalId(tal_id);
	packet_switch->learn(packet->getData(), tal_id);

	Ptr<NetBurst> burst = make_ptr<NetBurst>();
	burst->add(std::move(packet));

	for(auto &&context : this->contexts)
	{
		burst = context->encapsulate(std::move(burst));
		if(burst == nullptr)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle packet in %s context\n",
			    context->getName().c_str());
			return false;
		}
	}

	if (!this->enqueueMessage(std::move(burst), to_underlying(InternalMessageType::decap_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		return false;
	}

	return true;
}

bool Rt::UpwardChannel<BlockLanAdaptation>::onEvent(const Event& event)
{
	LOG(this->log_receive, LEVEL_ERROR,
	    "unknown event received %s",
	    event.getName().c_str());
	return false;
}

bool Rt::UpwardChannel<BlockLanAdaptation>::onEvent(const TimerEvent& event)
{
	if(delay != nullptr && event == delay_timer)
	{
		for (auto &&elem: delay_fifo)
		{
			auto packet = elem->releaseElem<NetPacket>();
			if (!this->writePacket(packet->getData()))
			{
				return false;
			}
		}

		return true;
	}

	LOG(this->log_receive, LEVEL_ERROR,
	    "unknown timer event received %s",
	    event.getName().c_str());
	return false;
}

bool Rt::UpwardChannel<BlockLanAdaptation>::onEvent(const MessageEvent& event)
{
	if(to_enum<InternalMessageType>(event.getMessageType()) == InternalMessageType::link_up)
	{
		// 'link is up' message advertised
		Ptr<T_LINK_UP> link_up_msg = event.getMessage<T_LINK_UP>();
		LOG(this->log_receive, LEVEL_INFO,
		    "link up message received (group = %u, tal = %u)\n",
		    link_up_msg->group_id,
		    link_up_msg->tal_id);

		if(this->state == SatelliteLinkState::UP)
		{
			LOG(this->log_receive, LEVEL_NOTICE, "duplicate link up msg\n");
			return false;
		}
		else
		{
			// save group id and TAL id sent by MAC layer
			this->tal_id = link_up_msg->tal_id;
			// initialize contexts
			for(auto&& context : this->contexts)
			{
				if(!context->initLanAdaptationContext(this->tal_id, packet_switch))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot initialize %s context\n",
					    context->getName().c_str());
					return false;
				}
			}
			this->state = SatelliteLinkState::UP;
			// transmit link up to opposite channel
			if(!this->shareMessage(std::move(link_up_msg), event.getMessageType()))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to transmit link up message to "
				    "opposite channel\n");
				return false;
			}
		}

		return true;
	}

	// not a link up message
	LOG(this->log_receive, LEVEL_INFO,
	    "packet received from lower layer\n");

	Ptr<NetBurst> burst = event.getMessage<NetBurst>();
	if(this->state != SatelliteLinkState::UP)
	{
		LOG(this->log_receive, LEVEL_NOTICE,
		    "packets received from lower layer, but "
		    "link is down => drop packets\n");
		return false;
	}

	return this->onMsgFromDown(std::move(burst));
}

bool Rt::UpwardChannel<BlockLanAdaptation>::onMsgFromDown(Ptr<NetBurst> burst)
{
	if(burst == nullptr)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		return false;
	}

	for(auto &&context : this->contexts)
	{
		burst = context->deencapsulate(std::move(burst));
		if(burst == nullptr)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle packet in %s context\n",
			    context->getName().c_str());
			return false;
		}
	}

	bool success = true;
	auto packet_iterator = burst->begin();
	Ptr<NetBurst> forward_burst = make_ptr<NetBurst>(nullptr);
	while(packet_iterator != burst->end())
	{
		Data packet = (*packet_iterator)->getData();
		tal_id_t pkt_tal_id_src = (*packet_iterator)->getSrcTalId();
		tal_id_t pkt_tal_id_dst = (*packet_iterator)->getDstTalId();
		bool forward = false;

		LOG(this->log_receive, LEVEL_INFO,
		    "packet from lower layer has terminal ID %u\n",
		    pkt_tal_id_dst);
		if(pkt_tal_id_src == this->tal_id)
		{
			// with broadcast, we would receive our own packets
			LOG(this->log_receive, LEVEL_INFO,
			    "reject packet with own terminal ID\n");
			++packet_iterator;
			continue;
		}
		// Learn source mac address
		if(packet_switch->learn(packet, pkt_tal_id_src))
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "The mac address %s learned from lower layer as "
			    "associated to tal_id %u\n",
			    Ethernet::getSrcMac(packet).str().c_str(),
			    pkt_tal_id_src);
		}

		if(packet_switch->isPacketForMe(packet, pkt_tal_id_src, forward))
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "%s packet received from lower layer & should be read\n",
			    (*packet_iterator)->getName().c_str());

			unsigned char head[TUNTAP_FLAGS_LEN];
			for(unsigned int i = 0; i < TUNTAP_FLAGS_LEN; i++)
			{
				// add the protocol flag in the header
				head[i] = (this->contexts.front())->getLanHeader(i, *packet_iterator);
				LOG(this->log_receive, LEVEL_DEBUG,
				    "Add 0x%2x for bit %u in TAP header\n",
				    head[i], i);
			}

			packet.insert(0, head, TUNTAP_FLAGS_LEN);
			if (delay == nullptr)
			{
				if(!this->writePacket(packet))
				{
					success = false;
					++packet_iterator;
					continue;
				}
			}
			else
			{
				if (!delay_fifo.push(make_ptr<NetPacket>(packet), delay->getSatDelay()))
				{
					LOG(this->log_receive, LEVEL_ERROR, "failed to push the message in the fifo\n");
					success = false;
					++packet_iterator;
					continue;
				}
			}

			LOG(this->log_receive, LEVEL_INFO,
			    "%s packet received from lower layer & forwarded "
			    "to network layer\n",
			    (*packet_iterator)->getName().c_str());
		}

		auto Conf = OpenSandModelConf::Get();
		//if(Conf->isGw(tal_id) && !Conf->isGw(pkt_tal_id))
		if(forward)
		{
			// packet should be forwarded
			if(!forward_burst)
			{
				try
				{
					forward_burst = make_ptr<NetBurst>();
				}
				catch (const std::bad_alloc& e)
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot create the burst for forward packets\n");
					++packet_iterator;
					continue;
				}
			}
			forward_burst->add(std::move(*packet_iterator));
			// remove packet from burst to avoid releasing it as it is now forwarded
			// erase return next element
			packet_iterator = burst->erase(packet_iterator);
		}
		else
		{
			// go to next element
			++packet_iterator;
		}
	}

	if(forward_burst)
	{
		for(auto&& context : this->contexts)
		{
			forward_burst = context->encapsulate(std::move(forward_burst));
			if(forward_burst == nullptr)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle packet in %s context\n",
				    context->getName().c_str());
				return false;
			}
		}

		LOG(this->log_receive, LEVEL_INFO,
		    "%d packet should be forwarded (multicast/broadcast or "
		    "unicast not for GW)\n", forward_burst->length());

		// transmit message to the opposite channel that will
		// send it to lower layer 
		if(!this->shareMessage(std::move(forward_burst), 0))
		{
			LOG(this->log_receive, LEVEL_ERROR,
					"failed to transmit forward burst to opposite "
					"channel\n");
			success = false;
		}
	}

	return success;
}

bool Rt::UpwardChannel<BlockLanAdaptation>::writePacket(const Data& packet)
{
	// TODO move into its own function for delay...
	if(write(this->fd, packet.data(), packet.length()) < 0)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to write data on tap interface: %s\n",
		    strerror(errno));
		return false;
	}
	return true;
}

bool BlockLanAdaptation::allocTap(int &fd)
{
	struct ifreq ifr;
	int err;

	fd = open("/dev/net/tun", O_RDWR);
	if(fd < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot open '/dev/net/tun': %s\n",
		    strerror(errno));
		return false;
	}

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */

	/* create TAP interface */
	LOG(this->log_init, LEVEL_INFO,
	    "create %s interface\n",
	    this->tap_iface.c_str());
	memcpy(ifr.ifr_name, this->tap_iface.c_str(), IFNAMSIZ);
	ifr.ifr_flags = IFF_TAP;

	err = ioctl(fd, TUNSETIFF, static_cast<void *>(&ifr));
	if(err < 0)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot set flags on file descriptor %s\n",
		    strerror(errno));
		close(fd);
		return false;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "TAP handle with fd %d initialized\n", fd);

	return true;
}
