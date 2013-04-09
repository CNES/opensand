/*
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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */
/* $Id: Types.h,v 1.1.1.1 2013/03/28 14:47:03 cgaillardet Exp $ */


#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>

typedef enum
{
	Created,
	Inited,
	Stopped,
	Running,
	Paused,
	Terminating
} DirectionThreadState;

typedef enum
{
	Fd,
	Timer,
	Message,
	Signal
} EventType;


#endif
