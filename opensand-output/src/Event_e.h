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
 * @file Event_e.h
 * @author TAS
 * @brief The Event_e file defines structures used to handle Events
 */

#ifndef Event_e
#   define Event_e

#   include "Types_e.h"
#   include "DominoConstants_e.h"

/* Component state values */
enum
{
	C_EVENT_STATE_START = 0,
	C_EVENT_STATE_INIT = 1,
	C_EVENT_STATE_RUN = 2,
	C_EVENT_STATE_STOP = 3,
};

typedef T_UINT8 T_EVENT;

enum
{
	/* COMMON EVENT */
	/*--------------*/
	C_EVENT_INIT_REF = 0,
	C_EVENT_END_SIMU,
	C_EVENT_COMP_STATE,

	C_EVENT_LOGIN_SENT,
	C_EVENT_LOGIN_RECEIVED,
	C_EVENT_LOGIN_RESPONSE,
	C_EVENT_LOGIN_COMPLETE,

	C_EVENT_CONNECTION_START,
	C_EVENT_CONNECTION_STOP,

#   ifndef _ASP_PEA_CONF
	/* DOMINO2 EVENT */
	/*---------------*/
	C_EVENT_DISCARD_BTP_DYNAMIC,
	C_EVENT_C2P_SETUP,
	C_EVENT_C2P_SETUP_ACK,
	C_EVENT_C2P_MODIF,
	C_EVENT_C2P_MODIF_ACK,
	C_EVENT_C2P_RELEASE,
	C_EVENT_C2P_RELEASE_ACK,

#   else
	/* _ASP_PEA_CONF */
	/* PEA EVENT */
	/*-----------*/
	C_EVENT_TS_C2P_EST_REQ_SENT,
	C_EVENT_CCRA_C2P_EST_REQ_SENT,
	C_EVENT_TS_C2P_EST_REQ_RECEIVED,
	C_EVENT_CCRA_C2P_EST_REQ_RECEIVED,
	C_EVENT_TS_C2P_EST_REP_SENT, /* EVENT Nb 10 */
	C_EVENT_CCRA_C2P_EST_REP_SENT,
	C_EVENT_TS_C2P_EST_REP_RECEIVED,
	C_EVENT_CCRA_C2P_EST_REP_RECEIVED,
	C_EVENT_CCRA_C2P_MOD_REQ_SENT,
	C_EVENT_TS_C2P_MOD_REQ_RECEIVED,
	C_EVENT_TS_C2P_MOD_REP_SENT,
	C_EVENT_CCRA_C2P_MOD_REP_RECEIVED,
	C_EVENT_TS_C2P_REL_REQ_SENT,
	C_EVENT_CCRA_C2P_REL_REQ_SENT,
	C_EVENT_TS_C2P_REL_REQ_RECEIVED,	/* EVENT Nb 20 */
	C_EVENT_CCRA_C2P_REL_REQ_RECEIVED,
	C_EVENT_TS_C2P_REL_REP_SENT,
	C_EVENT_CCRA_C2P_REL_REP_SENT,
	C_EVENT_TS_C2P_REL_REP_RECEIVED,
	C_EVENT_CCRA_C2P_REL_REP_RECEIVED,
	C_EVENT_CCRA_SOAP_EST_REQ_RECEIVED,
	C_EVENT_CCRA_SOAP_MOD_REQ_RECEIVED,
	C_EVENT_CCRA_SOAP_REL_REQ_RECEIVED,
	C_EVENT_CCRA_SOAP_REL_ALL_REQ_RECEIVED,
	C_EVENT_CCRA_SOAP_EST_REP_SENT,
	C_EVENT_CCRA_SOAP_MOD_REP_SENT,
	C_EVENT_CCRA_SOAP_REL_REP_SENT,
	C_EVENT_CCRA_SOAP_REL_ALL_REP_SENT,
	C_EVENT_RETRANSMISSION_C2P_EST_REQ,
	C_EVENT_RETRANSMISSION_C2P_MOD_REQ,
	C_EVENT_RETRANSMISSION_C2P_REL_REQ,
	C_EVENT_EST_REP_NOT_RECEIVED_REL,
	C_EVENT_SAP_OPEN_CNF_UNKNOWN_CALLID,	/* EVENT Nb 30 */
	C_EVENT_SAP_OPEN_IND_FLOW_UNKNOWN,
	C_EVENT_C2P_DULM_SIGNALISATION,
	C_EVENT_C2P_TIMU_SIGNALISATION,
	C_EVENT_TBTP_SIGNALISATION,
	C_EVENT_TRANSIT_SIGNALISATION,
	C_EVENT_C2P_DULM_REASS_LOST,
	C_EVENT_C2P_TIMU_REASS_LOST,
	C_EVENT_TBTP_REASS_LOST,
	C_EVENT_SACT_REASS_LOST,	  /* EVENT Nb 39 */
#   endif
		 /* _ASP_PEA_CONF */

	C_EVENT_NB
};

/* Event categories are set using EVENT_AGENT_SetEventCat(EventAgent, T_EVENT_CATEGORY cat) */
typedef T_UINT8 T_EVENT_CATEGORY;
enum
{
#   ifndef _ASP_PEA_CONF
	C_EVENT_INIT = C_CAT_INIT,
	C_EVENT_END = C_CAT_END,
	C_EVENT_SIMU = 2,
	C_EVENT_MAC_CONNECTION = 3,
	C_EVENT_MAC_CONNECTION_NOM = 3,
	C_EVENT_MAC_CONNECTION_DEG = 3,
	C_EVENT_SIG = 3,
	C_EVENT_TRAFFIC = 4,
#   else								  /* _ASP_PEA_CONF */
	C_EVENT_INIT = C_CAT_INIT,
	C_EVENT_END = C_CAT_END,
	C_EVENT_SIMU = 2,
	C_EVENT_TRAFFIC,
	C_EVENT_MAC_CONNECTION_NOM,
	C_EVENT_MAC_CONNECTION_DEG,
	C_EVENT_SIG,
#   endif
		 /* _ASP_PEA_CONF */

	C_EVENT_CAT_NB
};



/* Event indexes for C2P Mod Rep sent event */
typedef enum
{
	C_CRA_BDC_POS_EVENT = 0,
	C_CRA_NEG_BDC_POS_EVENT = 1,
	C_CRA_POS_BDC_NEG_EVENT = 2,
	C_CRA_BDC_NEG_EVENT = 3,
	C_MOD_RETRANSMISSION_EVENT = 4
} T_MOD_TYPE;

/* Event Indexes are set using EVENT_AGENT_SetEventIndex(EventAgent, T_EVENT_INDEX index) */
typedef T_UINT16 T_EVENT_INDEX;

/* Error Values are set using EVENT_AGENT_SetEventValue(EventAgent, T_EVENT_VALUE value) */
typedef T_UINT32 T_EVENT_VALUE;

#   ifdef _ASP_PEA_CONF

/* Macro to compute event Key from TS Id & CallId */
#      define EVENT_KEY(TSId,CallId) \
  ( (T_UINT32) ( ((((T_UINT16)TSId)<<8) & 0xFF00) + (((T_UINT16)CallId) & 0x00FF) ) )

#   endif



#endif