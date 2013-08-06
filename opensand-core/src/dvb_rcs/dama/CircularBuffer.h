/*
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
 * @file CicrularBuffer.h
 * @brief This is a circular buffer class.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include "OpenSandCore.h"

/**
 * @class CircularBuffer
 * @brief Manage a circular buffer with >= 1 elem, or a buffer saving only
 *        last value
 */
class CircularBuffer
{
 private:

	/// if size = 0 --> flag = true --> only last value is saved, sum = 0
	bool save_only_last_value;

	size_t size;              ///< circular buffer max size
	size_t index;       ///< current index
	size_t nbr_values;  ///< current nb of elem
	rate_kbps_t sum;    ///< sum of all values contained in the circular buffer
	rate_kbps_t min_value;    ///< min value contained in the circular buffer
	rate_kbps_t *values; ///< circular buffer array

 public:

	CircularBuffer(size_t buffer_size);
	virtual ~CircularBuffer();

	void Update(rate_kbps_t new_value);
	rate_kbps_t GetLastValue();
	rate_kbps_t GetPreviousValue();
	rate_kbps_t GetMean();
	rate_kbps_t GetMin();
	rate_kbps_t GetSum();
	void Debug();
};

#endif
