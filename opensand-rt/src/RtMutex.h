/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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


#include <opensand_output/OutputMutex.h>

/**
 * @class RtMutex
 * @brief Mutex for OpenSAND process using opensand-rt
 *        This is only a class inheriting from the mutex
 *        defined in Output in order to get a mutex
 *        in opensand-rt.
 *        We have the dependency on output due to logs
 */
class RtMutex: public OutputMutex
{
 public:

	/**
	 * Create RtMutex
	 *
	 * @param name  The name of the caller for debug
	 */
	RtMutex(string name):
		OutputMutex(name)
	{};
};


/**
 * @class RtLock
 * @brief Wrapper for using a mutex with RAII method
 */
class RtLock: public OutputLock
{
 public:
	RtLock(RtMutex &mutex):
		OutputLock(mutex)
	{};
};

#endif
