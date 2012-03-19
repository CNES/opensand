/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under
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
 * @file lib_circular_buffer.h
 * @brief This is a circular buffer class.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef _CIRCULAR_BUFFER_FUNC_HEADER
#define _CIRCULAR_BUFFER_FUNC_HEADER

#include "lib_dama_utils.h"


/**
 * @class CircularBuffer
 * @brief Manage a circular buffer with >= 1 elem, or a buffer saving only
 *        last value
 */
class CircularBuffer
{
 private:

	/// if size = 0 --> flag = true --> only last value is saved, sum = 0
	bool m_SaveOnlyLastValue;

	int m_Size;      ///< circular buffer max size
	int m_Index;     ///< current index
	int m_NbValues;  ///< current nb of elem
	double *m_Value; ///< circular buffer array
	double m_Min;    ///< min value contained in the circular buffer
	double m_Sum;    ///< sum of all values contained in the circular buffer

 public:

	CircularBuffer(int size);
	virtual ~CircularBuffer();

	void Update(double newValue);
	double GetLastValue();
	double GetPreviousValue();
	double GetMean();
	double GetMin();
	double GetSum();
	void Debug();
};

#endif
