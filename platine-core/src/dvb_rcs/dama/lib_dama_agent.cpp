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
 * Platine is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file lib_dama_agent.cpp
 * @brief This library defines DAMA agent interfaces
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <stdarg.h>
#include <ctype.h>

#include "lib_dvb_rcs.h"
#include "lib_dama_agent.h"
#include "platine_conf/conf.h"

#define DBG_PACKAGE PKG_DAMA_DA
#include "platine_conf/uti_debug.h"
#define DA_DBG_PREFIX "[generic]"


/**
 * Constructor
 */
DvbRcsDamaAgent::DvbRcsDamaAgent()
{
	const char *FUNCNAME = DA_DBG_PREFIX "[constructor]";
	int val;
	string encap_scheme;     // encapsulation scheme for the uplink/return link
	int encap_packet_length; // uplink/return link MAC packet size

	m_groupId = 0;
	m_talId = 0;
	m_CRCarrierId = 0;
	m_currentSuperFrame = 0;
	m_nrt_fifo = 0;
	m_next_allocated = 0;
	m_NRTMaxBandwidth = 0;

	resetStatsCxt();

	// Frame duration - in ms
	if(globalConfig.getIntegerValue(DVB_MAC_LAYER_SECTION,
	                                DVB_FRM_DURATION, val) < 0)
	{
		val = DFLT_FRM_DURATION;
		UTI_ERROR("%s Missing %s, taking default value (%d).\n", FUNCNAME,
		          DVB_FRM_DURATION, val);
	}
	m_frameDuration = val;

	// get encap packet length and frame duration
	if(globalConfig.getStringValue(GLOBAL_SECTION, OUT_ENCAP_SCHEME,
	                               encap_scheme) < 0)
	{
		UTI_INFO("%s Section %s, %s missing. Uplink encapsulation "
		         "scheme set to ATM/AAL5.\n", FUNCNAME, GLOBAL_SECTION,
		         OUT_ENCAP_SCHEME);
		encap_scheme = ENCAP_ATM_AAL5;
	}

	if(encap_scheme == ENCAP_ATM_AAL5 ||
	   encap_scheme == ENCAP_ATM_AAL5_ROHC)
	{
		encap_packet_length = AtmCell::length();
	}
	else if(encap_scheme == ENCAP_MPEG_ULE ||
	        encap_scheme == ENCAP_MPEG_ULE_ROHC)
	{
		encap_packet_length = MpegPacket::length();
	}
	//TODO only for DVB-S2 uplink (no implemented yet)
#if 0
	else if(encap_scheme == ENCAP_GSE)
	{
		encap_packet_length = GsePacket::length();
		//TODO get real GSE packet length for statistics (used for converter)
	}
#endif
	else
	{
		UTI_INFO("%s bad value for uplink encapsulation scheme. ATM/AAL5 used "
		         "instead\n", FUNCNAME);
		encap_packet_length = AtmCell::length();
	}

	UTI_INFO("%s uplink encapsulation scheme = %s\n", FUNCNAME, encap_scheme.c_str());

	// init DamaUtils class used for conversion kbits/s <-> MAC packet/frame
	m_converter = new DU_Converter((int) m_frameDuration, encap_packet_length);
}

/**
 * Destructor
 */
DvbRcsDamaAgent::~DvbRcsDamaAgent()
{
	delete(m_converter);
}


/**
 * Initializes all data structure based on configuration file
 * @param nrt_fifo points to nrt_fifo being under authorization process
 * @param max_bandwidth is the maximum nrt bdwdth available for the ST
 * @param carrier_id is where the CR goes
 * @return 0 on success -1 otherwise and set m_error
 */
int DvbRcsDamaAgent::init(dvb_fifo * nrt_fifo, long max_bandwidth,
                          long carrier_id)
{
	if(nrt_fifo == NULL)
	{
		UTI_ERROR("nrt_fifo, NULL pointer\n");
		goto error;
	}

	m_nrt_fifo = nrt_fifo;
	m_CRCarrierId = carrier_id;
	m_NRTMaxBandwidth = max_bandwidth;
	return 0;

 error:

	return -1;
}

/**
 * Called when the DVB RCS layer receive a start of Frame
 * Process the frame, set the SuperFrame number and validate
 * previous received authorizations on the NRT fifo
 * @param buf a pointer to a DVB frame structure
 * @param len the length of the structure
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaAgent::hereIsSOF(unsigned char *buf, long len)
{
	const char *FUNCNAME = DA_DBG_PREFIX "[onRcvSOF]";
	T_DVB_SOF *sof;
	long capa;

	sof = (T_DVB_SOF *) buf;

	if(sof->hdr.msg_type != MSG_TYPE_SOF)
	{
		UTI_ERROR("non SOF msg type (%ld)\n", sof->hdr.msg_type);
		goto error;
	}

	// update the frame numerotation
	m_currentSuperFrame = sof->frame_nr;

	// If things are normal, after the first initialisation period of
	// three superframe (one for CR, one for SACT and one for TBTP) with 0
	// allocation (or a predefined one) we should receive
	// a TBTP per superframe. Hence the previously m_next_allocated
	// becomes the new allocated (for this superframe);
	if(m_NRTMaxBandwidth < m_next_allocated)
	{
		capa = m_NRTMaxBandwidth;
	}
	else
	{
		capa = m_next_allocated;
	}
	m_nrt_fifo->setCapacity(capa);
	UTI_DEBUG_L3("%s fifo#%d, capacity %ld.\n", FUNCNAME,
	             m_nrt_fifo->getId(), capa);

	m_next_allocated = 0;

	return (0);

error:

	return (-1);
}

/**
 * Extract a valid tal id and logon Id from the logonResp buffer
 * @param buf a pointer to a DVB frame structure
 * @param len the length of the structure
 * @return 0 (always succeed)
 */
int DvbRcsDamaAgent::hereIsLogonResp(unsigned char *buf, long len)
{
	T_DVB_LOGON_RESP *logon_resp = (T_DVB_LOGON_RESP *) buf;

	m_groupId = logon_resp->group_id;
	m_talId = logon_resp->logon_id;

	return 0;

}

/**
 * Used to pass to DAMA the TBTP received
 * @param buf points to the TBTP buffer
 * @param len length of the buffer
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaAgent::hereIsTBTP(unsigned char *buf, long len)
{
	const char *FUNCNAME = DA_DBG_PREFIX "[hereIsTBTP]";
	T_DVB_TBTP *tbtp = (T_DVB_TBTP *) buf;
	T_DVB_FRAME *frame;
	T_DVB_BTP *btp;
	int i, j;

	if(tbtp->hdr.msg_type != MSG_TYPE_TBTP)
	{
		UTI_ERROR("non TBTP msg type (%ld)\n", tbtp->hdr.msg_type);
		goto error;
	}

	if(m_groupId != tbtp->group_id)
	{
		UTI_DEBUG_L3("%s TBTP with different group_id (%d).\n",
		             FUNCNAME, tbtp->group_id);
		goto end;
	}

	UTI_DEBUG_L3("%s tbtp->frame_loop_count (%d).\n", FUNCNAME,
	             tbtp->frame_loop_count);

	frame = first_frame_ptr(tbtp);
	for(i = 0; i < tbtp->frame_loop_count; i++)
	{
		UTI_DEBUG_L3("%s frame#%d.\n", FUNCNAME, i);
		btp = first_btp_ptr(frame);
		for(j = 0; j < frame->btp_loop_count; j++)
		{
			UTI_DEBUG_L3("%s btp#%d.\n", FUNCNAME, j);
			if(m_talId == btp->logon_id)
			{
				m_next_allocated += btp->assignment_count;
				UTI_DEBUG_L3("%s\t#sf=%ld assign=%ld\n", FUNCNAME,
				             m_currentSuperFrame, btp->assignment_count);
			}
			else
			{
				UTI_DEBUG_L3("count:%ld, type:%d, channelid:%d, logonid:%d,"
				             "mchannelflag:%d, startslot:%d.\n",
				             btp->assignment_count,
				             btp->assignment_type,
				             btp->channel_id,
				             btp->logon_id,
				             btp->multiple_channel_flag, btp->start_slot);
				UTI_DEBUG("%s\tBTP is not for this st (btp->logon_id=%d\n)",
				          FUNCNAME, btp->logon_id);
			}
			btp = next_btp_ptr(btp);
		}
		frame = (T_DVB_FRAME *) btp; // Equiv to "frame=next_frame_ptr(frame)"
	}

	UTI_DEBUG("%s #sf=%ld m_next_allocated=%ld\n", FUNCNAME,
	          m_currentSuperFrame, m_next_allocated);
end:
	return 0;

error:
	return -1;
}


/**
 * Returns statistics of the DAMA agent in a context
 * @return: statistics context
 */
DAStatContext *DvbRcsDamaAgent::getStatsCxt()
{
	return (&m_statContext);
}

/**
 * Reset statistics context
 * @return: none
 */
void DvbRcsDamaAgent::resetStatsCxt()
{
	m_statContext.rbdcRequest = 0;
	m_statContext.vbdcRequest = 0;
	m_statContext.craAlloc = 0;
	m_statContext.globalAlloc = 0;
	m_statContext.unusedAlloc = 0;
}
