/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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

#include "RtFifo.h"

#include <cstdlib>
#include <cstring>
#include <unistd.h>

#define DEFAULT_FIFO_SIZE 3


RtFifo::RtFifo():
	max_size(DEFAULT_FIFO_SIZE)
{
}

RtFifo::~RtFifo()
{
	close(this->r_sig_pipe);
	close(this->w_sig_pipe);

/*	while(!this->fifo.empty())
	{
		this->fifo.pop();
	}
	delete msg.data*/

	pthread_mutex_destroy(&(this->fifo_mutex));
	pthread_mutex_destroy(&(this->full_mutex));
}

bool RtFifo::init()
{
	int32_t pipefd[2];

	if(pthread_mutex_init(&(this->fifo_mutex), NULL) != 0 ||
	   pthread_mutex_init(&(this->full_mutex), NULL) != 0)
	{
		return false;
	}

	if(pipe(pipefd) != 0)
	{
		// TODO report error in channel
		return false;
	}
	this->r_sig_pipe = pipefd[0];
	this->w_sig_pipe = pipefd[1];

	return true;
}


bool RtFifo::push(void *data, size_t size, uint8_t type)
{
	fd_set wset;
	bool status = false;
	rt_msg_t msg;

	if(this->fifo.size() > this->max_size)
	{
		// fifo is full, take mutex that will block until fifo has space
		if(pthread_mutex_lock(&(this->full_mutex)) != 0)
		{
			return false;
		}
	}

	// lock mutex on fifo
	if(pthread_mutex_lock(&(this->fifo_mutex)) != 0)
	{
		return false;
	}

	msg.data = data;
	msg.length = size;
	msg.type = type;
	this->fifo.push(msg);
	FD_ZERO(&wset);
	FD_SET(this->w_sig_pipe, &wset);
	if(select(this->w_sig_pipe + 1, NULL, &wset, NULL, NULL) < 0)
	{
		goto error;
	}
	if(write(this->w_sig_pipe, MAGIC_WORD, strlen(MAGIC_WORD)) != strlen(MAGIC_WORD))
	{
		goto error;
	}
	status = true;

error:
	// unlock mutex on fifo
	if(pthread_mutex_unlock(&(this->fifo_mutex)) != 0)
	{
		return false;
	}
	return status;

}

bool RtFifo::pop(rt_msg_t &elem)
{
	bool full = false;

	// lock mutex on fifo
	if(pthread_mutex_lock(&(this->fifo_mutex)) != 0)
	{
		return false;
	}

	// check if fifo will be emptied
	// do not use > because if the fifo was resized we
	// may have more than max_size elements
	if(this->fifo.size() == this->max_size)
	{
		full = true;
	}

	// get element in queue
	elem = this->fifo.front();

	// remove element from queue
	this->fifo.pop();

	// unlock mutex on fifo
	if(pthread_mutex_unlock(&(this->fifo_mutex)) != 0)
	{
		return false;
	}
	if(full)
	{
		// fifo has empty space, we can unlock it
		if(pthread_mutex_unlock(&(this->full_mutex)) != 0)
		{
			return false;
		}
	}
	return true;
}


