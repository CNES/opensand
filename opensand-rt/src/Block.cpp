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
 * @file Block.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The block description
 *
 */


#include "Block.h"
#include "RtChannel.h"
#include "Rt.h"

#include <opensand_conf/uti_debug.h>

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>


Block::Block(const string &name, void *specific):
	name(name),
	initialized(false),
	chan_mutex(false)
{
	UTI_DEBUG("Block %s: created\n", this->name.c_str());
}


Block::~Block()
{
	if(this->chan_mutex)
	{
		int ret;
		ret = pthread_mutex_destroy(&(this->block_mutex));
		if(ret != 0)
		{
			Rt::reportError(this->name, pthread_self(), false,
			                "Mutex destroy failure [%u: %s]", ret, strerror(ret));
		}
	}

	if(this->downward != NULL)
	{
		delete this->downward;
	}

	if(this->upward != NULL)
	{
		delete this->upward;
	}
}

// TODO remove once onEvent will be specific to channel
bool Block::sendUp(void **data, size_t size, uint8_t type)
{
	return this->upward->enqueueMessage(data, size, type);
}

// TODO remove once onEvent will be specific to channel
bool Block::sendDown(void **data, size_t size, uint8_t type)
{
	return this->downward->enqueueMessage(data, size, type);
}

bool Block::init(void)
{
	// initialize channels
	if(!this->upward->init())
	{
		return false;
	}
	if(!this->downward->init())
	{
		return false;
	}

	return true;
}

bool Block::initSpecific(void)
{
	// specific block initialization
	if(!this->onInit())
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Block onInit failed");
		return false;
	}

	// initialize channels
	if(!this->upward->onInit())
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Upward onInit failed");
		return false;
	}
	if(!this->downward->onInit())
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Downward onInit failed");
		return false;
	}
	this->initialized = true;

	return true;
}

bool Block::isInitialized(void)
{
	return this->initialized;
}

bool Block::start(void)
{
	int ret;
	pthread_attr_t attr; // thread attribute

	// set thread detach state attribute to JOINABLE
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	UTI_DEBUG("Block %s: start upward channel\n", this->name.c_str());
	//create upward thread
	ret = pthread_create(&(this->up_thread_id), &attr,
	                     &Upward::startThread, this->upward);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot start upward thread [%u: %s]", ret, strerror(ret));
		return false;
	}
	UTI_DEBUG("Block %s: upward channel thread id %lu\n",
	          this->name.c_str(), this->up_thread_id);

	UTI_DEBUG("Block %s: start downward channel\n", this->name.c_str());
	//create upward thread
	ret = pthread_create(&(this->down_thread_id), &attr,
	                     &Downward::startThread, this->downward);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot downward start thread [%u: %s]", ret, strerror(ret));
		return false;
	}
	UTI_DEBUG("Block %s: downward channel thread id: %lu\n",
	          this->name.c_str(), this->up_thread_id);
	return true;
}

bool Block::stop(int signal)
{
	bool status = true;
	int ret;

	UTI_DEBUG("Block %s: stop channels\n", this->name.c_str());
	// the process may be already killed as the may have catch the stop signal first
	// So, do not report an error
	ret = pthread_kill(this->up_thread_id, signal);
	if(ret != 0 && ret != ESRCH)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot kill upward thread [%u: %s]", ret, strerror(ret));
		status = false;
	}

	ret = pthread_kill(this->down_thread_id, signal);
	if(ret != 0 && ret != ESRCH)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot kill downward thread [%u: %s]", ret, strerror(ret));
		status = false;
	}

	UTI_DEBUG("Block %s: join channels\n", this->name.c_str());
	ret = pthread_join(this->up_thread_id, NULL);
	if(ret != 0 && ret != ESRCH)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot join upward thread [%u: %s]", ret, strerror(ret));
		status = false;
	}

	ret = pthread_join(this->down_thread_id, NULL);
	if(ret != 0 && ret != ESRCH)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot join downward thread [%u: %s]", ret, strerror(ret));
		status = false;
	}
	return status;
}

bool Block::processEvent(const RtEvent *const event, chan_type_t chan)
{
	bool ret = false;
	if(this->chan_mutex)
	{
		int err;
		err = pthread_mutex_lock(&(this->block_mutex));
		if(err != 0)
		{
			Rt::reportError(this->name, pthread_self(), false,
			                "Mutex lock failure [%u: %s]", err, strerror(err));
			return false;
		}
	}
	if(chan == upward_chan)
	{
		ret = this->onUpwardEvent(event);
	}
	else if(chan == downward_chan)
	{
		ret = this->onDownwardEvent(event);
	}
	if(this->chan_mutex)
	{
		int err;
		err = pthread_mutex_unlock(&(this->block_mutex));
		if(err != 0)
		{
			Rt::reportError(this->name, pthread_self(), false,
			                "Mutex unlock failure [%u: %s]", err, strerror(err));
			return false;
		}
	}
	return ret;
}

clock_t Block::getCurrentTime(void)
{
	timeval current;
	gettimeofday(&current, NULL);
	return current.tv_sec * 1000 + current.tv_usec / 1000;
}

RtChannel *Block::getUpwardChannel(void) const
{
	return this->upward;
}
	
RtChannel *Block::getDownwardChannel(void) const
{
	return this->downward;
}

void Block::enableChannelMutex(void)
{
	int ret;
	ret = pthread_mutex_init(&(this->block_mutex), NULL);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "Mutex initialization failure [%u: %s]", ret, strerror(ret));
	}
	this->chan_mutex = true;
}
