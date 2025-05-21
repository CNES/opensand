/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file SlottedAloha.cpp
 * @brief The Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
*/

/**
 * @class SlottedAloha
 * @brief The Slotted Aloha
*/

#include "SlottedAloha.h"

#include "TerminalCategorySaloha.h"
#include "OpenSandModelConf.h"


SlottedAloha::SlottedAloha():
	sf_per_saframe(),
	frame_duration(0),
	is_parent_init(false),
	pkt_hdl(nullptr)
{
	this->log_saloha = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "Dvb.init");
}


bool SlottedAloha::initParent(time_us_t frame_duration,
                              std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl)
{
	srand(time(nullptr));
	this->frame_duration = frame_duration;
	this->pkt_hdl = pkt_hdl;

	if(!OpenSandModelConf::Get()->getSuperframePerSlottedAlohaFrame(this->sf_per_saframe))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "missing slotted aloha frame duration parameter\n");
		return false;
	}
	this->is_parent_init = true;

	return true;
}

SlottedAloha::~SlottedAloha()
{
}

bool SlottedAloha::isSalohaFrameTick(time_sf_t superframe_counter)
{
	if(!(superframe_counter % this->sf_per_saframe))
	{
		LOG(this->log_saloha, LEVEL_DEBUG,
		    "Slotted Aloha tick: %u", superframe_counter);
	}
	return !(superframe_counter % this->sf_per_saframe);
}

