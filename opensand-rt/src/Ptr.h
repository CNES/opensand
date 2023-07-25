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
 * @file Ptr.h
 * @author Mathias ETTINGER / <mathias.ettinger@viveris.fr>
 * @brief  Buffer data type for opensand-rt
 */


#ifndef PTR_H
#define PTR_H


#include <memory>


namespace Rt
{


template<typename T>
using Ptr = std::unique_ptr<T, void(*)(void*)>;


template<class T, class... Args>
Ptr<T> make_ptr(Args... args)
{
	auto instance = new T(std::forward<Args>(args)...);
	auto deleter = [](void* p){ delete static_cast<T*>(p); };
	return {instance, deleter};
}


template<class T>
Ptr<T> make_ptr(std::nullptr_t)
{
	return {nullptr, [](void*){}};
}


template<class T>
Ptr<T> make_ptr(T* ptr)
{
	return {ptr, [](void* p){ delete static_cast<T*>(p); }};
}


template<class T>
Ptr<T> make_ptr(std::unique_ptr<T[]> ptr)
{
	return {ptr.release(), [](void* p){ delete [] static_cast<T*>(p); }};
}


};


#endif
