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
 * @file EventController_e.h
 * @author TAS
 * @brief The event controller includes
 */

#ifndef _EVENT_CONTROLLER_H
#   define _EVENT_CONTROLLER_H

#   include "Types_e.h"
#   define EVENT_BUFFER_SIZE       1023
#   define EVENT_BUFFER_NB_FIELDS  8

typedef struct
{
	T_UINT8 telemetry_event_category;
	T_UINT32 telemetry_event_date;
	T_CHAR telemetry_event_name[SPRINT_MAX_LEN];
	T_CHAR telemetry_event_index_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_event_index_value;
	T_CHAR telemetry_event_value_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_event_value;
	T_CHAR telemetry_event_unit[SPRINT_MAX_LEN];
} T_EVENT_BUFFER_ELEMENT;


typedef T_EVENT_BUFFER_ELEMENT T_EVENT_BUFFER[EVENT_BUFFER_SIZE];


T_UINT16 get_event_counter(void);

T_UINT8 get_event_category(void);

T_UINT32 get_event_date(void);

T_STRING get_event_name(void);

T_STRING get_event_index_sign(void);

T_UINT32 get_event_index_value(void);

T_STRING get_event_value_sign(void);

T_UINT32 get_event_value(void);

T_STRING get_event_unit(void);


#endif /* _EVENT_CONTROLLER_H */
