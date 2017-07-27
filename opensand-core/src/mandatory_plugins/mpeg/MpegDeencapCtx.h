/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
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
 * @file MpegDeencapCtx.h
 * @brief MPEG2-TS desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_DEENCAP_CTX
#define MPEG_DEENCAP_CTX

#include <NetBurst.h>


/**
 * @class MpegDeencapCtx
 * @brief MPEG2-TS desencapsulation context
 */
class MpegDeencapCtx
{
 protected:

	/// Internal buffer to store the SNDU under build
	Data _data;

	/// The PID that identifies the encapsulation context
	uint16_t _pid;
	/// The Continuity Counter (CC) of the last MPEG2-TS frame received
	uint8_t _cc;

	/// Whether the context needs to synchronized on PUSI or not
	bool _need_pusi;

	/// The length (in bytes) of the SNDU currently being extracted from the
	/// MPEG2-TS frame
	unsigned int _sndu_len;

	/// The destination spot ID
	uint16_t _dest_spot;

	/// The output log
	OutputLog *log;

 public:

	/*
	 * Build a desencapsulation context identified with PID pid
	 *
	 * @param pid the PID that identifies the desencapsulation context
	 * @param dest_spot the destination spot ID
	 */
	MpegDeencapCtx(uint16_t pid, uint16_t spot_id);

	/**
	 * Destroy the desencapsulation context
	 */
	~MpegDeencapCtx();

	/**
	 * Clear the desencapsulation context, ie. empty the internal list
	 * of network packets
	 */
	void reset();

	/**
	 * Get the amount of data partially desencapsulated and temporary stored
	 * in the context
	 *
	 * @return the amount of data stored in the context
	 */
	unsigned int length();

	/**
	 * Get the PID of the desencapsulation context
	 *
	 * @return the PID of the desencapsulation context
	 */
	uint16_t pid();

	/**
	 * Get the Continuity Counter (CC) of the last MPEG frame desencapsulated by
	 * the context
	 *
	 * @return the CC of the context
	 */
	uint8_t cc();

	/**
	 * Increment the Continuity Counter (CC) of the context
	 */
	void incCc();

	/**
	 * Set the Continuity Counter (CC) of the context
	 *
	 * @param cc  the new CC of the context
	 */
	void setCc(uint8_t cc);

	/**
	 * Whether the context needs to synchronized on PUSI or not
	 *
	 * @return true if the context needs to synchronized, false otherwise
	 */
	bool need_pusi();

	/**
	 * Tell the context whether to synchronize on PUSI or not
	 *
	 * @param flag  true to tell the context to synchronize, false otherwise
	 */
	void set_need_pusi(bool flag);

	/**
	 * Get the length of the SNDU currently being extracted from the MPEG2-TS
	 * frame
	 *
	 * @return  the length of the SNDU in bytes
	 */
	unsigned int sndu_len();

	/**
	 * Tell the context the length of the SNDU currently being extracted from
	 * the MPEG2-TS frame
	 *
	 * @param len  the length of the SNDU in bytes
	 */
	void set_sndu_len(unsigned int len);

	/**
	 * Add data at the end of the SNDU
	 *
	 * @param data    the data set that contains the data to add
	 * @param offset  the offset of the data to add in the data set
	 * @param length  the length of the data to add
	 */
	void add(const Data &data, unsigned int offset, unsigned int length);


	/**
	 * Get the internal buffer that stores the SNDU under build
	 *
	 * @return the internal buffer that stores the SNDU under build
	 */
	Data data();

	/**
	 * Get the destination spot ID
	 *
	 * @return the destination spot ID
	 */
	uint16_t getDestSpot();
};

#endif
