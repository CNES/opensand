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
 * @brief The Probe<T> class represents a probe of a defined type.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#ifndef _PROBE_H
#define _PROBE_H

#include "BaseProbe.h"

#include <algorithm>
#include <string>
#include <iostream>

#include <stdint.h>

/**
 * @class the probe respresentation
 */
template<typename T>
class Probe : public BaseProbe
{
	friend class OutputInternal;

public:
	/**
	 * @brief adds a value to the probe, to be sent when \send_probes is called.
	 *
	 * @param value The value to add to the probe
	 **/
	void put(T value);

private:
	Probe(uint8_t id, const char* name, const char* unit,
	      bool enabled, sample_type_t type);
	
	virtual uint8_t storageTypeId();

	virtual void appendValueAndReset(std::string& str);

	/// the concatenation of all values
	T accumulator;
};

template<typename T>
Probe<T>::Probe(uint8_t id, const char* name, const char* unit,
                bool enabled, sample_type_t type):
	BaseProbe(id, name, unit, enabled, type)
{
}

template<typename T>
void Probe<T>::put(T value)
{
	if(this->values_count == 0)
	{
		this->accumulator = value;
	
	}
	else
	{
		switch (this->s_type)
		{
			case SAMPLE_LAST:
				this->accumulator = value;
			break;
			
			case SAMPLE_MIN:
				this->accumulator = std::min(this->accumulator, value);
			break;
			
			case SAMPLE_MAX:
				this->accumulator = std::max(this->accumulator, value);
			break;
			
			case SAMPLE_AVG:
			case SAMPLE_SUM:
				this->accumulator += value;
			break;
		}
	}
	
	this->values_count++;
}

#endif
