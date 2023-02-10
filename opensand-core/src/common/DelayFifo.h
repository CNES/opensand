/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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


#include <map>
#include <memory>

#include <opensand_rt/Ptr.h>
#include <opensand_rt/RtMutex.h>

#include "OpenSandCore.h"


class FifoElement;
class NetContainer;


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
	DelayFifo(vol_pkt_t max_size_pkt = 10000);

	virtual ~DelayFifo();

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
	 * @brief Add an element at the end of the list
	 *        (Increments new_size_pkt)
	 *
	 * @param elem      is the pointer on a NetContainer that will be
	 *                  wrapped into a FifoElement for storage
	 * @param duration  is the amount of time the element should stay in the fifo
	 * @return true on success, false otherwise
	 */
	virtual bool push(Rt::Ptr<NetContainer> elem, time_ms_t duration);

	/**
	 * @brief Flush the sat carrier fifo and reset counters
	 */
	virtual void flush();

 protected:
	using time_point_t = std::chrono::high_resolution_clock::time_point;

	std::map<time_point_t, std::unique_ptr<FifoElement>> queue; ///< the FIFO itself

	vol_pkt_t max_size_pkt;                          ///< the maximum size for that FIFO

	mutable Rt::Mutex fifo_mutex;                    ///< The mutex to protect FIFO from concurrent access

	/**
	 * @brief Remove an element at the head of the list
	 *
	 * @return NULL pointer if extraction failed because fifo is empty
	 *         pointer on extracted FifoElement otherwise
	 */
	virtual std::unique_ptr<FifoElement> pop();

	class sentinel
	{
		time_point_t end;

	 public:
		sentinel();
		bool isAfter(const time_point_t& date) const;
	};

	/**
	 * @brief Destructive forward iterator over elements in the fifo
	 *        that have an "exit time" in the past
	 */
	struct iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = void;
		using value_type        = std::unique_ptr<FifoElement>;
		using pointer           = value_type*;
		using reference         = value_type&;

		iterator(DelayFifo& fifo);
		value_type operator *();
		iterator& operator ++();

		bool operator ==(const sentinel& end) const;
		bool operator !=(const sentinel& end) const;

	 private:
		DelayFifo& m_fifo;
		time_point_t getTickOut() const;
	};

	struct iterator_wrapper
	{
		using wrapped_iterator  = decltype(DelayFifo::queue)::iterator;
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = wrapped_iterator::difference_type;
		using value_type        = std::unique_ptr<FifoElement>;
		using pointer           = value_type*;
		using reference         = value_type&;

		iterator_wrapper(const wrapped_iterator& it);
		reference operator *() const;
		iterator_wrapper& operator ++();

		bool operator ==(const iterator_wrapper& other) const;
		bool operator !=(const iterator_wrapper& other) const;

	 private:
		wrapped_iterator it;
	};

	friend iterator;
	friend iterator_wrapper;

 public:
	iterator begin();
	sentinel end();

	iterator_wrapper wbegin();
	iterator_wrapper wend();
};


#endif
