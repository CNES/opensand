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
 * @file DominoConstants_e.h
 * @author TAS
 * @brief The DominoConstants defines Domino system constants
 */

#ifndef DominoConstants_e
#   define DominoConstants_e

/* Category types */
typedef enum
{
	C_CAT_INIT = 0,
	C_CAT_END = 1,
	C_CAT_NB
} T_CATEGORY_TYPE;

/* Probe Aggregation modes */
typedef enum
{
	C_AGG_MIN = 0,
	C_AGG_MAX,
	C_AGG_MEAN,
	C_AGG_LAST,
	C_AGG_NB
} T_PROB_AGG;

/* Probe Analysis operator */
typedef enum
{
	C_ANA_RAW = 0,
	C_ANA_MIN,
	C_ANA_MAX,
	C_ANA_MEAN,
	C_ANA_STANDARD_DEV,
	C_ANA_SLIDING_MIN,
	C_ANA_SLIDING_MAX,
	C_ANA_SLIDING_MEAN,
	C_ANA_NB
} T_PROB_ANA;


/*
 * Macros for fast min/max.
 */
#   ifndef MIN
#      define MIN(a,b) (((a)<(b))?(a):(b))
#   endif
		 /* MIN */

#   ifndef MAX
#      define MAX(a,b) (((a)>(b))?(a):(b))
#   endif
		 /* MAX */


/* Component types */
typedef enum
{
	C_COMP_GW = 0,
	C_COMP_SAT = 1,
	C_COMP_ST = 2,
	C_COMP_ST_AGG = 3,
	C_COMP_OBPC = 4,
	C_COMP_TG = 5,
	C_COMP_PROBE_CTRL = 6,
	C_COMP_EVENT_CTRL = 7,
	C_COMP_ERROR_CTRL = 8,
	C_COMP_MAX = 9						  /* number of values in enum type */
} T_COMPONENT_TYPE;


#endif /* DominoConstants_e */
