/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
bool Probe<int32_t>::getData(unsigned char* buffer, size_t len) const
{
	if(!buffer || this->Probe<int32_t>::getDataSize() < len)
	{
		return false;
	}
	
	int32_t value = this->Probe<int32_t>::get();
	uint32_t data;
	memcpy(&data, &value, sizeof(value));
	data = htonl(data);
	memcpy(buffer, &data, sizeof(data));

	return true;
}

template<>
bool Probe<float>::getData(unsigned char* buffer, size_t len) const
{
	if(!buffer || this->Probe<float>::getDataSize() < len)
	{
		return false;
	}
	
	float value = this->Probe<float>::get();
	uint32_t data;
	memcpy(&data, &value, sizeof(value));
	data = htonl(data);
	memcpy(buffer, &data, sizeof(data));

	return true;
}

template<>
bool Probe<double>::getData(unsigned char* buffer, size_t len) const
{
	if(!buffer || this->Probe<double>::getDataSize() < len)
	{
		return false;
	}
	
	double value = this->Probe<double>::get();
	uint64_t data;
	memcpy(&data, &value, sizeof(value));
	data = htobe64(data);
	memcpy(buffer, &data, sizeof(data));

	return true;
}
