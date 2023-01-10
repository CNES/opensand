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
 * @file Types.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Mathias ETTINGER / <mathias.ettinger@viveris.fr>
 * @brief  Types for opensand-rt
 */



#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>


constexpr std::size_t MAX_SOCK_SIZE{9000};


namespace Rt
{


/// opensand-rt event types
enum class EventType
{
	NetSocket,   ///< Event of type NetSocket
	Timer,       ///< Event of type Timer
	Message,     ///< Event of type Message
	Signal,      ///< Event of type Signal
	File,        ///< Event of type File
	TcpListen,   ///< Event of type TcpListen
};


using event_id_t = int32_t;
struct Data: std::basic_string<unsigned char>
{
	using basic_string::basic_string;
	Data(const basic_string&);
	Data(basic_string&&);
};
template <class T> using Ptr = std::unique_ptr<T, void(*)(void*)>;


struct Message
{
	Message(std::nullptr_t):
		type{},
		data{nullptr, [](void*){}}
	{
	}
	template<class T> Message(T* ptr);
	template<class T> Message(Ptr<T>&& ptr);
	template<class T> Message& operator =(Ptr<T>&& ptr);

	Message(Message&& m):
		type{std::move(m.type)},
		data{std::move(m.data)}
	{
	};

	Message& operator =(Message&& m)
	{
		this->data = std::move(m.data);
		this->type = std::move(m.type);
		return *this;
	};

	template<class T> Ptr<T> release();

	uint8_t type;

 protected:
	Ptr<void> data;
};


template<class T, class... Args>
Ptr<T> make_ptr(Args... args)
{
	auto instance = new T(std::forward<Args>(args)...);
	auto deleter = [](void* p){ delete static_cast<T*>(p); };
	return {instance, deleter};
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


template<class T>
Message::Message(T* ptr):
	type{},
	data{make_ptr(ptr)}
{
}


template<class T>
Message::Message(Ptr<T>&& ptr):
	type{},
	data{std::move(ptr)}
{
}


template<class T>
Message& Message::operator =(Ptr<T>&& ptr)
{
	this->data = std::move(ptr);
	return *this;
}


template<class T>
Ptr<T> Message::release()
{
	return {static_cast<T*>(this->data.release()), std::move(this->data.get_deleter())};
}


};  // namespace Rt


#endif
