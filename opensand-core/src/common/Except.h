/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
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
 * @file Except.h
 * @brief User-defined OpenSAND exceptions
 */

#ifndef OPENSAND_EXCEPT_H
#define OPENSAND_EXCEPT_H


#include <stdexcept>


struct NotImplementedError: std::runtime_error
{
	NotImplementedError(const std::string& method);
};


struct BadPrecondition: std::runtime_error
{
	using std::runtime_error::runtime_error;
};


void ASSERT(bool condition, const std::string &error_message);


#endif
