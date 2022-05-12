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

#include "sat_carrier_channel_set.h"

#include <opensand_rt/Rt.h>


struct sc_specific
{
	tal_id_t tal_id;     ///< the terminal id for terminal
	string ip_addr;      ///< the IP address for emulation
	/// for sat only: destination handled by this part of the stack (terminal or gateway)
	component_t destination_host = component_t::unknown_compo;    
	/// for sat only: the spot handled by this part of the stack
	spot_id_t spot_id = 255;
};

/**
 * @class BlockSatCarrier
 * @brief This bloc implements a satellite carrier emulation
 */
class BlockSatCarrier: public Block
{
 public:

	/**
	 * @brief The satellite carrier block
	 *
	 * @param name      The block name
	 * @param specific  Specific block parameters
	 */
	BlockSatCarrier(const string &name,
	                struct sc_specific specific);

	class Upward: public RtUpward
	{
	 public:
		Upward(const string &name, struct sc_specific specific);

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the IP address for emulation newtork
		string ip_addr;
		/// the terminal id for the emulation newtork
		tal_id_t tal_id;
		/// List of input channels
		sat_carrier_channel_set in_channel_set;
		/// for sat only: destination handled by this part of the stack (terminal or gateway)
		component_t destination_host;
		/// for sat only: the spot handled by this part of the stack
		spot_id_t spot_id;

		/**
		 * @brief Handle a packt received from carrier
		 *
		 * @param carrier_id  The carrier of the packet
		 * @param data        The data read on socket
		 * @param length      The data length
		 */
		void onReceivePktFromCarrier(uint8_t carrier_id,
		                             spot_id_t spot_id,
		                             unsigned char *data,
		                             size_t length);
	};

	class Downward: public RtDownward
	{
	 public:
		Downward(const string &name, struct sc_specific specific);

		bool onInit(void);
		bool onEvent(const RtEvent *const event);

	 private:
		/// the IP address for emulation newtork
		string ip_addr;
		/// the terminal id for the emulation newtork
		tal_id_t tal_id;
		/// List of output channels
		sat_carrier_channel_set out_channel_set;
		/// for sat only: destination handled by this part of the stack (terminal or gateway)
		component_t destination_host;
		/// for sat only: the spot handled by this part of the stack
		spot_id_t spot_id;
	};

 protected:

	// initialization method
	bool onInit();
};

#endif
