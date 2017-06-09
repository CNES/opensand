/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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

#include <opensand_rt/Rt.h>


struct sc_specific
{
	tal_id_t tal_id;
	string ip_addr;      ///< the IP address for emulation
	string emu_iface;    ///< the name of the emulation interface
};

/**
 * @class TestSatCarriers
 * @brief This bloc implements a satellite carrier emulation
 */
class TestSatCarriers: public Block
{
 public:

	/**
	 * @brief The satellite carrier block
	 */
	TestSatCarriers(const string &name,
	                struct sc_specific UNUSED(specific));

	~TestSatCarriers();

	class Upward: public RtUpward
	{
	 public:
		Upward(const string &name, struct sc_specific specific):
			RtUpward(name),
			in_channel_set(specific.tal_id),
			ip_addr(specific.ip_addr),
			interface_name(specific.emu_iface)
		{};


		bool onInit(void);
		bool onEvent(const RtEvent *const event);

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
		string ip_addr;
		/// the interface name for emulation newtork
		string interface_name;
	};

	class Downward: public RtDownward
	{
	 public:
		Downward(const string &name, struct sc_specific specific):
			RtDownward(name),
			out_channel_set(specific.tal_id),
			ip_addr(specific.ip_addr),
			interface_name(specific.emu_iface)
		{};

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

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
		string ip_addr;
		/// the interface name for emulation newtork
		string interface_name;
		/// The tun output file descriptor
		int fd;
	};

 protected:

	// initialization method
	bool onInit();
};

#endif
