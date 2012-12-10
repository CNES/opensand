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
 * @file ErrorController_e.h
 * @author TAS
 * @brief Error Controller includes
 */

#ifndef _ERROR_CONTROLLER_H
#   define _ERROR_CONTROLLER_H

#   include "Types_e.h"

#   define ERROR_BUFFER_SIZE       1023
#   define ERROR_BUFFER_NB_FIELDS  8

typedef struct
{
	T_UINT8 telemetry_error_category;
	T_UINT32 telemetry_error_date;
	T_CHAR telemetry_error_name[SPRINT_MAX_LEN];
	T_CHAR telemetry_error_index_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_error_index_value;
	T_CHAR telemetry_error_value_sign[SPRINT_MAX_LEN];
	T_UINT32 telemetry_error_value;
	T_CHAR telemetry_error_unit[SPRINT_MAX_LEN];
} T_ERROR_BUFFER_ELEMENT;


typedef T_ERROR_BUFFER_ELEMENT T_ERROR_BUFFER[ERROR_BUFFER_SIZE];

T_UINT16 get_error_counter(void);

T_UINT8 get_error_category(void);

T_UINT32 get_error_date(void);

T_STRING get_error_name(void);

T_STRING get_error_index_sign(void);

T_UINT32 get_error_index_value(void);

T_STRING get_error_value_sign(void);

T_UINT32 get_error_value(void);

T_STRING get_error_unit(void);


#endif /* _ERROR_CONTROLLER_H */
