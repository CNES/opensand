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
 * @file Evc.h
 * @brief The EVC information
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 */

#ifndef EVC_H
#define EVC_H

#include "MacAddress.h"
#include "NetPacket.h"

#include <stdint.h>

/**
 * @class Evc
 * @brief The EVC information
 */
class Evc
{
private:
	/// The source MAC address
	const MacAddress mac_src;
	/// The destination MAC address
	const MacAddress mac_dst;
	/// 802.1Q TCI
	uint32_t q_tci;
	/// 802.1ad TCI
	uint32_t ad_tci;
	/// The EtherType of the packet carried by the Ethernet payload
	NET_PROTO ether_type;

public:
	/**
	 * @brief Build EVC information
	 *
	 * @param mac_src     The source MAC address
	 * @param mac_dst     The destination MAC address
	 * @param q_tci       The Q TCI
	 * @param ad_tci      The ad TCI
	 * @param ether_type  The EtherType of the packet carried by
	 *                    the Ethernet payload
	 */
	Evc(const MacAddress &mac_src, const MacAddress &mac_dst,
	    uint16_t q_tci, uint16_t ad_tci,
	    NET_PROTO ether_type);

	/**
	 * @brief Get the source MAC address
	 *
	 * @return the source MAC address
	 */
	const MacAddress &getMacSrc() const;

	/**
	 * @brief Get the destination MAC address
	 *
	 * @return the destination MAC address
	 */
	const MacAddress &getMacDst() const;

	/**
	 * @brief Get the 802.1Q TCI
	 *
	 * @return the Q TCI
	 */
	uint32_t getQTci() const;

	/**
	 * @brief Get the 802.1ad TCI
	 *
	 * @return the AD TCI
	 */
	uint32_t getAdTci() const;

	/**
	 * @brief Get the EtherType value
	 *        The value will depend on the type of ethernet frame
	 *
	 * @return the appropriate EtherType value
	 */
	NET_PROTO getEtherType() const;

	/**
	 * @brief check if our data match the EVC ones
	 *
	 * @param mac_src     The source MAC address
	 * @param mac_dst     The destination MAC address
	 * @param q_tci       The Q TCI
	 * @param ad_tci      The ad TCI
	 * @param ether_type  The EtherType of the packet carried by
	 *                    the Ethernet payload
	 * @return true if it matches, false otherwise
	 */
	bool matches(const MacAddress &mac_src,
	             const MacAddress &mac_dst,
	             uint16_t q_tci,
	             uint16_t ad_tci,
	             NET_PROTO ether_type) const;

	/**
	 * @brief check if our data match the EVC ones
	 *
	 * @param mac_src     The source MAC address
	 * @param mac_dst     The destination MAC address
	 * @param ether_type  The EtherType of the packet carried by
	 *                    the Ethernet payload
	 * @return true if it matches, false otherwise
	 */
	bool matches(const MacAddress &mac_src,
	             const MacAddress &mac_dst,
	             NET_PROTO ether_type) const;
	/**
	 * @brief check if our data match the EVC ones
	 *
	 * @param mac_src     The source MAC address
	 * @param mac_dst     The destination MAC address
	 * @param q_tci       The Q TCI
	 * @param ether_type  The EtherType of the packet carried by
	 *                    the Ethernet payload
	 * @return true if it matches, false otherwise
	 */
	bool matches(const MacAddress &mac_src,
	             const MacAddress &mac_dst,
	             uint16_t q_tci,
	             NET_PROTO ether_type) const;
};


#endif
