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
 * @file   OutputMutex.cpp
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Wrapper for using a mutex with RAII method
 */

#include "OutputMutex.h"

#include "Output.h"

#include <errno.h>
#include <string.h>
#include <syslog.h>

OutputMutex::OutputMutex(string name):
	name(name),
	locked(false)
{
	int ret;
	pthread_mutexattr_t mutex_attr;

	ret = pthread_mutexattr_init(&mutex_attr);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to initialize mutex '%s' attributes [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
		return;
	}
	/* choose mutexes depending on what you need:
	    - PTHREAD_MUTEX_ERRORCHECK for library validation
	    - PTHREAD_MUTEX_NORMAL for fast mutex */
	ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_NORMAL);
	if(ret != 0)
	{

		syslog(LEVEL_ERROR,
		       "Failed to set mutex '%s' attributes [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
		return;
	}

	ret = pthread_mutex_init(&(this->mutex), &mutex_attr);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Mutex '%s' initialization failure [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}
}

OutputMutex::~OutputMutex()
{
	int ret;

	ret = pthread_mutex_destroy(&(this->mutex));
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to destroy mutex '%s' [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}

}

void OutputMutex::acquireLock(void)
{
	int ret;

	ret = pthread_mutex_lock(&(this->mutex));
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to lock mutex '%s' [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}
	this->locked = true;
}
	
void OutputMutex::releaseLock(void)
{
	int ret;

	this->locked = false;
	ret = pthread_mutex_unlock(&(this->mutex));
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to unlock mutex '%s' [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}
}

bool OutputMutex::isLocked(void) const
{
	return this->locked;
}
