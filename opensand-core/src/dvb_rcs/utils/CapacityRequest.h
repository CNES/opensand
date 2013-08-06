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
 * @file    CapacityRequest.h
 * @brief   Represent a CR (Capacity Request)
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _CAPACITY_REQUEST_H_
#define _CAPACITY_REQUEST_H_


#include "OpenSandCore.h"

#include <vector>
#include <endian.h>

#define NBR_MAX_CR 2

using std::vector;

///> The type of capacity request associated to each FIFO among RBDC, VBDC or None
typedef enum
{
	cr_vbdc  = 0, /* Volume Based */
	cr_rbdc  = 1, /* Rate Based */
	cr_avbdc = 2, /* Absolute Volume Based */
	cr_none  = 3, /* No CR, only use Constant Allocation */
} cr_type_t;

/**
 * The Emulated Capacity Requests field
 */
typedef struct
{
#if __BYTE_ORDER == __BIG_ENDIAN
	uint8_t type:4;    ///< The CR type
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
	uint8_t value;     ///< The request value
	                   //   (the final requeted rate will be scale * value)
	uint8_t prio:2;    ///< The request priority
	uint8_t scale:2;   ///< The scale of the request according to the values
	                   //   below (should be as small as possible):
	                   //   for DVB-RCS: 00 => 1
	                   //   01 => 16
	uint8_t type:4;    ///< The CR type
	                   //   for DVB-RCS: 00 => VBDC
	                   //                01 => RBDC
	                   //                10 => AVBDC
#else
#error "Please fix <bits/endian.h>"
#endif
} __attribute__((packed)) emu_cr_t;

/**
 * The CR info for CR computation
 */
typedef struct
{
	uint8_t prio;    ///> Request priority
	uint8_t type;    ///> Request type
	uint32_t value;  ///> Request value
} cr_info_t;

/**
 * The Emulated SAC field
 */
typedef struct
{
	uint8_t tal_id; ///> The temirnal ID
	                //   size 5 for physical ST, 5->8 for simulated ST requests
	uint8_t cr_number;
	emu_cr_t cr[NBR_MAX_CR]; ///> The emulated CR field
	                         //   when in frame, the length should be correctly set
	                         //   in order to send only the CR which were filled
} __attribute__((packed)) emu_sac_t;



/**
 * @class CapacityRequest
 * @brief Represent a CR
 */
class CapacityRequest
{
 public:

	/**
	 * @brief Capacity Request constructor for agent
	 *
	 * @param tal_id   The terminal ID
	 */
	CapacityRequest(tal_id_t tal_id);

	/**
	 * @brief Capacity Request constructor for controller
	 */
	CapacityRequest()
	{};

	~CapacityRequest();

	/**
	 * @brief Set the requests
	 *
	 * @param prio  The request priority
	 * @param type  The request type
	 * @param value The request value
	 */
	void addRequest(uint8_t prio, uint8_t type, uint32_t value);

	/**
	 * @brief Parse CR data
	 *
	 * @param data   the RAW data containing the CR
	 * @param length the data length
	 * @return true on success, false otherwise
	 */
	bool parse(const unsigned char *data, size_t length);


	/**
	 * @brief   Get the terminal Id.
	 *
	 * @return  terminal Id.
	 */
	tal_id_t getTerminalId() const
	{
		return this->tal_id;
	};

	/**
	 * @brief  Get the requests
	 *
	 * @return  the requets
	 */
	vector<cr_info_t> getRequets() const
	{
		return this->requests;
	};

	/**
	 * @brief  Build a SAC field to be sent on the network
	 *
	 * @param frame  the frame containing the SAC field
	 * @param length the length of the frame
	 *
	 * @return true on success, false otherwise
	 */
	void build(unsigned char *sac, size_t &length);

 private:

	/// the terminal ID
	tal_id_t tal_id;
	/// the requets
	vector<cr_info_t> requests;
	/// the SAC avoid allocating a complete SAC
	emu_sac_t sac;
};

#endif

