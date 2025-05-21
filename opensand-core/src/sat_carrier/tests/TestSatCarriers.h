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
 * @file TestSatCarriers.h
 * @brief This bloc implements a satellite carrier emulation
 * @author AQL (ame)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef TEST_SAT_CARRIERS_H
#define TEST_SAT_CARRIERS_H

#include "sat_carrier_channel_set.h"

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>


struct sc_specific
{
	tal_id_t tal_id;
	std::string ip_addr;      ///< the IP address for emulation
};


template<>
class Rt::UpwardChannel<class TestSatCarriers>: public Channels::Upward<UpwardChannel<TestSatCarriers>>
{
 public:
	UpwardChannel(const std::string& name, sc_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Event& event) override;
	bool onEvent(const FileEvent& event) override;
	bool onEvent(const NetSocketEvent& event) override;

	/**
	 * @brief Set the network socket file descriptor
	 *
	 * @param fd  The socket file descriptor
	 */
	void setFd(int fd);

 private:
	/// List of input channels
	sat_carrier_channel_set in_channel_set;
	/// the IP address for emulation newtork
	std::string ip_addr;
};


template<>
class Rt::DownwardChannel<class TestSatCarriers>: public Channels::Downward<DownwardChannel<TestSatCarriers>>
{
 public:
	DownwardChannel(const std::string& name, sc_specific specific);

	bool onInit() override;

	using ChannelBase::onEvent;
	bool onEvent(const Rt::Event& event) override;
	bool onEvent(const Rt::MessageEvent& event) override;

	/**
	 * @brief Set the network socket file descriptor
	 *
	 * @param fd  The socket file descriptor
	 */
	void setFd(int fd);

 private:
	/// List of output channels
	sat_carrier_channel_set out_channel_set;
	/// the IP address for emulation newtork
	std::string ip_addr;
	/// The tun output file descriptor
	int fd;
};


/**
 * @class TestSatCarriers
 * @brief This bloc implements a satellite carrier emulation
 */
class TestSatCarriers: public Rt::Block<TestSatCarriers, sc_specific>
{
 public:
	using Rt::Block<TestSatCarriers, sc_specific>::Block;

 protected:
	// initialization method
	bool onInit() override;
};


#endif
