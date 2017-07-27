/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file EthernetHeader.h
 * @brief Generic Ethernet frame, 802.3, 802.1q or 802.1ad
 * @author Remy Pienne <remy.pienne@toulouse.viveris.com>
 */

#ifndef ETH_HEADER_H
#define ETH_HEADER_H

#include <string>

#include <Data.h>
#include <NetPacket.h>
#include <MacAddress.h>

#include <net/ethernet.h>


// use struct ether_header from /usr/include/net/ethernet.h
typedef struct ether_header eth_2_header_t;
typedef union
{
	struct
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		uint8_t:3 pcp;
		uint8_t:4 dei;
		uint8_t:4 vid_hi;
		uint8_t:8 vid_lo;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#else
		uint8_t:4 vid_hi;
		uint8_t:4 dei;
		uint8_t:3 pcp;
		uint8_t:8 vid_lo;
#error "Please fix <bits/endian.h>"
#endif
	} __attribute__ ((__packed__));
	struct
	{
		uint16_t tci;
	} __attribute__ ((__packed__));
} __attribute__ ((__packed__)) tci_u;

struct eth_1q_header
{
	uint8_t ether_dhost[ETH_ALEN];
	uint8_t ether_shost[ETH_ALEN];
	uint16_t TPID; /* Tag Protocol IDentifier : 0x8100 */
	tci_u TCI;
	uint16_t ether_type;
} __attribute__ ((__packed__));

typedef struct eth_1q_header eth_1q_header_t;

struct eth_1ad_header
{
	uint8_t ether_dhost[ETH_ALEN];
	uint8_t ether_shost[ETH_ALEN];
	uint16_t outer_TPID; /* Tag Protocol IDentifier : 0x9100 */
	tci_u outer_TCI;
	uint16_t inner_TPID; /* Tag Protocol IDentifier : 0x8100 */
	tci_u inner_TCI;
	uint16_t ether_type;
} __attribute__ ((__packed__));

typedef struct eth_1ad_header eth_1ad_header_t;

#endif
