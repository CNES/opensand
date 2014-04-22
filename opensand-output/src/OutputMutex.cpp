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
 * @file   OutputMutex.cpp
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Wrapper for using a mutex with RAII method
 */

#include "OutputMutex.h"

#include "Output.h"

#include <errno.h>
#include <string.h>
#include <syslog.h>

OutputMutex::OutputMutex(const string &name):
	name(name)
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

	ret = pthread_mutexattr_destroy(&mutex_attr);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Mutex '%s' attribute release failure [%d: %s]\n",
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
	if(pthread_mutex_lock(&(this->mutex)) != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to lock mutex '%s'\n",
		       this->name.c_str());
	}
}
	
void OutputMutex::releaseLock(void)
{
	if(pthread_mutex_unlock(&(this->mutex)) != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to unlock mutex '%s'\n",
		       this->name.c_str());
	}
}



/// Read Write Lock

OutputRwLock::OutputRwLock(const string &name):
	name(name)
{
	int ret;
	pthread_rwlockattr_t rwlock_attr;

	ret = pthread_rwlockattr_init(&rwlock_attr);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to initialize rwlock '%s' attributes [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
		return;
	}

	ret = pthread_rwlock_init(&(this->rwlock), &rwlock_attr);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "rwlock '%s' initialization failure [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}

	ret = pthread_rwlockattr_destroy(&rwlock_attr);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "rwlock '%s' attribute release failure [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}
}

OutputRwLock::~OutputRwLock()
{
	int ret;

	ret = pthread_rwlock_destroy(&(this->rwlock));
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to destroy rwlock '%s' [%d: %s]\n",
		       this->name.c_str(), ret, strerror(ret));
	}

}

void OutputRwLock::readLock(void)
{
	pthread_rwlock_rdlock(&(this->rwlock));
}
	
void OutputRwLock::writeLock(void)
{
	pthread_rwlock_wrlock(&(this->rwlock));
}

void OutputRwLock::releaseLock(void)
{
	pthread_rwlock_unlock(&(this->rwlock));
}


/// SpinLock

OutputSpinLock::OutputSpinLock()
{
	int ret;
	ret = pthread_spin_init(&(this->spinlock), 0);
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "SpinLock initialization failure [%d: %s]\n",
		       ret, strerror(ret));
	}
}

OutputSpinLock::~OutputSpinLock()
{
	int ret;

	ret = pthread_spin_destroy(&(this->spinlock));
	if(ret != 0)
	{
		syslog(LEVEL_ERROR,
		       "Failed to destroy spinlock [%d: %s]\n",
		       ret, strerror(ret));
	}

}

void OutputSpinLock::acquireLock(void)
{
	pthread_spin_lock(&(this->spinlock));
}
	
void OutputSpinLock::releaseLock(void)
{
	pthread_spin_unlock(&(this->spinlock));
}
