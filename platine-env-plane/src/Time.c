/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file Time.c
 * @author TAS
 * @brief Time functions
 */

/*  Cartouche AD  */

/* SYSTEM RESOURCES */
#include <time.h>
#include <sys/time.h>

/* PROJECT RESOURCES */
#include "Time_e.h"


/* Internal time reference (in seconds) */
static T_UINT32 _zero;

/* Init internal time reference */
T_ERROR TIME_Init()
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	_zero = ts.tv_sec;

	return C_ERROR_OK;
}


/* Get current time in seconds */
T_TIME TIME_GetTime()
{
	struct timeval ts;
	gettimeofday(&ts, NULL);
	return (double) (ts.tv_sec - _zero) + ((double) ts.tv_usec / 1000000.);

	/* !CB higher precision
	   struct timespec ts;

	   clock_gettime(CLOCK_REALTIME, &ts);
	   return (double)(ts.tv_sec - _zero) + ((double)ts.tv_nsec / 1000000000.);
	 */
}
