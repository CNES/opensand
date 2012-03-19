/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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

/**
 * @file lib_dama_agent_uor.cpp
 * @brief This library defines UoR DAMA agent
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 *
 * UoR DAMA agent is still now the same of Legacy DAMA agent.
 */

#include <string>
#include <math.h>
#include <stdlib.h>

#include "platine_margouilla/mgl_time.h"
#include "lib_dvb_rcs.h"
#include "lib_dama_agent_uor.h"
#include "lib_dama_utils.h"

#define DBG_PACKAGE PKG_DAMA_DA
#define DA_DBG_PREFIX "[UOR]"
#include "platine_conf/uti_debug.h"


/**
 * Constructor
 */
DvbRcsDamaAgentUoR::DvbRcsDamaAgentUoR(EncapPlugin::EncapPacketHandler *packet,
                                       double frame_duration):
	DvbRcsDamaAgentLegacy(packet, frame_duration)
{
	// DAMA Legacy initialises everything, nothing more to do
}
