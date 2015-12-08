/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 *
 */

/**
 * @file MacFifoElement.h
 * @brief Fifo element
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef FIFO_ELEMENT_H
#define FIFO_ELEMENT_H

#include "NetContainer.h"

/**
 * @class MacFifoElement
 * @brief Fifo element
 */
class MacFifoElement
{
 protected:

	/// The element stored in the FIFO
	NetContainer *elem;

	/// The arrival time of packet in FIFO (in ms)
	time_t tick_in;
	/// The minimal time the packet will output the FIFO (in ms)
	time_t tick_out;


 public:

	/**
	 * Build a fifo element
	 * @param elem       The element to store in the FIFO
	 * @param tick_in    The arrival time of element in FIFO (in ms)
	 * @param tick_out   The minimal time the element will output the FIFO (in ms)
	 */
	MacFifoElement(NetContainer *elem,
	               time_t tick_in, time_t tick_out);

	/**
	 * Destroy the fifo element
	 */
	~MacFifoElement();

	/**
	 * Get the FIFO elelement
	 * @return The FIFO element
	 */
	NetContainer *getElem() const;

	/**
	 * Get the FIFO elelement
	 * @return The FIFO element
	 */
	template<class T>
	T *getElem() const;


	/**
	 * Set the FIFO element
	 *
	 * @param packet The new FIFO element
	 */
	void setElem(NetContainer *elem);

	/**
	 * Get the element length
	 * @return The element length
	 */
	size_t getTotalLength() const;

	/**
	 * Get the arrival time of packet in FIFO (in ms)
	 * @return The arrival time of packet in FIFO
	 */
	time_t getTickIn() const;

	/**
	 * Get the minimal time the packet will output the FIFO (in ms)
	 * @return The minimal time the packet will output the FIFO
	 */
	time_t getTickOut() const;
};

// TODO check that, static cast ? operator () ?
template<class T>
T *MacFifoElement::getElem() const
{
	return (T *)this->getElem();
}

#endif
