/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file BlockSatCarrier.cpp
 * @brief This bloc implements a satellite carrier emulation.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "BlockSatCarrier.h"

// logs configuration
#define DBG_PACKAGE PKG_SAT_CARRIER
#include "opensand_conf/uti_debug.h"
#include "msg_dvb_rcs.h"
#include "lib_dvb_rcs.h"
#include "OpenSandCore.h"


/**
 * Constructor, use mgl_bloc default constructor
 * @see mgl_bloc::mgl_bloc(mgl_blocmgr *ip_blocmgr, mgl_id i_fatherid, char *ip_name)
 */
BlockSatCarrier::BlockSatCarrier(const string &name,
                                 component_t host,
                                 struct sc_specific specific): 
	Block(name),
	host(host),
	ip_addr(specific.ip_addr),
	interface_name(specific.iface_name)
{
}

/**
 * Destructor
 */
BlockSatCarrier::~BlockSatCarrier()
{
}


bool BlockSatCarrier::onDownwardEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			T_DVB_META *lp_ptr;
			long l_len;
			long l_ret;

			UTI_DEBUG_L3("message event received: %s\n", event->getName().c_str());

			lp_ptr = (T_DVB_META *)((MessageEvent *)event)->getData();
			l_len = ((MessageEvent *)event)->getLength();

			l_ret = m_channelSet.send(lp_ptr->carrier_id,
			                          (unsigned char *) (lp_ptr->hdr), l_len);
			free(lp_ptr->hdr);
			free(lp_ptr);
		}
		break;

		default:
			UTI_ERROR("unknown event received %s", event->getName().c_str());
			return false;
	}
	return true;
}


bool BlockSatCarrier::onUpwardEvent(const RtEvent *const event)
{
	bool status = true;

	switch(event->getType())
	{
		case evt_net_socket:
		{
			// Data to read in Sat_Carrier socket buffer
			unsigned int l_lg;
			// TODO #define !
			static unsigned char l_buf[9000];

			unsigned int carrier_id;
			int ret;

			UTI_DEBUG_L3("FD event received\n");

			do
			{
				ret = this->m_channelSet.receive(event->getFd(),
				                                 &carrier_id, l_buf, &l_lg,
				                                 sizeof(l_buf), 1000);
				UTI_DEBUG_L3("%d bytes of data received on carrier ID %u\n",
				             l_lg, carrier_id);
				if(ret < 0)
				{
					UTI_ERROR("failed to receive data on any "
					          "input channel (code = %d)\n", l_lg);
					status = false;
				}
				else
				{
					if(l_lg != 0)
					{
						this->onReceivePktFromCarrier(carrier_id, l_buf, l_lg);
					}
					else
					{
						status = false;
					}
				}
			} while(ret > 0);
		}
		break;

		default:
			UTI_ERROR("unknown event received %s", event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockSatCarrier::onInit()
{
	std::vector < sat_carrier_channel * >::iterator it;
	sat_carrier_channel *channel;

	// initialize all channels from the configuration file
	if(m_channelSet.readConfig(this->host,
	                           this->ip_addr,
	                           this->interface_name) < 0)
	{
		UTI_ERROR("Wrong channel set configuration\n");
		return false;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(it = m_channelSet.begin(); it != m_channelSet.end(); it++)
	{
		channel = *it;

		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			ostringstream name;

			UTI_INFO("Listen on fd %d for channel %d\n",
			         channel->getChannelFd(), channel->getChannelID());
			name << "Channel_" << channel->getChannelFd();
			this->upward->addNetSocketEvent(name.str(),
			                                channel->getChannelFd());
		}
	}

	return true;
}

/**
 * Packet reception
 * @param i_carrier the carrier of the packet
 * @param ip_buf a pointer to an internal static buffer
 * @param i_len the length of the data carried by *ip_buf
 */
void BlockSatCarrier::onReceivePktFromCarrier(unsigned int i_carrier,
                                              unsigned char *ip_buf,
                                              unsigned int i_len)
{
	const char FUNCNAME[] = "[onReceivePktFromCarrier]";
	unsigned char *lp_ptr;
	T_DVB_META *lp_meta;

	if(!ip_buf)
	{
		UTI_ERROR("%s ip_buf == 0, frame drop.", FUNCNAME);
		return;
	}

	if(i_len <= 0)
	{
		UTI_ERROR("%s i_len==%d <= 0, frame drop.", FUNCNAME, i_len);
		return;
	}

	if(i_len > MSG_BBFRAME_SIZE_MAX)
	{
		UTI_ERROR("%s i_len==%d > max==%ld, frame drop.", FUNCNAME,
		          i_len, MSG_BBFRAME_SIZE_MAX);
		return;
	}

	lp_ptr = (unsigned char *)malloc(MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX);
	if(!lp_ptr)
	{
		UTI_ERROR("%s Unable to get a packet from dvb pool, frame drop.",
		          FUNCNAME);
		return;
	}
	lp_meta = new T_DVB_META;
	if(!lp_meta)
	{
		UTI_ERROR("%s Unable to get a packet from dvb pool, frame drop.",
		          FUNCNAME);
		goto release_data;
	}

	// TODO: remove memcpy here ?
	memcpy(lp_ptr, ip_buf, i_len);	// Copy data
	// Configure meta
	lp_meta->carrier_id = i_carrier;
	lp_meta->hdr = (T_DVB_HDR *) lp_ptr;
	if(!this->sendUp((void **)(&lp_meta)))
	{
		UTI_ERROR("failed to send frame to upper layer\n");
		goto release_meta;
	}

	UTI_DEBUG("%s Msg (carrier %d) sent to upper", FUNCNAME, i_carrier);

	return;

release_meta:
	delete lp_meta;
release_data:
	delete lp_ptr;
}
