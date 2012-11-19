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
 * @file EnumCouple_e.h
 * @author TAS
 * @brief this class implements a (String, Int32) couple value
 *        corresponding to enum (string + associated INT32 value)
 */

#ifndef EnumCouple_e
#   define EnumCouple_e

#   include "Types_e.h"

/* When defining an array of EnumCouple, an additional Couple shall
   be inserted as the last array parameter, with String value set to NULL.
   For example : T_ENUM_COUPLE toto[] = { "Value1", C_VALUE_1,
                                          "Value2", C_VALUE_2,
                                          NULL};                     */

#   define C_ENUM_COUPLE_MAX_STRING_LEN 32
#   define C_ENUM_COUPLE_NULL {"\0", 0}

typedef struct
{

	T_CHAR _StrValue[C_ENUM_COUPLE_MAX_STRING_LEN];
	T_INT32 _IntValue;

} T_ENUM_COUPLE;

typedef struct
{

	T_CHAR _StrValue[C_ENUM_COUPLE_MAX_STRING_LEN];
	T_INT64 _IntValue;

} T_ENUM_LONGCOUPLE;


#endif
