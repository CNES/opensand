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
 * @file OutputMutex.h
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Wrapper for using a mutex with RAII method
 *
 */



#ifndef OUTPUT_MUTEX_H
#define OUTPUT_MUTEX_H

#include <pthread.h>
#include <string>

using std::string;

class OutputMutex
{
 public:

	/**
	 * Create OutputMutex
	 *
	 * @param name  The name of the caller for debug
	 */
	OutputMutex(string name);

	~OutputMutex();

	void acquireLock(void);
	
	void releaseLock(void);

	bool isLocked(void) const;

 private:
 
	/// The mutex
	pthread_mutex_t mutex;
	
	/// The name of the caller for debug messages
	string name;

	/// Whether the mutex is locked or not
	bool locked;
};

/**
 * @class OutputLock
 * @brief Wrapper for using a mutex with RAII method
 */
class OutputLock
{
 public:

	/**
	 * Create Lock
	 *
	 * @param mutex  The OutputMutex on which we want to take lock
	 */
	OutputLock(OutputMutex &mutex):
		mutex(mutex)
	{
		this->mutex.acquireLock();
	};

	~OutputLock()
	{
		this->mutex.releaseLock();
	};

 private:

	/// The OutputMutex
	OutputMutex &mutex;
};


#endif
