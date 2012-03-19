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
 * @file ErrorAgent.c
 * @author TAS
 * @brief The ErrorAgent class implements the error agent services
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "GenericPacket_e.h"
#include "ProtoConstants_e.h"
#include "GenericPort_e.h"
#include "IPAddr_e.h"
#include "ErrorAgent_e.h"
#include "EnvironmentAgent_e.h"


/*  @ROLE    : This function initialises the error agent
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_Init(
									/* INOUT */ T_ERROR_AGENT * ptr_this,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_INT32 ComponentId,
									/* IN    */ T_INT32 InstanceId,
									/* IN    */ T_UINT32 * FRSRef,
									/* IN    */ T_UINT8 * FSMRef)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_ERROR_AGENT));

	/* Allocate the generic packet */
  /*-----------------------------*/
	JUMP_ERROR(FIN, rid,
				  GENERIC_PACKET_Create(&(ptr_this->_ptr_genPacket),
												C_MAX_ERROR_PKT_ELT_NB));

	/* Init generic packet header */
  /*----------------------------*/
	ptr_this->_ptr_genPacket->_componentId =
		MAKE_COMPONENT_ID(ComponentId, InstanceId);

	/* Init Generic Packet Socket */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid,
				  GENERIC_PORT_InitSender(&(ptr_this->_genericPort), ptr_ipAddr,
												  C_MAX_ERROR_PKT_ELT_NB));


	/* Init reference to FRSFrame number and FSM number */
  /*--------------------------------------------------*/
	ptr_this->_FRSFramecount = FRSRef;
	ptr_this->_FSMNumber = FSMRef;
	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() successful for componentName=%s instanceId=%u",
				  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
														(((ptr_this->_ptr_genPacket->
															_componentId >> 4) & 0xF))),
				  (ptr_this->_ptr_genPacket->_componentId) & 0xF));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_INIT_REF=%u C_ERROR_INIT_PID=%u C_ERROR_INIT_COMPO=%u C_ERROR_END_SIMU=%u C_ERROR_ALLOC=%u C_ERROR_FILE_OPEN=%u C_ERROR_FILE_READ=%u C_ERROR_FILE_WRITE=%u",
				  C_ERROR_INIT_REF,
				  C_ERROR_INIT_PID,
				  C_ERROR_INIT_COMPO,
				  C_ERROR_END_SIMU,
				  C_ERROR_ALLOC,
				  C_ERROR_FILE_OPEN, C_ERROR_FILE_READ, C_ERROR_FILE_WRITE));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_FRS_SYNC=%u C_ERROR_SOCK_OPEN=%u C_ERROR_SOCK_READ=%u C_ERROR_SOCK_WRITE=%u C_ERROR_SMEM_OPEN=%u C_ERROR_SMEM_READ=%u C_ERROR_SMEM_WRITE=%u",
				  C_ERROR_FRS_SYNC,
				  C_ERROR_SOCK_OPEN,
				  C_ERROR_SOCK_READ,
				  C_ERROR_SOCK_WRITE,
				  C_ERROR_SMEM_OPEN, C_ERROR_SMEM_READ, C_ERROR_SMEM_WRITE));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_CONF_INVAL=%u C_ERROR_BUF_OVERFLOW=%u C_ERROR_BUF_UNDERFLOW=%u C_ERROR_BUF_EMPTY=%u C_ERROR_BAD_PARAM=%u C_ERROR_THREAD_CREATE=%u",
				  C_ERROR_CONF_INVAL,
				  C_ERROR_BUF_OVERFLOW,
				  C_ERROR_BUF_UNDERFLOW,
				  C_ERROR_BUF_EMPTY, C_ERROR_BAD_PARAM, C_ERROR_THREAD_CREATE));

	/* SYSTEM ERROR */
  /*---------------*/

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_NCC_MAPPING=%u C_ERROR_NCC_REQUEST=%u C_ERROR_NCC_CHANNEL_NOT_CREATED=%u C_ERROR_NCC_CC_NO_MORE_VCI=%u",
				  C_ERROR_NCC_MAPPING,
				  C_ERROR_NCC_REQUEST,
				  C_ERROR_NCC_CHANNEL_NOT_CREATED, C_ERROR_NCC_CC_NO_MORE_VCI));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_NCC_C2P_TIMEOUT=%u C_ERROR_NCC_SP_CONN_NOT_ESTABL_CAC=%u C_ERROR_NCC_SP_CONN_NOT_ESTABL_DAMA=%u C_ERROR_NCC_SP_UNKNOWN_STATE=%u",
				  C_ERROR_NCC_C2P_TIMEOUT,
				  C_ERROR_NCC_SP_CONN_NOT_ESTABL_CAC,
				  C_ERROR_NCC_SP_CONN_NOT_ESTABL_DAMA,
				  C_ERROR_NCC_SP_UNKNOWN_STATE));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_NCC_OD_CONN_EST_CAC=%u C_ERROR_NCC_OD_CONN_EST_DAMA=%u C_ERROR_NCC_OD_CONN_EST_ST_DEST=%u C_ERROR_NCC_OD_CONN_EST_UNVALID_IP=%u C_ERROR_NCC_OD_CONN_MOD_CAC=%u",
				  C_ERROR_NCC_OD_CONN_EST_CAC,
				  C_ERROR_NCC_OD_CONN_EST_DAMA,
				  C_ERROR_NCC_OD_CONN_EST_ST_DEST,
				  C_ERROR_NCC_OD_CONN_EST_UNVALID_IP, C_ERROR_NCC_OD_CONN_MOD_CAC));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Init() read error def: C_ERROR_NCC_OD_CONN_MOD_DAMA=%u C_ERROR_NCC_OD_CONN_MOD_ST_DEST=%u C_ERROR_NCC_OD_CONN_MOD_REL_UNKNOWN_CONN=%u C_ERROR_NCC_OD_CONN_MOD_UNKNOWN_CONN=%u C_ERROR_NCC_OD_PENDING_RELEASE =%u",
				  C_ERROR_NCC_OD_CONN_MOD_DAMA,
				  C_ERROR_NCC_OD_CONN_MOD_ST_DEST,
				  C_ERROR_NCC_OD_CONN_MOD_UNVALID_IP,
				  C_ERROR_NCC_OD_CONN_MOD_REL_UNKNOWN_CONN,
				  C_ERROR_NCC_OD_PENDING_RELEASE));

 FIN:
	return rid;
}


/*  @ROLE    : This function terminates the error agent
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_Terminate(
										  /* INOUT */ T_ERROR_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
#ifdef _ASP_TRACE
	T_UINT8 ComponentId = ptr_this->_ptr_genPacket->_componentId;
#endif

	JUMP_ERROR(FIN, rid, GENERIC_PACKET_Delete(&(ptr_this->_ptr_genPacket)));

	JUMP_ERROR(FIN, rid, GENERIC_PORT_Terminate(&(ptr_this->_genericPort)));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_Terminate() successful for componentName=%s instanceId=%d",
				  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
														(((ComponentId >> 4) & 0xF))),
				  (ComponentId) & 0xF));

 FIN:
	return rid;
}


/*  @ROLE    : This function set the error category/index, the error value is set to errno
    @RETURN  : None */
T_ERROR ERROR_AGENT_SetLastErrorErrno(
													 /* INOUT */ T_ERROR_AGENT * ptr_this,
													 /* IN    */ T_ERROR_CATEGORY cat,
													 /* IN    */ T_ERROR_INDEX index,
													 /* IN    */ T_ERROR error)
{
	T_ERROR rid = C_ERROR_OK;
	extern int errno;

	ptr_this->_LastErrorCat = cat;
	ptr_this->_LastErrorIndex = index;
	ptr_this->_rid = error - 1;
	ptr_this->_LastErrorValue = errno;	/* the errno value */

	ERROR_AGENT_SendError(ptr_this);

	return rid;

}


/*  @ROLE    : This function set the error category/index/value
    @RETURN  : None */
T_ERROR ERROR_AGENT_SetLastError(
											  /* INOUT */ T_ERROR_AGENT * ptr_this,
											  /* IN    */ T_ERROR_CATEGORY cat,
											  /* IN    */ T_ERROR_INDEX index,
											  /* IN    */ T_ERROR_VALUE value,
											  /* IN    */ T_ERROR error)
{
	T_ERROR rid = C_ERROR_OK;

	ptr_this->_LastErrorCat = cat;
	ptr_this->_LastErrorIndex = index;
	ptr_this->_LastErrorValue = value;
	ptr_this->_rid = error - 1;

	ERROR_AGENT_SendError(ptr_this);

	return rid;
}



/*  @ROLE    : This function sends Error to Error Controller
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_SendError(
										  /* INOUT */ T_ERROR_AGENT * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_ELT_GEN_PKT * eltGenPkt;


	JUMP_ERROR(FIN, rid,
				  GENERIC_PACKET_GetEltPkt(ptr_this->_ptr_genPacket, 0,
													&eltGenPkt));

	/* init the generic packet fields */
  /*--------------------------------*/
	ptr_this->_ptr_genPacket->_elementNumber = 1;
	if(ptr_this->_FRSFramecount != NULL)
		ptr_this->_ptr_genPacket->_frameNumber = *(ptr_this->_FRSFramecount);
	else
		ptr_this->_ptr_genPacket->_frameNumber = 0;

	/* T_ERROR starts at 1 instead of 0 because of C_ERROR_OK=0 */
	eltGenPkt->_id = ptr_this->_rid;
	eltGenPkt->_categoryId = ptr_this->_LastErrorCat;

	eltGenPkt->_index = ptr_this->_LastErrorIndex;
	if(ptr_this->_FSMNumber != NULL)
		ptr_this->_ptr_genPacket->_FSMNumber = *(ptr_this->_FSMNumber);
	else
		ptr_this->_ptr_genPacket->_FSMNumber = 0;
	eltGenPkt->_value = ptr_this->_LastErrorValue;

	/* Trace generic packet */
	TRACE_LOG_GENERIC_PACKET((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR,
									  C_TRACE_VALID, stdout, ptr_this->_ptr_genPacket,
									  "GENERIC PACKET before sending to Error controller"));

	JUMP_ERROR_TRACE(FIN, rid,
						  GENERIC_PORT_SendGenPacket(&(ptr_this->_genericPort),
															  ptr_this->_ptr_genPacket),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_ERROR,
							"ERROR_AGENT_SendError() cannot send packet for componentName=%s instanceId=%d",
							ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
																 (((ptr_this->_ptr_genPacket->
																	 _componentId >> 4) & 0xF))),
							(ptr_this->_ptr_genPacket->_componentId) & 0xF));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
				  "ERROR_AGENT_SendError() cmpt(%#x) id (%d) cat(%d) "
				  "index(%d) value(%d) frame(%d) FSM(%d) for componentName=%s instanceId=%d",
				  ptr_this->_ptr_genPacket->_componentId, eltGenPkt->_id,
				  eltGenPkt->_categoryId, eltGenPkt->_index, eltGenPkt->_value,
				  ptr_this->_ptr_genPacket->_frameNumber,
				  ptr_this->_ptr_genPacket->_FSMNumber,
				  ENV_AGENT_FindComponentName((T_COMPONENT_TYPE)
														(((ptr_this->_ptr_genPacket->
															_componentId >> 4) & 0xF))),
				  (ptr_this->_ptr_genPacket->_componentId) & 0xF));


 FIN:
	return rid;
}
