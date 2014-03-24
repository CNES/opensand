/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file Gate.cpp
 * @brief Class that implements Error Insertion, Attribute class of 
 *        Channel that manages how bit errors affect the frames.
 *        Process the packets with an ON/OFF perspective.
 *        If the Carrier to Noise ratio is below a certain threshold,
 *        the whole packet will corrupted in all its bits.
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */



#include "Gate.h"

#include <opensand_output/Output.h>

#include <strings.h>
#include <cstring>

Gate::Gate():ErrorInsertionPlugin()
{
}


Gate::~Gate()
{
}

bool Gate::init()
{
	return true;
}

bool Gate::isToBeModifiedPacket(double cn_total,
                                double threshold_qef)
{
	bool do_modify;

	// Comparison between current and required C/N values
	if(cn_total >= threshold_qef)
	{
		Output::sendLog(LEVEL_DEBUG, "Packet should not be modified \n");
		do_modify = false;
	}
	else
	{
		Output::sendLog(LEVEL_DEBUG, "Packet should be modified\n");
		do_modify = true;
	}
	return do_modify;
}

bool Gate::modifyPacket(const Data &UNUSED(payload))
{
	Output::sendLog(LEVEL_DEBUG, "Payload is modified\n");
	// not needed, we will reject frame in DVB layer as we return true
	// memset(payload, '\0', length);
	return true;
}

