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
 * @file    CapacityRequest.h
 * @brief   Represent a CR (Capacity Request)
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _CAPACITY_REQUEST_H_
#define _CAPACITY_REQUEST_H_


#include "OpenSandCore.h"

#include <vector>

#define NBR_MAX_CR 2

///> The type of capacity request associated to each FIFO among RBDC, VBDC or None
typedef enum
{
	cr_vbdc  = 0, /* Volume Based */
	cr_rbdc  = 1, /* Rate Based */
	cr_avbdc = 2, /* Absolute Volume Based */
	cr_fca   = 3, /* Free Capacity Assignment */
	cr_none  = 4, /* No CR, only use Constant Allocation */
} cr_type_t;


/**
 * The CR info for CR computation
 */
typedef struct
{
	uint8_t prio;
	uint8_t type;
	uint32_t value;
} cr_info_t;

/**
 * The Emulated Capacity Requests field
 */
typedef struct
{
	uint8_t type:4;  ///< The CR type
	                   //   for DVB-RCS: 00 => VBDC
	                   //                01 => RBDC
	                   //                10 => AVBDC
	uint8_t prio:2;    ///< The request priority
	uint8_t scale:2;   ///< The scale of the request according to the values
	                   //   below (should be as small as possible):
	                   //   for DVB-RCS: 00 => 1
	                   //                01 => 16
	uint8_t value;     ///< The request value
	                   //   (the final requeted rate will be scale * value)
} __attribute__((packed)) emu_cr_t;

/**
 * The Emulated SAC field
 */
typedef struct
{
	uint8_t tal_id; // size 5 for physical ST, 5->8 for simulated ST requests
	uint8_t cr_number;
	emu_cr_t cr[NBR_MAX_CR]; // when in frame, the length should be correctly set
	                         // in order to send only the CR which were filled
} __attribute__((packed)) emu_sac_t;


/**
 * @brief decode the capacity request in function of the
 *        encoded value  and scaling factor
 *
 * @param cr the emulated capacity request
 * @return the capacity request value
 */
uint16_t getDecodedCrValue(const emu_cr_t &cr);


/**
 * @class CapacityRequest
 * @brief Represent a CR
 */
class CapacityRequest
{
 public:

	CapacityRequest(tal_id_t tal_id,
	                std::vector<cr_info_t> requests):
		tal_id(tal_id),
		requests(requests)
	{};

	/**
	 * @brief   Get the terminal Id.
	 *
	 * @return  terminal Id.
	 */
	tal_id_t getTerminalId() const
	{
		return this->tal_id;
	}
	
	/**
	 * @brief  Build a SAC field
	 * 
	 * @param frame  the frame containing the SAC field
	 * @param length the length of the frame
	 * 
	 * @return true on success, false otherwise
	 */
	void build(unsigned char *sac, size_t &length);

 protected:

	tal_id_t tal_id;
	std::vector<cr_info_t> requests;
};

#endif

