/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file   RtMutex.cpp
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Wrapper for using a mutex with RAII method
 */

// TODO REMOVE FILE
#if 0
#include "RtMutex.h"
#include "Rt.h"


RtMutex::RtMutex(string name):
	name(name)
{
	int ret;
	pthread_mutexattr_t mutex_attr;

	ret = pthread_mutexattr_init(&mutex_attr);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "Failed to initialize mutex attributes [%d: %s]\n",
		                ret, strerror(ret));
	}
	/* choose mutexes depending on what you need:
	    - PTHREAD_MUTEX_ERRORCHECK for library validation
	    - PTHREAD_MUTEX_NORMAL for fast mutex */
	ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "Failed to set mutex attributes [%d: %s]\n",
		                ret, strerror(ret));
	}

	ret = pthread_mutex_init(&(this->mutex), &mutex_attr);
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "Mutex initialization failure [%u: %s]",
		                ret, strerror(ret));
	}
}

RtMutex::~RtMutex()
{
	int ret;

	ret = pthread_mutex_destroy(&(this->mutex));
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Mutex destroy failure [%u: %s]",
		                ret, strerror(ret));
	}
}

void RtMutex::acquireLock(void)
{
	int ret;

	ret = pthread_mutex_lock(&(this->mutex));
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Failed to lock mutex [%d: %s]\n",
		                ret, strerror(ret));
	}
}
	
void RtMutex::releaseLock(void)
{
	int ret;

	ret = pthread_mutex_unlock(&(this->mutex));
	if(ret != 0)
	{
		Rt::reportError(this->name, pthread_self(), false,
		                "Failed to unlock mutex [%d: %s]\n",
		                ret, strerror(ret));
	}
}

#endif
