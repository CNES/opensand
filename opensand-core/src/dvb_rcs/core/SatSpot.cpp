/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file SatSpot.cpp
 * @brief This bloc implements a satellite spots
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "SatSpot.h"
#include "OpenSandFrames.h"
#include "MacFifoElement.h"
#include <opensand_output/Output.h>

#include <stdlib.h>

// TODO add spot fifos in configuration
// TODO size per fifo ?
// TODO to not do...: do not create all fifo in the regenerative case
SatSpot::SatSpot(spot_id_t spot_id):
	spot_id(spot_id),
	sat_gws()
{
	// Output Log
	this->log_init = Output::registerLog(LEVEL_WARNING, "Dvb.init");
}

SatSpot::~SatSpot()
{
	this->sat_gws.clear();
}

void SatSpot::addGw(SatGw* gw)
{
	this->sat_gws.push_back(gw);
}

uint8_t SatSpot::getSpotId(void) const
{
	return this->spot_id;
}

list<SatGw *> SatSpot::getGwList(void) const
{
	return this->sat_gws;
}

SatGw* SatSpot::getGw(tal_id_t gw_id)
{
	list<SatGw *>::iterator iter;

	for(iter = this->sat_gws.begin() ; iter != this->sat_gws.end() ;
	    ++iter)
	{
		SatGw *gw = *iter;
		if(gw->getGwId() == gw_id)
		{
			return gw;
		}
	}
	return NULL;
}
