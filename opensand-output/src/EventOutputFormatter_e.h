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
 * @file EventOutputFormatter_e.h
 * @author TAS
 * @brief The OutputFormatter class implements the event message fields
 */

#ifndef EventOutputFormatter_e
#   define EventOutputFormatter_e

#   include "Types_e.h"
#   include "Error_e.h"
#   include "GenericPacket_e.h"
#   include "EventsDef_e.h"


#   define C_MAX_CAR_EVT_TRACE_FIELD   32
													/* Maximum number of characters in one output message field */


typedef struct
{
	T_UINT32 _frame_number;
	T_UINT8 _FSM_number;
} T_OF_EVENT_DATE;

typedef struct
{
	T_CHAR _componentType[C_MAX_CAR_EVT_TRACE_FIELD];
	T_UINT8 _InstanceId;
} T_OF_EVENT_ORIGIN;


typedef struct
{
	T_OF_EVENT_DATE _event_date;
	T_OF_EVENT_ORIGIN _event_origin;
	T_CHAR _event_name[C_EVT_DEF_MAX_CAR_NAME];
	T_CHAR _category[C_MAX_CAR_EVT_TRACE_FIELD];
	T_CHAR _index_signification[C_EVT_DEF_MAX_CAR_IDX_SIGN];
	T_UINT32 _index_value;
	T_CHAR _value_signification[C_EVT_DEF_MAX_CAR_VAL_SIGN];
	T_UINT32 _value;
	T_CHAR _unit[C_EVT_DEF_MAX_CAR_UNIT];
} T_EVENT_OUTPUT_FORMATTER;


/*  @ROLE    : Initialise output formatter class  
               configuration file
    @RETURN  : Error code */
T_ERROR T_EVENT_OUTPUT_FORMATTER_Init(
/* INOUT */ T_EVENT_OUTPUT_FORMATTER *
													 ptr_this);


/*  @ROLE    : create event message corresponding to a given element of 
               event generic packet
    @RETURN  : Error code */
T_ERROR T_EVENT_OUTPUT_FORMATTER_Formatter(
/* INOUT */
															T_EVENT_OUTPUT_FORMATTER
															* ptr_this,
/* IN    */
															T_EVENTS_DEF * ptr_eventsDef,
/* IN    */
															T_GENERIC_PKT * ptr_gen_pkt,
/* IN    */
															T_ELT_GEN_PKT * ptr_elt_pkt);


#endif
