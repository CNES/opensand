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
 * @file bloc_sat_carrier.cpp
 * @brief This bloc implements a satellite carrier emulation.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "bloc_sat_carrier.h"

// logs configuration
#define DBG_PACKAGE PKG_SAT_CARRIER
#include "platine_conf/uti_debug.h"
#include "msg_dvb_rcs.h"
#include "lib_dvb_rcs.h"


/**
 * Constructor, use mgl_bloc default constructor
 * @see mgl_bloc::mgl_bloc(mgl_blocmgr *ip_blocmgr, mgl_id i_fatherid, char *ip_name)
 */
BlocSatCarrier::BlocSatCarrier(mgl_blocmgr *blocmgr, mgl_id fatherid,
                              const char *name): mgl_bloc(blocmgr, fatherid, name)
{
	initOk = false;
}

/**
 * Destructor
 */
BlocSatCarrier::~BlocSatCarrier()
{
}

/**
 * Event handler
 * @param event event to handle
 */
mgl_status BlocSatCarrier::onEvent(mgl_event *event)
{
	mgl_status status = mgl_ok;
	T_DVB_META *lp_ptr;
	long l_len;
	long l_ret;
	unsigned int l_lg;
	static unsigned char l_buf[9000];

	// Receive a dvb message from upper layer stack
	// The carrier id to use is set in field 'carrier_id'
	if(MGL_EVENT_IS_INIT(event))
	{
		// The first event received by each bloc is : mgl_event_type_init
		if(onInit() < 0)
		{
			fprintf(stderr, "Unable to initializes bloc_sat_carrier. Check log file. "
			        "Exiting.\n");
			exit(-1);
		}

		UTI_DEBUG("sat_carrier bloc is now ready\n");
		this->initOk = true;
	}
	else if(!this->initOk)
	{
		UTI_ERROR("sat_carrier bloc not initialized, ignore "
		          "non-init event\n");
	}
	else if(MGL_EVENT_IS_MSG(event))
	{
		UTI_DEBUG_L3("message event received\n");

		if(MGL_EVENT_MSG_IS_TYPE(event, msg_dvb) &&
		   MGL_EVENT_MSG_GET_SRCBLOC(event) == this->getUpperLayer())
		{
			lp_ptr = (T_DVB_META *) MGL_EVENT_MSG_GET_BODY(event);
			l_len = MGL_EVENT_MSG_GET_BODYLEN(event);

			UTI_DEBUG("message received from upper layer\n");

			l_ret = m_channelSet.send(lp_ptr->carrier_id, (unsigned char *) (lp_ptr->hdr), l_len);
			g_memory_pool_dvb_rcs.release((char *) (lp_ptr->hdr));
			g_memory_pool_dvb_rcs.release((char *) lp_ptr);
		}
		else
		{
			UTI_ERROR("message type is unknown\n");
			status = mgl_ko;
		}
	}
	else if(MGL_EVENT_IS_FD(event))
	{
		// Data to read in Sat_Carrier socket buffer

		unsigned int carrier_id;
		int ret;

		UTI_DEBUG_L3("FD event received\n");

		do
		{
			ret = this->m_channelSet.receive(MGL_EVENT_FD_GET_FD(event),
			                                 &carrier_id, l_buf, &l_lg,
			                                 sizeof(l_buf), 1000);
			UTI_DEBUG_L3("%d bytes of data received on carrier ID %u\n",
			             l_lg, carrier_id);
			if(ret < 0)
			{
				UTI_ERROR("failed to receive data on any "
				          "input channel (code = %d)\n", l_lg);
				status = mgl_ko;
			}
			else
			{
				if(l_lg != 0)
				{
					this->onReceivePktFromCarrier(carrier_id, l_buf, l_lg);
				}
				else
				{
					status = mgl_ko;
				}
			}
		} while(ret > 0);
	}
	else
	{
		UTI_ERROR("unknown event (type %ld) received\n", event->type);
		status = mgl_ko;
	}

	return status;
}


/**
 * Manage the initialization of the bloc
 * @return -1 if failed, 0 if succeed
 */
int BlocSatCarrier::onInit()
{
	std::vector < sat_carrier_channel * >::iterator it;
	sat_carrier_channel *channel;

	// initialize all channels from the configuration file
	if(m_channelSet.readConfig() < 0)
	{
		UTI_ERROR("[onInit] Wrong channel set configuration\n");
		return -1;
	}

	// ask the runtime to manage channel file descriptors
	// (only for channels that accept input)
	for(it = m_channelSet.begin(); it != m_channelSet.end(); it++)
	{
		channel = *it;

		if(channel->isInputOk() && channel->getChannelFd() != -1)
		{
			UTI_INFO("[onInit] Listen on fd %d for channel %d\n",
			         channel->getChannelFd(), channel->getChannelID());
			this->addFd(channel->getChannelFd());
		}
	}

	return 0;
}

/**
 * Packet reception
 * @param i_carrier the carrier of the packet
 * @param ip_buf a pointer to an internal static buffer
 * @param i_len the length of the data carried by *ip_buf
 */
void BlocSatCarrier::onReceivePktFromCarrier(unsigned int i_carrier,
                                             unsigned char *ip_buf,
                                             unsigned int i_len)
{
	const char FUNCNAME[] = "[onReceivePktFromCarrier]";
	char *lp_ptr;
	T_DVB_META *lp_meta;
	mgl_msg *lp_msg;

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

	//lp_ptr = g_memory_pool_dvb_rcs.get(HERE());
	lp_ptr = g_memory_pool_dvb_rcs.get();
	if(lp_ptr == 0)
	{
		UTI_ERROR("%s Unable to get a packet from dvb pool, frame drop.",
		          FUNCNAME);
		return;
	}
	lp_meta = (T_DVB_META *) g_memory_pool_dvb_rcs.get();
	if(lp_meta == 0)
	{
		UTI_ERROR("%s Unable to get a packet from dvb pool, frame drop.",
		          FUNCNAME);
		goto release;
	}

	memcpy(lp_ptr, ip_buf, i_len);	// Copy data
	// Configure meta
	lp_meta->carrier_id = i_carrier;
	lp_meta->hdr = (T_DVB_HDR *) lp_ptr;
	lp_msg = newMsgWithBodyPtr(msg_dvb, lp_meta, i_len);	// Get associated message
	if(lp_msg == 0)
	{
		UTI_ERROR("%s Failed to allocate mgl msg. Frame dropped.", FUNCNAME);
		goto release;
	}
	sendMsgTo(getUpperLayer(), lp_msg);	// Send message

	UTI_DEBUG("%s Msg (carrier %d) sent to upper", FUNCNAME, i_carrier);

	return;

release:
	g_memory_pool_dvb_rcs.release(lp_ptr);
	if(lp_ptr == 0)
	{
		g_memory_pool_dvb_rcs.release((char *) lp_meta);
	}
}
