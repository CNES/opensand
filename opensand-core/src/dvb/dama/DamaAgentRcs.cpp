/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file    DamaAgentRcs.cpp
 * @brief   Implementation of the DAMA agent for DVB-RCS emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 */


#include "DamaAgentRcs.h"
#include "UnitConverterFixedBitLength.h"
#include "ModulationTypes.h"

#include <opensand_output/Output.h>


DamaAgentRcs::DamaAgentRcs(FmtDefinitionTable *ret_modcod_def):
	DamaAgentRcsCommon(ret_modcod_def)
{
}

DamaAgentRcs::~DamaAgentRcs()
{
}

UnitConverter *DamaAgentRcs::generateUnitConverter() const
{
	LOG(this->log_init, LEVEL_DEBUG,
	    "Packet length: %u bytes (%u bits)\n",
	    this->packet_handler->getFixedLength(), this->packet_handler->getFixedLength() << 3);
	return new UnitConverterFixedBitLength(this->frame_duration_ms,
		0,
		this->packet_handler->getFixedLength() << 3);
}

ReturnSchedulingRcsCommon *DamaAgentRcs::generateReturnScheduling() const
{
	return new ReturnSchedulingRcs(this->packet_handler, this->dvb_fifos);
}
