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
 * @file CircularBuffer.cpp
 * @brief This is a circular buffer class.
 * @author Viveris Technologies
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "CircularBuffer.h"

#include <opensand_output/Output.h>


/**
 * Create and initialize the circular buffer
 *
 * @param buffer_size Circular buffer size
 */
CircularBuffer::CircularBuffer(size_t buffer_size):
	nbr_values(0),
	sum(0),
	min_value(0)
{
	// Output Log
	this->log_circular_buffer = Output::registerLog(LEVEL_WARNING,
	                                                "Dvb.CircularBuffer");

	if(buffer_size == 0)
	{
		this->save_only_last_value = true;
		this->size = 1;
		LOG(this->log_circular_buffer, LEVEL_NOTICE,
		    "Circular buffer size was %zu --> set to %zu, with "
		    " only saving last value option (sum = 0)\n",
		    buffer_size, this->size);
	}
	else
	{
		this->save_only_last_value = false;
		this->size = buffer_size;
	}

	this->index = this->size - 1;
	this->nbr_values = 0;
	this->values = (rate_kbps_t *) calloc(this->size, sizeof(rate_kbps_t));
	if(this->values == NULL)
	{
		LOG(this->log_circular_buffer, LEVEL_ERROR,
		    "cannot allocate memory for circular buffer\n");
		goto err_alloc;
	}

err_alloc:
	return;
}

/**
 * Destructor
 */
CircularBuffer::~CircularBuffer()
{
	if(this->values != NULL)
		free(this->values);
}


/**
 * Update the circular buffer : insert a new value
 *
 * @param value New value to be inserted
 */
void CircularBuffer::Update(rate_kbps_t value)
{
	size_t i;
	rate_kbps_t min = 0;

	if(this->values == NULL)
	{
		LOG(this->log_circular_buffer, LEVEL_ERROR,
		    "circular buffer not initialized\n");
		return;
	}

	// number of value update
	this->nbr_values = std::min(this->nbr_values + 1, this->size);

	// circular buffer index update
	this->index = (this->index + 1) % this->size;

	// sum calculation
	this->sum = this->sum - this->values[this->index] + value;

	// minimum update
	// if the new value is smaller it becames the MIN
	if(value <= this->min_value)
	{
		this->min_value = value;
	}
	// otherwise, if the updated value was the MIN, one has to search for
	// the new MIN around the whole buffer
	else if(this->values[this->index] == this->min_value)
	{
		// minimum calculation
		for(i = 0; i < this->nbr_values; i++)
		{
			if(i == this->index)
				min = std::min(min, value);
			else
				min = std::min(min, this->values[i]);
		}
		this->min_value = min;
	}

	// new value insertion
	this->values[this->index] = value;
}

/**
 * Get the circular buffer last, i.e. one buffer turn before, value (returns 0
 * if the buffer is not fullfiled yet)
 *
 * @return Last value
 */
rate_kbps_t CircularBuffer::GetLastValue()
{
	size_t index;
	rate_kbps_t last_value;

	index = (this->index + 1) % this->size;

	if(this->values == NULL)
	{
		LOG(this->log_circular_buffer, LEVEL_ERROR,
		    "circular buffer not initialized\n");
		last_value = 0;
	}
	else
		last_value = this->values[index];

	return last_value;
}

/**
 * Get the circular buffer previous, i.e. last inserted, value (returns 0 if
 * the buffer is empty)
 *
 * @return Previous value
 */
rate_kbps_t CircularBuffer::GetPreviousValue()
{
	rate_kbps_t previous_value;

	if(this->values == NULL)
	{
		LOG(this->log_circular_buffer, LEVEL_ERROR,
		    "circular buffer not initialized\n");
		previous_value = 0;
	}
	else
		previous_value = this->values[this->index];

	return previous_value;
}

/**
 * Get the circular buffer mean value
 *
 * @return Mean value
 */
rate_kbps_t CircularBuffer::GetMean()
{
	return (this->sum / this->nbr_values);
}

/**
 * Get the circular buffer min value
 *
 * @return Min value
 */
rate_kbps_t CircularBuffer::GetMin()
{
	return (this->min_value);
}

/**
 * Get the circular buffer sum value
 *
 * @return Sum value
 */
rate_kbps_t CircularBuffer::GetSum()
{
	if(this->save_only_last_value)
		return 0;
	else
		return this->sum;
}

/**
 * Get the sum of a part of the value stored in the circular buffer starting
 * from the newest value (return 0 if the buffer is empty)
 *
 * @return the partial sum
 */
rate_kbps_t CircularBuffer::GetPartialSumFromPrevious(int value_number)
{
	rate_kbps_t partial_sum_kbps = 0;
	if (this->values == NULL)
		LOG(this->log_circular_buffer, LEVEL_ERROR,
		    "circular buffer not initialized\n");
	else
	{   
		for(int i = 0; i < value_number; i++) 
			partial_sum_kbps += (this->GetValueIndex(-i));
	}   
	return partial_sum_kbps;
}

/**
 * Get the value at index (return 0 if the buffer is empty)
 *
 * @return value at index
 */
rate_kbps_t CircularBuffer::GetValueIndex(int i)
{
	double value;
	if(this->values == NULL)
	{
		LOG(this->log_circular_buffer, LEVEL_ERROR,
		    "circular buffer not initialized\n");
		value = 0;
	}     
	else
	{
		i = (i + this->index) % this->size;
		value = this->values[i];
	}
	return value;
}

/**
 * Trace the circular buffer contents
 */
void CircularBuffer::Debug()
{
	size_t i;
	fprintf(stderr, "CB : size %zu index %zu nbr_alues %zu min_value %u sum %u\n",
	        this->size, this->index, this->nbr_values, this->min_value, this->sum);
	fprintf(stderr, "CB : ");
	if(this->values != NULL)
	{
		for(i = 0; i < this->size; i++)
			fprintf(stderr, "%u ", this->values[i]);
	}
	else
		fprintf(stderr, "null");
	fprintf(stderr, "\n");
}
