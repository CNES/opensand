/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 */

#ifndef BlockSatCarrier_H
#define BlockSatCarrier_H

#include "sat_carrier_channel_set.h"

#include <opensand_rt/Rt.h>

struct sc_specific
{
	string ip_addr;      ///< the IP address for emulation
	string emu_iface;    ///< the name of the emulation interface
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
	 * @param host      The type of host
	 * @param specific  Specific block parameters
	 */
	BlockSatCarrier(const string &name,
	                struct sc_specific specific);

	~BlockSatCarrier();


 protected:

	/// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

	/// List of channels
	sat_carrier_channel_set m_channelSet;

 private:

	/// the IP address for emulation newtork
	string ip_addr;
	/// the interface name for emulation newtork
	string interface_name;

	void onReceivePktFromCarrier(unsigned int i_channel,
	                             unsigned char *ip_buf,
	                             unsigned int i_len);
};

#endif
