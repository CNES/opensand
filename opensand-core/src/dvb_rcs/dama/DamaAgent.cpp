/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
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
 * @file     DamaAgent.cpp
 * @brief    This class defines the DAMA Agent interfaces
 * @author   Audric Schiltknecht / Viveris Technologies
 */

#include "DamaAgent.h"

#include "lib_dvb_rcs.h"

#define DBG_PACKAGE PKG_DAMA_DA
#include "opensand_conf/uti_debug.h"

DamaAgent::DamaAgent(const EncapPlugin::EncapPacketHandler *pkt_hdl,
                     const std::map<unsigned int, DvbFifo *> &dvb_fifos):
	is_parent_init(false),
	packet_handler(pkt_hdl),
	dvb_fifos(dvb_fifos),
	group_id(0),
	current_superframe_sf(0),
	rbdc_enabled(false),
	vbdc_enabled(false),
	frame_duration_ms(0.0),
	cra_kbps(0.0),
	max_rbdc_kbps(0.0),
	rbdc_timeout_sf(0),
	max_vbdc_pkt(0),
	msl_sf(0),
	cr_output_only(false)
{
	resetStatsCxt();
}

DamaAgent::~DamaAgent()
{
}

bool DamaAgent::initParent(time_ms_t frame_duration_ms,
                           rate_kbps_t cra_kbps,
                           rate_kbps_t max_rbdc_kbps,
                           time_sf_t rbdc_timeout_sf,
                           vol_pkt_t max_vbdc_pkt,
                           time_sf_t msl_sf,
                           time_sf_t obr_period_sf,
                           bool cr_output_only)
{
	if(this->packet_handler == NULL)
	{
		UTI_ERROR("Packet handler is Null\n");
		goto error;
	}

	this->frame_duration_ms = frame_duration_ms;
	this->cra_kbps = cra_kbps;
	this->max_rbdc_kbps = max_rbdc_kbps;
	this->rbdc_timeout_sf = rbdc_timeout_sf;
	this->max_vbdc_pkt = max_vbdc_pkt;
	this->msl_sf = msl_sf;
	this->obr_period_sf = obr_period_sf;
	this->cr_output_only = cr_output_only;

	// Check if RBDC or VBDC CR are activated
	for(std::map<unsigned int, DvbFifo *>::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		cr_type_t cr_type = (*it).second->getCrType();
		switch(cr_type)
		{
			case cr_rbdc:
				this->rbdc_enabled = true;
				break;
			case cr_vbdc:
				this->vbdc_enabled = true;
				break;
			case cr_none:
				break;
			default:
				UTI_ERROR("Unknown CR type for FIFO #%d: %d\n",
				          (*it).second->getId(), cr_type);
			goto error;
		}
	}

	this->is_parent_init = true;

	return true;

 error:
	return false;
}

bool DamaAgent::hereIsLogonResp(const LogonResponse &response)
{
	this->group_id = response.group_id;
	this->tal_id = response.logon_id;
	return true;
}

bool DamaAgent::hereIsSOF(time_sf_t superframe_number_sf)
{
	this->current_superframe_sf = superframe_number_sf;
	return true;
}

bool DamaAgent::processOnFrameTick()
{
	this->stat_context.cra_alloc_kbps = this->cra_kbps;
	return true;
}

da_stat_context_t DamaAgent::getStatsCxt() const
{
	return this->stat_context;
}

void DamaAgent::resetStatsCxt()
{
	this->stat_context.rbdc_request_kbps = 0;
	this->stat_context.vbdc_request_pkt = 0;
	this->stat_context.cra_alloc_kbps = 0;
	this->stat_context.global_alloc_kbps = 0;
	this->stat_context.unused_alloc_kbps = 0;
}

/************************** Wrappers *****************************************/
//TODO remove all wrappers !!!
bool DamaAgent::hereIsSOF(unsigned char *buf, size_t len)
{
	T_DVB_SOF *sof;
	sof = (T_DVB_SOF *) buf;
	if(sof->hdr.msg_type != MSG_TYPE_SOF)
	{
		UTI_ERROR("Non SOF msg type (%ld)\n",
				  sof->hdr.msg_type);
		goto error;
	}

	if(!this->hereIsSOF(sof->frame_nr))
	{
		goto error;
	}

	return true;

 error:
	return false;
}

bool DamaAgent::hereIsLogonResp(unsigned char *buf, size_t len)
{
	T_DVB_LOGON_RESP *resp;
	resp = (T_DVB_LOGON_RESP *) buf;
	if(resp->hdr.msg_type != MSG_TYPE_SESSION_LOGON_RESP)
	{
		UTI_ERROR("Non logon resp msg type (%ld)\n",
				  resp->hdr.msg_type);
		goto error;
	}

	if(!this->hereIsLogonResp(*resp))
	{
		goto error;
	}

	return true;

 error:
	return false;
}
