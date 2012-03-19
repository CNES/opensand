/*
 *
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
 * Platine is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

#ifndef RLSS_AGENT_H__
#   define RLSS_AGENT_H__
/******************************************************************************
**                                                                           **
**                                 ALCATEL                                   **
**                                                                           **
******************************************************************************/ 
	
	
/*************************** COPYRIGHT INFORMATION ****************************
**                                                                           **
** This program contains proprietary information which is a trade            **
** secret of ALCATEL and also is protected as an unpublished                 **
** work under applicable Copyright laws. Recipient is to retain this         **
** program in confidence and is not permitted to use or make copies          **
** thereof other than as permitted in a written agreement with ALCATEL.      **
**                                                                           **
******************************************************************************/ 
	
/****************************** IDENTIFICATION ********************************
**
** Project          : GAC
**
** Module           : Simulators/RLSS_agent
**
** File name        : rlss_agent.h
**
** Description      : 
**
** Reference(s)     : document title and or id, ...
**
** Contact          : Poucet B.
**
** Creation date    : 03/06/2002
**
******************************************************************************/ 
	
/*#######################################################################
 #                                                                      #
 #  INCLUDES                                                            #
 #                                                                      #
 ######################################################################*/ 
/*   We use 'header_generic' from the util_funcs module, so make sure this module is included in the agent.
*/ 
	config_require(util_funcs)  
/*   Magic number definitions.
     These must be unique for each object implemented within a single mib module callback routine.
     Typically, these will be the last OID sub-component for each entry, or integers incrementing from 1.
     (which may well result in the same values anyway).
     Here, the second and third objects are form a 'sub-table' and the magic numbers are chosen to match these OID sub-components.
     This is purely for programmer convenience. All that really matters is that the numbers are unique.
*/ 
	
/*#######################################################################
 #                                                                      #
 # DEFINES, MACROS                                                      #
 #                                                                      #
 ######################################################################*/ 
#   define MN_sitDescr			        101
#   define MN_tmSitId       			102
#   define MN_tmMsgLatency            	       	103
#   define MN_terminalId       			104
#   define MN_terminalMacAddr      		105
#   define MN_terminalCap				106
#   define MN_terminalAreaId      			107
#   define MN_terminalSegId       			108
#   define MN_terminalState			109
#   define MN_terminalCraTrfc     			110
#   define MN_terminalRbdcTrfc    			111
#   define MN_terminalVbdcTrfc			112
#   define MN_terminalFcaTrfc			113
#   define MN_terminalInAreaPrevId			114
#   define MN_terminalInAreaNextId			115
#   define MN_terminalTimeStamp   			116
#   define MN_terminalControl			117
#   define MN_fwdInteractionPathDescr	       	118
	
#   define MN_rmRlssState  	       		201
#   define MN_rmRlssPrimeShipFlag	       		202
#   define MN_rmRlssPrimeIpAddress	       		203
#   define MN_rmRlssSecondIpAddress       		204
	
/*#######################################################################
 #                                                                      #
 # INTERFACE CONSTANT AND INTERFACE VARIABLE EXTERN DECLARATIONS        #
 #                                                                      #
 ######################################################################*/ 
/*   Declare our publically-visible functions.
     Typically, these will include the initialization and shutdown functions, the main request callback routine and any writeable object methods.
     Function prototypes are provided for the callback routine ('FindVarMethod') and writeable object methods ('WriteMethod').
*/ 
	  extern void init_rlss_agent(void);
	  extern FindVarMethod var_mibdb_simplevar;
	  extern FindVarMethod var_mibdb_tablevar;
	  extern WriteMethod write_mibdb_simplevar;
	  extern WriteMethod write_mibdb_tablevar;
	  int requestCallback(int majorID, int minorID, void *serverarg,
									void *clientarg);

#endif /* RLSS_AGENT_H__ */
