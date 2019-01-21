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
 * @file BlockLanAdaptation.cpp
 * @brief Interface between network interfaces and OpenSAND
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#include "BlockLanAdaptation.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "OpenSandFrames.h"

extern "C"
{
	#include "bridge_utils.h"
}

#include <cstdio>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>

#define TUNTAP_FLAGS_LEN 4 // Flags [2 bytes] + Proto [2 bytes]


/**
 * constructor
 */
BlockLanAdaptation::BlockLanAdaptation(const string &name, string lan_iface):
	Block(name),
	lan_iface(lan_iface),
	is_tap(false)
{
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
}

bool BlockLanAdaptation::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			if(((MessageEvent *)event)->getMessageType() == msg_link_up)
			{
				T_LINK_UP *link_up_msg;

				// 'link is up' message advertised

				link_up_msg = (T_LINK_UP *)((MessageEvent *)event)->getData();
				// save group id and TAL id sent by MAC layer
				this->group_id = link_up_msg->group_id;
				this->tal_id = link_up_msg->tal_id;
				this->state = link_up;
				delete link_up_msg;
				break;
			}

			// this is not a link up message, this should be a forward burst
			LOG(this->log_receive, LEVEL_DEBUG,
			    "Get a forward burst from opposite channel\n");
			NetBurst *forward_burst;
			forward_burst = (NetBurst *)((MessageEvent *)event)->getData();
			if(!this->enqueueMessage((void **)&forward_burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to forward burst to lower layer\n");
				delete forward_burst;
				return false;
			}
		}
		break;

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
	string str;

	switch(event->getType())
	{
		case evt_message:
		{
			if(((MessageEvent *)event)->getMessageType() == msg_link_up)
			{
				T_LINK_UP *link_up_msg;

				// 'link is up' message advertised

				link_up_msg = (T_LINK_UP *)((MessageEvent *)event)->getData();
				LOG(this->log_receive, LEVEL_INFO,
				    "link up message received (group = %u, "
				    "tal = %u)\n", link_up_msg->group_id,
				    link_up_msg->tal_id);

				if(this->state == link_up)
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
						                                          this->satellite_type,
						                                          &this->sarp_table))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "cannot initialize %s context\n",
							    (*ctx_iter)->getName().c_str());
							delete link_up_msg;
							return false;
						}
					}
					this->state = link_up;
					// transmit link up to opposite channel
					if(!this->shareMessage((void **)&link_up_msg,
					                       ((MessageEvent *)event)->getLength(),
					                       ((MessageEvent *)event)->getMessageType()))
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
			NetBurst *burst;
			LOG(this->log_receive, LEVEL_INFO,
			    "packet received from lower layer\n");

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			if(this->state != link_up)
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
	NetBurst *forward_burst = NULL;
	NetBurst::iterator burst_it;

	if(burst == NULL)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "burst is not valid\n");
		return false;
	}

	for(lan_contexts_t::reverse_iterator ctx_iter = this->contexts.rbegin();
	    ctx_iter != this->contexts.rend(); ++ctx_iter)
	{
		burst = (*ctx_iter)->deencapsulate(burst);
		if(burst == NULL)
		{
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle packet in %s context\n",
			    (*ctx_iter)->getName().c_str());
			return false;
		}
	}

	burst_it = burst->begin();
	while(burst_it != burst->end())
	{
		tal_id_t pkt_tal_id = (*burst_it)->getDstTalId();
		LOG(this->log_receive, LEVEL_INFO,
		    "packet from lower layer has terminal ID %u\n",
		    pkt_tal_id);
		if((*burst_it)->getSrcTalId() == this->tal_id)
		{
			// with broadcast, we would receive our own packets
			LOG(this->log_receive, LEVEL_INFO,
			    "reject packet with own terminal ID\n");
			++burst_it;
			continue;
		}

		if(pkt_tal_id == BROADCAST_TAL_ID || pkt_tal_id == this->tal_id)
		{
			LOG(this->log_receive, LEVEL_INFO,
			    "%s packet received from lower layer & should "
			    "be read\n", (*burst_it)->getName().c_str());
			
			Data packet = (*burst_it)->getData();
			unsigned char head[TUNTAP_FLAGS_LEN];
			for(unsigned int i = 0; i < TUNTAP_FLAGS_LEN; i++)
			{
				// add the protocol flag in the header
				head[i] = (this->contexts.front())->getLanHeader(i, *burst_it);
				LOG(this->log_receive, LEVEL_DEBUG,
				    "Add 0x%2x for bit %u in TUN/TAP header\n",
				    head[i], i);
			}

			packet.insert(0, head, TUNTAP_FLAGS_LEN);
			if(write(this->fd, packet.data(), packet.length()) < 0)
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Unable to write data on tun or tap "
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

		if(OpenSandConf::isGw(tal_id) &&
		   this->satellite_type == TRANSPARENT &&
		   !OpenSandConf::isGw(pkt_tal_id))
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
			//forward_burst->add(forward_packet);
			forward_burst->add(*burst_it);
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

bool BlockLanAdaptation::Downward::onMsgFromUp(NetSocketEvent *const event)
{
	unsigned char *read_data;
	const unsigned char *data;
	unsigned int length;
	NetPacket *packet;
	NetBurst *burst;

	// read  data received on tun/tap interface
	length = event->getSize() - TUNTAP_FLAGS_LEN;
	read_data = event->getData();
	data = read_data + TUNTAP_FLAGS_LEN;

	if(this->state != link_up)
	{
		LOG(this->log_receive, LEVEL_NOTICE,
		    "packets received from TUN/TAP, but link is down "
		    "=> drop packets\n");
		free(read_data);
		return false;
	}

	LOG(this->log_receive, LEVEL_INFO,
	    "new %u-bytes packet received from network\n", length);
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
			LOG(this->log_receive, LEVEL_ERROR,
			    "failed to handle packet in %s context\n",
			    (*iter)->getName().c_str());
			return false;
		}
	}

	if(!this->enqueueMessage((void **)&burst))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to send burst to lower layer\n");
		delete burst;
		return false;
	}

	return true;
}

bool BlockLanAdaptation::allocTunTap(int &fd)
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

	/* create TUN or TAP interface */
	LOG(this->log_init, LEVEL_INFO,
	    "create interface opensand_%s\n",
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
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot set flags on file descriptor %s\n",
		    strerror(errno));
		close(fd);
		return false;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "TUN/TAP handle with fd %d initialized\n", fd);

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
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to init bridge: %s\n", strerror(errno));
		return false;
	}

	// remove interface if it is in bridge to avoid error when adding it
	br_del_interface(br, this->lan_iface.c_str());
	err = br_add_interface(br, this->lan_iface.c_str());
	if(err)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to add %s interface in bridge: %s\n",
		    this->lan_iface.c_str(), strerror(errno));
		br_shutdown();
		return false;
	}
	br_shutdown();

	// wait for bridge to be ready
	LOG(this->log_init, LEVEL_INFO,
	    "Wait for bridge to be ready\n");
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
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to init bridge: %s\n", strerror(errno));
		return false;
	}

	err = br_del_interface(br, this->lan_iface.c_str());
	if(err)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to remove %s interface from bridge: %s\n",
		    this->lan_iface.c_str(), strerror(errno));
		br_shutdown();
		return false;
	}
	br_shutdown();
	return true;
}

