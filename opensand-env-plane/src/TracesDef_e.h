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
 * @file TracesDef_e.h
 * @author TAS
 * @brief The TraceDefinition class implements the reading of
 *        trace definition configuration file
 */

#ifndef TracesDef_e
#   define TracesDef_e

#   include "Types_e.h"
#   include "Error_e.h"
#   include "EnumCouple_e.h"

/* All these limits shall be reconsidered at integration-time  */
#   define C_TRACE_DEF_MAX_CAR_NAME     64
#   define C_TRACE_DEF_MAX_CAR_MODE     64

typedef struct
{
	T_INT64 _Name;
	T_INT64 _Mode;
} T_TRACE_DEF;

#   define C_TRACE_DEF_MAX_TRACES  500

typedef struct
{
	T_UINT32 _nbTrace;
	T_TRACE_DEF _Trace[C_TRACE_DEF_MAX_TRACES];

	T_ENUM_LONGCOUPLE C_TRACE_MODE_choices[C_TRACE_DEF_MAX_TRACES];
	T_ENUM_LONGCOUPLE C_TRACE_COMP_choices[C_TRACE_DEF_MAX_TRACES];

} T_TRACES_DEF;


T_ERROR TRACES_DEF_ReadConfigFile(
												/* INOUT */ T_TRACES_DEF * ptr_this,
												/* IN    */ T_UINT16 SimReference,
												/* IN    */ T_UINT16 SimRun);


#endif
