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
 * @file Probe_e.h
 * @author TAS
 * @brief Probes declaration
 */

#ifndef	Probe_e
#   define	Probe_e

/* SYSTEM RESOURCES */
/* PROJECT RESOURCES */
#   include "Types_e.h"

typedef enum
{
	C_PROBE_ST_TERMINAL_QUEUE_SIZE = 1,
	C_PROBE_ST_PHYSICAL_OUTGOING_THROUGHPUT,
	C_PROBE_ST_RBDC_REQUEST_SIZE,
	C_PROBE_ST_VBDC_REQUEST_SIZE,
	C_PROBE_ST_ALLOCATION_SIZE,
	C_PROBE_ST_UNUSED_CAPACITY,
	C_PROBE_ST_REAL_INCOMING_THROUGHPUT,
	C_PROBE_ST_REAL_OUTGOING_THROUGHPUT,
	C_PROBE_ST_CRA,
	C_PROBE_ST_OUTPUT_HDLB_RATE,
	C_PROBE_ST_OUTPUT_HDLB_DROPS,
	C_PROBE_ST_OUTPUT_HDLB_BACKLOG,
	C_PROBE_ST_BBFRAME_DROPED_RATE,
	C_PROBE_ST_REAL_MODCOD,
	C_PROBE_ST_USED_MODCOD
} T_PROBE_ST_ID;

/* Probe ID for GW : Do not use it directly but use macros defined below*/
typedef enum
{
	C_PROBE_GW_RBDC_REQUEST_NUMBER = 1,
	C_PROBE_GW_RBDC_REQUESTED_CAPACITY,
	C_PROBE_GW_VBDC_REQUEST_NUMBER,
	C_PROBE_GW_VBDC_REQUESTED_CAPACITY,
	C_PROBE_GW_UPLINK_FAIR_SHARE,
	C_PROBE_GW_CRA_ALLOCATION,
	C_PROBE_GW_CRA_ST_ALLOCATION,
	C_PROBE_GW_RBDC_ALLOCATION,
	C_PROBE_GW_RBDC_ST_ALLOCATION,
	C_PROBE_GW_RBDC_MAX_ALLOCATION,
	C_PROBE_GW_RBDC_MAX_ST_ALLOCATION,
	C_PROBE_GW_VBDC_ALLOCATION,
	C_PROBE_GW_FCA_ALLOCATION,
	C_PROBE_GW_LOGGED_ST_NUMBER,
	C_PROBE_GW_BBFRAME_SENT_PER_FRAME,
} T_PROBE_GW_ID;



#endif
