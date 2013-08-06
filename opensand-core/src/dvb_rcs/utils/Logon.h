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
 * @file    Logon.h
 * @brief   Represent a Logon Request and Response
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _LOGON_H_
#define _LOGON_H_


#include "OpenSandCore.h"
#include "OpenSandFrames.h"


/**
 * @class LogonRequest
 * @brief Represent a Logon request
 */
class LogonRequest: public OpenSandFrame<T_DVB_LOGON_REQ>
{
 public:

	/**
	 * @brief Logon request constructor for terminal (sender)
	 *
	 * @param mac           The terminal MAC id
	 * @param rt_bandwidth  The terminal fixed bandwidth for RT applications
	 *                      (used for CRA)
	 */
	LogonRequest(uint16_t mac, uint16_t rt_bandwidth);

	/**
	 * @brief Logon request constructor for NCC (receiver)
	 *
	 * @param frame   The DVB frame containing the logon request
	 * @ aram length  The DVB frame length
	 */
	LogonRequest(unsigned char *frame, size_t length);

	~LogonRequest();

	/**
	 * @brief Get the mac field
	 *
	 * @return the mac field
	 */
	uint16_t getMac(void) const;

	/**
	 * @brief Get the rt_bandwidth field
	 *
	 * @return the RT bandwidth field
	 */
	uint16_t getRtBandwidth(void) const;

};


/**
 * @class LogonResponse
 * @brief Represent a Logon response
 */
class LogonResponse: public OpenSandFrame<T_DVB_LOGON_RESP>
{
 public:

	/**
	 * @brief Logon response constructor for NCC ((sender)
	 *
	 * @param mac       The terminal MAC id
	 * @param group_id  The terminal group ID
	 * @param logon_id  The terminal logon ID
	 */
	LogonResponse(uint16_t mac, uint8_t group_id, uint16_t logon_id);

	/**
	 * @brief Logon response constructor for Terminal (receiver)
	 *
	 * @param frame  The DVB frame containing the logon response
	 * @ aram length  The DVB frame length
	 */
	LogonResponse(unsigned char *frame, size_t length);

	~LogonResponse();

	/**
	 * @brief Get the mac field
	 *
	 * @return the mac field
	 */
	uint16_t getMac(void) const;

	/**
	 * @brief Get group_id field
	 *
	 * @return the group ID field
	 */
	uint8_t getGroupId(void) const;

	/**
	 * @brief Get the logon_id field
	 *
	 * @return the logon ID field
	 */
	uint16_t getLogonId(void) const;
};


#endif

