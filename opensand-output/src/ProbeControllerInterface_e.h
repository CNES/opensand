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
 * @file ProbeControllerInterface_e.h
 * @author TAS
 * @brief The ProbeController class implements the probe controller process
 */

#ifndef ProbeController_e
#   define ProbeController_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "UDPSocket_e.h"
#   include "GenericPort_e.h"
#   include "GenericPacket_e.h"
#   include "ProbesDef_e.h"
#   include "ProbeHolder_e.h"
#   include "ErrorAgent_e.h"


#   define C_UDP_SEND_MAX_PKG  200  	/* the UDP socket buffer size */
#   define C_CMPT_MAX          4	/* the maximun number of probe component */
#   define C_ST_MAX            5 	/* the maximun number of ST instance */


typedef struct
{
	T_UINT8 _componentId;
	T_UINT8 _probeId;
	T_UINT16 _type;
	T_UINT32 _index;
	T_UINT32 _value;
	T_FLOAT _time;
} T_DISPLAY_DATA;

typedef struct
{
	T_BOOL _displayPortReady;
	T_FLOAT _FRSDuration;		  /* FRS duration in s */
	T_UINT32 _FSMNb;
	T_UINT32 _actifCmptIndex;	  /* indicates the index of the actif component */
	T_ERROR_AGENT _errorAgent;	  /* the error agent */
	T_BOOL _simuIsRunning;		  /* the simulation is running */
	/* statistic information */
	T_PROBES_DEF _probesDef[C_CMPT_MAX];	/* the probe definition */
	T_PROBE_HOLDER *_ptr_probeData[C_CMPT_MAX];	/* the probe data */
	T_UINT8 _instanceNumber[C_CMPT_MAX];	/* the instance number for each component */
	/* socket communication */
	T_GENERIC_PORT _probePort;	  /* the probe receiver */
	T_UDP_SOCKET _displayPort;	  /* the display port */
	T_GENERIC_PKT *_ptr_genPacket;	/* the generic packet */

	T_ENUM_COUPLE C_PROB_AGGREGATE_choices[C_AGG_NB + 1];
	T_ENUM_COUPLE C_PROB_ANALYSIS_choices[C_ANA_NB + 1];

} T_PRB_CTRL;


/*  @ROLE    : This function starts controller's interface
    @RETURN  : Error code */
int startProbeControllerInterface(int argc, char *argv[]);


T_ERROR PRB_CTRL_Init(
/*  IN     */ T_PRB_CTRL * ptr_this);

T_ERROR PRB_CTRL_InitSimulation(
											 /* INOUT */ T_PRB_CTRL * ptr_this);


#endif /* ProbeController_e */
