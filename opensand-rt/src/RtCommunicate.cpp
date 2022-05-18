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
 * @file RtCommunicate.cpp
 * @author Mathias ETTIINGER / <mathias.ettinger@viveris.fr>
 * @brief  Simple read and write checks for communication through file descriptors
 */


#include <unistd.h>
#include <cstring>

#include "RtCommunicate.h"


constexpr const char *MAGIC_WORD = "GO";


bool check_write(int32_t fd)
{
	const auto length = std::strlen(MAGIC_WORD);
	return write(fd, MAGIC_WORD, length) == length;
}


bool check_read(int32_t fd)
{
	const auto length = std::strlen(MAGIC_WORD);
	char data[length];
	auto received = read(fd, data, length);
	return received == length && std::strncmp(data, MAGIC_WORD, length) == 0;
}
