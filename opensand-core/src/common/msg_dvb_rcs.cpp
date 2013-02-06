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
 * @file msg_dvb_rcs.cpp
 * @brief This files defines messages type numbers for DVB-S/RCS related packets
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "msg_dvb_rcs.h"

mgl_memory_pool g_memory_pool_dvb_rcs(MSG_BBFRAME_SIZE_MAX + MSG_PHYFRAME_SIZE_MAX,
                                      10000, (const char *)"gmp_dvb_rcs");
