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
#include "TrafficCategory.h"
#include "Ethernet.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>
#include <opensand_rt/NetSocketEvent.h>
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
BlockLanAdaptation::BlockLanAdaptation(const std::string &name, struct la_specific specific):
	Block{name},
	tap_iface{specific.tap_iface}
{
	this->specific = specific;
}


BlockLanAdaptation::Downward::Downward(const std::string &name, struct la_specific):
	RtDownward{name},
	stats_period_ms{},
	contexts{},
	state{SatelliteLinkState::DOWN}
{
}
 

BlockLanAdaptation::Upward::Upward(const std::string &name, struct la_specific):
	RtUpward{name},
	sarp_table{},
	contexts{},
	state{SatelliteLinkState::DOWN}
{
}

/**
 * initializers and setters
*/

void BlockLanAdaptation::generateConfiguration()
{
	Ethernet::generateConfiguration();
}

bool BlockLanAdaptation::onInit(void)
{
	LanAdaptationPlugin *plugin = Ethernet::constructPlugin();
	LanAdaptationPlugin::LanAdaptationContext *context = plugin->getContext();
	if(!context->setUpperPacketHandler(nullptr))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot use %s for packets read on the interface",
		    context->getName().c_str());
		return false;
	}
	LOG(this->log_init, LEVEL_INFO,
	    "add lan adaptation: %s\n",
	    plugin->getName().c_str());

	// create TAP virtual interface
	int fd = -1;
	if(!this->allocTap(fd))
	{
		return false;
	}

	BlockLanAdaptation::packet_switch = this->specific.packet_switch;
	lan_contexts_t contexts;
	contexts.push_back(context);
	((Upward *)this->upward)->setContexts(contexts);
	((Downward *)this->downward)->setContexts(contexts);
	// we can share FD as one thread will write, the second will read
	((Upward *)this->upward)->setFd(fd);
	((Downward *)this->downward)->setFd(fd);

	return true;
}

bool BlockLanAdaptation::Downward::onInit(void)
{
	// statistics timer
	if(!OpenSandModelConf::Get()->getStatisticsPeriod(this->stats_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'timers': missing parameter 'statistics'\n");
		return false;
	}
	LOG(this->log_init, LEVEL_NOTICE,
	    "statistics_timer set to %d\n",
	    this->stats_period_ms);
	this->stats_timer = this->addTimerEvent("LanAdaptationStats",
	                                        this->stats_period_ms);
	return true;
}

bool BlockLanAdaptation::Upward::onInit(void)
{
	//return OpenSandModelConf::Get()->getSarp(this->sarp_table);
	return true;
}

void BlockLanAdaptation::Upward::setContexts(const lan_contexts_t &contexts)
{
	this->contexts = contexts;
}

void BlockLanAdaptation::Downward::setContexts(const lan_contexts_t &contexts)
{
	this->contexts = contexts;
}

void BlockLanAdaptation::Upward::setFd(int fd)
{
	this->fd = fd;
}

void BlockLanAdaptation::Downward::setFd(int fd)
{
	// add file descriptor for TAP interface
	this->addFileEvent("tap", fd, TUNTAP_BUFSIZE + 4);
}


/**
 * destructor : Free all resources
 */
BlockLanAdaptation::~BlockLanAdaptation()
{
}

bool BlockLanAdaptation::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
    case EventType::Message:
		{
			auto msg_event = static_cast<const MessageEvent *>(event);
			if(to_enum<InternalMessageType>(msg_event->getMessageType()) == InternalMessageType::link_up)
			{
				// 'link is up' message advertised
				T_LINK_UP *link_up_msg = static_cast<T_LINK_UP *>(msg_event->getData());
				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = SatelliteLinkState::UP;
				delete link_up_msg;
				break;
			}

			// this is not a link up message, this should be a forward burst
			LOG(this->log_receive, LEVEL_DEBUG,
			    "Get a forward burst from opposite channel\n");
			NetBurst *forward_burst = static_cast<NetBurst *>(msg_event->getData());
			if (!this->enqueueMessage((void **)&forward_burst, 0, to_underlying(InternalMessageType::decap_data)))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to forward burst to lower layer\n");
				delete forward_burst;
				return false;
			}
		}
		break;

    case EventType::File:
			// input data available on TAP handle
			this->onMsgFromUp(static_cast<const NetSocketEvent *>(event));
			break;

    case EventType::Timer:
			if(*event == this->stats_timer)
			{
				for(lan_contexts_t::iterator it= this->contexts.begin();
				    it != this->contexts.end(); ++it)
				{
					(*it)->updateStats(this->stats_period_ms);
				}
			}
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "unknown timer event received %s\n",
				    event->getName().c_str());
				return false;
			}
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockLanAdaptation::Upward::onEvent(const RtEvent *const event)
{
  std::string str;

	switch(event->getType())
	{
    case EventType::Message:
		{
      auto msg_event = static_cast<const MessageEvent *>(event);
			if(to_enum<InternalMessageType>(msg_event->getMessageType()) == InternalMessageType::link_up)
			{
				// 'link is up' message advertised
				T_LINK_UP *link_up_msg = static_cast<T_LINK_UP *>(msg_event->getData());
				LOG(this->log_receive, LEVEL_INFO,
				    "link up message received (group = %u, "
				    "tal = %u)\n", link_up_msg->group_id,
				    link_up_msg->tal_id);

				if(this->state == SatelliteLinkState::UP)
				{
					LOG(this->log_receive, LEVEL_NOTICE,
					    "duplicate link up msg\n");
					delete link_up_msg;
					return false;
				}
				else
				{
					// save group id and TAL id sent by MAC layer
					this->group_id = link_up_msg->group_id;
					this->tal_id = link_up_msg->tal_id;
					// initialize contexts
					for(lan_contexts_t::iterator ctx_iter = this->contexts.begin();
					    ctx_iter != this->contexts.end(); ++ctx_iter)
					{
						if(!(*ctx_iter)->initLanAdaptationContext(this->tal_id,
							                                  this->group_id,
						                                          BlockLanAdaptation::packet_switch))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "cannot initialize %s context\n",
							    (*ctx_iter)->getName().c_str());
							delete link_up_msg;
							return false;
						}
					}
					this->state = SatelliteLinkState::UP;
					// transmit link up to opposite channel
					if(!this->shareMessage((void **)&link_up_msg,
					                       msg_event->getLength(),
					                       msg_event->getMessageType()))
					{
						LOG(this->log_receive, LEVEL_ERROR,
						    "failed to transmit link up message to "
						    "opposite channel\n");
						return false;
					}
				}
				break;
			}
			// not a link up message
			LOG(this->log_receive, LEVEL_INFO,
			    "packet received from lower layer\n");

			NetBurst *burst = static_cast<NetBurst *>(msg_event->getData());

			if(this->state != SatelliteLinkState::UP)
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "packets received from lower layer, but "
				    "link is down => drop packets\n");
				delete burst;
				return false;
			}
			if(!this->onMsgFromDown(burst))
			{
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "unknown event received %s",
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockLanAdaptation::Upward::onMsgFromDown(NetBurst *burst)
{
	bool success = true;
	NetBurst *forward_burst = nullptr;

	if(burst == nullptr)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		return false;
	}

	for(auto &&context : this->contexts)
	{
		burst = context->deencapsulate(burst);
		if(burst == nullptr)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle packet in %s context\n",
			    context->getName().c_str());
			return false;
		}
	}

	auto burst_it = burst->begin();
	while(burst_it != burst->end())
	{
		Data packet = (*burst_it)->getData();
		tal_id_t pkt_tal_id_src = (*burst_it)->getSrcTalId();
		tal_id_t pkt_tal_id_dst = (*burst_it)->getDstTalId();
		bool forward = false;
		
		LOG(this->log_receive, LEVEL_INFO,
		    "packet from lower layer has terminal ID %u\n",
		    pkt_tal_id_dst);
		if(pkt_tal_id_src == this->tal_id)
		{
			// with broadcast, we would receive our own packets
			LOG(this->log_receive, LEVEL_INFO,
			    "reject packet with own terminal ID\n");
			++burst_it;
			continue;
		}
		// Learn source mac address
		if(BlockLanAdaptation::packet_switch->learn(packet, pkt_tal_id_src))
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "The mac address %s learned from lower layer as "
			    "associated to tal_id %u\n", Ethernet::getSrcMac(packet).str().c_str(), pkt_tal_id_src);
			
		}

		if(BlockLanAdaptation::packet_switch->isPacketForMe(packet, pkt_tal_id_src, forward))
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "%s packet received from lower layer & should "
			    "be read\n", (*burst_it)->getName().c_str());
			
			unsigned char head[TUNTAP_FLAGS_LEN];
			for(unsigned int i = 0; i < TUNTAP_FLAGS_LEN; i++)
			{
				// add the protocol flag in the header
				head[i] = (this->contexts.front())->getLanHeader(i, *burst_it);
				LOG(this->log_receive, LEVEL_DEBUG,
				    "Add 0x%2x for bit %u in TAP header\n",
				    head[i], i);
			}

			packet.insert(0, head, TUNTAP_FLAGS_LEN);
			if(write(this->fd, packet.data(), packet.length()) < 0)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Unable to write data on tap "
				    "interface: %s\n", strerror(errno));
				success = false;
				++burst_it;
				continue;
			}

			LOG(this->log_receive, LEVEL_INFO,
			    "%s packet received from lower layer & forwarded "
			    "to network layer\n",
			    (*burst_it)->getName().c_str());
		}

		auto Conf = OpenSandModelConf::Get();
		//if(Conf->isGw(tal_id) && !Conf->isGw(pkt_tal_id))
		if(forward)
		{
			// packet should be forwarded
			/*  TODO avoid allocating new packet here !
			/  => remove packet from burst and use it*/
			if(!forward_burst)
			{
				forward_burst = new NetBurst();
				if(!forward_burst)
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "cannot create the burst for forward "
					    "packets\n");
					++burst_it;
					continue;
				}
			}
			forward_burst->add(std::move(*burst_it));
			// remove packet from burst to avoid releasing it as it is now forwarded
			// erase return next element
			burst_it = burst->erase(burst_it);
		}
		else
		{
			// go to next element
			++burst_it;
		}
	}
	if(forward_burst)
	{
		for(lan_contexts_t::iterator iter = this->contexts.begin();
		    iter != this->contexts.end(); ++iter)
		{
			forward_burst = (*iter)->encapsulate(forward_burst);
			if(forward_burst == NULL)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle packet in %s context\n",
				    (*iter)->getName().c_str());
				return false;
			}
		}

		LOG(this->log_receive, LEVEL_INFO,
		    "%d packet should be forwarded (multicast/broadcast or "
		    "unicast not for GW)\n", forward_burst->length());

		// transmit message to the opposite channel that will
		// send it to lower layer 
		if(!this->shareMessage((void **)&forward_burst))
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to transmit forward burst to opposite "
			    "channel\n");
			delete forward_burst;
			success = false;
		}
	}

	delete burst;
	return success;
}

bool BlockLanAdaptation::Downward::onMsgFromUp(const NetSocketEvent *const event)
{
	unsigned char *read_data;
	const unsigned char *data;
	unsigned int length;

	// read  data received on tap interface
	length = event->getSize() - TUNTAP_FLAGS_LEN;
	read_data = event->getData();
	data = read_data + TUNTAP_FLAGS_LEN;

	if(this->state != SatelliteLinkState::UP)
	{
		LOG(this->log_receive, LEVEL_NOTICE,
		    "packets received from TAP, but link is down "
		    "=> drop packets\n");
		delete [] read_data;
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "new %u-bytes packet received from network\n", length);
	auto packet = std::unique_ptr<NetPacket>(new NetPacket(data, length));
	// Learn source_mac address
	tal_id_t pkt_tal_id_src = packet->getSrcTalId();
	BlockLanAdaptation::packet_switch->learn(packet->getData(), pkt_tal_id_src);

	NetBurst *burst = new NetBurst();
	burst->add(std::move(packet));
	delete [] read_data;

	for(auto &&context : this->contexts)
	{
		burst = context->encapsulate(burst);
		if(burst == nullptr)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle packet in %s context\n",
			    context->getName().c_str());
			return false;
		}
	}

	if (!this->enqueueMessage((void **)&burst, 0, to_underlying(InternalMessageType::decap_data)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		delete burst;
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

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
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
