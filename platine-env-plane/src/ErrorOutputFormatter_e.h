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
 * @file ErrorOutputFormatter_e.h
 * @author TAS
 * @brief The OutputFormatter class implements the error message fields
 */

#ifndef ErrorOutputFormatter_e
#   define ErrorOutputFormatter_e

#   include "Types_e.h"
#   include "Error_e.h"
#   include "GenericPacket_e.h"
#   include "ErrorDef_e.h"


#   define C_MAX_CAR_ERR_TRACE_FIELD   32
													/* Maximum number of characters in one output message field */


typedef struct
{
	T_UINT32 _frame_number;
	T_UINT8 _FSM_number;
} T_OF_ERR_DATE;

typedef struct
{
	T_CHAR _componentType[C_MAX_CAR_ERR_TRACE_FIELD];
	T_UINT8 _InstanceId;
} T_OF_ERR_ORIGIN;


typedef struct
{
	T_OF_ERR_DATE _error_date;
	T_OF_ERR_ORIGIN _error_origin;
	T_CHAR _error_name[C_ERR_DEF_MAX_CAR_NAME];
	T_UINT32 _error_index;
	T_CHAR _category[C_MAX_CAR_ERR_TRACE_FIELD];
	T_CHAR _index_signification[C_ERR_DEF_MAX_CAR_IDX_SIGN];
	T_INDEX_VALUE _index_value;
	T_UINT32 _index;
	T_CHAR _value_signification[C_ERR_DEF_MAX_CAR_VAL_SIGN];
	T_UINT32 _value;
	T_CHAR _unit[C_ERR_DEF_MAX_CAR_UNIT];
} T_ERROR_OUTPUT_FORMATTER;


/*  @ROLE    : Initialise output formatter class  
               configuration file
    @RETURN  : Error code */
T_ERROR T_ERROR_OUTPUT_FORMATTER_Init(
/* INOUT */ T_ERROR_OUTPUT_FORMATTER *
													 ptr_this);


/*  @ROLE    : create error message corresponding to a given element of 
               error generic packet
    @RETURN  : Error code */
T_ERROR T_ERROR_OUTPUT_FORMATTER_Formatter(
/* INOUT */
															T_ERROR_OUTPUT_FORMATTER
															* ptr_this,
/* IN    */
															T_ERRORS_DEF * ptr_errorsDef,
/* IN    */
															T_GENERIC_PKT * ptr_gen_pkt,
/* IN    */
															T_ELT_GEN_PKT * ptr_elt_pkt);


#endif
