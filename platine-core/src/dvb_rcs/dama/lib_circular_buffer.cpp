/*
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
 * @file lib_circular_buffer.cpp
 * @brief This is a circular buffer class.
 * @author Viveris Technologies
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "lib_circular_buffer.h"

#define DBG_PACKAGE PKG_DAMA_DA
#include "platine_conf/uti_debug.h"


/**
 * Create and initialize the circular buffer
 *
 * @param BufferSize Circular buffer size
 */
CircularBuffer::CircularBuffer(int BufferSize)
{
	const char *FUNCNAME = "[CircularBuffer]";
	int i;

	if(BufferSize == 0)
	{
		m_SaveOnlyLastValue = true;
		m_Size = 1;
		UTI_INFO("%s Circular buffer size was %d --> set to %d, with only "
		         "saving last value option (sum = 0) \n",
		         FUNCNAME, BufferSize, m_Size);
	}
	else
	{
		m_SaveOnlyLastValue = false;
		m_Size = BufferSize;
	}

	m_Index = m_Size - 1;
	m_NbValues = 0;
	m_Value = (double *) malloc(m_Size * sizeof(double));
	if(this->m_Value == NULL)
	{
		UTI_ERROR("%s cannot allocate memory for circular buffer\n",
		          FUNCNAME);
		goto err_alloc;
	}

	for(i = 0; i < m_Size; i++)
	{
		m_Value[i] = 0.0;
	}

err_alloc:
	m_Sum = 0.0;
	m_Min = 65536.0;
}

/**
 * Destructor
 */
CircularBuffer::~CircularBuffer()
{
	if(this->m_Value != NULL)
		free(this->m_Value);
}


/**
 * Update the circular buffer : insert a new value
 *
 * @param Value New value to be inserted
 */
void CircularBuffer::Update(double Value)
{
	int i;
	double Min = 65536.0;

	if(this->m_Value == NULL)
	{
		UTI_ERROR("[CircularBuffer::Update] circular buffer not initialized\n");
		return;
	}

	// number of value update
	m_NbValues = MIN(m_NbValues + 1, m_Size);

	// circular buffer index update
	m_Index = (m_Index + 1) % m_Size;

	// sum calculation
	m_Sum = m_Sum - m_Value[m_Index] + Value;

	// minimum update
	// if the new value is smaller it becames the MIN
	if(Value <= m_Min)
	{
		m_Min = Value;
	}
	// otherwise, if the updated value was the MIN, one has to search for
	// the new MIN around the whole buffer
	else if(m_Value[m_Index] == m_Min)
	{
		// minimum calculation
		for(i = 0; i < m_NbValues; i++)
		{
			if(i == m_Index)
				Min = MIN(Min, Value);
			else
				Min = MIN(Min, m_Value[i]);
		}
		m_Min = Min;
	}

	// new value insertion
	m_Value[m_Index] = Value;
}

/**
 * Get the circular buffer last, i.e. one buffer turn before, value (returns 0
 * if the buffer is not fullfiled yet)
 *
 * @return Last value
 */
double CircularBuffer::GetLastValue()
{
	int Index;
	double last_value;

	Index = (m_Index + 1) % m_Size;

	if(this->m_Value == NULL)
	{
		UTI_ERROR("[CircularBuffer::GetLastValue] circular buffer not "
		          "initialized\n");
		last_value = 0;
	}
	else
		last_value = this->m_Value[Index];

	return last_value;
}

/**
 * Get the circular buffer previous, i.e. last inserted, value (returns 0 if
 * the buffer is empty)
 *
 * @return Previous value
 */
double CircularBuffer::GetPreviousValue()
{
	double previous_value;

	if(this->m_Value == NULL)
	{
		UTI_ERROR("[CircularBuffer::GetPreviousValue] circular buffer not "
		          "initialized\n");
		previous_value = 0;
	}
	else
		previous_value = this->m_Value[m_Index];

	return previous_value;
}

/**
 * Get the circular buffer mean value
 *
 * @return Mean value
 */
double CircularBuffer::GetMean()
{
	return (m_Sum / (double) m_NbValues);
}

/**
 * Get the circular buffer min value
 *
 * @return Min value
 */
double CircularBuffer::GetMin()
{
	return (m_Min);
}

/**
 * Get the circular buffer sum value
 *
 * @return Sum value
 */
double CircularBuffer::GetSum()
{
	if(m_SaveOnlyLastValue)
		return 0.0;
	else
		return m_Sum;
}

/**
 * Trace the circular buffer contents
 */
void CircularBuffer::Debug()
{
	int i;
	fprintf(stderr, "CB : Size %d Index %d NbValue %d Min %f Sum %f\n",
	        m_Size, m_Index, m_NbValues, m_Min, m_Sum);
	fprintf(stderr, "CB : ");
	if(this->m_Value != NULL)
	{
		for(i = 0; i < m_Size; i++)
			fprintf(stderr, "%4.2f ", m_Value[i]);
	}
	else
		fprintf(stderr, "null");
	fprintf(stderr, "\n");
}
