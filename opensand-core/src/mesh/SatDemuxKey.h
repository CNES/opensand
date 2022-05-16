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
 * @file SatDemuxKey.h
 * @brief The demux key to select the right network stack in the satellite
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef SATDEMUXKEY_H
#define SATDEMUXKEY_H

#include "OpenSandCore.h"

struct SatDemuxKey
{
	spot_id_t spot_id;
	Component dest;
	bool operator==(SatDemuxKey o) const;
};

namespace std
{
template <>
struct hash<SatDemuxKey>
{
	size_t operator()(const SatDemuxKey &k) const
	{
		return 4 * k.spot_id + to_underlying(k.dest);
	}
};
} // namespace std

#endif
