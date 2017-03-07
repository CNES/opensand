/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @file TestDelayFifo.h
 * @brief FIFO queue containing MAC packets used for emulating delay
 * @author Joaquin MUGUERZA / Viveris Technologies
 */

#ifndef TEST_DELAY_FIFO_H
#define TEST_DELAY_FIFO_H

#include "OpenSandCore.h"
#include "TestDelayFifoElement.h"

#include <opensand_rt/RtMutex.h>

#include <vector>
#include <map>
#include <sys/times.h>

using std::vector;
using std::map;


/**
 * @class TestDelayFifo
 * @brief Defines a Delay fifo
 *
 * Manages a Sat Carrier fifo, for queuing, statistics, ...
 */
class TestDelayFifo
{
 public:

	/**
	 * @brief Create the TestDelayFifo
	 *
	 * @param max_size_pkt  the fifo maximum size
	 */
	TestDelayFifo(vol_pkt_t max_size_pkt);

	~TestDelayFifo();

	/**
	 * @brief Get the fifo current size
	 *
	 * @return the queue current size
	 */
	vol_pkt_t getCurrentSize() const;

	/**
	 * @brief Get the fifo maximum size
	 *
	 * @return the queue maximum size
	 */
	vol_pkt_t getMaxSize() const;
	
	/**
	 * @brief Get the head element tick out
	 *
	 * @return the head element tick out
	 */
	clock_t getTickOut() const;

	/**
	 * @brief Add an element at the end of the list
	 *        (Increments new_size_pkt)
	 *
	 * @param elem is the pointer on TestDelayFifoElement
	 * @return true on success, false otherwise
	 */
	bool push(TestDelayFifoElement *elem);

	/**
	 * @brief Add an element at the head of the list
	 * @warning This function should be use only to replace a fragment of
	 *          previously removed data in the fifo
	 *
	 * @param elem is the pointer on TestDelayFifoElement
	 * @return true on success, false otherwise
	 */
	bool pushFront(TestDelayFifoElement *elem);

	/**
	 * @brief Add an element at the back of the list
	 *
	 * @param elem is the pointer on TestDelayFifoElement
	 * @return true on success, false otherwise
	 */
	bool pushBack(TestDelayFifoElement *elem);

	/**
	 * @brief Remove an element at the head of the list
	 *
	 * @return NULL pointer if extraction failed because fifo is empty
	 *         pointer on extracted TestDelayFifoElement otherwise
	 */
	TestDelayFifoElement *pop();

	/**
	 * @brief Flush the sat carrier fifo and reset counters
	 */
	void flush();

	vector<TestDelayFifoElement *> getQueue(void);

 protected:

	/**
	 * @brief Get the index where time_out should be placed
	 * @param time_out the tick out 
	 * @return -1 on error, the index on success
	 */
	int getTickOutPosition(time_t time_out);

	vector<TestDelayFifoElement *> queue; ///< the FIFO itself

	vol_pkt_t max_size_pkt;         ///< the maximum size for that FIFO

	mutable RtMutex fifo_mutex; ///< The mutex to protect FIFO from concurrent access
};

#endif
