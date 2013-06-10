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
 * @file BaseProbe.h
 * @brief The BaseProbe class represents an untyped probe
 *        (base class for Probe<T> classes).
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */

#ifndef _BASE_PROBE_H
#define _BASE_PROBE_H

#include <stdint.h>

#include <string>

using std::string;

class OutputInternal;

/**
 * @brief Probe sample type
 **/
enum sample_type_t
{
	SAMPLE_LAST,   /*!< Keep the last value */
	SAMPLE_MIN,    /*!< Keep the minimum value */
	SAMPLE_MAX,    /*!< Keep the maximum value */
	SAMPLE_AVG,    /*!< Calculate the average */
	SAMPLE_SUM     /*!< Calculate the sum */
};

/**
 * @class the probe representation
 */
class BaseProbe
{
	friend class OutputInternal;

public:

	/**
	 * @brief Check if the probe is enabled
	 *
	 * @return true if the probe is currently enabled
	 **/
	inline bool isEnabled() const { return this->enabled; };

	/**
	 * @brief Get the name of the probe
	 *
	 * @return the name of the probe
	 **/
	inline const string getName() const { return this->name; };

	/**
	 * @brief Get the unit of the probe
	 *
	 * @return the unit of the probe
	 **/
	inline const string getUnit() const { return this->unit; };

protected:
	BaseProbe(uint8_t id, const string &name,
	          const string &unit,
	          bool enabled, sample_type_t type);
	virtual ~BaseProbe();

	/**
	 * @brief get the storage type ID
	 *
	 * @return the sorage type ID
	 */
	virtual uint8_t storageTypeId() = 0;

	/**
	 * @brief Add a value in probe
	 *
	 * @para str the value
	 */
	virtual void appendValueAndReset(string &str) = 0;

	/// the probe ID
	uint8_t id;
	/// the probe name
	string name;
	/// the probe unit
	string unit;
	/// whether the probe is enabled
	bool enabled;
	/// the probe sample type
	sample_type_t s_type;

	/// the number of values in probe
	uint16_t values_count;
};

#endif
