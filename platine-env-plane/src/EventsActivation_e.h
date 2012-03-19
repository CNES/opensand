#ifndef EventsActivation_e
#   define EventsActivation_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The EventsActivation class implements the reading of 
               events activation configuration file 
    @HISTORY :
    03-02-26 : Creation
    03-10-17 : Add XML data (GM)
*/
/*--------------------------------------------------------------------------*/

#   include "Error_e.h"

/* All these limits shall be reconsidered at integration-time  */

/********************/
/* MACRO DEFINITION */
/********************/
#   define C_EVT_CATEGORY_MAX_NB  7

/************************/
/* STRUCTURE DEFINITION */
/***********************/
typedef struct
{
	T_UINT32 _nbCategory;
	T_INT32 _EventCategory[C_EVT_CATEGORY_MAX_NB];
	T_UINT32 _DoNotUsed;
} T_EVENTS_ACTIVATION;


T_ERROR EVENTS_ACTIVATION_ReadConfigFile(
														 /* INOUT */ T_EVENTS_ACTIVATION *
														 ptr_this);

T_ERROR EVENTS_ACTIVATION_PrintConfigFile(
														  /* INOUT */ T_EVENTS_ACTIVATION *
														  ptr_this);


#endif /* EventsActivation_e */
