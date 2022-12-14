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
 * @file    Sac.h
 * @brief   Represent a Satellite Access Control message
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _SAC_H_
#define _SAC_H_


#include "OpenSandCore.h"
#include "OpenSandFrames.h"
#include "DvbFrame.h"

#include <memory>
#include <vector>
#include <endian.h>


/// The maximum number of CR in a SAC
constexpr const uint8_t NBR_MAX_CR = 2;


class OutputLog;


///> The type of access for return/up link
enum class ReturnAccessType : uint8_t
{
	dama_vbdc   = 0, /* Volume Based CR */
	dama_rbdc   = 1, /* Rate Based CR */
	dama_avbdc  = 2, /* Absolute Volume Based */
	dama_cra    = 3, /* No CR, only use Constant Allocation */
	saloha      = 4, /* Slotted Aloha */
};


/**
 * The Emulated Capacity Requests field
 */
typedef struct
{
#if __BYTE_ORDER == __BIG_ENDIAN
	ReturnAccessType type:4;    ///< The CR type
	                   //   for DVB-RCS: 00 => VBDC
	                   //                01 => RBDC
	                   //                10 => AVBDC
	uint8_t prio:2;    ///< The request priority
	uint8_t scale:2;   ///< The scale of the request according to the values
	                   //   below (should be as small as possible):
	                   //   for DVB-RCS: 00 => 1
	                   //   01 => 16
	uint8_t value;     ///< The request value
	                   //   (the final requeted rate will be scale * value)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t prio:2;    ///< The request priority
	uint8_t scale:2;   ///< The scale of the request according to the values
	                   //   below (should be as small as possible):
	                   //   for DVB-RCS: 00 => 1
	                   //   01 => 16
	ReturnAccessType type:4;    ///< The CR type
	                   //   for DVB-RCS: 00 => VBDC
	                   //                01 => RBDC
	                   //                10 => AVBDC
	uint8_t value;     ///< The request value
	                   //   (the final requeted rate will be scale * value)
#else
#error "Please fix <bits/endian.h>"
#endif
} __attribute__((packed)) emu_cr_t;

/**
 * The CR info for CR computation
 */
typedef struct
{
	uint8_t prio;    ///< Request priority
	ReturnAccessType type;    ///< Request type
	uint32_t value;  ///< Request value
} cr_info_t;

/**
 * The ACM field
 */
typedef struct
{
	uint32_t cni;   ///< The C/N of the forward link
} __attribute__((packed)) emu_acm_t;

/**
 * The Emulated SAC field
 */
typedef struct
{
	tal_id_t tal_id;      ///< The terminal ID (logon_id)
	                      //   size 5 for physical ST, 5->max for simulated
	                      //   ST requests
	group_id_t group_id;  ///< The group ID 
	uint8_t cr_number;    ///< The number of CR in SAC
	emu_acm_t acm;        ///< The emulated ACM fields
	emu_cr_t cr[0];       ///< The emulated CR field
	                      //   when in frame, the length should be correctly set
	                      //   in order to send only the CR which were filled
} __attribute__((packed)) emu_sac_t;


/**
 * Satellite Access Control
 */
typedef struct
{
	T_DVB_HDR hdr;           ///< Basic DVB Header
	emu_sac_t sac;
} __attribute__((packed)) T_DVB_SAC;


/**
 * @class Sac
 * @brief Represent a SAC
 */
class Sac: public DvbFrameTpl<T_DVB_SAC>
{
public:
	/**
	 * @brief SAC constructor for agent
	 *
	 * @param tal_id   The terminal ID
	 * @param group_id The group ID
	 */
	Sac(tal_id_t tal_id, group_id_t group_id = 0);


	~Sac();

	/**
	 * @brief Set the requests
	 *
	 * @param prio  The request priority
	 * @param type  The request type
	 * @param value The request value
	 *
	 * @return true on success, false otherwise
	 */
	bool addRequest(uint8_t prio, ReturnAccessType type, uint32_t value);

	/**
	 * @brief Set the ACM parameters
	 *
	 * @param cni  The CNI parameter value
	 */
	void setAcm(double cni);

	/**
	 * @brief   Get the terminal Id.
	 *
	 * @return  terminal Id.
	 */
	tal_id_t getTerminalId(void) const;

	/**
	 * @brief  Get the group Id
	 *
	 * @return the group ID
	 */
	group_id_t getGroupId(void) const;

	/**
	 * @brief  Get the requests
	 *
	 * @return  the requets
	 */
	std::vector<cr_info_t> getRequests(void) const;

	/**
	 * @brief Get the C/N0 ratio
	 *
	 * @return the CNI value
	 */
	//TODO add a marge which must be configurable from manager
	double getCni() const;

	/// The log for sac
	static std::shared_ptr<OutputLog> sac_log;

private:
	/// the number of requests
	uint8_t request_nbr;
};


#endif
