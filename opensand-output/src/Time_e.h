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
 * @file Time_e.h
 * @author TAS
 * @brief Time fonctions
 */

#ifndef Time_e
#   define Time_e

#   include <sys/types.h>
#   include <sys/times.h>
#   include <limits.h>

#   include "Types_e.h"
#   include "Error_e.h"

typedef T_DOUBLE T_TIME;

/* Init internal time reference */
T_ERROR TIME_Init(void);

/* Get current time in seconds */
T_TIME TIME_GetTime(void);

/* Get current time in tick (struct tms *ptr_buffer) */
#      define TIME_GetTimeTick(ptr_buffer) times(ptr_buffer)

/* Get a time difference in second (struct tms *ptr_bufferEnd,struct tms *ptr_bufferBegin) */
#      define TIME_GetTimeDiff(ptr_bufferEnd,ptr_bufferBegin) \
((T_TIME)((((T_DOUBLE)(ptr_bufferEnd)->tms_utime - (T_DOUBLE)(ptr_bufferBegin)->tms_utime) \
           + ((T_DOUBLE)(ptr_bufferEnd)->tms_stime - (T_DOUBLE)(ptr_bufferBegin)->tms_stime)) \
          / (T_DOUBLE)CLK_TCK))

#endif /* Time_e */
