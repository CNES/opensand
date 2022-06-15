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
 * @file    Logon.cpp
 * @brief   Represent a Logon request and response
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include "Logon.h"


/* REQUEST */

LogonRequest::LogonRequest(tal_id_t mac,
                           rate_kbps_t rt_bandwidth,
                           rate_kbps_t max_rbdc,
                           vol_kb_t max_vbdc):
	DvbFrameTpl<T_DVB_LOGON_REQ>()
{
	this->setMessageType(EmulatedMessageType::SessionLogonReq);
	this->setMessageLength(sizeof(T_DVB_LOGON_REQ));
	this->frame()->mac = htons(mac);
	this->frame()->rt_bandwidth = htons(rt_bandwidth);
	this->frame()->max_rbdc = htons(max_rbdc);
	this->frame()->max_vbdc = htons(max_vbdc);
	this->frame()->is_scpc = false;
}

LogonRequest::LogonRequest(tal_id_t mac,
                           rate_kbps_t rt_bandwidth,
                           rate_kbps_t max_rbdc,
                           vol_kb_t max_vbdc, 
                           bool is_scpc):
	DvbFrameTpl<T_DVB_LOGON_REQ>()
{
	this->setMessageType(EmulatedMessageType::SessionLogonReq);
	this->setMessageLength(sizeof(T_DVB_LOGON_REQ));
	this->frame()->mac = htons(mac);
	this->frame()->rt_bandwidth = htons(rt_bandwidth);
	this->frame()->max_rbdc = htons(max_rbdc);
	this->frame()->max_vbdc = htons(max_vbdc);
	this->frame()->is_scpc = is_scpc;
}

LogonRequest::LogonRequest():
	DvbFrameTpl<T_DVB_LOGON_REQ>()
{
}

LogonRequest::~LogonRequest()
{
}

tal_id_t LogonRequest::getMac(void) const
{
	return ntohs(this->frame()->mac);
}

rate_kbps_t LogonRequest::getRtBandwidth(void) const
{
	return ntohs(this->frame()->rt_bandwidth);
}

rate_kbps_t LogonRequest::getMaxRbdc(void) const
{
	return ntohs(this->frame()->max_rbdc);
}

rate_kbps_t LogonRequest::getMaxVbdc(void) const
{
	return ntohs(this->frame()->max_vbdc);
}

bool LogonRequest::getIsScpc(void) const
{
	return this->frame()->is_scpc;
}

/* RESPONSE */

LogonResponse::LogonResponse(tal_id_t mac, group_id_t group_id, tal_id_t logon_id):
	DvbFrameTpl<T_DVB_LOGON_RESP>()
{
	this->setMessageType(EmulatedMessageType::SessionLogonResp);
	this->setMessageLength(sizeof(T_DVB_LOGON_RESP));
	this->frame()->mac = htons(mac);
	this->frame()->group_id = group_id;
	this->frame()->logon_id = htons(logon_id);
}

LogonResponse::~LogonResponse()
{
}

tal_id_t LogonResponse::getMac(void) const
{
	return ntohs(this->frame()->mac);
}

group_id_t LogonResponse::getGroupId(void) const
{
	return this->frame()->group_id;
}

tal_id_t LogonResponse::getLogonId(void) const
{
	return ntohs(this->frame()->logon_id);
}


