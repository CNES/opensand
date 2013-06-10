/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file Data.h
 * @brief A set of data for network packets
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef DATA_H
#define DATA_H

#include <string>


/**
 * @class Data
 * @brief A set of data for network packets
 */
class Data: public std::basic_string<unsigned char>
{
 public:

	/**
	 * Create an empty set of data
	 */
	Data();

	/**
	 * Create a set of data from a string of unsigned characters
	 *
	 * @param string  the string of unsigned characters
	 */
	Data(std::basic_string<unsigned char> string);

	/**
	 * Create a set of data from unsigned characters
	 *
	 * @param data  the unsigned characters to copy
	 * @param len   the number of unsigned characters to copy
	 */
	Data(unsigned char *data, unsigned int len);

	/**
	 * Create a set of data from a subset of data
	 *
	 * @param data  the set of data to copy data from
	 * @param pos   the index of first byte to copy from
	 * @param len   the number of bytes to copy
	 */
	Data(Data data, unsigned int pos, unsigned int len);

};

#endif
