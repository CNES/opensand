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
 * @file Trace_e.h
 * @author TAS
 * @brief The Trace class implements the trace mechanism
 */

#ifndef Trace_e
#   define Trace_e

/* SYSTEM RESOURCES */
#   include <errno.h>
#   include <stdio.h>
/* PROJECT RESOURCES */
#   include "Types_e.h"

#   define C_TRACE_PACKET_STR_MAX_SIZE 4000
#   define C_TRACE_STR_MAX_SIZE        300
#   define C_TRACE_PACKET_DATA_MAX_SIZE 700
/* Trace types */
/*-------------*/
#      define TRACE_INIT_ULL 0x1ULL

typedef T_UINT64 T_TRACE_LEVEL;
/* Define Level for Trace */
#   define C_TRACE_DEBUG_0              (TRACE_INIT_ULL   << 0)
#   define C_TRACE_DEBUG_1              (C_TRACE_DEBUG_0  << 1)
#   define C_TRACE_DEBUG_2              (C_TRACE_DEBUG_1  << 1)
#   define C_TRACE_DEBUG_3              (C_TRACE_DEBUG_2  << 1)
#   define C_TRACE_DEBUG_4              (C_TRACE_DEBUG_3  << 1)
#   define C_TRACE_DEBUG_5              (C_TRACE_DEBUG_4  << 1)
#   define C_TRACE_DEBUG_6              (C_TRACE_DEBUG_5  << 1)
#   define C_TRACE_DEBUG_7              (C_TRACE_DEBUG_6  << 1)
#   define C_TRACE_DEBUG_8              (C_TRACE_DEBUG_7  << 1)
#   define C_TRACE_DEBUG_9              (C_TRACE_DEBUG_8  << 1)
#   define C_TRACE_DEBUG_10             (C_TRACE_DEBUG_9  << 1)
#   define C_TRACE_DEBUG_11             (C_TRACE_DEBUG_10 << 1)
#   define C_TRACE_DEBUG_12             (C_TRACE_DEBUG_11 << 1)
#   define C_TRACE_DEBUG_13             (C_TRACE_DEBUG_12 << 1)
#   define C_TRACE_DEBUG_14             (C_TRACE_DEBUG_13 << 1)
#   define C_TRACE_DEBUG_15             (C_TRACE_DEBUG_14 << 1)
#   define C_TRACE_DEBUG_16             (C_TRACE_DEBUG_15 << 1)
#   define C_TRACE_DEBUG_17             (C_TRACE_DEBUG_16 << 1)
#   define C_TRACE_DEBUG_18             (C_TRACE_DEBUG_17 << 1)
#   define C_TRACE_DEBUG_19             (C_TRACE_DEBUG_18 << 1)
#   define C_TRACE_DEBUG_20             (C_TRACE_DEBUG_19 << 1)
#   define C_TRACE_DEBUG_21             (C_TRACE_DEBUG_20 << 1)
#   define C_TRACE_DEBUG_22             (C_TRACE_DEBUG_21 << 1)
#   define C_TRACE_DEBUG_23             (C_TRACE_DEBUG_22 << 1)
#   define C_TRACE_DEBUG_24             (C_TRACE_DEBUG_23 << 1)
#   define C_TRACE_DEBUG_25             (C_TRACE_DEBUG_24 << 1)
#   define C_TRACE_DEBUG_26             (C_TRACE_DEBUG_25 << 1)
#   define C_TRACE_DEBUG_27             (C_TRACE_DEBUG_26 << 1)
#   define C_TRACE_VALID_0              (C_TRACE_DEBUG_27 << 1)
#   define C_TRACE_VALID_1              (C_TRACE_VALID_0  << 1)
#   define C_TRACE_VALID_2              (C_TRACE_VALID_1  << 1)
#   define C_TRACE_VALID_3              (C_TRACE_VALID_2  << 1)
#   define C_TRACE_VALID_4              (C_TRACE_VALID_3  << 1)
#   define C_TRACE_VALID_5              (C_TRACE_VALID_4  << 1)
#   define C_TRACE_VALID_6              (C_TRACE_VALID_5  << 1)
#   define C_TRACE_VALID_7              (C_TRACE_VALID_6  << 1)
#   define C_TRACE_VALID_8              (C_TRACE_VALID_7  << 1)
#   define C_TRACE_VALID_9              (C_TRACE_VALID_8  << 1)
#   define C_TRACE_VALID_10             (C_TRACE_VALID_9  << 1)
#   define C_TRACE_VALID_11             (C_TRACE_VALID_10 << 1)
#   define C_TRACE_VALID_12             (C_TRACE_VALID_11 << 1)
#   define C_TRACE_VALID_13             (C_TRACE_VALID_12 << 1)
#   define C_TRACE_VALID_14             (C_TRACE_VALID_13 << 1)
#   define C_TRACE_VALID_15             (C_TRACE_VALID_14 << 1)
#   define C_TRACE_VALID_16             (C_TRACE_VALID_15 << 1)
#   define C_TRACE_VALID_17             (C_TRACE_VALID_16 << 1)
#   define C_TRACE_VALID_18             (C_TRACE_VALID_17 << 1)
#   define C_TRACE_VALID_19             (C_TRACE_VALID_18 << 1)
#   define C_TRACE_VALID_20             (C_TRACE_VALID_19 << 1)
#   define C_TRACE_VALID_21             (C_TRACE_VALID_20 << 1)
#   define C_TRACE_VALID_22             (C_TRACE_VALID_21 << 1)
#   define C_TRACE_VALID_23             (C_TRACE_VALID_22 << 1)
#   define C_TRACE_VALID_24             (C_TRACE_VALID_23 << 1)
#   define C_TRACE_VALID_25             (C_TRACE_VALID_24 << 1)
#   define C_TRACE_VALID_26             (C_TRACE_VALID_25 << 1)
#   define C_TRACE_VALID_27             (C_TRACE_VALID_26 << 1)
#   define C_TRACE_FUNC                 (C_TRACE_VALID_27 << 1)
#   define C_TRACE_ERROR                (C_TRACE_FUNC     << 1)
#   define C_TRACE_MINOR                (C_TRACE_ERROR    << 1)

/* define default value */
#   define C_TRACE_DEBUG C_TRACE_DEBUG_0
#   define C_TRACE_VALID C_TRACE_VALID_0


/* Trace Component */
typedef enum
{
#   ifdef _ASP_PEA_CONF
	C_TRACE_COMP_TS = 0,
#   else								  /* DOMINO2 */
	C_TRACE_COMP_ST = 0,
#   endif
		 /* _ASP_PEA_CONF */
	C_TRACE_COMP_TG,
	C_TRACE_COMP_EIA_IN,
	C_TRACE_COMP_EIA_OUT,
#   ifdef _ASP_PEA_CONF
	C_TRACE_COMP_TS_AGG,
#   else
		/* DOMINO2 */
	C_TRACE_COMP_ST_AGG,
#   endif
		 /* _ASP_PEA_CONF */
	C_TRACE_COMP_NAT,
#   ifdef _ASP_PEA_CONF
	C_TRACE_COMP_CCRA,
#   else
		/* DOMINO2 */
	C_TRACE_COMP_NCC,
#   endif
		 /* _ASP_PEA_CONF */
	C_TRACE_COMP_SAT,
	C_TRACE_COMP_OBPC,
	C_TRACE_COMP_AIE_IN,
	C_TRACE_COMP_AIE_OUT,

	C_TRACE_COMP_CONFIG,
	C_TRACE_COMP_INTERFACES,
	C_TRACE_COMP_SHARED_MEMORY,
	C_TRACE_COMP_TRANSPORT,
	C_TRACE_COMP_UTILITIES,
	C_TRACE_COMP_SCHEDULER,
	C_TRACE_COMP_PROBE,
	C_TRACE_COMP_ERROR,
	C_TRACE_COMP_EVENT,
	C_TRACE_COMP_PROTOCOL,
	C_TRACE_COMP_TESTER,
	C_TRACE_COMP_OBPCTESTER,	  /* value 22 */

	C_TRACE_COMP_UNKNOWN,		  /* DO NOT USED THIS FLAG */
	C_TRACE_COMP_MAX
} T_TRACE_COMPONENT_TYPE;


/* Trace Thread */
typedef enum
{
#   ifdef _ASP_PEA_CONF
	C_TRACE_THREAD_TS = 0,		  /* Up to 6 TS threads */
	C_TRACE_THREAD_TS_1,
	C_TRACE_THREAD_TS_2,
	C_TRACE_THREAD_TS_3,
	C_TRACE_THREAD_TS_4,
	C_TRACE_THREAD_TS_5,
#   else								  /* DOMINO2 */
	C_TRACE_THREAD_ST = 0,		  /* Up to 6 ST threads */
	C_TRACE_THREAD_ST_1,
	C_TRACE_THREAD_ST_2,
	C_TRACE_THREAD_ST_3,
	C_TRACE_THREAD_ST_4,
	C_TRACE_THREAD_ST_5,
#   endif
		 /* _ASP_PEA_CONF */
	C_TRACE_THREAD_TG,			  /* Up to 8 TG threads (1 for every ST,NAT,STAGG) */
	C_TRACE_THREAD_TG_1,
	C_TRACE_THREAD_TG_2,
	C_TRACE_THREAD_TG_3,
	C_TRACE_THREAD_TG_4,
	C_TRACE_THREAD_TG_5,
	C_TRACE_THREAD_TG_6,
	C_TRACE_THREAD_TG_7,
	C_TRACE_THREAD_EIA_IN,		  /* Up to 7 EIA_IN threads (1 for every ST, 1 for NAT) */
	C_TRACE_THREAD_EIA_IN_1,
	C_TRACE_THREAD_EIA_IN_2,
	C_TRACE_THREAD_EIA_IN_3,
	C_TRACE_THREAD_EIA_IN_4,
	C_TRACE_THREAD_EIA_IN_5,
	C_TRACE_THREAD_EIA_IN_6,
	C_TRACE_THREAD_EIA_OUT,		  /* Up to 7 EIA_OUT threads (1 for every ST, 1 for NAT) */
	C_TRACE_THREAD_EIA_OUT_1,
	C_TRACE_THREAD_EIA_OUT_2,
	C_TRACE_THREAD_EIA_OUT_3,
	C_TRACE_THREAD_EIA_OUT_4,
	C_TRACE_THREAD_EIA_OUT_5,
	C_TRACE_THREAD_EIA_OUT_6,
#   ifdef _ASP_PEA_CONF
	C_TRACE_THREAD_TS_AGG,
#   else
		/* DOMINO2 */
	C_TRACE_THREAD_ST_AGG,
#   endif
		 /* _ASP_PEA_CONF */
	C_TRACE_THREAD_NAT,
#   ifdef _ASP_PEA_CONF
	C_TRACE_THREAD_CCRA_ALLOC,
	C_TRACE_THREAD_CCRA_UL,
	C_TRACE_THREAD_CCRA_DL,
#   else
		/* DOMINO2 */
	C_TRACE_THREAD_NCC_ALLOC,
	C_TRACE_THREAD_NCC_UL,
	C_TRACE_THREAD_NCC_DL,
#   endif
		 /* _ASP_PEA_CONF */
	C_TRACE_THREAD_OBP,
	C_TRACE_THREAD_OBPC,

	C_TRACE_THREAD_SCHED_MAIN,
	C_TRACE_THREAD_SCHED_MASTER,
	C_TRACE_THREAD_SCHED_SLAVE,

	C_TRACE_THREAD_TESTER,		  /* Value 38 */

	C_TRACE_THREAD_MAX,

	C_TRACE_THREAD_UNKNOWN
} T_TRACE_THREAD_TYPE;

#   ifdef _ASP_TRACE_ERROR
#      define C_TRACE_THREADS_EIA_OUT  C_TRACE_THREAD_EIA_OUT
#      define C_TRACE_THREADS_EIA_IN  C_TRACE_THREAD_EIA_IN
#      define C_TRACE_THREADS_TG      C_TRACE_THREAD_TG
#      define C_TRACE_THREADS_ST      C_TRACE_THREAD_ST
#      define C_TRACE_THREADS_TS      C_TRACE_THREAD_TS
#   endif
		 /* _ASP_TRACE_ERROR */

  /* threads */
  /*---------*/
#   ifdef _ASP_PEA_CONF
#      define C_TRACE_TT_THREAD_TS              (TRACE_INIT_ULL << C_TRACE_THREAD_TS)
#      define C_TRACE_TT_THREAD_TS_1              (TRACE_INIT_ULL << C_TRACE_THREAD_TS_1)
#      define C_TRACE_TT_THREAD_TS_2              (TRACE_INIT_ULL << C_TRACE_THREAD_TS_2)
#      define C_TRACE_TT_THREAD_TS_3              (TRACE_INIT_ULL << C_TRACE_THREAD_TS_3)
#      define C_TRACE_TT_THREAD_TS_4              (TRACE_INIT_ULL << C_TRACE_THREAD_TS_4)
#      define C_TRACE_TT_THREAD_TS_5              (TRACE_INIT_ULL << C_TRACE_THREAD_TS_5)
#   else
		/* DOMINO2 */
#      define C_TRACE_TT_THREAD_ST              (TRACE_INIT_ULL << C_TRACE_THREAD_ST)
#      define C_TRACE_TT_THREAD_ST_1              (TRACE_INIT_ULL << C_TRACE_THREAD_ST_1)
#      define C_TRACE_TT_THREAD_ST_2              (TRACE_INIT_ULL << C_TRACE_THREAD_ST_2)
#      define C_TRACE_TT_THREAD_ST_3              (TRACE_INIT_ULL << C_TRACE_THREAD_ST_3)
#      define C_TRACE_TT_THREAD_ST_4              (TRACE_INIT_ULL << C_TRACE_THREAD_ST_4)
#      define C_TRACE_TT_THREAD_ST_5              (TRACE_INIT_ULL << C_TRACE_THREAD_ST_5)
#   endif
		 /* _ASP_PEA_CONF */
#   define C_TRACE_TT_THREAD_TG              (TRACE_INIT_ULL << C_TRACE_THREAD_TG)
#   define C_TRACE_TT_THREAD_TG_1              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_1)
#   define C_TRACE_TT_THREAD_TG_2              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_2)
#   define C_TRACE_TT_THREAD_TG_3              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_3)
#   define C_TRACE_TT_THREAD_TG_4              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_4)
#   define C_TRACE_TT_THREAD_TG_5              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_5)
#   define C_TRACE_TT_THREAD_TG_6              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_6)
#   define C_TRACE_TT_THREAD_TG_7              (TRACE_INIT_ULL << C_TRACE_THREAD_TG_7)
#   define C_TRACE_TT_THREAD_EIA_IN          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN)
#   define C_TRACE_TT_THREAD_EIA_IN_1          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN_1)
#   define C_TRACE_TT_THREAD_EIA_IN_2          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN_2)
#   define C_TRACE_TT_THREAD_EIA_IN_3          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN_3)
#   define C_TRACE_TT_THREAD_EIA_IN_4          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN_4)
#   define C_TRACE_TT_THREAD_EIA_IN_5          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN_5)
#   define C_TRACE_TT_THREAD_EIA_IN_6          (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_IN_6)
#   define C_TRACE_TT_THREAD_EIA_OUT         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT)
#   define C_TRACE_TT_THREAD_EIA_OUT_1         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT_1)
#   define C_TRACE_TT_THREAD_EIA_OUT_2         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT_2)
#   define C_TRACE_TT_THREAD_EIA_OUT_3         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT_3)
#   define C_TRACE_TT_THREAD_EIA_OUT_4         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT_4)
#   define C_TRACE_TT_THREAD_EIA_OUT_5         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT_5)
#   define C_TRACE_TT_THREAD_EIA_OUT_6         (TRACE_INIT_ULL << C_TRACE_THREAD_EIA_OUT_6)
#   ifdef _ASP_PEA_CONF
#      define C_TRACE_TT_THREAD_TS_AGG          (TRACE_INIT_ULL << C_TRACE_THREAD_TS_AGG)
#   else
		/* DOMINO2 */
#      define C_TRACE_TT_THREAD_ST_AGG          (TRACE_INIT_ULL << C_TRACE_THREAD_ST_AGG)
#   endif
		 /* _ASP_PEA_CONF */
#   define C_TRACE_TT_THREAD_NAT             (TRACE_INIT_ULL << C_TRACE_THREAD_NAT)
#   ifdef _ASP_PEA_CONF
#      define C_TRACE_TT_THREAD_CCRA_ALLOC       (TRACE_INIT_ULL << C_TRACE_THREAD_CCRA_ALLOC)
#      define C_TRACE_TT_THREAD_CCRA_UL          (TRACE_INIT_ULL << C_TRACE_THREAD_CCRA_UL)
#      define C_TRACE_TT_THREAD_CCRA_DL          (TRACE_INIT_ULL << C_TRACE_THREAD_CCRA_DL)
#   else
		/* DOMINO2 */
#      define C_TRACE_TT_THREAD_NCC_ALLOC       (TRACE_INIT_ULL << C_TRACE_THREAD_NCC_ALLOC)
#      define C_TRACE_TT_THREAD_NCC_UL          (TRACE_INIT_ULL << C_TRACE_THREAD_NCC_UL)
#      define C_TRACE_TT_THREAD_NCC_DL          (TRACE_INIT_ULL << C_TRACE_THREAD_NCC_DL)
#   endif
		 /* _ASP_PEA_CONF */
#   define C_TRACE_TT_THREAD_OBP             (TRACE_INIT_ULL << C_TRACE_THREAD_OBP)
#   define C_TRACE_TT_THREAD_OBPC            (TRACE_INIT_ULL << C_TRACE_THREAD_OBPC)
/* scheduler */
#   define C_TRACE_TT_THREAD_SCHED_MAIN      (TRACE_INIT_ULL << C_TRACE_THREAD_SCHED_MAIN)
#   define C_TRACE_TT_THREAD_SCHED_MASTER    (TRACE_INIT_ULL << C_TRACE_THREAD_SCHED_MASTER)
#   define C_TRACE_TT_THREAD_SCHED_SLAVE     (TRACE_INIT_ULL << C_TRACE_THREAD_SCHED_SLAVE)
/* Tester */
#   define C_TRACE_TT_THREAD_TESTER              (TRACE_INIT_ULL << C_TRACE_THREAD_TESTER)


#   ifdef _ASP_PEA_CONF
  /* components */
#      define C_TRACE_TT_COMP_TS                (TRACE_INIT_ULL << C_TRACE_THREAD_MAX)
#      define C_TRACE_TT_COMP_TG                (C_TRACE_TT_COMP_TS << C_TRACE_COMP_TG)
#      define C_TRACE_TT_COMP_EIA_IN            (C_TRACE_TT_COMP_TS << C_TRACE_COMP_EIA_IN)
#      define C_TRACE_TT_COMP_EIA_OUT           (C_TRACE_TT_COMP_TS << C_TRACE_COMP_EIA_OUT)
#      define C_TRACE_TT_COMP_TS_AGG            (C_TRACE_TT_COMP_TS << C_TRACE_COMP_TS_AGG)
#      define C_TRACE_TT_COMP_NAT               (C_TRACE_TT_COMP_TS << C_TRACE_COMP_NAT)
#      define C_TRACE_TT_COMP_CCRA              (C_TRACE_TT_COMP_TS << C_TRACE_COMP_CCRA)
#      define C_TRACE_TT_COMP_OBP               (C_TRACE_TT_COMP_TS << C_TRACE_COMP_OBP)
#      define C_TRACE_TT_COMP_OBPC              (C_TRACE_TT_COMP_TS << C_TRACE_COMP_OBPC)
#      define C_TRACE_TT_COMP_AIE_IN            (C_TRACE_TT_COMP_TS << C_TRACE_COMP_AIE_IN)
#      define C_TRACE_TT_COMP_AIE_OUT           (C_TRACE_TT_COMP_TS << C_TRACE_COMP_AIE_OUT)
  /* common */
#      define C_TRACE_TT_COMP_CONFIG            (C_TRACE_TT_COMP_TS << C_TRACE_COMP_CONFIG)
#      define C_TRACE_TT_COMP_INTERFACES        (C_TRACE_TT_COMP_TS << C_TRACE_COMP_INTERFACES)
#      define C_TRACE_TT_COMP_SHARED_MEMORY     (C_TRACE_TT_COMP_TS << C_TRACE_COMP_SHARED_MEMORY)
#      define C_TRACE_TT_COMP_TRANSPORT         (C_TRACE_TT_COMP_TS << C_TRACE_COMP_TRANSPORT)
#      define C_TRACE_TT_COMP_UTILITIES         (C_TRACE_TT_COMP_TS << C_TRACE_COMP_UTILITIES)
#      define C_TRACE_TT_COMP_SCHEDULER         (C_TRACE_TT_COMP_TS << C_TRACE_COMP_SCHEDULER)
#      define C_TRACE_TT_COMP_PROBE             (C_TRACE_TT_COMP_TS << C_TRACE_COMP_PROBE)
#      define C_TRACE_TT_COMP_ERROR             (C_TRACE_TT_COMP_TS << C_TRACE_COMP_ERROR)
#      define C_TRACE_TT_COMP_EVENT             (C_TRACE_TT_COMP_TS << C_TRACE_COMP_EVENT)
#      define C_TRACE_TT_COMP_PROTOCOL          (C_TRACE_TT_COMP_TS << C_TRACE_COMP_PROTOCOL)
#      define C_TRACE_TT_COMP_TESTER            (C_TRACE_TT_COMP_TS << C_TRACE_COMP_TESTER)
#      define C_TRACE_TT_COMP_OBPCTESTER        (C_TRACE_TT_COMP_TS << C_TRACE_COMP_OBPCTESTER)

#   else
		/* DOMINO2 */
  /* components */
#      define C_TRACE_TT_COMP_ST                (TRACE_INIT_ULL << C_TRACE_THREAD_MAX)
#      define C_TRACE_TT_COMP_TG                (C_TRACE_TT_COMP_ST << C_TRACE_COMP_TG)
#      define C_TRACE_TT_COMP_EIA_IN            (C_TRACE_TT_COMP_ST << C_TRACE_COMP_EIA_IN)
#      define C_TRACE_TT_COMP_EIA_OUT           (C_TRACE_TT_COMP_ST << C_TRACE_COMP_EIA_OUT)
#      define C_TRACE_TT_COMP_ST_AGG            (C_TRACE_TT_COMP_ST << C_TRACE_COMP_ST_AGG)
#      define C_TRACE_TT_COMP_NAT               (C_TRACE_TT_COMP_ST << C_TRACE_COMP_NAT)
#      define C_TRACE_TT_COMP_NCC               (C_TRACE_TT_COMP_ST << C_TRACE_COMP_NCC)
#      define C_TRACE_TT_COMP_OBP               (C_TRACE_TT_COMP_ST << C_TRACE_COMP_OBP)
#      define C_TRACE_TT_COMP_OBPC              (C_TRACE_TT_COMP_ST << C_TRACE_COMP_OBPC)
#      define C_TRACE_TT_COMP_AIE_IN            (C_TRACE_TT_COMP_ST << C_TRACE_COMP_AIE_IN)
#      define C_TRACE_TT_COMP_AIE_OUT           (C_TRACE_TT_COMP_ST << C_TRACE_COMP_AIE_OUT)
  /* common */
#      define C_TRACE_TT_COMP_CONFIG            (C_TRACE_TT_COMP_ST << C_TRACE_COMP_CONFIG)
#      define C_TRACE_TT_COMP_INTERFACES        (C_TRACE_TT_COMP_ST << C_TRACE_COMP_INTERFACES)
#      define C_TRACE_TT_COMP_SHARED_MEMORY     (C_TRACE_TT_COMP_ST << C_TRACE_COMP_SHARED_MEMORY)
#      define C_TRACE_TT_COMP_TRANSPORT         (C_TRACE_TT_COMP_ST << C_TRACE_COMP_TRANSPORT)
#      define C_TRACE_TT_COMP_UTILITIES         (C_TRACE_TT_COMP_ST << C_TRACE_COMP_UTILITIES)
#      define C_TRACE_TT_COMP_SCHEDULER         (C_TRACE_TT_COMP_ST << C_TRACE_COMP_SCHEDULER)
#      define C_TRACE_TT_COMP_PROBE             (C_TRACE_TT_COMP_ST << C_TRACE_COMP_PROBE)
#      define C_TRACE_TT_COMP_ERROR             (C_TRACE_TT_COMP_ST << C_TRACE_COMP_ERROR)
#      define C_TRACE_TT_COMP_EVENT             (C_TRACE_TT_COMP_ST << C_TRACE_COMP_EVENT)
#      define C_TRACE_TT_COMP_PROTOCOL          (C_TRACE_TT_COMP_ST << C_TRACE_COMP_PROTOCOL)
#      define C_TRACE_TT_COMP_TESTER            (C_TRACE_TT_COMP_ST << C_TRACE_COMP_TESTER)
#      define C_TRACE_TT_COMP_OBPCTESTER        (C_TRACE_TT_COMP_ST << C_TRACE_COMP_OBPCTESTER)
#   endif
		 /* _ASP_PEA_CONF */

#   ifndef _ASP_PEA_CONF
#      define TRACE_THREAD_TYPE(ComponentType,Instance) \
  (ComponentType==C_COMP_NAT    ? (T_TRACE_THREAD_TYPE)C_TRACE_THREAD_NAT : \
  ComponentType==C_COMP_ST     ? (T_TRACE_THREAD_TYPE)((T_INT32)C_TRACE_THREAD_ST+Instance) : \
  ComponentType==C_COMP_ST_AGG ? (T_TRACE_THREAD_TYPE)C_TRACE_THREAD_ST_AGG : \
  (T_TRACE_THREAD_TYPE)C_TRACE_THREAD_UNKNOWN)

#      define TRACE_COMPONENT_TYPE(ComponentType) \
  (ComponentType==C_COMP_NAT    ? C_TRACE_COMP_NAT : \
  ComponentType==C_COMP_ST     ? C_TRACE_COMP_ST : \
  ComponentType==C_COMP_ST_AGG ? C_TRACE_COMP_ST_AGG : \
  C_TRACE_COMP_UNKNOWN)

#   else

#      define TRACE_THREAD_TYPE(ComponentType,Instance) \
  (ComponentType==C_COMP_TS    ? (T_TRACE_THREAD_TYPE)((T_INT32)C_TRACE_THREAD_TS+Instance) : \
  ComponentType==C_COMP_TS_AGG ? (T_TRACE_THREAD_TYPE)C_TRACE_THREAD_TS_AGG : \
  (T_TRACE_THREAD_TYPE)C_TRACE_THREAD_UNKNOWN)

#      define TRACE_COMPONENT_TYPE(ComponentType) \
  (ComponentType==C_COMP_TS     ? C_TRACE_COMP_TS : \
  ComponentType==C_COMP_TS_AGG ? C_TRACE_COMP_TS_AGG : \
  C_TRACE_COMP_UNKNOWN)
#   endif
		 /* _ASP_PEA_CONF */

#   ifdef _ASP_TRACE

/*  @ROLE    : This macro is used to print trace message */
#      define TRACE_LOG(param) TRACE_Printf param;
#      define TRACE_ERROR(param) TRACE_Printf param;
#      define TRACE_LOG_FORCE(param) TRACE_Printf param;

/*  @ROLE    : This macro is used to print trace message to a stream */
#      define TRACE_LOG_STREAM(param) TRACE_Fprintf param;

/*  @ROLE    : This macro is used to print the "function-enter" message */
#      define TRACE_ENTER(traceThread,traceComponent,funcName) \
  TRACE_Printf(traceThread,traceComponent,C_TRACE_FUNC,\
               "ENTER @ file(%s) func("funcName") line(%d)",\
               __FILE__,__LINE__);

/*  @ROLE    : This macro is used to print the "function-exit" message */
#      define TRACE_EXIT(traceThread,traceComponent,funcName,exitCode) \
  TRACE_Printf(traceThread,traceComponent,C_TRACE_FUNC,\
               "EXIT  @ file(%s) func("funcName") line(%d) exit(%d)",\
               __FILE__,__LINE__,exitCode);

/*  @ROLE    : This macro is used to print sytem error message */
#         define TRACE_SYSERROR(traceThread,traceComponent,funcName) \
  TRACE_Printf(traceThread,traceComponent,C_TRACE_ERROR,\
               "SYSERROR @ file(%s) line(%d) func("funcName"): (%d)%s",\
               __FILE__,__LINE__,errno,strerror(errno));

/*  @ROLE    : This function activates some trace display*/
#      define TRACE_ACTIVATE(traceType,traceLevel) TRACE_ActivateTrace(traceType,traceLevel);

/*  @ROLE    : This function activates all trace display*/
#      define TRACE_ACTIVATE_ALL(traceLevel) TRACE_ActivateAllTrace(traceLevel);

/*  @ROLE    : This function disactivates some trace display*/
#      define TRACE_DISACTIVATE(traceType) TRACE_DisactivateTrace(traceType);

/*  @ROLE    : This function disactivates all trace display*/
#      define TRACE_DISACTIVATE_ALL() TRACE_DisactivateAllTrace();

#   else
		/* _ASP_TRACE */
#      define TRACE_LOG(param)
#      ifdef _ASP_TRACE_ERROR
#         define TRACE_ERROR(param) TRACE_ForcePrintf param;
#         define TRACE_LOG_FORCE(param) TRACE_ForcePrintf param;
#         define TRACE_SYSERROR(traceThread,traceComponent,funcName) \
  TRACE_ForcePrintf(traceThread,traceComponent,C_TRACE_ERROR,\
                    "SYSERROR @ file(%s) line(%d) func("funcName"): (%d)%s",\
                    __FILE__,__LINE__,errno,strerror(errno));
#      else
#         define TRACE_ERROR(param)
#         define TRACE_LOG_FORCE(param)
#         define TRACE_SYSERROR(traceThread,traceComponent,funcName)
#      endif
		 /* _ASP_TRACE_ERROR */
#      define TRACE_LOG_STREAM(param)
#      define TRACE_ENTER(traceThread,traceComponent,funcName)
#      define TRACE_EXIT(traceThread,traceComponent,funcName,exitCode)
#      define TRACE_ACTIVATE(traceType,traceLevel)
#      define TRACE_ACTIVATE_ALL(traceLevel)
#      define TRACE_DISACTIVATE(traceType)
#      define TRACE_DISACTIVATE_ALL()
#   endif
		 /* _ASP_TRACE */


#   if defined(_ASP_TRACE) || defined(_ASP_TESTER)
/*----------------------------*/
/* MACRO TO TRACE PACKET DATA */
/*----------------------------*/
#      define C_TRACE_MAX_INDEX (C_TRACE_THREAD_MAX+C_TRACE_COMP_MAX)
extern T_BOOL _trace_activationFlag[C_TRACE_MAX_INDEX];
extern T_UINT64 _trace_levelFlag[C_TRACE_MAX_INDEX];

#      define TRACE_LOG_PACKET_DECLARATION() \
  va_list args; \
  T_CHAR globalMsg[C_TRACE_PACKET_STR_MAX_SIZE];

#      define TRACE_LOG_PACKET_TITLE(title,format,args) \
{ \
   T_CHAR tempString[256]; \
    sprintf(&globalMsg[0],"[%s] ",title); \
    va_start(args,format); \
    vsprintf(&tempString[0],format,args); \
    strcat(&globalMsg[0],&tempString[0]); \
}

#      define TRACE_LOG_PACKET_COMMENT(comment) \
{ \
   T_CHAR tempString[256]; \
   strcat(&globalMsg[0],"\n\t"); \
   sprintf(&tempString[0],comment); \
   strcat(&globalMsg[0],&tempString[0]); \
}

#      define TRACE_LOG_PACKET_BEGIN_FIELDS(text,title) \
{ \
    sprintf(&text[0],"\n\t%s: ",title); \
}

#      define TRACE_LOG_PACKET_ADD_FIELDS(text,title,value) \
{ \
   T_CHAR tempString[256]; \
   sprintf(&tempString[0],"%s[0x%llX] ",title,(T_UINT64)(value)); \
   strcat(&text[0],&tempString[0]); \
}

#      define TRACE_LOG_PACKET_ADD_FIELDS_NOT_USED(text,title) \
{ \
   T_CHAR tempString[256]; \
   sprintf(&tempString[0],"%s[not used] ",title); \
   strcat(&text[0],&tempString[0]); \
}

#      define TRACE_LOG_PACKET_ADD_FIELDS_CAST(text,title,ptr_value,type) \
{ \
   T_CHAR tempString[256]; \
   sprintf(&tempString[0],"%s[0x%llX] ",title,(T_UINT64)(*(type*)(ptr_value))); \
   strcat(&text[0],&tempString[0]); \
}

#      define TRACE_LOG_PACKET_ADD_FIELDS_NB(text,title,ptr_value,nbData) \
{ \
   T_CHAR tempString[256]; \
   T_UINT32 traceFieldNb; \
   sprintf(&tempString[0],"%s[0x",title); \
   strcat(&text[0],&tempString[0]); \
   for(traceFieldNb=0;traceFieldNb<nbData;traceFieldNb++) { \
     sprintf(&tempString[0],"%02X",*((T_UINT8*)ptr_value+traceFieldNb)); \
     strcat(&text[0],&tempString[0]); \
   } \
   sprintf(&tempString[0],"] "); \
   strcat(&text[0],&tempString[0]); \
}

#      define TRACE_LOG_PACKET_END_FIELDS(text) \
{ \
   strcat(&globalMsg[0],&text[0]); \
}

#      define TRACE_LOG_PACKET_DATA(title,ptr_data,size) \
{ \
   T_CHAR   tempString[256]; \
   T_UINT8  *dataPtr; \
   T_UINT32 cmpt; \
   T_UINT32 printedSize = size; \
   \
   if(size > C_TRACE_PACKET_DATA_MAX_SIZE) \
    printedSize = C_TRACE_PACKET_DATA_MAX_SIZE;\
   sprintf(&tempString[0],"\n\t%s:",title); \
   strcat(&globalMsg[0],&tempString[0]); \
   \
   dataPtr = (T_UINT8*)(ptr_data); \
   for(cmpt=0;cmpt<printedSize;cmpt++) { \
     if((cmpt%30) == 0) { \
       sprintf(&tempString[0],"\n\t[%04lu] ",cmpt); \
       strcat(&globalMsg[0],&tempString[0]); \
     } \
     sprintf(&tempString[0],"%02X ",*dataPtr); \
     strcat(&globalMsg[0],&tempString[0]); \
     dataPtr = dataPtr + 1; \
   } \
   if(size > C_TRACE_PACKET_DATA_MAX_SIZE) {\
     sprintf(&tempString[0],"\n\t PAYLOAD TOO LONG"); \
     strcat(&globalMsg[0],&tempString[0]); \
   }\
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* DO NOT USE THE FUNCTION CALL */
/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
extern void TRACE_Printf(
									/* IN    */ T_TRACE_THREAD_TYPE traceThread,
									/* IN    */ T_TRACE_COMPONENT_TYPE traceComponent,
									/* IN    */ T_TRACE_LEVEL traceLevel,
									/* IN    */ T_STRING format, ...);
extern void TRACE_Fprintf(
									 /* IN    */ T_TRACE_THREAD_TYPE traceThread,
									 /* IN    */ T_TRACE_COMPONENT_TYPE traceComponent,
									 /* IN    */ T_TRACE_LEVEL traceLevel,
									 /* IN    */ FILE * stream,
									 /* IN    */ T_CHAR * format, ...);
extern void TRACE_ActivateTrace(
											 /* IN    */ T_UINT64 traceType,
											 /* IN    */ T_UINT64 traceLevel);
extern void TRACE_ActivateAllTrace(
												 /* IN    */ T_UINT64 traceLevel);
extern void TRACE_DisactivateTrace(
												 /* IN    */ T_UINT64 traceType);
extern void TRACE_DisactivateAllTrace(void);
#   else
/*----------------------------*/
/* MACRO TO TRACE PACKET DATA */
/*----------------------------*/
#      define TRACE_LOG_PACKET_DECLARATION()
#      define TRACE_LOG_PACKET_TITLE(title,format,args)
#      define TRACE_LOG_PACKET_COMMENT(comment)
#      define TRACE_LOG_PACKET_BEGIN_FIELDS(text,title)
#      define TRACE_LOG_PACKET_ADD_FIELDS(text,title,value)
#      define TRACE_LOG_PACKET_ADD_FIELDS_NOT_USED(text,title)
#      define TRACE_LOG_PACKET_ADD_FIELDS_CAST(text,title,ptr_value,type)
#      define TRACE_LOG_PACKET_ADD_FIELDS_NB(text,title,ptr_value,nb)
#      define TRACE_LOG_PACKET_END_FIELDS(text)
#      define TRACE_LOG_PACKET_DATA(title,ptr_data,size)

#   endif


/*  @ROLE    : This function is used to print trace error in force mode
    @RETURN  : None */
extern void TRACE_ForcePrintf(
										  /* IN    */ T_TRACE_THREAD_TYPE traceThread,
										  /* IN    */
										  T_TRACE_COMPONENT_TYPE traceComponent,
										  /* IN    */ T_TRACE_LEVEL traceLevel,
										  /* IN    */ T_CHAR * format, ...);


#endif /* Trace_e */
