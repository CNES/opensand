/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file SvnoRequest.h
 * @brief Allocation or release request from a SVNO component
 * @author Adrien THIBAUD / Viveris Technologies
 */

#ifndef SVNO_REQUEST_H
#define SVNO_REQUEST_H

#include "OpenSandCore.h"

/**
 * @brief The different type of requests the SVNO may send
 */
typedef enum
{
	SVNO_REQUEST_RELEASE = 0,     /**< a request for resources release */
	SVNO_REQUEST_ALLOCATION = 1,  /**< a request for resources allocation */
} svno_request_type_t;

/**
 * @brief The band
 */
typedef enum
{
	FORWARD = 0,
	RETURN = 1,
} band_t;

/**
 * @class SvnoRequest
 * @brief Allocation or release request from a SVNO component
 */
class SvnoRequest
{

 private:

	/** The spot concerned by the request */
	spot_id_t spot_id;
	/** The type of Svno request */
	svno_request_type_t type;
	/** Band **/
	band_t band;
	/** The label of the requester */
	std::string label;
	/** The new rate requested */
	rate_kbps_t new_rate_kbps;

 public:

	SvnoRequest(spot_id_t spot_id,
	            svno_request_type_t type,
	            band_t band,
	            std::string label,
	            rate_kbps_t new_rate_kbps);

	~SvnoRequest();

	/**
	 * @brief Get the spot concerned by the request
	 *
	 * @return  the type of SVNO request
	 */
	spot_id_t getSpotId() const;

	/**
	 * @brief Get the type of SVNO request
	 *
	 * @return  the type of SVNO request
	 */
	svno_request_type_t getType() const;

	/**
	 * @brief  Get the band
	 *
	 * @return the band
	 */
	band_t getBand() const;

	/**
	 * @brief   Get the label of the requester
	 *
	 * @return  the label of the requester
	 */
	std::string getLabel() const;

	/**
	 * @brief   Get the new rate
	 *
	 * @return  the new rate
	 */
	rate_kbps_t getNewRate() const;
};

#endif
