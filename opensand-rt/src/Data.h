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
 * @file Data.h
 * @author Mathias ETTINGER / <mathias.ettinger@viveris.fr>
 * @brief  Buffer data type for opensand-rt
 */


#ifndef DATA_H
#define DATA_H


#include <string>
#include <sstream>


namespace Rt
{
using DataStream = std::basic_stringstream<unsigned char>;
using ODataStream = std::basic_ostringstream<unsigned char>;
using IDataStream = std::basic_istringstream<unsigned char>;
using Data = std::basic_string<unsigned char>;
};


#endif
