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
#include "Channel.h"
#include "Rt.h"

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>

Block::Block(const string &name):
	name(name),
	initialized(false)
{
#ifdef DEBUG_BLOCK_MUTEX
	int ret;
	ret = pthread_mutex_init(&(this->block_mutex), NULL);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "Mutex initialization failure", ret);
	}
#endif
	std::cout << "Block " << this->name << ": created" << std::endl;
}

Block::~Block()
{
#ifdef DEBUG_BLOCK_MUTEX
	ret = pthread_mutex_destroy(&(this->block_mutex), NULL);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Mutex destroy failure", ret);
	}
#endif

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
bool Block::sendUp(void *message)
{
	return this->upward->enqueueMessage(message);
}

// TODO remove once onEvent will be specific to channel
bool Block::sendDown(void *message)
{
	return this->downward->enqueueMessage(message);
}

bool Block::init(void)
{
	// specific block initialization
	if(!this->onInit())
	{
		return false;
	}

	// initialize channels
	if(!this->upward->init())
	{
		return false;
	}
	if(!this->downward->init())
	{
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

	std::cout << "Block " << this->name << ": start upward channel" << std::endl;
	//create upward thread
	ret = pthread_create(&(this->up_thread_id), &attr,
	                     &Upward::startThread, this->upward);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot start upward thread", ret);
		return false;
	}
	std::cout << "Block " << this->name << ": upward channel thread id: "
	          <<this->up_thread_id << std::endl;

	std::cout << "Block " << this->name << ": start downward channel" << std::endl;
	//create upward thread
	ret = pthread_create(&(this->down_thread_id), &attr,
	                     &Downward::startThread, this->downward);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot downward start thread", ret);
		return false;
	}
	std::cout << "Block " << this->name << ": downward channel thread id: "
	          <<this->up_thread_id << std::endl;
	return true;
}

bool Block::stop(int signal)
{
	bool status = true;
	int ret;

	std::cout << "Block " << this->name << ": stop channels" << std::endl;
	ret = pthread_kill(this->up_thread_id, signal);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot kill upward thread", ret);
		status = false;
	}
	ret = pthread_kill(this->down_thread_id, signal);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot kill downward thread", ret);
		status = false;
	}

	std::cout << "Block " << this->name << ": join channels" << std::endl;
	ret = pthread_join(this->up_thread_id, NULL);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot join upward thread", ret);
		status = false;
	}
	ret = pthread_join(this->down_thread_id, NULL);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "cannot join downward thread", ret);
		status = false;
	}
	return status;
}

bool Block::processEvent(const Event *const event, chan_type_t chan)
{
	bool ret = false;
#ifdef DEBUG_BLOCK_MUTEX
	int err;
	err = pthread_mutex_lock(&(this->block_mutex));
	if(err != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Mutex lock failure", err);
		return false;
	}
#endif
	if(chan == upward_chan)
	{
		ret = this->onUpwardEvent(event);
	}
	else if(chan == downward_chan)
	{
		ret = this->onDownwardEvent(event);
	}
#ifdef DEBUG_BLOCK_MUTEX
	err = pthread_mutex_unlock(&(this->block_mutex));
	if(err != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Mutex unlock failure", err);
		return false;
	}
#endif
	return ret;
}

Channel *Block::getUpwardChannel(void) const
{
	return this->upward;
}
	
Channel *Block::getDownwardChannel(void) const
{
	return this->downward;
}
