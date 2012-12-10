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
 * @file Controller_e.h
 * @author TAS
 * @brief The ExecContext class implements the execution context of template
 */

#ifndef Controller_e
#   define Controller_e

#   include "EnumCouple_e.h"
#   include "DominoConstants_e.h"

/* define correspondance between component names and integer */
/* values used in methods implementation                     */
#   define COMPONENT_CHOICES(name) \
T_ENUM_COUPLE name[C_COMP_MAX+1] \
  = {{"GW", C_COMP_GW}, \
    {"SAT", C_COMP_SAT}, \
    {"ST", C_COMP_ST}, \
    {"AGGREGATE_ST", C_COMP_ST_AGG}, \
    {"OBPC", C_COMP_OBPC}, \
    {"TRAFFIC", C_COMP_TG}, \
    {"PROBE_CONTROLLER", C_COMP_PROBE_CTRL}, \
    {"EVENT_CONTROLLER", C_COMP_EVENT_CTRL}, \
    {"ERROR_CONTROLLER", C_COMP_ERROR_CTRL}, \
    C_ENUM_COUPLE_NULL};


#endif
