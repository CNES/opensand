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
 * @file    Logon.cpp
 * @brief   Represent a Logon request and response
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#include <opensand_conf/uti_debug.h>

#include "Logon.h"


/* REQUEST */

LogonRequest::LogonRequest(tal_id_t mac, rate_kbps_t rt_bandwidth):
	OpenSandFrame<T_DVB_LOGON_REQ>(sizeof(T_DVB_LOGON_REQ))
{
	this->setMessageType(MSG_TYPE_SESSION_LOGON_REQ);
	this->setLength(sizeof(T_DVB_LOGON_REQ));
	this->frame->mac = htons(mac);
	this->frame->rt_bandwidth = htons(rt_bandwidth);
}

LogonRequest::LogonRequest(unsigned char *frame, size_t length):
	OpenSandFrame<T_DVB_LOGON_REQ>(frame, length)
{
	if(this->getMessageType() != MSG_TYPE_SESSION_LOGON_REQ)
	{
		UTI_ERROR("Frame is not a logon request\n");
	}
}

LogonRequest::~LogonRequest()
{
}

tal_id_t LogonRequest::getMac(void) const
{
	return ntohs(this->frame->mac);
}

rate_kbps_t LogonRequest::getRtBandwidth(void) const
{
	return ntohs(this->frame->rt_bandwidth);
}

/* RESPONSE */

LogonResponse::LogonResponse(tal_id_t mac, uint8_t group_id, tal_id_t logon_id):
	OpenSandFrame<T_DVB_LOGON_RESP>(sizeof(T_DVB_LOGON_RESP))
{
	this->setMessageType(MSG_TYPE_SESSION_LOGON_RESP);
	this->setLength(sizeof(T_DVB_LOGON_RESP));
	this->frame->mac = htons(mac);
	this->frame->group_id = group_id;
	this->frame->logon_id = htons(logon_id);
}

LogonResponse::LogonResponse(unsigned char *frame, size_t length):
	OpenSandFrame<T_DVB_LOGON_RESP>(frame, length)
{
	if(this->getMessageType() != MSG_TYPE_SESSION_LOGON_RESP)
	{
		UTI_ERROR("Frame is not a logon response\n");
	}
}

LogonResponse::~LogonResponse()
{
}

tal_id_t LogonResponse::getMac(void) const
{
	return ntohs(this->frame->mac);
}

uint8_t LogonResponse::getGroupId(void) const
{
	return this->frame->group_id;
}

tal_id_t LogonResponse::getLogonId(void) const
{
	return ntohs(this->frame->logon_id);
}


