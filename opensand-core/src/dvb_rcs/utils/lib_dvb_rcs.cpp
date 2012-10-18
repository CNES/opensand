/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file lib_dvb_rcs.cpp
 * @brief This library defines DVB-RCS messages and provides helper functions.
 * @author Viveris Technologies
 */

#include <string.h>
#include <math.h>

#include "lib_dvb_rcs.h"
#define DBG_PACKAGE PKG_DVB_RCS
#include "opensand_conf/uti_debug.h"


/**
 * return the ith frame pointer associated with a buffer pointing to a
 * T_DVB_TBTP struct. Count is done from 0. There is no error check, the
 * structure must be correctly filled.
 * @param i the index
 * @param buff the pointer to the T_DVB_TBTP struct
 */
T_DVB_FRAME * ith_frame_ptr(int i, unsigned char *buff)
{
	int j;
	T_DVB_FRAME *ret;

	ret = first_frame_ptr(buff);
	for(j = 0; j < i; j++)
	{
		ret = next_frame_ptr(ret);
	};

	return ret;
}


