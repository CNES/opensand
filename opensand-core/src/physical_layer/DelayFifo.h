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
 * @file DvbFifo.h
 * @brief FIFO queue containing MAC packets used for emulating delay
 * @author Joaquin MUGUERZA / Viveris Technologies
 */

#ifndef DELAY_FIFO_H
#define DELAY_FIFO_H

#include "OpenSandCore.h"
#include "DelayFifoElement.h"

#include <opensand_rt/RtMutex.h>
#include <opensand_output/OutputLog.h>

#include <vector>
#include <map>
#include <sys/times.h>

using std::vector;
using std::map;


/**
 * @class DelayFifo
 * @brief Defines a Delay fifo
 *
 * Manages a Sat Carrier fifo, for queuing, statistics, ...
 */
class DelayFifo
{
 public:

	/**
	 * @brief Create the DelayFifo
	 *
	 * @param max_size_pkt  the fifo maximum size
	 */
	DelayFifo(vol_pkt_t max_size_pkt);

	DelayFifo():
  	DelayFifo(10000)
	{};

	~DelayFifo();

	/**
	 * @brief Get the fifo current size
	 *
	 * @return the queue current size
	 */
	vol_pkt_t getCurrentSize() const;

	/**
	 * @brief Set the fifo maximum size
	 *
	 * @param max_size_pkt, the max number of packets
	 * @return true on success, false otherwise
	 */
	bool setMaxSize(vol_pkt_t max_size_pkt);
	
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
	 * @param elem is the pointer on DelayFifoElement
	 * @return true on success, false otherwise
	 */
	bool push(DelayFifoElement *elem);

	/**
	 * @brief Add an element at the head of the list
	 * @warning This function should be use only to replace a fragment of
	 *          previously removed data in the fifo
	 *
	 * @param elem is the pointer on DelayFifoElement
	 * @return true on success, false otherwise
	 */
	bool pushFront(DelayFifoElement *elem);

	/**
	 * @brief Add an element at the back of the list
	 *
	 * @param elem is the pointer on DelayFifoElement
	 * @return true on success, false otherwise
	 */
	bool pushBack(DelayFifoElement *elem);

	/**
	 * @brief Remove an element at the head of the list
	 *
	 * @return NULL pointer if extraction failed because fifo is empty
	 *         pointer on extracted DelayFifoElement otherwise
	 */
	DelayFifoElement *pop();

	/**
	 * @brief Flush the sat carrier fifo and reset counters
	 */
	void flush();

	vector<DelayFifoElement *> getQueue(void);

 protected:

	/**
	 * @brief Get the index where time_out should be placed
	 * @param time_out the tick out 
	 * @return -1 on error, the index on success
	 */
	int getTickOutPosition(time_t time_out);

	vector<DelayFifoElement *> queue; ///< the FIFO itself

	vol_pkt_t max_size_pkt;         ///< the maximum size for that FIFO

	mutable RtMutex fifo_mutex; ///< The mutex to protect FIFO from concurrent access

	// Output log
	OutputLog *log_delay_fifo;
};

#endif
