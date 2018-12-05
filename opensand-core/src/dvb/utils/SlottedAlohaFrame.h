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
 * @file SlottedAlohaFrame.h
 * @brief The Slotted Aloha frame
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
 * @author Julien Bernard / Viveris technologies
 */

#ifndef SALOHA_FRAME_H
#define SALOHA_FRAME_H

#include "DvbFrame.h"

/**
 * @class SlottedAlohaFrame
 * @brief The Slotted Aloha scheduling
 */
class SlottedAlohaFrame: public DvbFrameTpl<T_DVB_SALOHA>
{
 public:

	/**
	 * Build a Slotted Aloha frame
	 *
	 * @param data    raw data from which a Slotted Aloha frame can be created
	 * @param length  length of raw data
	 */
	SlottedAlohaFrame(const unsigned char *data, size_t length);

	/**
	 * Build a Slotted Aloha frame
	 *
	 * @param data  raw data from which a Slotted Aloha frame can be created
	 */
	SlottedAlohaFrame(const Data &data);

	/**
	 * Build a Slotted Aloha frame
	 *
	 * @param data    raw data from which a Slotted Aloha frame can be created
	 * @param length  length of raw data
	 */
	SlottedAlohaFrame(const Data &data, size_t length);

	/**
	 * Duplicate a Slotted Aloha frame
	 *
	 * @param frame  the Slotted Aloha frame to duplicate
	 */
	SlottedAlohaFrame(DvbFrame *frame);

	/**
	 * Build an empty Slotted Aloha frame
	 */
	SlottedAlohaFrame();

	/**
	 * Destroy the Slotted Aloha frame
	 */
	~SlottedAlohaFrame();

	// Implementation of virtual functions
	bool addPacket(NetPacket* packet);
	void empty();
	uint16_t getDataLength(void) const;
};


class SlottedAlohaFrameCtrl: public SlottedAlohaFrame
{
  public:
	SlottedAlohaFrameCtrl();
};


class SlottedAlohaFrameData: public SlottedAlohaFrame
{
  public:
	SlottedAlohaFrameData();
};

#endif

