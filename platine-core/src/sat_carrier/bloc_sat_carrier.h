/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
 * @file bloc_sat_carrier.h
 * @brief This bloc implements a satellite carrier emulation
 * @author AQL (ame)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef BLOC_SAT_CARRIER_H
#define BLOC_SAT_CARRIER_H

#include "platine_margouilla/mgl_bloc.h"
#include "sat_carrier_channel_set.h"


/**
 * @class BlocSatCarrier
 * @brief This bloc implements a satellite carrier emulation
 */
class BlocSatCarrier: public mgl_bloc
{
 public:

	/// Use mgl_bloc default constructor
	BlocSatCarrier(mgl_blocmgr *blocmgr, mgl_id fatherid, const char *name,
	               const t_component host, const string ip_addr);

	~BlocSatCarrier();

	// Event handlers
	mgl_status onEvent(mgl_event *event);

 protected:

	/// List of channels
	sat_carrier_channel_set m_channelSet;

 private:

	/// Whether the bloc has been initialized or not
	bool init_ok;
	/// the component type
	t_component host;
	/// the IP address for emulation newtork
	string ip_addr;
	// Internal event handlers
	int onInit();
	void onReceivePktFromCarrier(unsigned int i_channel,
	                             unsigned char *ip_buf,
	                             unsigned int i_len);
};

#endif
