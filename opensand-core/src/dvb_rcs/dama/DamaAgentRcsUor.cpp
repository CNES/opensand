/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
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
 * @file    DamaAgentRcsUor.cpp
 * @brief   Dama Agent for UoR algorithm.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 *
 */

#include "DamaAgentRcsUor.h"

#define DBG_PACKAGE PKG_DAMA_DA
#include "opensand_conf/uti_debug.h"
#define DA_DBG_PREFIX "[UOR]"

DamaAgentRcsUor::DamaAgentRcsUor(const EncapPlugin::EncapPacketHandler *pkt_hdl,
                                 const std::map<unsigned int, DvbFifo *> &dvb_fifos)
	: DamaAgentRcsLegacy(pkt_hdl, dvb_fifos)
{
}
