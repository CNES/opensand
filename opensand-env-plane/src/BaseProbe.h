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

/**
 * @brief Probe sample type
 **/
enum sample_type
{
	SAMPLE_LAST,	/*!< Keep the last value */
	SAMPLE_MIN,		/*!< Keep the minimum value */
	SAMPLE_MAX,		/*!< Keep the maximum value*/
	SAMPLE_AVG,		/*!< Calculate the average */
	SAMPLE_SUM		/*!< Calculate the sum */
};

class BaseProbe
{
	friend class EnvPlaneInternal;

public:
	
	/**
	 * @brief Return true if the probe is currently enabled
	 **/
	inline bool is_enabled() const { return this->enabled; };
	
	/**
	 * @brief Return the name of the probe
	 **/
	inline const char* get_name() const { return this->name; };
	
	/**
	 * @brief Return the unit of the probe
	 **/
	inline const char* get_unit() const { return this->unit; };

protected:
	BaseProbe(uint8_t id, const char* name, const char* unit, bool enabled, sample_type type);
	virtual ~BaseProbe();
	
	virtual uint8_t storage_type_id() = 0;
	virtual void append_value_and_reset(std::string& str) = 0;

	uint8_t id;
	char* name;
	char* unit;
	sample_type s_type;
	bool enabled;
	
	uint16_t values_count;
};

#endif
