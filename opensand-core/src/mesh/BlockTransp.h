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
 * @file BlockTransp.h
 * @brief
 * @author Yohan Simard <yohan.simard@viveris.fr>
 */

#ifndef BLOCKTRANSP_H
#define BLOCKTRANSP_H

#include <memory>

#include <opensand_rt/Rt.h>

#include "DvbFrame.h"


struct TranspDemuxKey
{
	spot_id_t spot_id;
	Component dest;
	bool operator==(TranspDemuxKey o) const;
};


namespace std
{
template <>
struct hash<TranspDemuxKey>
{
	size_t operator()(const TranspDemuxKey &k) const
	{
		return 4 * k.spot_id + to_underlying(k.dest);
	}
};
} // namespace std


/**
 * @class BlockTransp
 * @brief Block that sends DVB frames back to the opposite SatCarrier block
 */
class BlockTransp: public Block
{
 public:
	BlockTransp(const std::string &name);

	class Upward: public RtUpwardMux
	{
	  public:
		Upward(const std::string &name);

	  private:
		bool onEvent(const RtEvent *const event) override;
		bool handleDvbFrame(std::unique_ptr<const DvbFrame> burst);
	};

	class Downward: public RtDownwardDemux<TranspDemuxKey>
	{
	 public:
		Downward(const std::string &name);

	 private:
		bool onEvent(const RtEvent *const event) override;
		bool handleDvbFrame(std::unique_ptr<DvbFrame> burst);
		bool sendToLowerBlock(TranspDemuxKey key, std::unique_ptr<const DvbFrame> frame);
	};

  private:
};

#endif
