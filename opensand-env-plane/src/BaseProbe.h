/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 CNES
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
 * @file BaseProbe.h
 * @brief The BaseProbe class represents an untyped probe (base class for Probe<T> classes).
 */

#ifndef _BASE_PROBE_H
#define _BASE_PROBE_H

#include <stdint.h>

#include <string>

class EnvPlaneInternal;

enum sample_type
{
	SAMPLE_LAST,
	SAMPLE_MIN,
	SAMPLE_MAX,
	SAMPLE_AVG,
	SAMPLE_SUM
};

class BaseProbe
{
	friend class EnvPlaneInternal;

public:
	inline bool is_enabled() const { return this->enabled; };
	inline const char* name() const { return this->_name; };
	inline const char* unit() const { return this->_unit; };

protected:
	BaseProbe(uint8_t id, const char* name, const char* unit, bool enabled, sample_type type);
	virtual ~BaseProbe();
	
	virtual uint8_t storage_type_id() = 0;
	virtual void append_value_and_reset(std::string& str) = 0;

	uint8_t id;
	char* _name;
	char* _unit;
	sample_type s_type;
	bool enabled;
	
	uint16_t values_count;
};

#endif
