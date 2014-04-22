/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file GseEncapCtx.h
 * @brief GSE encapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_ENCAP_CTX
#define GSE_ENCAP_CTX

#include <NetPacket.h>

#include "GseIdentifier.h"

extern "C"
{
	#include <gse/constants.h>
	#include <gse/status.h>
	#include <gse/virtual_fragment.h>
}

/**
 * @class GseEncapCtx
 * @brief GSE encapsulation context
 */
class GseEncapCtx
{
 protected:

	/// Internal buffer to store the GSE packet under build
	gse_vfrag_t *vfrag;
	/// Source Tal Id from first packet
	uint8_t src_tal_id;
	/// Destination Tal Id from first packet
	uint8_t dst_tal_id;
	/// QoS from first packet
	uint8_t qos;
	/// protocol of the packets sotred in virtual buffer
	uint16_t protocol;
	/// name of the packets sotred in virtual buffer
	std::string name;
	/// Tell if virtual buffer is full or not
	bool is_full;
	/// The destination spot ID
	uint16_t dest_spot;
	/// The output log
	OutputLog *log;

 public:

	/*
	 * Build an sencapsulation context identified with PID
	 *
	 * @param identifier Context identifier that identifies the encapsulation context
	 * @param spot_id    The destination spot ID
	 */
	GseEncapCtx(GseIdentifier *identifier, uint16_t spot_id);

	/**
	 * Destroy the encapsulation context
	 */
	~GseEncapCtx();

	/**
	 * Add data at the end of the virtual buffer
	 *
	 * @param data    the data set that contains the data to add
	 */
	gse_status_t add(NetPacket *data);

	/**
	 * Get the data stored in the virtual bufferd
	 *
	 * @return the virtual buffer that stores the GSE packet under build
	 */
	gse_vfrag_t *data();

	/**
	 * Get the amount of data stored in the context (in bytes)
	 *
	 * @return the amount of data (in bytes) stored in the context
	 */
	size_t length();

	/**
	 * Test if the virtual buffer is full
	 *
	 * @return True if the virtual buffer is full, else False
	 */
	bool isFull();

	/**
	 * Get the source Tal Id of the context
	 *
	 * @return the Tal Id
	 */
	uint8_t getSrcTalId();

	/**
	 * Get the destination Tal Id of the context
	 *
	 * @return the Tal Id
	 */
	uint8_t getDstTalId();

	/**
	 * Get the QoS of the context
	 *
	 * @return the QoS
	 */
	uint8_t getQos();

	/**
	 * Get the protocol of the packets stored in virtual buffer
	 *
	 * @return the protocol
	 */
	uint16_t getProtocol();

	/**
	 * Get the name of the packets stored in virtual buffer
	 *
	 * @return the name
	 */
	std::string getPacketName();

	/**
	 * Get the destination spot ID
	 *
	 * @return the destination spot ID
	 */
	uint16_t getDestSpot();

};

#endif
