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
 * @file Error_e.h
 * @author TAS
 * @brief the error list
 */

#ifndef Error_e
#   define Error_e

/*  Cartouche AD  */

#   include "Types_e.h"
#   include "DominoConstants_e.h"

/* -------------------------------------------------------------------------------------------------- */
/* Errors detection and dispensation */

/* Error Ids are returned as type T_ERROR */
/* Beware that those values are 1 more than the Id given in DOM2-ASPI-ID-O4O9 (because of C_ERROR_OK) */
typedef T_UINT8 T_ERROR;

enum
{
	/* COMMON ERROR */
  /*--------------*/
	C_ERROR_OK = 0,
	C_ERROR_INIT_REF,
	C_ERROR_INIT_PID,
	C_ERROR_INIT_COMPO,
	C_ERROR_END_SIMU,
	C_ERROR_ALLOC,
	C_ERROR_FILE_OPEN,
	C_ERROR_FILE_READ,
	C_ERROR_FILE_WRITE,
	C_ERROR_FRS_SYNC,
	C_ERROR_SOCK_OPEN,
	C_ERROR_SOCK_READ,			  /* NB 10 */
	C_ERROR_SOCK_WRITE,
	C_ERROR_SMEM_OPEN,
	C_ERROR_SMEM_READ,
	C_ERROR_SMEM_WRITE,
	C_ERROR_CONF_INVAL,
	C_ERROR_BUF_OVERFLOW,
	C_ERROR_BUF_UNDERFLOW,
	C_ERROR_BUF_EMPTY,
	C_ERROR_BAD_PARAM,
	C_ERROR_THREAD_CREATE,		  /* NB 20 */

	/* SYSTEM ERROR */
  /*---------------*/
	C_ERROR_C2P_MESSAGE_DESTRUCTION,
	C_ERROR_C2P_TIMER_EXPIRATION,
	C_ERROR_NCC_MAPPING,
	C_ERROR_NCC_REQUEST,
	C_ERROR_NCC_CHANNEL_NOT_CREATED,
	C_ERROR_NCC_CC_NO_MORE_VCI,
	C_ERROR_NCC_C2P_TIMEOUT,
	C_ERROR_NCC_SP_CONN_NOT_ESTABL_CAC,
	C_ERROR_NCC_SP_CONN_NOT_ESTABL_DAMA,
	C_ERROR_NCC_SP_UNKNOWN_STATE,	/* NB 30 */
	C_ERROR_NCC_OD_CONN_EST_CAC,
	C_ERROR_NCC_OD_CONN_EST_DAMA,
	C_ERROR_NCC_OD_CONN_EST_ST_DEST,
	C_ERROR_NCC_OD_CONN_EST_UNVALID_IP,
	C_ERROR_NCC_OD_CONN_MOD_CAC,
	C_ERROR_NCC_OD_CONN_MOD_DAMA,
	C_ERROR_NCC_OD_CONN_MOD_ST_DEST,
	C_ERROR_NCC_OD_CONN_MOD_UNVALID_IP,
	C_ERROR_NCC_OD_CONN_MOD_REL_UNKNOWN_CONN,	/* NC A 00075 : same error for MOD ou REL */
	C_ERROR_NCC_OD_PENDING_RELEASE,	/* NB 40 *//* NC A 00075 : minor error added */
	C_ERROR_ST_C2P_TIMEOUT,
	C_ERROR_ST_UNKNOWN_TRF,
	C_ERROR_ST_UNKNOWN_CNC,		  /* NC A 00075 : minor error for unknown connection */
	C_ERROR_ST_NO_MORE_CONTEXT,
	C_ERROR_NAT_MULTICAST_WRONG_QOS,	/* minor error for wrong multicast connection or flow */
	C_ERROR_NAT_MULTICAST_NOT_UNIDIR,	/* minor error for wrong multicast connection */
	C_ERROR_NAT_MULTICAST_WRONG_DESTINATION,	/* critical error for multicast sp connection of flow */
	C_ERROR_NAT_NOT_ALONE,		  /* PM on NAT gestion by NCC : this error is MINOR */
	C_ERROR_NCC_NO_CHANNEL_AVAILABLE,	/* NC A 00155 */
	C_ERROR_NCC_NO_FREE_CONTEXT, /* Gestion des cas limites : CRITICAL dans D2D */
	C_ERROR_NCC_MAX_CONN_NB_REACHED,	/* Gestion des cas limites : MINOR  */
	C_ERROR_NCC_CONGESTION_ON_SIG,	/* MINOR : cas de congestion sur la SIG  */

	C_ERROR_NB
};

/* -------------------------------------------------------------------------------------------------- */
/* Errors characterization */

/* Error categories are set using ERROR_AGENT_SetErrorCat(ErrorAgent, T_ERROR_CATEGORY cat) */
typedef T_UINT8 T_ERROR_CATEGORY;
enum
{
	C_ERROR_INIT = C_CAT_INIT,
	C_ERROR_END = C_CAT_END,
	C_ERROR_CRITICAL = 2,
	C_ERROR_MINOR = 3,

	C_ERROR_CAT_NB
};

/* Error Indexes are set using ERROR_AGENT_SetErrorIndex(ErrorAgent, T_ERROR_INDEX index) */
typedef T_UINT16 T_ERROR_INDEX;

/* Error indexes for file open/write/read (to be completed) */
typedef enum
{

	C_ARCHITECTURE_FILE = 0,
	C_PROBE_DEF_FILE,
	C_PROBE_ACT_FILE,
	C_EVENT_DEF_FILE,
	C_EVENT_ACT_FILE,
	C_ERROR_DEF_FILE,
	C_MAIN_CONFIG_FILE,
	C_RADIO_RESOURCES_FILE,
	C_COM_PARAMETERS_FILE,
	C_EVENT_PARAM_FILE,
	C_ERROR_PARAM_FILE,
	C_TRACE_DEF_FILE,
	C_COMPONENT_CONFIG_FILE,

	/* Liste à compléter ...  */

	C_PROBE_LOG_FILE,				  /* Keep these 2 values as the last ones of this enum */
	C_EVENT_LOG_FILE				  /* Keep these 2 values as the last ones of this enum */
} T_FILE_INFOS_INDEX;


/* Error indexes for socket open/write/read  */
typedef enum
{
	C_SI_UL_SOCKET = 0,
	C_SI_DL_SOCKET = 1,
	C_SI_UR_SOCKET = 2,
	C_II_E_SOCKET = 3,
	C_II_P_SOCKET = 4,
	C_II_PF_SOCKET = 5,
	C_EI_PD_SOCKET = 6,
	C_EI_RT_SOCKET = 7,
	C_TE_RA_SOCKET = 8,
	C_ST_RS_SOCKET = 9,
	C_ARC_SOCKET = 10,
    C_PEP_SOCKET = 11
} T_SOCKET_INFOS_INDEX;

/* Error indexes for buffer overflow/underflow/empty and bad parameter */
typedef enum
{
	C_UNKNOWN_BUFFER = 0,
	C_PROBE_COMMAND = 1,
	C_ST_COMMAND = 2,
	C_TRANSPORT_COMMAND = 3,
	C_EVENT_COMMAND = 4,
	C_INTERFACE_COMMAND = 5,
	C_ST_TIMU_BUFFER = 6,
	C_ST_TBTP_BUFFER = 7,
	C_ST_TRF_BUFFER = 8,
	C_ST_SIG_BUFFER = 9,
	C_ST_RRC_BUFFER = 10,

	C_NCC_TRANSPORT_DL,
	C_NCC_C2P_DULM_BUFFER,
	C_NCC_SACT_BUFFER,
	C_NCC_C2P_TIMu_BUFFER,
	C_NCC_TBTP_BUFFER,
	C_OBP_BUFFER,
	C_NAT_COMMAND
} T_BUFFER_PARAM_INFOS_INDEX;

/* Error indexes for shared memory overflow/underflow/empty and bad parameter */
typedef enum
{
	C_SI_ST_SMEM = 0,
	C_SI_RT_NTOA_SMEM = 1,
	C_SI_RT_ATON_SMEM = 2,
	C_SI_OS_SMEM = 3,
	C_SE_RA_NTOA_SMEM = 4,
	C_SE_RA_ATON_SMEM = 5
} T_SMEM_INFOS_INDEX;

/* Error Values are set using ERROR_AGENT_SetErrorValue(ErrorAgent, T_ERROR_VALUE value) */
typedef T_UINT32 T_ERROR_VALUE;

/* Error indexes for overun/underun */
typedef enum
{
	C_OVERRUN_ERROR = 0,
	C_UNDERRUN_ERROR = 1,
	C_FRS_JOIN_ERROR = 2,
	C_FRS_YIELD = 3
} T_FRS_SYNCHRONISATION_INDEX;

/* Error indexes for C2P overflow */
typedef enum
{
	C_RRC_BUFFER_OVERFLOW_ERROR = 0,
	C_REASSEMBLY_BUFFER_OVERFLOW_ERROR = 1,
	C_OBP_BUFFER_OVERFLOW_ERROR = 2
} T_C2P_MESSAGE_DESTRUTION_INDEX;

/* Error indexes for C2P timer expiration */
typedef enum
{
	C_CC_TIMER_EXPIRATION_ERROR = 0,
	C_REASSEMBLY_TIMER_EXPIRATION_ERROR = 1
} T_C2P_TIMER_EXPIRATION_INDEX;

/* Error indexes for C2P event unexpected */
typedef enum
{
	C_EST_REQ_ERROR = 0,
	C_EST_REP_ERROR = 1,
	C_MOD_REQ_ERROR = 2,
	C_MOD_REP_ERROR = 3,
	C_REL_REQ_ERROR = 4,
	C_REL_REP_ERROR = 5
} T_C2P_UNEXPECTED;

/* Error for CR */
typedef enum
{
	C_CR_BAD_TYPE_ERROR = 0,
	C_CR_UNKNOWN_ST_ERROR,
	C_CR_BAD_VALUE_ERROR
} T_CR_INVALID_INDEX;

/* -------------------------------------------------------------------------------------------------- */
/* Useful macros to improve code clarity and compacity (see below) */
#   include "Trace_e.h"


/* Executes the specified function call, 
   if any error raised send error and trace error. */
#   define SEND_ERRNO(rid, function, ptr_execContext, cat, index, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastErrorErrno(ptr_execContext,cat,index,rid); \
      TRACE_LOG_FORCE(msg); \
    } \
  }

/* Executes the specified function call, 
   if any error raised send error, jump to 'etiquette' and trace error. */
#   define SEND_ERRNO_JUMP(etiquette, rid, function, ptr_execContext, cat, index, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastErrorErrno(ptr_execContext,cat,index,rid); \
      TRACE_LOG_FORCE(msg); \
      goto etiquette; \
    } \
  }

/* Executes the specified function call,  
   if any error raised send error and trace . */
#   define SEND_ERRNO_MINOR(function, ptr_execContext, index, msg) \
  { \
    T_ERROR minorRid = function; \
    if (minorRid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastErrorErrno(&((ptr_execContext)->_ErrorAgent),C_ERROR_MINOR,index,minorRid); \
      TRACE_LOG(msg); \
    } \
  }

/* Executes the specified function call, 
   if any error raised send error, jump to 'etiquette' and trace error. */
#   define SEND_ERROR_JUMP(etiquette, rid, function, ptr_execContext, cat, index, value, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastError(&((ptr_execContext)->_ErrorAgent),cat,index,value,rid); \
      TRACE_LOG_FORCE(msg); \
      goto etiquette; \
    } \
  }

/* Executes the specified function call, 
   if any error raised send error and trace error. */
#   define SEND_AG_ERRNO(rid, function, agent, cat, index, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastErrorErrno(agent,cat,index,rid); \
      TRACE_LOG_FORCE(msg); \
    } \
  }

/* Executes the specified function call, 
   if any error raised send error and trace error. */
#   define SEND_AG_ERRNO_MINOR(rid, function, agent, index, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastErrorErrno(agent,C_ERROR_MINOR,index,rid); \
      TRACE_LOG(msg); \
    } \
  }

/* Executes the specified function call, 
   if any error raised send error, jump to 'etiquette' and trace error. */
#   define SEND_AG_ERRNO_JUMP(etiquette, rid, function, agent, cat, index, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      ERROR_AGENT_SetLastErrorErrno(agent,cat,index,rid); \
      TRACE_LOG_FORCE(msg); \
      goto etiquette; \
    } \
  }

/* Jump to the specified 'etiquette' with the specified error code */
#   define JUMP(etiquette, rid, code) \
  { \
    rid = code; \
    TRACE_LOG_FORCE((C_TRACE_THREAD_UNKNOWN,C_TRACE_COMP_UNKNOWN,C_TRACE_ERROR, \
               "JUMP @ file(%s) line(%d) rid(%d)",\
               __FILE__,__LINE__,rid)); \
    goto etiquette; \
  }

/* Jump to the specified 'etiquette' with the specified error code */
#   define JUMP_TRACE(etiquette, rid, code, msg) \
  { \
    rid = code; \
    TRACE_LOG_FORCE(msg); \
    goto etiquette; \
  }

/* Executes the specified function call, jump to 'etiquette' if any error raised. */
#   define JUMP_ERROR(etiquette, rid, function) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      TRACE_LOG_FORCE((C_TRACE_THREAD_UNKNOWN,C_TRACE_COMP_UNKNOWN,C_TRACE_ERROR, \
                 "JUMP_ERROR @ file(%s) line(%d) rid(%d)",\
                 __FILE__,__LINE__,rid)); \
      goto etiquette; \
    } \
  }

/* Executes the specified function call, jump to 'etiquette' and trace error if any error raised. */
#   define JUMP_ERROR_TRACE(etiquette, rid, function, msg) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) { \
      TRACE_LOG_FORCE(msg); \
      goto etiquette; \
    } \
  }

/* Executes the specified function call, jump to 'etiquette' if major (critical or init) error raised,
    else send error to controler. 
    Use of this macro requires thread's cxt's Error Agent pointer */
#   define JUMP_MAJOR(agent, etiquette, rid, function) \
  { \
    rid = function; \
    if (rid != C_ERROR_OK) \
    { \
      if (ERROR_GetErrorCat() != C_ERROR_MINOR) \
        goto etiquette; \
      else \
        ERROR_AGENT_SendError(agent, rid); \
    } \
  }


/* -------------------------------------------------------------------------------------------------- */
/* Error Usage Template

Errors must be detected, caracterized and propagated in a standard manner.
These templates should be respected through the whole ASP simulation components.

1/ Methods should (in many cases) return the T_ERROR type. The following is a standard template.

T_ERROR CLASS_Function(...)
{
T_ERROR rid = C_ERROR_OK;

  ...
  if (ptr_toto == NULL)
    JUMP(rid, C_ERROR_ALLOC, FIN);
  ...

FIN:
  return rid;
}


2/ Methods should dispensate errors. This can lead to bory and redundant coding... 
    The JUMP_x macros (see before) can be used to improve code clarity and compacity 
    in the case of linear call sequences.
    Notice that the 'FIN' label scope is local, and that there may be 1,2 or more ending labels.
    This tends to mimic the "try..catch..finaly" construction.

T_ERROR CLASS_Function(...)
{
T_ERROR rid = C_ERROR_OK;

  ...INITIALISATION code (open files, allocate memory, etc.)...

  JUMP_ERROR(FIN, rid, call1);

  JUMP_ERROR(FIN, rid, call2);

  ...

  JUMP_MAJOR(cxt, FIN, rid, calln);  This call can raise minor errors without global dammage

FIN:
  ...TERMINATION code (close files, free memory, etc.)...

  return rid;
}


3/ Component-level methods should detect and characterize errors. The following provide code samples.

T_ERROR CLASS_Function1(...)
{
T_ERROR rid = C_ERROR_OK;

  ...
  f = fopen(name, "rt");
  if (f == NULL) {
    rid = C_ERROR_FILE_OPEN;
    ERROR_AGENT_SetErrorInfos(C_ERROR_CRITICAL, C_CONFIG_FILE_ARCHITECTURE);
    goto FIN;    
  }
  ...
  
FIN:
  return rid;
}

  At least Error category must be set prior to dispensation.

T_ERROR CLASS_Function2(...)
{
T_ERROR rid = C_ERROR_OK;

  ...
  f = fopen(name, "rt");
  if (f == NULL) {
    rid = C_ERROR_FILE_OPEN;
    ERROR_SetErrorCat(C_ERROR_CRITICAL);
    ERROR_SetErrorValue(errno);
     Error index must be set by the caller... 
    goto FIN;    
  }
  ...
  
FIN:
  return rid;
}

  
*/

#endif
