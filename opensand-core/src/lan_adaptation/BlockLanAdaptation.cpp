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
 * @file BlockLanAdaptation.cpp
 * @brief Interface between network interfaces and OpenSAND
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

// FIXME we need to include uti_debug.h before...
#define DBG_PACKAGE PKG_QOS_DATA
#include "opensand_conf/uti_debug.h"

#include "BlockLanAdaptation.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandFrames.h"

extern "C"
{
	#include "bridge_utils.h"
}

#include <cstdio>

#define TUNTAP_BUFSIZE MAX_ETHERNET_SIZE // ethernet header + mtu + options, crc not included
#define TUNTAP_FLAGS_LEN 4 // Flags [2 bytes] + Proto [2 bytes]

// TODO add it
//Event* BlockLanAdaptation::error_init = NULL;

/**
 * constructor
 */
BlockLanAdaptation::BlockLanAdaptation(const string &name, component_t host,
                                       string lan_iface):
	Block(name),
	sarp_table(),
	lan_iface(lan_iface),
	state(link_down),
	is_tap(false),
	// TODO add a parameter for that
	stats_period(53)
{
	// TODO we need a mutex here because some parameters may be used in upward and downward
	this->enableChannelMutex();

/*	if(error_init == NULL)
	{
		error_init = Output::registerEvent("blocLanAdaptation:init", LEVEL_ERROR);
	}*/
}


/**
 * destructor : Free all resources
 */
BlockLanAdaptation::~BlockLanAdaptation()
{
	if(this->is_tap)
	{
		this->delFromBridge();
	}

	// free some ressources of LanAdaptation block
	this->terminate();
}

bool BlockLanAdaptation::onInit()
{
	std::basic_ostringstream<char> cmd;

	// retrieve bloc config
	if(!this->getConfig())
	{
		UTI_ERROR("cannot load lan adaptation bloc configuration\n");
		return false;
	}

	this->is_tap = (*this->contexts.begin())->handleTap();
	// create TUN or TAP virtual interface
	if(!allocTunTap())
	{
		return false;
	}

	// add file descriptor for TUN interface
	this->downward->addFileEvent("tun", this->fd, TUNTAP_BUFSIZE + 4);

	UTI_INFO("TUN/TAP handle with fd %d initialized\n",
	         this->fd);

	return true;
}

bool BlockLanAdaptation::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_file:
			// input data available on TUN handle
			this->onMsgFromUp((NetSocketEvent *)event);
			break;

		case evt_timer:
			if(*event == this->stats_timer)
			{
				for(lan_contexts_t::iterator it= this->contexts.begin();
				    it != this->contexts.end(); ++it)
				{
					(*it)->updateStats(this->stats_period);
	
				}
			}
			else
			{
				UTI_ERROR("unknown timer event received %s\n",
				          event->getName().c_str());
				return false;
			}
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockLanAdaptation::onUpwardEvent(const RtEvent *const event)
{
	string str;
	NetBurst *burst;

	switch(event->getType())
	{
		case evt_message:
			if(((MessageEvent *)event)->getMessageType() == msg_link_up)
			{
				T_LINK_UP *link_up_msg;

				// 'link is up' message advertised

				link_up_msg = (T_LINK_UP *)((MessageEvent *)event)->getData();
				UTI_DEBUG("link up message received (group = %u, tal = %u)\n",
				          link_up_msg->group_id, link_up_msg->tal_id);

				if(this->state == link_up)
				{
					UTI_INFO("duplicate link up msg\n");
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
						                                          this->satellite_type,
						                                          &this->sarp_table))
						{
							UTI_ERROR("cannot initialize %s context\n",
									  (*ctx_iter)->getName().c_str());
							return false;
						}
					}
					this->state = link_up;
				}

				delete link_up_msg;
				break;
			}
			// not a link up message
			UTI_DEBUG("packet received from lower layer\n");

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			if(this->state != link_up)
			{
				UTI_INFO("packets received from lower layer, but link is down "
				         "=> drop packets\n");
				delete burst;
				return false;
			}
			if(!this->onMsgFromDown(burst))
			{
				return false;
			}
			break;

		default:
			UTI_ERROR("unknown event received %s",
			          event->getName().c_str());
			return false;
	}

	return true;
}

/**
 * Manage packets received from lower layer:
 *  - build the TUN or TAP header with appropriate protocol identifier
 *  - write TUN/TAP header + packet to TUN/TAP interface
 *
 * @param burst  burst of packets received from lower layer
 * @return       true on success, false otherwise
 */
bool BlockLanAdaptation::onMsgFromDown(NetBurst *burst)
{
	bool success = true;
	NetBurst *forward_burst = NULL;

	if(burst == NULL)
	{
		UTI_ERROR("burst is not valid\n");
		return false;
	}

	for(lan_contexts_t::reverse_iterator ctx_iter = this->contexts.rbegin();
	    ctx_iter != this->contexts.rend(); ++ctx_iter)
	{
		burst = (*ctx_iter)->deencapsulate(burst);
		if(burst == NULL)
		{
			UTI_ERROR("failed to handle packet in %s context\n",
			          (*ctx_iter)->getName().c_str());
			return false;
		}
	}

	for(NetBurst::iterator iter = burst->begin(); iter != burst->end();
	    ++iter)
	{
		size_t length;
		tal_id_t pkt_tal_id = (*iter)->getDstTalId();
		UTI_DEBUG("packet from lower layer has terminal ID %u\n", pkt_tal_id);

		if(pkt_tal_id == BROADCAST_TAL_ID || pkt_tal_id == this->tal_id)
		{
			unsigned char *lan_packet;

			UTI_DEBUG("%s packet received from lower layer & should be read\n", 
			          (*iter)->getName().c_str());
			
			// allocate memory for data
			length = (*iter)->getTotalLength();
			lan_packet = (unsigned char *) calloc(length + TUNTAP_FLAGS_LEN,
			                                      sizeof(unsigned char));
			if(lan_packet == NULL)
			{
				UTI_ERROR("cannot allocate memory for sending %s data on TUN/TAP\n",
				          (*iter)->getName().c_str());
				delete *iter;
				success = false;
				continue;
			}
			memcpy(lan_packet + TUNTAP_FLAGS_LEN, (*iter)->getData().c_str(), length);

			for(unsigned int i = 0; i < TUNTAP_FLAGS_LEN; i++)
			{
				// add the protocol flag in the header
				lan_packet[i] = (this->contexts.front())->getLanHeader(i, *iter);
				UTI_DEBUG_L3("Add 0x%2x for bit %u in TUN/TAP header\n",
				             lan_packet[i], i);
			}

			// write data on TUN/TAP device
			if(write(this->fd, lan_packet, length + TUNTAP_FLAGS_LEN) < 0)
			{
				UTI_ERROR("Unable to write data on tun or tap interface: %s\n",
				          strerror(errno));
				free(lan_packet);
				success = false;
				continue;
			}

			UTI_DEBUG("%s packet received from lower layer & forwarded to "
					  "network layer\n", (*iter)->getName().c_str());
			free(lan_packet);
		}

		if(this->tal_id == GW_TAL_ID &&
		   this->satellite_type == TRANSPARENT &&
		   pkt_tal_id != GW_TAL_ID)
		{
			// packet should be forwarded
			NetPacket *forward_packet = new NetPacket((*iter)->getData());
			if(!forward_packet)
			{
				UTI_ERROR("cannot create the packet to forward\n");
				continue;
			}
			if(!forward_burst)
			{
				forward_burst = new NetBurst();
				if(!forward_burst)
				{
					UTI_ERROR("cannot create the burst for forward packets\n");
					continue;
				}
			}
			forward_burst->add(forward_packet);
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
				UTI_ERROR("failed to handle packet in %s context\n",
				          (*iter)->getName().c_str());
				return false;
			}
		}

		UTI_DEBUG("%d packet should be forwarded (multicast/broadcast or "
		          "unicast not for GW)\n", forward_burst->length());

		// FIXME we call a function from the opposite channel, this could create
		//       interblocking
		//       Create a communication interface between channels in Rt
		if(!this->sendDown((void **)&forward_burst, sizeof(forward_burst)))
		{
			UTI_ERROR("failed to send burst to lower layer\n");
			delete forward_burst;
			success = false;
		}
	}

	delete burst;
	return success;
}

/**
 * Manage an packet received from upper layer:
 *  - read data from TUN or TAP interface
 *  - create a packet with data
 *
 * @param event  The event on TUN/TAP interface
 * @return    true on success, false if there was an error
 */
bool BlockLanAdaptation::onMsgFromUp(NetSocketEvent *const event)
{
	unsigned char *read_data;
	unsigned char *data;
	unsigned int length;
	NetPacket *packet;
	NetBurst *burst;

	// read  data received on tun/tap interface
	// we need to memcpy as start pointer is not the same
	length = event->getSize() - TUNTAP_FLAGS_LEN;
	read_data = event->getData();
	data = read_data + TUNTAP_FLAGS_LEN;

	if(this->state != link_up)
	{
		UTI_INFO("packets received from TUN/TAP, but link is down "
		         "=> drop packets\n");
		free(read_data);
		return false;
	}

	UTI_DEBUG("new %u-bytes packet received from network\n", length);
	packet = new NetPacket(data, length);
	burst = new NetBurst();
	burst->add(packet);
	free(read_data);
	for(lan_contexts_t::iterator iter = this->contexts.begin();
	    iter != this->contexts.end(); ++iter)
	{
		burst = (*iter)->encapsulate(burst);
		if(burst == NULL)
		{
			UTI_ERROR("failed to handle packet in %s context\n",
			          (*iter)->getName().c_str());
			return false;
		}
	}

	if(!this->sendDown((void **)&burst, sizeof(burst)))
	{
		UTI_ERROR("failed to send burst to lower layer\n");
		delete burst;
		return false;
	}

	return true;
}

bool BlockLanAdaptation::allocTunTap()
{
	struct ifreq ifr;
	int err;

	this->fd = open("/dev/net/tun", O_RDWR);
	if(this->fd < 0)
	{
		UTI_ERROR("cannot open '/dev/net/tun': %s\n",
		          strerror(errno));
		return false;
	}

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */

	/* create TUN or TAP interface */
	UTI_DEBUG("create interface opensand_%s",
	          (this->is_tap ? "tap" : "tun"));
	snprintf(ifr.ifr_name, IFNAMSIZ, "opensand_%s",
	        (this->is_tap ? "tap" : "tun"));
	ifr.ifr_flags = (this->is_tap ? IFF_TAP : IFF_TUN);
	if(this->is_tap && !this->addInBridge())
	{
		return false;
	}

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if(err < 0)
	{
		UTI_ERROR("cannot set flags on file descriptor %s\n",
		          strerror(errno));
		close(this->fd);
		return false;
	}

	return true;
}


bool BlockLanAdaptation::addInBridge()
{
	struct ifreq ifr_br;
	const char *br = "opensand_br";
	int err = -1;

	memset(&ifr_br, 0, sizeof(ifr_br));
	snprintf(ifr_br.ifr_name, IFNAMSIZ, "%s", br);

	err = br_init();
	if(err)
	{
		UTI_ERROR("Failed to init bridge: %s\n", strerror(errno));
		return false;
	}

	// remove interface if it is in bridge to avoid error when adding it
	br_del_interface(br, this->lan_iface.c_str());
	err = br_add_interface(br, this->lan_iface.c_str());
	if(err)
	{
		UTI_ERROR("Failed to add %s interface in bridge: %s\n",
		          this->lan_iface.c_str(), strerror(errno));
		br_shutdown();
		return false;
	}
	br_shutdown();

	// wait for bridge to be ready
	UTI_DEBUG("Wait for bridge to be ready\n");
	sleep(10);
	
	return true;
}

bool BlockLanAdaptation::delFromBridge()
{
	struct ifreq ifr_br;
	const char *br = "opensand_br";
	int err = -1;

	memset(&ifr_br, 0, sizeof(ifr_br));
	snprintf(ifr_br.ifr_name, IFNAMSIZ, "%s", br);

	err = br_init();
	if(err)
	{
		UTI_ERROR("Failed to init bridge: %s\n", strerror(errno));
		return false;
	}

	err = br_del_interface(br, this->lan_iface.c_str());
	if(err)
	{
		UTI_ERROR("Failed to remove %s interface from bridge: %s\n",
		          this->lan_iface.c_str(), strerror(errno));
		br_shutdown();
		return false;
	}
	br_shutdown();
	return true;
}

