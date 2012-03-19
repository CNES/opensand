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
 * @file ErrorAgent_e.h
 * @author TAS
 * @brief The ErrorAgent class implements the error agent services
 */

#ifndef ErrorAgent_e
#   define ErrorAgent_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "DominoConstants_e.h"
#   include "IPAddr_e.h"
#   include "GenericPort_e.h"
#   include "GenericPacket_e.h"

#   define C_MAX_ERROR_PKT_ELT_NB 1
											/* Maximum Number of Elements in 1 Error Packet */

typedef struct
{

	T_GENERIC_PORT _genericPort;
	T_GENERIC_PKT *_ptr_genPacket;
	T_ERROR_CATEGORY _LastErrorCat;
	T_ERROR_INDEX _LastErrorIndex;
	T_ERROR_VALUE _LastErrorValue;
	T_ERROR _rid;

	T_UINT32 *_FRSFramecount;	  /* Used to retrieve FRSFrame from execution context */
	T_UINT8 *_FSMNumber;			  /* Used to retrieve FSM number from execution context */

} T_ERROR_AGENT;


/*  @ROLE    : This function initialises the error agent
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_Init(
									/* INOUT */ T_ERROR_AGENT * ptr_this,
									/* IN    */ T_IP_ADDR * ptr_ipAddr,
									/* IN    */ T_INT32 ComponentId,
									/* IN    */ T_INT32 InstanceId,
									/* IN    */ T_UINT32 * FRSRef,
									/* IN    */ T_UINT8 * FSMRef);


/*  @ROLE    : This function terminates the error agent
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_Terminate(
										  /* INOUT */ T_ERROR_AGENT * ptr_this);


/*  @ROLE    : This function set the error category/index, the error value is set to errno
    @RETURN  : None */
T_ERROR ERROR_AGENT_SetLastErrorErrno(
													 /* INOUT */ T_ERROR_AGENT * ptr_this,
													 /* IN    */ T_ERROR_CATEGORY cat,
													 /* IN    */ T_ERROR_INDEX index,
													 /* IN    */ T_ERROR error);

/*  @ROLE    : This function set the error category/index/value
    @RETURN  : None */
T_ERROR ERROR_AGENT_SetLastError(
											  /* INOUT */ T_ERROR_AGENT * ptr_this,
											  /* IN    */ T_ERROR_CATEGORY cat,
											  /* IN    */ T_ERROR_INDEX index,
											  /* IN    */ T_ERROR_VALUE value,
											  /* IN    */ T_ERROR error);


/*  @ROLE    : This function sends Error to Error Controller
    @RETURN  : Error code */
T_ERROR ERROR_AGENT_SendError(
										  /* INOUT */ T_ERROR_AGENT * ptr_this);


#endif
