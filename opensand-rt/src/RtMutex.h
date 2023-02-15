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
 * @file RtMutex.h
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  Wrapper for using a mutex with RAII method
 */



#ifndef RT_MUTEX_H
#define RT_MUTEX_H


#include <mutex>
#include <condition_variable>


namespace Rt
{


using Mutex = std::recursive_mutex;
using Lock = std::lock_guard<Mutex>;


/**
 * @class Semaphore
 * @brief A simple semaphore implementation to protect access to critical sections
 */
class Semaphore
{
 public:
	Semaphore(std::size_t = 1);

	Semaphore(const Semaphore&) = delete;
	Semaphore& operator =(const Semaphore&) = delete;

	void wait();
	void notify();

 private:
	std::mutex lock;
	std::condition_variable condition;
	std::size_t count;
};


};


#endif
