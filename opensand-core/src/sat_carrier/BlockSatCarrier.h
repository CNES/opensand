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
 * @file BlockSatCarrier.h
 * @brief This bloc implements a satellite carrier emulation
 * @author AQL (ame)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef BlockSatCarrier_H
#define BlockSatCarrier_H


#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>

#include "sat_carrier_channel_set.h"


struct sc_specific
{
	tal_id_t tal_id;     ///< the terminal id for terminal
	std::string ip_addr;      ///< the IP address for emulation
	/// for sat only: destination handled by this part of the stack (terminal or gateway)
	Component destination_host = Component::unknown;    
	/// for sat only: the spot handled by this part of the stack
	spot_id_t spot_id = 255;
};


template<>
class Rt::UpwardChannel<class BlockSatCarrier>: public Channels::Upward<UpwardChannel<BlockSatCarrier>>
{
 public:
	UpwardChannel(const std::string &name, sc_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const NetSocketEvent &event) override;

 private:
	/// the IP address for emulation newtork
	std::string ip_addr;
	/// the terminal id for the emulation newtork
	tal_id_t tal_id;
	/// List of input channels
	sat_carrier_channel_set in_channel_set;
	/// for sat only: destination handled by this part of the stack (terminal or gateway)
	Component destination_host;
	/// for sat only: the spot handled by this part of the stack
	spot_id_t spot_id;

	/**
	 * @brief Handle a packt received from carrier
	 *
	 * @param carrier_id  The carrier of the packet
	 * @param spot_id     The spot of the packet
	 * @param data        The data read on socket
	 */
	void onReceivePktFromCarrier(uint8_t carrier_id,
	                             spot_id_t spot_id,
								 Ptr<Data> data);
};


template<>
class Rt::DownwardChannel<class BlockSatCarrier>: public Channels::Downward<DownwardChannel<BlockSatCarrier>>
{
 public:
	DownwardChannel(const std::string &name, sc_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const MessageEvent &event) override;

 private:
	/// the IP address for emulation newtork
	std::string ip_addr;
	/// the terminal id for the emulation newtork
	tal_id_t tal_id;
	/// List of output channels
	sat_carrier_channel_set out_channel_set;
	/// for sat only: destination handled by this part of the stack (terminal or gateway)
	Component destination_host;
	/// for sat only: the spot handled by this part of the stack
	spot_id_t spot_id;
};


/**
 * @class BlockSatCarrier
 * @brief This bloc implements a satellite carrier emulation
 */
class BlockSatCarrier: public Rt::Block<BlockSatCarrier, sc_specific>
{
 public:
	using Rt::Block<BlockSatCarrier, sc_specific>::Block;
};


#endif
