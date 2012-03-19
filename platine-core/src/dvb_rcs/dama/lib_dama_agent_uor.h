/*
 *
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
 * @file lib_dama_agent_uor.h
 * @brief This is the Legacy algorithm.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 *
 *  Is the same of lib_dama_agent_legacy.h, only renamed
 *
 */

#ifndef LIB_DAMA_AGENT_UOR_H
#define LIB_DAMA_AGENT_UOR_H

#include "lib_dama_agent_legacy.h"


/**
 * @class DvbRcsDamaAgentUoR
 * @brief This is the Legacy DAMA agent
 */
class DvbRcsDamaAgentUoR: public DvbRcsDamaAgentLegacy
{

 public:

	DvbRcsDamaAgentUoR(EncapPlugin::EncapPacketHandler *packet,
	                   double frame_duration);

};

#endif
