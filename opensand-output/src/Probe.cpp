/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file Probe.h
 * @brief Template specialization of the storage type ID for Probe<T> classes.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */

#include <Probe.h>

#include <arpa/inet.h>
#include <endian.h>
#include <string.h>


template<>
uint8_t Probe<int32_t>::storageTypeId()
{
	return 0;
}

template<>
uint8_t Probe<float>::storageTypeId()
{
	return 1;
}

template<>
uint8_t Probe<double>::storageTypeId()
{
	return 2;
}

template<>
void Probe<int32_t>::appendValueAndReset(std::string& str)
{
	int32_t accumulator = this->accumulator;
	uint32_t value;

	if(this->s_type == SAMPLE_AVG)
	{
		accumulator /= this->values_count;
	}

	value = accumulator;
	value = htonl(value);
	str.append((char*)&value, sizeof(value));

	this->values_count = 0;
}

template<>
void Probe<float>::appendValueAndReset(std::string& str)
{
	float accumulator = this->accumulator;
	uint32_t value;

	if(this->s_type == SAMPLE_AVG)
	{
		accumulator /= this->values_count;
	}

	memcpy(&value, &accumulator, sizeof(value));
	value = htonl(value);
	str.append((char*)&value, sizeof(value));

	this->values_count = 0;
}

template<>
void Probe<double>::appendValueAndReset(std::string& str)
{
	double accumulator = this->accumulator;
	uint64_t value;

	if(this->s_type == SAMPLE_AVG)
	{
		accumulator /= this->values_count;
	}

	memcpy(&value, &accumulator, sizeof(value));
	value = htobe64(value);
	str.append((char*)&value, sizeof(value));

	this->values_count = 0;
}
