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
 * @file RtMutex.h
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Wrapper for using a mutex with RAII metho
 *
 */



#ifndef RT_MUTEX_H
#define RT_MUTEX_H

#include <pthread.h>
#include <string>

using std::string;

class RtMutex
{
 public:

	/**
	 * Create RtMutex
	 *
	 * @param name  The name of the caller for debug
	 */
	RtMutex(string name);

	~RtMutex();

	void acquireLock(void);
	
	void releaseLock(void);

 private:
 
	/// The mutex
	pthread_mutex_t mutex;
	
	/// The name of the caller for debug messages
	string name;
};

/**
 * @class RtLock
 * @brief Wrapper for using a mutex with RAII method
 */
class RtLock
{
 public:

	/**
	 * Create Lock
	 *
	 * @param mutex  The RtMutex on which we want to take lock
	 */
	RtLock(RtMutex &mutex):
		mutex(mutex)
	{
		this->mutex.acquireLock();
	};

	~RtLock()
	{
		this->mutex.releaseLock();
	};

 private:

	/// The RtMutex
	RtMutex &mutex;
};


#endif
