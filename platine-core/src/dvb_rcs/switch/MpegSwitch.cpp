/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file MpegSwitch.cpp
 * @brief MPEG switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include "MpegSwitch.h"


MpegSwitch::MpegSwitch()
{
}

MpegSwitch::~MpegSwitch()
{
}

long MpegSwitch::find(NetPacket *packet)
{
	MpegPacket *mpeg_packet;
	std::map < long, long >::iterator it;
	long spot_id = 0;

	if(packet == NULL)
		goto error;

	if(packet->type() != NET_PROTO_MPEG)
		goto error;

	mpeg_packet = (MpegPacket *) packet;

	it = this->switch_table.find(mpeg_packet->talId());

	if(it != this->switch_table.end())
		spot_id = (*it).second;

error:
	return spot_id;
}
