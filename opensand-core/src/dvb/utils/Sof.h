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
 * @file    Sof.h
 * @brief   Represent a SOF
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _SOF_H_
#define _SOF_H_


#include "OpenSandCore.h"
#include "DvbFrame.h"


/**
 * @class Sof
 * @brief Represent a SOF
 */
class Sof: public DvbFrameTpl<T_DVB_SOF>
{
 public:

	/**
	 * @brief SOF constructor for NCC (sender)
	 *
	 * @param mac  The superframe number
	 */
	Sof(time_sf_t sf_nbr);

	~Sof();

	/**
	 * @brief Get the mac field
	 *
	 * @return the mac field
	 */
	time_sf_t getSuperFrameNumber(void) const;

};



#endif

