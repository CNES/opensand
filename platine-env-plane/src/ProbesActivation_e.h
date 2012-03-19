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
 * @file ProbesActivation_e.h
 * @author TAS
 * @brief The ProbesActivation class implements the reading of
 *        statistics activation configuration file
 */

#ifndef ProbesActivation_e
#   define ProbesActivation_e

#   include "EnumCouple_e.h"
#   include "Error_e.h"
#   include "DominoConstants_e.h"
#   include "ProbesDef_e.h"


/********************/
/*     CONSTANTS    */
/********************/
#   define C_MAX_ACTIVATED_PROBE     32/* Max number of active statistics probed */

/********************/
/* ENUM DEFINITIONS */
/********************/


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/
typedef struct
{										  /* LEVEL 2 */
	T_PROBE_DEF _Statistic;
	T_PROB_AGG _AggregationMode;
	T_INT32 _DisplayFlag;
	T_PROB_ANA _AnalysisOperator;
	T_INT32 _OperatorParameter;
} T_ACTIVATED_PROBE;


typedef struct
{										  /* LEVEL 1 */
	T_UINT32 _nbActivatedProbes;
	T_ACTIVATED_PROBE _Probe[C_MAX_ACTIVATED_PROBE];

	T_ENUM_COUPLE C_PROB_AGGREGATE_choices[C_AGG_NB + 1];
	T_ENUM_COUPLE C_PROB_ANALYSIS_choices[C_ANA_NB + 1];
} T_ACTIVATED_PROBE_TAB;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _StartFrame;
	T_UINT32 _StopFrame;
	T_UINT32 _SamplingPeriod;
	T_ACTIVATED_PROBE_TAB _ActivatedProbes;

	T_ENUM_COUPLE C_PROBES_ACTIVATION_ComponentChoices[C_COMP_MAX + 1];
} T_PROBES_ACTIVATION;


T_ERROR PROBES_ACTIVATION_ReadConfigFile(
														 /* INOUT */ T_PROBES_ACTIVATION *
														 ptr_this,
														 /* IN    */
														 T_COMPONENT_TYPE ComponentLabel);


T_ERROR PROBES_ACTIVATION_UpdateDefinition(
															/* INOUT */ T_PROBES_ACTIVATION *
															ptr_this,
															/* IN    */
															T_PROBES_DEF * ptr_probesDef);

#endif /* ProbesActivation_e */
