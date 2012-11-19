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
 * @file EventsDef_e.h
 * @author TAS
 * @brief The EventsDefinition class implements the reading of
 *        events definition configuration file
 */

#ifndef EventsDef_e
#   define EventsDef_e

#   include "Error_e.h"

/* All these limits shall be reconsidered at integration-time  */
#   define C_EVT_DEF_MAX_CAR_NAME     32
#   define C_EVT_DEF_MAX_CAR_IDX_SIGN 32
#   define C_EVT_DEF_MAX_CAR_VAL_SIGN 32
#   define C_EVT_DEF_MAX_CAR_UNIT     32
#   define C_EVT_DEF_MAX_EVENTS       50


typedef struct
{										  /* LEVEL 1 */
	T_INT32 _EventId;
	T_CHAR _Name[C_EVT_DEF_MAX_CAR_NAME];
	T_INT32 _Category;
	T_CHAR _IndexSignification[C_EVT_DEF_MAX_CAR_IDX_SIGN];
	T_CHAR _ValueSignification[C_EVT_DEF_MAX_CAR_VAL_SIGN];
	T_CHAR _Unit[C_EVT_DEF_MAX_CAR_UNIT];
} T_EVENT_DEF;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _nbEvent;
	T_EVENT_DEF _Event[C_EVT_DEF_MAX_EVENTS];
} T_EVENTS_DEF;


T_ERROR EVENTS_DEF_ReadConfigFile(
												/* INOUT */ T_EVENTS_DEF * ptr_this);


#endif /* EventsDef_e */
