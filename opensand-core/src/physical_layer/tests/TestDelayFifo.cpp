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
 * @file TestDelayFifo.cpp
 * @brief  FIFO queue containing MAC packets used for emulating delay
 * @author Joaquin MUGUERZA / Viveris Technologies
 */


#include "TestDelayFifo.h"

#include <opensand_output/Output.h>

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>


TestDelayFifo::TestDelayFifo(vol_pkt_t max_size_pkt):
	queue(),
	max_size_pkt(max_size_pkt),
	fifo_mutex("delay_fifo_mutex")
{
}

/**
 * Destructor
 */
TestDelayFifo::~TestDelayFifo()
{
	this->flush();
}

vol_pkt_t TestDelayFifo::getCurrentSize() const
{
	RtLock lock(this->fifo_mutex);
	return this->queue.size();
}

vol_pkt_t TestDelayFifo::getMaxSize() const
{
	RtLock lock(this->fifo_mutex);
	return this->max_size_pkt;
}

clock_t TestDelayFifo::getTickOut() const
{
	RtLock lock(this->fifo_mutex);
	if(queue.size() > 0)
	{
		return this->queue.front()->getTickOut();
	}
	return 0;
}

vector<TestDelayFifoElement *> TestDelayFifo::getQueue(void)
{
	return this->queue;
}

bool TestDelayFifo::push(TestDelayFifoElement *elem)
{
	int pos;
	RtLock lock(this->fifo_mutex);

	if(this->queue.size() >= this->max_size_pkt)
	{
		return false;
	}

	pos = this->getTickOutPosition(elem->getTickOut());

	// insert in correct position
	if(pos >= 0)
	{
		printf("fifo pos %d", pos);
		this->queue.insert(this->queue.begin()+pos, elem);
	}

	return true;
}

bool TestDelayFifo::pushFront(TestDelayFifoElement *elem)
{
	RtLock lock(this->fifo_mutex);

	// insert in head of fifo
	if(this->queue.size() < this->max_size_pkt)
	{
		this->queue.insert(this->queue.begin(), elem);
		return true;
	}

	return false;

}

bool TestDelayFifo::pushBack(TestDelayFifoElement *elem)
{
	RtLock lock(this->fifo_mutex);

	// insert in head of fifo
	if(this->queue.size() < this->max_size_pkt)
	{
		this->queue.insert(this->queue.end(), elem);
		return true;
	}

	return false;

}
TestDelayFifoElement *TestDelayFifo::pop()
{
	RtLock lock(this->fifo_mutex);
	TestDelayFifoElement *elem;

	if(this->queue.size() <= 0)
	{
		return NULL;
	}

	elem = this->queue.front();

	// remove the packet
	this->queue.erase(this->queue.begin());

	return elem;
}

void TestDelayFifo::flush()
{
	RtLock lock(this->fifo_mutex);
	vector<TestDelayFifoElement *>::iterator it;
	for(it = this->queue.begin(); it != this->queue.end(); ++it)
	{
		delete *it;
	}

	this->queue.clear();
}

int TestDelayFifo::getTickOutPosition(time_t time_out)
{
	time_t time_elem;
	int pos = -1;
	int start = 0;
	int test = 0;
	int end = this->queue.size() - 1;

	// Implement a divide and conquer approach
	while(end > start)
	{
		test = (end + start)/2;
		time_elem = this->queue.at(test)->getTickOut();
		if(time_elem > time_out)
			end = test;
		else
			start = test + 1;
	}
	if(end == start)
	{
		time_elem = this->queue.at(start)->getTickOut();
		if(time_elem > time_out)
			pos = start;
		else
			pos = start + 1;
	}
	else if(end < 0)
	{
		pos = 0;
	}
	printf("Position: %d\n", pos);
	return pos;
}
