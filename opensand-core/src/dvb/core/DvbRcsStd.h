/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
 * Copyright © 2018 CNES
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
 * @file DvbRcsStd.h
 * @brief DVB-RCS Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_RCS_STD_H
#define DVB_RCS_STD_H

#include "PhysicStd.h"
#include "DvbRcsFrame.h"
#include "GenericSwitch.h"

#include <opensand_output/OutputLog.h>

/**
 * @class DvbRcsStd
 * @brief DVB-RCS Transmission Standard
 */
class DvbRcsStd: public PhysicStd
{

 public:

	/**
	 * Build a DVB-RCS Transmission Standard
	 *
	 * @param packet_handler The packet handler
	 */
	DvbRcsStd(EncapPlugin::EncapPacketHandler *pkt_hdl = NULL);

	/**
	 * Destroy the DVB-RCS Transmission Standard
	 */
	virtual ~DvbRcsStd();

	bool onRcvFrame(DvbFrame *dvb_frame,
	                tal_id_t tal_id,
	                NetBurst **burst);

	/* function for regenerative satellite */
	bool setSwitch(GenericSwitch *generic_switch);

 protected:
	/**
	 * Build a DVB-RCS Transmission Standard
	 *
	 * @param type              The type name
	 * @param has_fixed_length  The fixed length status
	 * @param packet_handler    The packet handler
	 */
	DvbRcsStd(string ype, bool has_fixed_length,
		EncapPlugin::EncapPacketHandler *pkt_hdl = NULL);

	// Output log and debug
	OutputLog* log_rcv_from_down;

	/// Has encapsulation packet fixed length
	bool has_fixed_length;

 private:

	/// Switch which manages the different spots
	/// (for regenerative satellite only)
	GenericSwitch *generic_switch;

};

class DvbRcs2Std: public DvbRcsStd
{
  public:
	DvbRcs2Std(EncapPlugin::EncapPacketHandler *pkt_hdl = NULL);
};

#endif
