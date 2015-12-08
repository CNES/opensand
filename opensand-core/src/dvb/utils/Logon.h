/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 * Copyright © 2015 CNES
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
#include "DvbFrame.h"


/**
 * @class LogonRequest
 * @brief Represent a Logon request
 */
class LogonRequest: public DvbFrameTpl<T_DVB_LOGON_REQ>
{
 public:

	/**
	 * @brief Logon request constructor for terminal (sender)
	 *
	 * @param mac           The terminal MAC id
	 * @param rt_bandwidth  The terminal fixed bandwidth for RT applications
	 *                      (used for CRA)
	 * @param max_rbdc      The maximum RBDC value
	 * @param max_vbdc      The maximum VBDC value
	 */
	LogonRequest(tal_id_t mac,
	             rate_kbps_t rt_bandwidth,
	             rate_kbps_t max_rbdc,
	             rate_kbps_t max_vbdc);
	
	/**
	 * @brief Logon request constructor for terminal (sender)
	 *
	 * @param mac           The terminal MAC id
	 * @param rt_bandwidth  The terminal fixed bandwidth for RT applications
	 *                      (used for CRA)
	 * @param max_rbdc      The maximum RBDC value
	 * @param max_vbdc      The maximum VBDC value
	 * @param is_scpc       The terminal is Scpc
	 */
	LogonRequest(tal_id_t mac,
	             rate_kbps_t rt_bandwidth,
	             rate_kbps_t max_rbdc,
	             rate_kbps_t max_vbdc,
	             bool is_scpc);

	/**
	 * @brief Logon request constructor for NCC (receiver)
	 */
	LogonRequest();

	~LogonRequest();

	/**
	 * @brief Get the mac field
	 *
	 * @return the mac field
	 */
	tal_id_t getMac(void) const;

	/**
	 * @brief Get the rt_bandwidth field
	 *
	 * @return the RT bandwidth field
	 */
	rate_kbps_t getRtBandwidth(void) const;

	/**
	 * @brief Get the max_rbdc field
	 *
	 * @return the maximum RBDC field
	 */
	rate_kbps_t getMaxRbdc(void) const;

	/**
	 * @brief Get the max_vbdc field
	 *
	 * @return the maximum VBDC field
	 */
	rate_kbps_t getMaxVbdc(void) const;

	/**
	 * @brief get the if the terminal is scpc
	 *
	 * @return the scpc value
	 */ 
	bool getIsScpc(void) const;	

};


/**
 * @class LogonResponse
 * @brief Represent a Logon response
 */
class LogonResponse: public DvbFrameTpl<T_DVB_LOGON_RESP>
{
 public:

	/**
	 * @brief Logon response constructor for NCC ((sender)
	 *
	 * @param mac       The terminal MAC id
	 * @param group_id  The terminal group ID
	 * @param logon_id  The terminal logon ID
	 */
	LogonResponse(tal_id_t mac, group_id_t group_id, tal_id_t logon_id);

	~LogonResponse();

	/**
	 * @brief Get the mac field
	 *
	 * @return the mac field
	 */
	tal_id_t getMac(void) const;

	/**
	 * @brief Get group_id field
	 *
	 * @return the group ID field
	 */
	group_id_t getGroupId(void) const;

	/**
	 * @brief Get the logon_id field
	 *
	 * @return the logon ID field
	 */
	tal_id_t getLogonId(void) const;
};


#endif

