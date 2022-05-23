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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file   RtFifo.cpp
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The fifo and signaling pipres for opensand-rt
 *         intra-block messages
 */

#include <unistd.h>
#include <cstring>

#include "RtFifo.h"
#include "Rt.h"
#include "RtCommunicate.h"


#define DEFAULT_FIFO_SIZE 3


RtFifo::RtFifo():
	fifo{},
	max_size{DEFAULT_FIFO_SIZE},
	fifo_mutex{},
  fifo_size_sem{DEFAULT_FIFO_SIZE}
{
}


RtFifo::~RtFifo()
{
	close(this->r_sig_pipe);
	close(this->w_sig_pipe);
}


bool RtFifo::init()
{
	int32_t pipefd[2];
	if(pipe(pipefd) != 0)
	{
		return false;
	}

	this->r_sig_pipe = pipefd[0];
	this->w_sig_pipe = pipefd[1];

	return true;
}


bool RtFifo::push(void *data, size_t size, uint8_t type)
{
	// we need a semaphore here to block while fifo is full
	fifo_size_sem.wait();
	RtLock acquire{fifo_mutex};

	if(this->fifo.size() >= this->max_size)
	{
		Rt::reportError("fifo", std::this_thread::get_id(), false,
		                "Size is greater than maximum size (%u > %u), "
		                "this should not happend\n",
		                this->fifo.size(), this->max_size);
	}
	this->fifo.push({data, size, type});

	fd_set wset;
	FD_ZERO(&wset);
	FD_SET(this->w_sig_pipe, &wset);
	if(select(this->w_sig_pipe + 1, NULL, &wset, NULL, NULL) < 0)
	{
		Rt::reportError("fifo", std::this_thread::get_id(), false,
		                "Select failed on pipe [%d: %s]\n",
		                errno, strerror(errno));
		return false;
	}
	if (!check_write(this->w_sig_pipe))
	{
		Rt::reportError("fifo", std::this_thread::get_id(), false,
		                "Failed to write on pipe\n");
		return false;
	}

	return true;
}


bool RtFifo::pop(rt_msg_t &elem)
{
  {
    RtLock acquire{fifo_mutex};

    if(this->fifo.empty())
    {
      Rt::reportError("fifo", std::this_thread::get_id(), false,
                      "Fifo is already empty, this should not happend\n");
      return false;
    }
    else
    {
      // get element in queue
      elem = this->fifo.front();

      // remove element from queue
      this->fifo.pop();
    }

  }

	// fifo has empty space, we can unlock it
  fifo_size_sem.notify();

	return true;
}
