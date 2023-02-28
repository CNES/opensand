/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file Printf.h
 * @brief C++ wrapper around the C printf function
 * @author Mathias Ettinger   <mathias.ettinger@viveris.fr>
 */


#ifndef PRINTF_H
#define PRINTF_H


#include <cstdio>
#include <cstdint>
#include <string>


template<typename T>
inline T ArgumentWrapper(T value) noexcept
{
	return value;
}


template<typename T>
inline T const * ArgumentWrapper(std::basic_string<T> const & value) noexcept
{
	return value.c_str();
}


inline unsigned int ArgumentWrapper(uint8_t value) noexcept
{
	return value;
}


inline unsigned int ArgumentWrapper(uint16_t value) noexcept
{
	return value;
}


inline unsigned int ArgumentWrapper(uint32_t value) noexcept
{
	return value;
}


template<typename T>
inline T const * ArgumentWrapper(std::unique_ptr<T> const & value) noexcept
{
	return value.get();
}


template<typename T>
inline T const * ArgumentWrapper(std::shared_ptr<T> const & value) noexcept
{
	return value.get();
}


template<typename... Args>
void Print(char const * const format, Args const & ... args) noexcept
{
	printf(format, ArgumentWrapper(args)...);
}


template<typename... Args>
int StringPrint(char * const buffer,
                std::size_t const bufferCount,
                char const * const format,
                Args const & ... args) noexcept
{
	return snprintf(buffer, bufferCount, format, ArgumentWrapper(args)...);
}


template<typename... Args>
std::string Format(char const * const format, Args const & ... args)
{
	std::string result;
	const int size = StringPrint(nullptr, 0, format, args...);

	if (size < 1)
	{
		return result;
	}

	result.resize(static_cast<std::size_t>(size));
	StringPrint(&result[0], result.size() + 1, format, args...);

	return result;
}


#endif
