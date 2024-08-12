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
 * @file DvbRcsStd.h
 * @brief DVB-RCS Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_RCS_STD_H
#define DVB_RCS_STD_H

#include "PhysicStd.h"
#include "DvbRcsFrame.h"

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
	DvbRcsStd(SimpleEncapPlugin* pkt_hdl = nullptr);

	/**
	 * Destroy the DVB-RCS Transmission Standard
	 */
	virtual ~DvbRcsStd();

	bool onRcvFrame(Rt::Ptr<DvbFrame> dvb_frame,
	                tal_id_t tal_id,
	                Rt::Ptr<NetBurst> &burst);

protected:
	/**
	 * Build a DVB-RCS Transmission Standard
	 *
	 * @param type              The type name
	 * @param has_fixed_length  The fixed length status
	 * @param packet_handler    The packet handler
	 */
	DvbRcsStd(std::string ype, bool has_fixed_length,
	          SimpleEncapPlugin* pkt_hdl = nullptr);

	// Output log and debug
	std::shared_ptr<OutputLog>  log_rcv_from_down;

	/// Has encapsulation packet fixed length
	bool has_fixed_length;
};

class DvbRcs2Std: public DvbRcsStd
{
public:
	DvbRcs2Std(SimpleEncapPlugin* pkt_hdl = nullptr);
};

#endif
