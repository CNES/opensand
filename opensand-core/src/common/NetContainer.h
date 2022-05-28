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
 * @file NetContainer.h
 * @brief Network data container
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef NET_CONTAINER_H
#define NET_CONTAINER_H


#include <string>

#include "OpenSandCore.h"
#include "Data.h"


/**
 * @class NetContainer
 * @brief Network data container
 * @warning change the copy constructor when you add
 *          a attribut  @ref DvbFrameTpl and @ref NetPacket
 */
class NetContainer
{
 protected:
	/// Internal buffer for packet data
	Data data;

	/// The name of the network protocol
	std::string name;

	/// The packet header length
	std::size_t header_length;

	/// The packet trailer length
	std::size_t trailer_length;
	
	/// The destination spot ID
	spot_id_t spot;

 public:
	/**
	 * Build a generic OpenSAND network container
	 *
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetContainer(const unsigned char *data, std::size_t length);

	/**
	 * Build a generic OpenSAND network container
	 *
	 * @param data raw data from which a network-layer packet can be created
	 */
	NetContainer(const Data &data);

	/**
	 * Build a generic OpenSAND network container
	 *
	 * @param data raw data from which a network-layer packet can be created
	 * @param length length of raw data
	 */
	NetContainer(const Data &data, std::size_t length);

	/**
	 * Build an empty generic OpenSAND network container
	 */
	NetContainer();

	/**
	 * Destroy the network-layer packet
	 */
	virtual ~NetContainer();

	/**
	 * Get the name of the network protocol
	 *
	 * @return the name of the network protocol
	 */
	std::string getName() const;

	/**
	 * Retrieve the total length of the packet
	 *
	 * @return the total length of the packet
	 */
	virtual std::size_t getTotalLength() const;

	/**
	 * Get data string
	 *
	 * @return the data string
	 */
	Data getData() const;

	/**
	 * Retrieve data from the desired position
	 *
	 * @param  the position of the data beginning
	 * @return the data starting at the given position
	 */
	virtual Data getData(std::size_t pos) const;

	/**
	 * Retrieve the length of the packet payload
	 *
	 * @return the length of the packet payload
	 */
	virtual std::size_t getPayloadLength() const;

	/**
	 * Retrieve the data corresponding to the payload of the packet
	 *
	 * @return the payload of the packet
	 */
	virtual Data getPayload() const;

	/**
	 * Retrieve data from the payload
	 *
	 * @param  the position of the data beginning in the payload
	 * @return the data starting at the given position from the payload
	 */
	virtual Data getPayload(std::size_t pos) const;

	/**
	 * Get the packet header length
	 *
	 * @return the header length
	 */
	virtual std::size_t getHeaderLength() const;

	/**
	 * Set the destination spot ID
	 *
	 * @param spot_id  The destination spot id
	 */
	void setSpot(spot_id_t spot_id);

	/**
	 * Get the destination spot ID
	 *
	 * @return the destination spot ID
	 */
	spot_id_t getSpot() const;

};

#endif
