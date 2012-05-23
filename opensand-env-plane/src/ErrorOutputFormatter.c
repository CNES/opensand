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
 * @file ErrorOutputFormatter.c
 * @author TAS
 * @brief The OutputFormatter class implements the error message fields
 */

/* PROJECT RESOURCES */
#include "ErrorOutputFormatter_e.h"
#include "ErrorDef_e.h"
#include "EnumParser_e.h"
#include "Controller_e.h"

/* SYSTEM RESOURCES */
#include <string.h>

/*  @ROLE    : Initialise output formatter class by reading error definition 
               configuration file
    @RETURN  : Error code */
T_ERROR T_ERROR_OUTPUT_FORMATTER_Init(
/* INOUT */ T_ERROR_OUTPUT_FORMATTER *
													 ptr_this)
{
	/* Initialise all fields to 0 */
  /*----------------------------*/
	memset(ptr_this, 0, sizeof(T_ERROR_OUTPUT_FORMATTER));

	return C_ERROR_OK;
}


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
															T_ELT_GEN_PKT * ptr_elt_pkt)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT8 componentTypeValue = 0;

	static T_ENUM_COUPLE C_CAT_ID_choices[5] = { {"INIT", C_CAT_INIT},
	{"END", C_CAT_END},
	{"CRITICAL", C_ERROR_CRITICAL},
	{"MINOR", C_ERROR_MINOR},
	{"", 0}
	};

	static T_ENUM_COUPLE nameComp[C_COMP_MAX + 1] = { {"GW", C_COMP_GW},
	{"SAT", C_COMP_SAT},
	{"ST", C_COMP_ST},
	{"AGGREGATE", C_COMP_ST_AGG},
	{"OBPC", C_COMP_OBPC},
	{"TRAFFIC", C_COMP_TG},
	{"PROBE_CONTROLLER", C_COMP_PROBE_CTRL},
	{"EVENT_CONTROLLER", C_COMP_EVENT_CTRL},
	{"ERROR_CONTROLLER", C_COMP_ERROR_CTRL},
	{"", 0}
	};


	/* Fill in Error Date fields */
  /*---------------------------*/
	ptr_this->_error_date._frame_number = ptr_gen_pkt->_frameNumber;
	ptr_this->_error_date._FSM_number = ptr_gen_pkt->_FSMNumber;

	/* Fill in Error Origin fields */
  /*-----------------------------*/
	componentTypeValue = (ptr_gen_pkt->_componentId & 0xF0) >> 4;


/*  JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseString((T_INT32)componentTypeValue, 
    OUTPUT_FORMATTER_ComponentChoices,
    ptr_this->_error_origin._componentType)); 
*/
	JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseString((T_INT32) componentTypeValue,
																nameComp,
																ptr_this->_error_origin.
																_componentType));


	ptr_this->_error_origin._InstanceId =
		(T_UINT8) ptr_gen_pkt->_componentId & 0x0F;

	/* Fill in Error Name field */
  /*--------------------------*/
	strcpy(ptr_this->_error_name, ptr_errorsDef->_Error[ptr_elt_pkt->_id]._Name);
	ptr_this->_error_index = ptr_elt_pkt->_id;

	/* Fill in Error Category Id field */
  /*---------------------------------*/
	JUMP_ERROR(FIN, rid,
				  ENUM_PARSER_ParseString((T_INT32) (ptr_elt_pkt->_categoryId),
												  C_CAT_ID_choices, ptr_this->_category));

	/* Fill in index signification field */
  /*-----------------------------------*/
	strcpy(ptr_this->_index_signification,
			 ptr_errorsDef->_Error[ptr_elt_pkt->_id]._IndexSignification);

	/* Fill in index value field */
  /*---------------------------*/
	strcpy(ptr_this->_index_value,
			 ptr_errorsDef->_Error[ptr_elt_pkt->_id]._IndexTab.
			 _IndexValues[ptr_elt_pkt->_index]);
	ptr_this->_index = ptr_elt_pkt->_index;

	/* Fill in value signification field */
  /*-----------------------------------*/
	strcpy(ptr_this->_value_signification,
			 ptr_errorsDef->_Error[ptr_elt_pkt->_id]._ValueSignification);


	/* Fill in value field */
  /*---------------------*/
	ptr_this->_value = ptr_elt_pkt->_value;

	/* Fill in unit field */
  /*--------------------*/
	strcpy(ptr_this->_unit, ptr_errorsDef->_Error[ptr_elt_pkt->_id]._Unit);

 FIN:
	return rid;
}
