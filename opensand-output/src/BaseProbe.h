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

#include <string>


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

enum datatype_t {
	INT32_TYPE = 0,
	FLOAT_TYPE = 1,
	DOUBLE_TYPE = 2
};


/**
 * @class the probe representation
 */
class BaseProbe
{
	friend class Output;

public:

	virtual ~BaseProbe();

  /**
   * @brief Enable or disable the probe
   **/
  void enable(bool enabled);

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
	inline const std::string getName() const { return this->name; };

	/**
	 * @brief Get the unit of the probe
	 *
	 * @return the unit of the probe
	 **/
	inline const std::string getUnit() const { return this->unit; };

	/**
	 * @brief get the byte size of data
	 *
	 * @return the size of data
	 **/
	virtual size_t getDataSize() const = 0;
	
	/**
	 * @brief get data in byte
	 *
	 * @return data
	 **/
	virtual std::string getData() const = 0;

	/**
	 * @brief get data type
	 *
	 * @return data type
	 */
	virtual datatype_t getDataType() const = 0;

	/**
	 * @brief reset values count
	 *
	 **/
	void reset();

  inline bool isEmpty() const { return values_count == 0; };

protected:
	BaseProbe(const std::string &name, const std::string& unit, bool enabled, sample_type_t sample_type);

  std::string name;
	std::string unit;
	bool enabled;
  sample_type_t s_type;

	/// the number of values in probe
	uint16_t values_count;
};

#endif
