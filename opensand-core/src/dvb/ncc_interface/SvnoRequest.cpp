/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 CNES
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
 * @file SvnoRequest.cpp
 * @brief Allocation or release request from a SVNO component
 * @author Adrien THIBAUD <athibaud@toulouse.viveris.com>
 */

#include "SvnoRequest.h"


/**
 * @brief Build a new release request from SVNO
 */
SvnoRequest::SvnoRequest(spot_id_t spot_id,
                         svno_request_type_t type,
                         band_t band,
                         std::string label,
                         rate_kbps_t new_rate_kbps):
	spot_id(spot_id),
	type(type),
	band(band),
	label(label),
	new_rate_kbps(new_rate_kbps)
{
}

/**
 * @brief Destroy the interface between NCC and SVNO components
 */
SvnoRequest::~SvnoRequest()
{
	// nothing to do
}

spot_id_t SvnoRequest::getSpotId() const
{
	return this->spot_id;
}

svno_request_type_t SvnoRequest::getType() const
{
	return this->type;
}

band_t SvnoRequest::getBand() const
{
	return this->band;
}

std::string SvnoRequest::getLabel() const
{
	return this->label;
}

rate_kbps_t SvnoRequest::getNewRate() const
{
	return this->new_rate_kbps;
}
