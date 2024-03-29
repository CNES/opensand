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
 * @file    Logoff.h
 * @brief   Represent a Logoff
 * @author  Julien Bernard / Viveris Technologies
 */

#ifndef _LOGOFF_H_
#define _LOGOFF_H_


#include "DvbFrame.h"


/**
 * @class Logoff
 * @brief Represent a Logoff request
 */
class Logoff: public DvbFrameTpl<T_DVB_LOGOFF>
{
public:
	/**
	 * @brief Logoff request constructor for terminal (sender)
	 *
	 * @param mac           The terminal MAC id
	 */
	Logoff(tal_id_t  mac);

	~Logoff();

	/**
	 * @brief Get the mac field
	 *
	 * @return the mac field
	 */
	tal_id_t getMac(void) const;

};



#endif

