/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file PepRequest.cpp
 * @brief Allocation or release request from a PEP component
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "PepRequest.h"


/**
 * @brief Build a new release request from PEP
 */
PepRequest::PepRequest(pep_request_type_t type, tal_id_t st_id,
                       rate_kbps_t cra_kbps, rate_kbps_t rbdc_kbps,
                       rate_kbps_t rbdc_max_kbps)
{
	this->type = type;
	this->st_id = st_id;
	this->cra_kbps = cra_kbps;
	this->rbdc_kbps = rbdc_kbps;
	this->rbdc_max_kbps = rbdc_max_kbps;
}

/**
 * @brief Destroy the interface between NCC and PEP components
 */
PepRequest::~PepRequest()
{
	// nothing to do
}


/**
 * @brief Get the type of PEP request
 *
 * @return  the type of PEP request
 */
pep_request_type_t PepRequest::getType() const
{
	return this->type;
}


/**
 * @brief Get the ST the PEP request is for
 *
 * @return  the ID of the ST the PEP request is for
 */
tal_id_t PepRequest::getStId() const
{
	return this->st_id;
}


/**
 * @brief Get the CRA of PEP request
 *
 * @return  the CRA of PEP request
 */
rate_kbps_t PepRequest::getCra() const
{
	return this->cra_kbps;
}


/**
 * @brief Get the RBDC of PEP request
 *
 * @return  the RBDC of PEP request
 */
rate_kbps_t PepRequest::getRbdc() const
{
	return this->rbdc_kbps;
}


/**
 * @brief Get the RBDCmax of PEP request
 *
 * @return  the RBDCmax of PEP request
 */
rate_kbps_t PepRequest::getRbdcMax() const
{
	return this->rbdc_max_kbps;
}
