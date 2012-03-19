/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file DvbRcsFrame.h
 * @brief DVB-RCS frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef DVB_RCS_FRAME_H
#define DVB_RCS_FRAME_H

#include <DvbFrame.h>


/**
 * @class DvbRcsFrame
 * @brief DVB-RCS frame
 */
class DvbRcsFrame: public DvbFrame
{
 public:

	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data    raw data from which a DVB-RCS frame can be created
	 * @param length  length of raw data
	 */
	DvbRcsFrame(unsigned char *data, unsigned int length);

	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data  raw data from which a DVB-RCS frame can be created
	 */
	DvbRcsFrame(Data data);

	/**
	 * Duplicate a DVB-RCS frame
	 *
	 * @param frame  the DVB-RCS frame to duplicate
	 */
	DvbRcsFrame(DvbRcsFrame *frame);

	/**
	 * Build an empty DVB-RCS frame
	 */
	DvbRcsFrame();

	/**
	 * Destroy the DVB-RCS frame
	 */
	~DvbRcsFrame();

	// implementation of virtual functions
	uint16_t payloadLength();
	Data payload();
	bool addPacket(NetPacket *packet);
	void empty(void);
	void setEncapPacketType(int type);
	t_pkt_type getEncapPacketType();

};

#endif
