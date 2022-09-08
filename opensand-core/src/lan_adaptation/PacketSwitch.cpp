/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 CNES
 * Copyright © 2020 TAS
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
 * @file PacketSwitch.cpp
 * @brief Get switch information about packets
 *
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Francklin SIMO <fsimo@toulouse.viveris.com>
 */

#include "PacketSwitch.h"
#include "Data.h"
#include "MacAddress.h"
#include "Ethernet.h"

#include <string>
#include <vector>
#include <memory>


PacketSwitch::PacketSwitch(tal_id_t tal_id):
	mutex(),
	tal_id(tal_id),
	sarp_table()
{
	OpenSandModelConf::Get()->getSarp(this->sarp_table);
}

SarpTable *PacketSwitch::getSarpTable()
{
	return &this->sarp_table;
}

bool PacketSwitch::learn(const Data &packet, tal_id_t src_id)
{
	MacAddress src_mac = Ethernet::getSrcMac(packet);
	RtLock(this->mutex);
	if (!this->sarp_table.add(std::unique_ptr<MacAddress>{new MacAddress{src_mac.str()}}, src_id))
	{
		return false;
	}
	return true;
}

bool TerminalPacketSwitch::getPacketDestination(const Data &packet, tal_id_t &src_id, tal_id_t &dst_id)
{
	MacAddress dst_mac = Ethernet::getDstMac(packet);
	RtLock(this->mutex);
	src_id = this->tal_id;
	if (!this->sarp_table.getTalByMac(dst_mac, dst_id))
	{
		dst_id = this->gw_id;
	}
	return true;
}

bool TerminalPacketSwitch::isPacketForMe(const Data &UNUSED(packet), tal_id_t UNUSED(src_id), bool &forward)
{
	forward = false;
	return true;
}

bool GatewayPacketSwitch::getPacketDestination(const Data &packet, tal_id_t &src_id, tal_id_t &dst_id)
{
	MacAddress dst_mac = Ethernet::getDstMac(packet);
	MacAddress src_mac = Ethernet::getSrcMac(packet);
	RtLock(this->mutex);
	src_id = this->tal_id;	

	if(!this->sarp_table.getTalByMac(dst_mac, dst_id))
	{
		return false;
	}
	return true;
}

bool GatewayPacketSwitch::isPacketForMe(const Data &packet, tal_id_t src_id, bool &forward)
{
	tal_id_t dst_id;
	MacAddress dst_mac = Ethernet::getDstMac(packet);
	MacAddress src_mac = Ethernet::getSrcMac(packet);
	RtLock(this->mutex);
	if(!this->sarp_table.getTalByMac(dst_mac, dst_id))
	{
		return false;
	}
	forward = true;
	return  ((dst_id == BROADCAST_TAL_ID) ||  (dst_id == this->tal_id));
}

SatellitePacketSwitch::SatellitePacketSwitch(tal_id_t tal_id, std::unordered_set<tal_id_t> isl_entities):
	PacketSwitch{tal_id},
	isl_entities{std::move(isl_entities)}
{
}

bool SatellitePacketSwitch::getPacketDestination(const Data &packet, tal_id_t &src_id, tal_id_t &dst_id)
{
	MacAddress dst_mac = Ethernet::getDstMac(packet);
	MacAddress src_mac = Ethernet::getSrcMac(packet);
	RtLock(this->mutex);
	if (!this->sarp_table.getTalByMac(dst_mac, dst_id))
	{
		return false;
	}
	if (!this->sarp_table.getTalByMac(src_mac, src_id))
	{
		return false;
	}
	return true;
}

bool SatellitePacketSwitch::isPacketForMe(const Data &packet, tal_id_t, bool &forward)
{
	tal_id_t dst_id;
	MacAddress dst_mac = Ethernet::getDstMac(packet);
	RtLock(this->mutex);
	if (!this->sarp_table.getTalByMac(dst_mac, dst_id))
	{
		return false;
	}
	bool to_isl = isl_entities.find(dst_id) != isl_entities.end();
	forward = (dst_id == BROADCAST_TAL_ID) || !to_isl;
	return (dst_id == BROADCAST_TAL_ID) || to_isl;
}
