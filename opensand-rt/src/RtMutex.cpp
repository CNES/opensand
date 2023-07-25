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
 * @file RtMutex.cpp
 * @author Mathias ETTINGER / <mathias.ettinger@viveris.fr>
 * @brief  Wrapper for using a mutex with RAII method
 */


#include "RtMutex.h"


namespace Rt
{


using InnerLock = std::unique_lock<std::mutex>;


Semaphore::Semaphore(std::size_t initial_value):
	lock{},
	condition{},
	count{initial_value}
{
}


void Semaphore::wait()
{
	InnerLock take{lock};
	condition.wait(take, [this]() {return count != 0;});
	--count;
}


void Semaphore::notify()
{
	InnerLock take{lock};
	++count;
	condition.notify_one();
}


};
