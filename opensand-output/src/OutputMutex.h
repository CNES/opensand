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
	OutputMutex(const string &name);

	~OutputMutex();

	void acquireLock(void);
	
	void releaseLock(void);

 private:
 
	/// The mutex
	pthread_mutex_t mutex;
	
	/// The name of the caller for debug messages
	string name;
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




class OutputRwLock
{
 public:

	/**
	 * Create OutputRwLock
	 *
	 * @param name  The name of the caller for debug
	 */
	OutputRwLock(const string &name);

	~OutputRwLock();

	void readLock(void);
	void writeLock(void);
	
	void releaseLock(void);

 private:
 
	/// The rwlock
	pthread_rwlock_t rwlock;
	
	/// The name of the caller for debug messages
	string name;
};


/**
 * @class OutputRLock
 * @brief Wrapper for using a rwlock read lock with RAII method
 */
class OutputRLock
{
 public:

	/**
	 * Create Lock
	 *
	 * @param rwlock  The OutputRwLock on which we want to take lock
	 */
	OutputRLock(OutputRwLock &rwlock):
		rwlock(rwlock)
	{
		this->rwlock.readLock();
	};

	~OutputRLock()
	{
		this->rwlock.releaseLock();
	};

 private:

	/// The OutputRwLock
	OutputRwLock &rwlock;
};

/**
 * @class OutputWLock
 * @brief Wrapper for using a rwlock write lock with RAII method
 */
class OutputWLock
{
 public:

	/**
	 * Create Lock
	 *
	 * @param rwlock  The OutputRwLock on which we want to take lock
	 */
	OutputWLock(OutputRwLock &rwlock):
		rwlock(rwlock)
	{
		this->rwlock.writeLock();
	};

	~OutputWLock()
	{
		this->rwlock.releaseLock();
	};

 private:

	/// The OutputRwLock
	OutputRwLock &rwlock;
};


class OutputSpinLock
{
 public:

	/**
	 * Create OutputSpinLock
	 *
	 * @param name  The name of the caller for debug
	 */
	OutputSpinLock();

	~OutputSpinLock();

	void acquireLock(void);
	
	void releaseLock(void);

 private:
 
	/// The spinlock
	pthread_spinlock_t spinlock;
};

/**
 * @class OutputLock
 * @brief Wrapper for using a spinlock with RAII method
 */
class OutputSLock
{
 public:

	/**
	 * Create Lock
	 *
	 * @param spinlock  The OutputSpinLock on which we want to take lock
	 */
	OutputSLock(OutputSpinLock &spinlock):
		spinlock(spinlock)
	{
		this->spinlock.acquireLock();
	};

	~OutputSLock()
	{
		this->spinlock.releaseLock();
	};

 private:

	/// The OutputSpinLock
	OutputSpinLock &spinlock;
};


#endif
