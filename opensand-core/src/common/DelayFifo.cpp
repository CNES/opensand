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

/*
 * @file DelayFifo.cpp
 * @brief  FIFO queue containing MAC packets used for emulating delay
 * @author Joaquin MUGUERZA / Viveris Technologies
 */


#include <unistd.h>
#include <stdlib.h>
#include <cstring>

#include "DelayFifo.h"
#include "NetContainer.h"
#include "FifoElement.h"


DelayFifo::DelayFifo(vol_pkt_t max_size_pkt):
	queue(),
	max_size_pkt(max_size_pkt),
	fifo_mutex()
{
}


/**
 * Destructor
 */
DelayFifo::~DelayFifo()
{
	this->flush();
}


vol_pkt_t DelayFifo::getCurrentSize() const
{
	Rt::Lock lock(this->fifo_mutex);
	return this->queue.size();
}


bool DelayFifo::setMaxSize(vol_pkt_t max_size_pkt)
{
	Rt::Lock lock(this->fifo_mutex);
	// check if current size is bigger than the new max value
	if(this->queue.size() > max_size_pkt)
		return false;
	this->max_size_pkt = max_size_pkt;
	return true;
}


vol_pkt_t DelayFifo::getMaxSize() const
{
	Rt::Lock lock(this->fifo_mutex);
	return this->max_size_pkt;
}


bool DelayFifo::push(Rt::Ptr<NetContainer> elem, time_ms_t duration)
{
	Rt::Lock lock(this->fifo_mutex);

	if(this->queue.size() < this->max_size_pkt)
	{
		auto end_date = std::chrono::high_resolution_clock::now() + duration;
		this->queue.emplace(end_date, std::make_unique<FifoElement>(std::move(elem)));
		return true;
	}

	return false;
}


std::unique_ptr<FifoElement> DelayFifo::pop()
{
	Rt::Lock lock(this->fifo_mutex);

	auto elem = this->queue.begin();
	if (elem != this->queue.end())
	{
		std::unique_ptr<FifoElement> result = std::move(elem->second);
		this->queue.erase(elem);
		return result;
	}

	return {nullptr};
}


void DelayFifo::flush()
{
	Rt::Lock lock(this->fifo_mutex);
	this->queue.clear();
}


DelayFifo::iterator DelayFifo::begin()
{
	return iterator(*this);
}


DelayFifo::sentinel DelayFifo::end()
{
	return sentinel();
}


DelayFifo::iterator_wrapper DelayFifo::wbegin()
{
	return iterator_wrapper(this->queue.begin());
}


DelayFifo::iterator_wrapper DelayFifo::wend()
{
	return iterator_wrapper(this->queue.end());
}


DelayFifo::sentinel::sentinel():
	end{std::chrono::high_resolution_clock::now()}
{
}


bool DelayFifo::sentinel::isAfter(const DelayFifo::time_point_t& date) const
{
	return end > date;
}


bool DelayFifo::iterator::operator ==(const DelayFifo::sentinel& end) const
{
	if (this->m_fifo.getCurrentSize())
	{
		return !end.isAfter(this->getTickOut());
	}
	return true;
}


bool DelayFifo::iterator::operator !=(const DelayFifo::sentinel& end) const
{
	if (this->m_fifo.getCurrentSize())
	{
		return end.isAfter(this->getTickOut());
	}
	return false;
}


DelayFifo::iterator::iterator(DelayFifo& fifo):
	m_fifo{fifo}
{
}


inline DelayFifo::time_point_t DelayFifo::iterator::getTickOut() const
{
	return m_fifo.queue.begin()->first;
}


DelayFifo::iterator::value_type DelayFifo::iterator::operator *()
{
	return m_fifo.pop();
}


DelayFifo::iterator& DelayFifo::iterator::operator ++()
{
	return *this;
}


DelayFifo::iterator_wrapper::iterator_wrapper(const DelayFifo::iterator_wrapper::wrapped_iterator& it):
	it{it}
{
}


std::unique_ptr<FifoElement>& DelayFifo::iterator_wrapper::operator *() const
{
	return it->second;
}


DelayFifo::iterator_wrapper& DelayFifo::iterator_wrapper::operator ++()
{
	++it;
	return *this;
}


bool DelayFifo::iterator_wrapper::operator ==(const DelayFifo::iterator_wrapper& other) const
{
	return it == other.it;
}


bool DelayFifo::iterator_wrapper::operator !=(const DelayFifo::iterator_wrapper& other) const
{
	return it != other.it;
}
