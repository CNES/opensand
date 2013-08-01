/*
 *
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
 * @file PepRequest.h
 * @brief Allocation or release request from a PEP component
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef PEP_REQUEST_H
#define PEP_REQUEST_H

#include "OpenSandCore.h"

/**
 * @brief The different type of requests the PEP may send
 */
typedef enum
{
	PEP_REQUEST_RELEASE = 0,     /**< a request for resources release */
	PEP_REQUEST_ALLOCATION = 1,  /**< a request for resources allocation */
	PEP_REQUEST_UNKNOWN = 2,     /**< for error handling */
} pep_request_type_t;



/**
 * @class PepRequest
 * @brief Allocation or release request from a PEP component
 */
class PepRequest
{

 private:

	/** The type of PEP request */
	pep_request_type_t type;
	/** The ST the PEP request is for */
	tal_id_t st_id;
	/** The CRA of the PEP request */
	rate_kbps_t cra_kbps;
	/** The RBDC of the PEP request */
	rate_kbps_t rbdc_kbps;
	/** The RBDCmax of the PEP request */
	rate_kbps_t rbdc_max_kbps;


 public:

	PepRequest(pep_request_type_t type, tal_id_t st_id,
	           rate_kbps_t cra_kbps, rate_kbps_t rbdc_kbps,
	           rate_kbps_t rbdc_max_kbps);

	~PepRequest();

	pep_request_type_t getType() const;
	tal_id_t getStId() const;
	rate_kbps_t getCra() const;
	rate_kbps_t getRbdc() const;
	rate_kbps_t getRbdcMax() const;
};

#endif
