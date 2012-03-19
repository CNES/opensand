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
 * @file GseEncapCtx.h
 * @brief GSE encapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_ENCAP_CTX
#define GSE_ENCAP_CTX

#include <NetPacket.h>
#include <GseIdentifier.h>

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
	gse_vfrag_t *_vfrag;
	/// Tal Id get from first packet
	long _tal_id;
	/// Mac Id get from first packet
	unsigned long _mac_id;
	/// QoS get from first packet
	long _qos;
	/// protocol of the packets sotred in virtual buffer
	uint16_t _protocol;
	/// name of the packets sotred in virtual buffer
	std::string _name;
	/// Tell if virtual buffer is full or not
	bool _is_full;

 public:

	/*
	 * Build an sencapsulation context identified with PID
	 *
	 * @param identifier Context identifier that identifies the encapsulation context
	 */
	GseEncapCtx(GseIdentifier *identifier);

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
	 * Get the Tal Id of the context
	 *
	 * @return the Tal Id
	 */
	long talId();

	/**
	 * Get the Mac Id of the context
	 *
	 * @return the Mac Id
	 */
	unsigned long macId();

	/**
	 * Get the QoS of the context
	 *
	 * @return the QoS
	 */
	int qos();

	/**
	 * Get the protocol of the packets stored in virtual buffer
	 *
	 * @return the protocol
	 */
	uint16_t protocol();

	/**
	 * Get the name of the packets stored in virtual buffer
	 *
	 * @return the name
	 */
	std::string packetName();

};

#endif
