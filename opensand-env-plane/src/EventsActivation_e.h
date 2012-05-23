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
 * @file EventsActivation_e.h
 * @author TAS
 * @brief The EventsActivation class implements the reading of
 *        events activation configuration file
 */

#ifndef EventsActivation_e
#   define EventsActivation_e

#   include "Error_e.h"

/* All these limits shall be reconsidered at integration-time  */

/********************/
/* MACRO DEFINITION */
/********************/
#   define C_EVT_CATEGORY_MAX_NB  7

/************************/
/* STRUCTURE DEFINITION */
/***********************/
typedef struct
{
	T_UINT32 _nbCategory;
	T_INT32 _EventCategory[C_EVT_CATEGORY_MAX_NB];
	T_UINT32 _DoNotUsed;
} T_EVENTS_ACTIVATION;


T_ERROR EVENTS_ACTIVATION_ReadConfigFile(
														 /* INOUT */ T_EVENTS_ACTIVATION *
														 ptr_this);

T_ERROR EVENTS_ACTIVATION_PrintConfigFile(
														  /* INOUT */ T_EVENTS_ACTIVATION *
														  ptr_this);


#endif /* EventsActivation_e */
