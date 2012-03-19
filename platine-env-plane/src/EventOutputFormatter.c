/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The OutputFormatter class implements the event message fields 
    @HISTORY :
    03-03-04 : Creation
*/
/*--------------------------------------------------------------------------*/

/* PROJECT RESOURCES */
#include "EventOutputFormatter_e.h"
#include "EnumParser_e.h"
#include "Event_e.h"
#include "Controller_e.h"

/* SYSTEM RESOURCES */
#include <string.h>

COMPONENT_CHOICES(EVENT_OUTPUT_FORMATTER_ComponentChoices)	/* ENUM_COUPLE array is defined in execcontext_e.h */
T_ENUM_COUPLE C_EVENT_CAT_ID_choices[C_EVENT_CAT_NB + 1]
		  = { {"INIT", C_CAT_INIT},
		  {"END", C_CAT_END},
		  {"SIMU", C_EVENT_SIMU},
		  {"TRAFFIC", C_EVENT_TRAFFIC},
#ifndef _ASP_PEA_CONF
		  {"MAC_Connection", C_EVENT_MAC_CONNECTION},

#else	/* _ASP_PEA_CONF */
		  {"MAC_Connection_Nom", C_EVENT_MAC_CONNECTION_NOM},
		  {"MAC_Connection_Deg", C_EVENT_MAC_CONNECTION_DEG},
		  {"SIG", C_EVENT_SIG},

#endif /* _ASP_PEA_CONF */
		  C_ENUM_COUPLE_NULL
	  };


/*  @ROLE    : Initialise output formatter class by reading error definition 
               configuration file
    @RETURN  : Error code */
T_ERROR T_EVENT_OUTPUT_FORMATTER_Init(
/* INOUT */ T_EVENT_OUTPUT_FORMATTER *
													 ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	/* Initialise all fields to 0 */
  /*----------------------------*/
	memset(ptr_this, 0, sizeof(T_EVENT_OUTPUT_FORMATTER));

	return rid;
}


/*  @ROLE    : create event message corresponding to a given element of 
               error generic packet
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
															T_ELT_GEN_PKT * ptr_elt_pkt)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT8 componentTypeValue = 0;

	/* Fill in Event Date fields */
  /*---------------------------*/
	ptr_this->_event_date._frame_number = ptr_gen_pkt->_frameNumber;
	ptr_this->_event_date._FSM_number = (T_UINT8) (ptr_elt_pkt->_value >> 24);

	/* Fill in Event Origin fields */
  /*-----------------------------*/
	componentTypeValue = (ptr_gen_pkt->_componentId & 0xF0) >> 4;
	JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseString((T_INT32) componentTypeValue,
																EVENT_OUTPUT_FORMATTER_ComponentChoices,
																ptr_this->_event_origin.
																_componentType));

	ptr_this->_event_origin._InstanceId = ptr_gen_pkt->_componentId & 0x0F;

	/* Fill in Event Name field */
  /*--------------------------*/
	strcpy(ptr_this->_event_name, ptr_eventsDef->_Event[ptr_elt_pkt->_id]._Name);

	/* Fill in Event Category Id field */
  /*---------------------------------*/
	JUMP_ERROR(FIN, rid,
				  ENUM_PARSER_ParseString((T_INT32) (ptr_elt_pkt->_categoryId),
												  C_EVENT_CAT_ID_choices,
												  ptr_this->_category));

	/* Fill in index signification field */
  /*-----------------------------------*/
	strcpy(ptr_this->_index_signification,
			 ptr_eventsDef->_Event[ptr_elt_pkt->_id]._IndexSignification);

	/* Fill in index value field */
  /*---------------------------*/
	ptr_this->_index_value = ptr_elt_pkt->_index;

	/* Fill in value signification field */
  /*-----------------------------------*/
	strcpy(ptr_this->_value_signification,
			 ptr_eventsDef->_Event[ptr_elt_pkt->_id]._ValueSignification);

	/* Fill in value field */
  /*---------------------*/
#ifdef _ASP_PEA_CONF
	ptr_this->_value = (ptr_elt_pkt->_value & 0x00FFFFFF);	/* only the last 24 bits */
#else
	/* !CB rajout du decalage */
	ptr_this->_value = (ptr_elt_pkt->_value & 0x00FFFFFF);
#endif /* _ASP_PEA_CONF */

	/* Fill in unit field */
  /*--------------------*/
	strcpy(ptr_this->_unit, ptr_eventsDef->_Event[ptr_elt_pkt->_id]._Unit);

 FIN:
	return rid;
}
