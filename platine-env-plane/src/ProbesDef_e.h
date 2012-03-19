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
 * @file ProbesDef_e.h
 * @author TAS
 * @brief The ProbesDef class implements the reading of statistics
 *        definition configuration file
 */

#ifndef ProbesDef_e
#   define ProbesDef_e

/********************/
/* SYSTEM RESOURCES */
/********************/
#   include "Error_e.h"
#   include "EnumCouple_e.h"

/*********************/
/* MACRO DEFINITIONS */
/*********************/

/* These limits shall be reconsidered at integration-time  */
/*---------------------------------------------------------*/
#   define C_PROB_DEF_MAX_CAR_NAME         48
#   define C_PROB_DEF_MAX_CAR_UNIT         32
#   define C_PROB_DEF_MAX_CAR_GRAPH_TYPE   16
#   define C_PROB_DEF_MAX_CAR_COMMENT      48
#   define C_PROB_DEF_MAX_CAR_LABEL        32
#   define C_MAX_PROBE_VALUE_NUMBER      1024/* Max number of probe value per socket */
#   define C_PROB_MAX_STAT_NUMBER          50
#   define C_PROB_MAX_LABEL_VALUE         864/* This value is computed by : 
															   3 UL beams * 8 channel types * 3 DL beams * 3 QoS * 4 types of throughput */

typedef T_CHAR T_STAT_LABEL[C_PROB_DEF_MAX_CAR_LABEL];


/*******************/
/* ENUM DEFINITION */
/*******************/

/* Define enum with all types available for statistics */
/*-----------------------------------------------------*/
enum
{
	C_PROBE_TYPE_INT = 0,
	C_PROBE_TYPE_FLOAT = 1,

	C_PROBE_TYPE_NB
};


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/

typedef struct
{										  /* LEVEL 2 */
	T_UINT32 _nbLabels;
	T_STAT_LABEL _StatLabelValue[C_PROB_MAX_LABEL_VALUE];
} T_STAT_LABEL_TAB;


typedef struct
{										  /* LEVEL 1 */
	T_INT32 _probeId;
	T_CHAR _Name[C_PROB_DEF_MAX_CAR_NAME];
	T_INT32 _Category;
	T_INT32 _Type;
	T_CHAR _Unit[C_PROB_DEF_MAX_CAR_UNIT];
	T_CHAR _Graph_Type[C_PROB_DEF_MAX_CAR_GRAPH_TYPE];
	T_CHAR _Comment[C_PROB_DEF_MAX_CAR_COMMENT];
	T_STAT_LABEL_TAB _StatLabels;

} T_PROBE_DEF;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _nbStatistics;
	T_PROBE_DEF _Statistic[C_PROB_MAX_STAT_NUMBER];
	T_ENUM_COUPLE C_PROBES_DEFINITION_ComponentChoices[C_COMP_MAX + 1];
	T_ENUM_COUPLE C_PROBE_TYPE_choices[C_PROBE_TYPE_NB + 1];
} T_PROBES_DEF;


T_ERROR PROBES_DEF_ReadConfigFile(
												/* INOUT */ T_PROBES_DEF * ptr_this,
												/* IN    */
												T_COMPONENT_TYPE ComponentLabel);


#endif /* ProbesDef_e */
