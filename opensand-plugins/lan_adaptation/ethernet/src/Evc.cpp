/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file Evc.cpp
 * @brief The EVC information for header rebuild
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 */

#include "Evc.h"

#include "NetPacket.h"
#include "MacAddress.h"

#include <cstring>
#include <algorithm>

Evc::Evc(const MacAddress *mac_src,
         const MacAddress *mac_dst,
         uint16_t q_tag,
         uint16_t ad_tag,
         uint16_t ether_type):
	mac_src(mac_src),
	mac_dst(mac_dst),
	q_tag(q_tag & 0x0000FFFF),
	ad_tag(ad_tag & 0x0000FFFF),
	ether_type(ether_type)
{
}

Evc::~Evc()
{
	delete this->mac_src;
	delete this->mac_dst;
}

const MacAddress *Evc::getMacSrc() const
{
	return this->mac_src;
}

const MacAddress *Evc::getMacDst() const
{
	return this->mac_dst;
}

uint32_t Evc::getQTag() const
{
	uint32_t val = 0;
	val |= NET_PROTO_802_1Q;
	return (this->q_tag & 0x0000FFFF) | (val << 16);
}

uint32_t Evc::getAdTag() const
{
	uint32_t val = 0;
	// TODO for kernel support
	//val |= NET_PROTO_802_1AD;
	val |= NET_PROTO_802_1Q;
	return (this->ad_tag & 0x0000FFFF) | (val << 16);
}

uint16_t Evc::getEtherType() const
{
	return this->ether_type;
}

bool Evc::matches(const MacAddress *mac_src,
                  const MacAddress *mac_dst,
                  uint16_t q_tag,
                  uint16_t ad_tag,
                  uint16_t ether_type) const
{
	if(!this->mac_src->matches(mac_src) ||
	   !this->mac_dst->matches(mac_dst) ||
	   this->q_tag != q_tag ||
	   this->ad_tag != ad_tag ||
	   this->ether_type != ether_type)
	{
	   	return false;
	}
	return true;
}

bool Evc::matches(const MacAddress *mac_src,
                  const MacAddress *mac_dst,
                  uint16_t ether_type) const
{
	if(!this->mac_src->matches(mac_src) ||
	   !this->mac_dst->matches(mac_dst) ||
	   this->ether_type != ether_type)
	{
	   	return false;
	}
	return true;
}

bool Evc::matches(const MacAddress *mac_src,
                  const MacAddress *mac_dst,
                  uint16_t q_tag,
                  uint16_t ether_type) const
{
	if(!this->mac_src->matches(mac_src) ||
	   !this->mac_dst->matches(mac_dst) ||
	   this->q_tag != q_tag ||
	   this->ether_type != ether_type)
	{
	   	return false;
	}
	return true;
}
