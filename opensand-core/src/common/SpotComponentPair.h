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
 * @file SpotComponentPair.h
 * @brief A simple struct containing a spot ID and a component type.
 *        It is used as demux key to select the right network stack in the satellite.
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef SPOTCOMPONENTPAIR_H
#define SPOTCOMPONENTPAIR_H

#include "OpenSandCore.h"


struct SpotComponentPair
{
	spot_id_t spot_id;
	Component dest;
	bool operator==(const SpotComponentPair &o) const;
};


struct IslComponentPair
{
	tal_id_t connected_sat;
	bool is_data_channel;
	bool operator ==(const IslComponentPair &o) const;
};


namespace std
{
template <>
struct hash<SpotComponentPair>
{
	size_t operator()(const SpotComponentPair &k) const
	{
		return k.spot_id << 2 | to_underlying(k.dest);
	}
};

template <>
struct hash<IslComponentPair>
{
	size_t operator()(const IslComponentPair &k) const
	{
		return k.connected_sat << 1 | k.is_data_channel;
	}
};
} // namespace std

#endif
