/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The GenericPacket class implements the generic packet 
               mechanism
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "GenericPacket_e.h"
#include "unused.h"


/*  @ROLE    : This function creates a generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_Create(
										  /* INOUT */ T_GENERIC_PKT ** ptr_this,
										  /* IN    */ T_UINT16 nb_elt_pkt)
{
	T_ERROR rid = C_ERROR_OK;
	T_INT32 sizeGenericPacket;

	/* Size of all generic elements */
  /*------------------------------*/
	sizeGenericPacket = HD_GEN_PKT_SIZE + (nb_elt_pkt * ELT_GEN_PKT_SIZE);

	/* create generic packet with nb_elt_pkt elements */
  /*------------------------------------------------*/
	*ptr_this = (T_GENERIC_PKT *) malloc(sizeGenericPacket);
	if(*ptr_this == NULL)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_ALLOC,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROTOCOL, C_TRACE_ERROR,
						"GENERIC_PACKET_Create() malloc failed"));
	}

	/* Initialise generic packets with O */
  /*-----------------------------------*/
	memset((*ptr_this), 0, sizeGenericPacket);

	/* number of elements in packet updated in Header */
  /*------------------------------------------------*/
	(*ptr_this)->_elementNumber = nb_elt_pkt;

 FIN:
	return rid;
}


/*  @ROLE    : This function deletes a generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_Delete(
										  /* INOUT */ T_GENERIC_PKT ** ptr_this)
{
	free(*ptr_this);
	*ptr_this = NULL;

	return C_ERROR_OK;
}


/*  @ROLE    : This function creates a generic packet for the init command
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_MakeInit(
											 /* INOUT */ T_GENERIC_PKT ** ptr_this,
											 /* IN    */ T_UINT32 simRef,
											 /* IN    */ T_UINT8 componentType)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT16 nbElem = 1;
	T_UINT8 componentIndex = 0;
	T_ELT_GEN_PKT *eltGenPkt;

	/* Create Packet */
  /*---------------*/
	JUMP_ERROR(FIN, rid, GENERIC_PACKET_Create(ptr_this, nbElem));

	/* Fill in Init packet header fields */
  /*-----------------------------------*/
	(*ptr_this)->_componentId = (componentType << 4) + (componentIndex);
	(*ptr_this)->_frameNumber = 0;
	(*ptr_this)->_FSMNumber = 0;

	/* Fill in Init first packet element fields */
  /*------------------------------------------*/
	JUMP_ERROR(FIN, rid, GENERIC_PACKET_GetEltPkt(*ptr_this, 0, &eltGenPkt));
	eltGenPkt->_id = 0;
	eltGenPkt->_categoryId = C_CAT_INIT;
	eltGenPkt->_index = 0;
	eltGenPkt->_value = simRef;


 FIN:
	return rid;
}


/*  @ROLE    : This function creates a generic packet for the end of simulation 
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_MakeEnd(
											/* INOUT */ T_GENERIC_PKT ** ptr_this,
											/* IN    */ T_UINT32 frameNumber,
											/* IN    */ T_UINT8 UNUSED(fsmNumber),
											/* IN    */ T_CONTROLLER_TYPE controllerType)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT16 nbElem = 1;
	T_UINT8 componentType = C_COMP_EVENT_CTRL;	/* !CB Init packet is sent by undefined (before scheduling controller) */
	T_UINT8 componentIndex = 0;
	T_ELT_GEN_PKT *eltGenPkt;

	/* Create Packet */
  /*---------------*/
	JUMP_ERROR(FIN, rid, GENERIC_PACKET_Create(ptr_this, nbElem));

	/* Fill in End packet header fields */
  /*----------------------------------*/
	(*ptr_this)->_componentId = (componentType << 4) + (componentIndex);
	(*ptr_this)->_frameNumber = frameNumber;
	(*ptr_this)->_FSMNumber = 0;

	/* Fill in End packet element fields */
  /*-----------------------------------*/
	JUMP_ERROR(FIN, rid, GENERIC_PACKET_GetEltPkt(*ptr_this, 0, &eltGenPkt));
	if(controllerType == C_CONTROLLER_ERROR)
	{
		eltGenPkt->_id = 2;
	}
	else
	{
		eltGenPkt->_id = 1;
	}
	eltGenPkt->_categoryId = C_CAT_END;
	eltGenPkt->_index = 0;
	eltGenPkt->_value = 0;

	TRACE_LOG_GENERIC_PACKET((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROTOCOL,
									  C_TRACE_VALID, stdout, *ptr_this,
									  "GENERIC PACKET End "));


 FIN:
	return rid;
}


/*  @ROLE    : This function return the size (in byte) of the generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_SizeOf(
										  /* INOUT */ T_GENERIC_PKT * ptr_this,
										  /*   OUT */ T_UINT32 * ptr_size)
{
	*ptr_size = HD_GEN_PKT_SIZE + (ptr_this->_elementNumber * ELT_GEN_PKT_SIZE);

	return C_ERROR_OK;
}


/*  @ROLE    : return the element packet [0..(elementNumber-1)] of generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_GetEltPkt(
											  /* INOUT */ T_GENERIC_PKT * ptr_this,
											  /* IN    */ T_UINT16 eltPktIndex,
											  /*   OUT */ T_ELT_GEN_PKT ** eltGenPkt)
{
	T_ERROR rid = C_ERROR_OK;
	if(eltPktIndex >= ptr_this->_elementNumber)
	{
		JUMP_TRACE(FIN, rid, C_ERROR_BAD_PARAM,
					  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROTOCOL, C_TRACE_ERROR,
						"GENERIC_PACKET_GetEltPkt() index out of range"));
	}

	*eltGenPkt =
		(T_ELT_GEN_PKT *) ((T_UINT8 *) ptr_this->_genEltPkt +
								 (ELT_GEN_PKT_SIZE * eltPktIndex));

 FIN:
	return rid;
}


/*  @ROLE    : return the header packet of generic packet
    @RETURN  : Error code */
T_ERROR GENERIC_PACKET_GetHdPkt(
											 /* INOUT */ T_GENERIC_PKT * ptr_this,
											 /*   OUT */ T_HD_GEN_PKT ** hdGenPkt)
{
	*hdGenPkt = (T_HD_GEN_PKT *) ptr_this;

	return C_ERROR_OK;
}


/* ========================================================================== */
/*  @ROLE    : This function prints the GenericPacket data
    @RETURN  : None                                                           */
/* ========================================================================== */
void GENERIC_PrintPacket(
									/* IN    */ T_TRACE_THREAD_TYPE traceThread,
									/* IN    */ T_TRACE_COMPONENT_TYPE traceComponent,
									/* IN    */ T_TRACE_LEVEL traceLevel,
									/* IN    */ FILE * stream,
									/* IN    */ T_GENERIC_PKT * GenericPacket,
									/* IN    */ T_CHAR * format, ...)
{
#ifdef _ASP_TRACE
	T_ERROR rid = C_ERROR_OK;
	T_UINT16 iPkt;
	T_ELT_GEN_PKT *eltGenPkt;
	/* T_UINT32
	   Size; */
	T_CHAR fieldMsg[256];
	TRACE_LOG_PACKET_DECLARATION();

	if(GenericPacket != NULL)
	{
		/* check the activation flag */
		if(((traceThread != C_TRACE_THREAD_UNKNOWN)
			 && (_trace_activationFlag[traceThread] == TRUE))
			|| (_trace_activationFlag[C_TRACE_THREAD_MAX + traceComponent] == TRUE)
			|| (traceLevel == C_TRACE_ERROR))
		{
			/* check the level flag */
			if(((traceThread != C_TRACE_THREAD_UNKNOWN)
				 && (_trace_levelFlag[traceThread] & traceLevel))
				|| (_trace_levelFlag[C_TRACE_THREAD_MAX + traceComponent] &
					 traceLevel) || (traceLevel == C_TRACE_ERROR))
			{

				TRACE_LOG_PACKET_TITLE("GenericPacket", format, args);
				TRACE_LOG_PACKET_DATA("Header Data", GenericPacket,
											 HD_GEN_PKT_SIZE);
				TRACE_LOG_PACKET_BEGIN_FIELDS(fieldMsg, "Header Fields");
				TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "Number of elements",
													 GenericPacket->_elementNumber);
				TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "ComponentId",
													 GenericPacket->_componentId);
				TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "FSM number",
													 GenericPacket->_FSMNumber);
				TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "Frame number",
													 GenericPacket->_frameNumber);
				TRACE_LOG_PACKET_END_FIELDS(fieldMsg);

				/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
				/* !!!!!!!!!!!!!!!!!! MAXIMUM DE DEUX PACKET GENERIQUE !!!!!!!!!!!! */
				/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
				for(iPkt = 0; (iPkt < GenericPacket->_elementNumber) && (iPkt < 2);
					 iPkt++)
				{
					rid = GENERIC_PACKET_GetEltPkt(GenericPacket, iPkt, &eltGenPkt);
					if(rid == C_ERROR_OK)
					{
						TRACE_LOG_PACKET_DATA("Element Data", eltGenPkt,
													 ELT_GEN_PKT_SIZE);
						/* Display detail */
						TRACE_LOG_PACKET_BEGIN_FIELDS(fieldMsg, "Element Fields");
						TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "Id", eltGenPkt->_id);
						TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "CategoryId",
															 eltGenPkt->_categoryId);
						TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "Index",
															 eltGenPkt->_index);
						TRACE_LOG_PACKET_ADD_FIELDS(fieldMsg, "Value",
															 eltGenPkt->_value);
						TRACE_LOG_PACKET_END_FIELDS(fieldMsg);
					}
				}
				if(GenericPacket->_elementNumber > 2)
				{
					TRACE_LOG_PACKET_COMMENT
						("Trace only the first two element packet !!!");
				}

				TRACE_LOG_STREAM((traceThread, traceComponent, traceLevel, stream,
										"%s", globalMsg));

			}
		}
	}

#else	/*_ASP_TRACE  */
	fprintf(stderr,
			  "The _ASP_TRACE is not set:DO NOT USE GenericPacket_PrintPacket\n");
#endif /*_ASP_TRACE  */

}
