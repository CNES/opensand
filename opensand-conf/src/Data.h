/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file Data.h
 * @brief Represents a data value
 */

#ifndef OPENSAND_CONF_DATA_H
#define OPENSAND_CONF_DATA_H

#include <memory>
#include <string>


namespace OpenSANDConf
{

class DataTypesList;
class DataType;

/**
 * @brief Generic data
 */
class Data
{
 public:
	friend class DataElement;
	friend class DataParameter;
	friend class DataList;
	friend class DataModel;
	friend class MetaModel;

	/**
	 * @brief Destructor.
	 */
	virtual ~Data();

	/**
	 * @brief Check if data is set.
	 *
	 * @return  True if data is set, false otherwise
	 */
	virtual bool isSet() const;

	/**
	 * @brief Reset data.
	 */
	virtual void reset();

	/**
	 * @brief Get the data value as string.
	 *
	 * @return  Data value as string
	 */
	virtual std::string toString() const = 0;

	/**
	 * @brief Set the data value from string.
	 *
	 * @param  val  Data value as string
	 *
	 * @return  True on success, False otherwise
	 */
	virtual bool fromString(std::string val) = 0;

	/**
	 * @brief Compare to another element
	 *
	 * @param  other  Element to compare to
	 *
	 * @return  True if elements are equals, false otherwise
	 */
	virtual bool equal(const Data &other) const;

	friend bool operator== (const Data &v1, const Data &v2);
	friend bool operator!= (const Data &v1, const Data &v2);

 protected:
	/**
	 * @brief Constructor.
	 */
	Data();

	/**
	 * @brief Clone the current object.
	 *
	 * @param  types  The new types list
	 *
	 * @return New object
	 */
	virtual std::shared_ptr<Data> clone(std::shared_ptr<DataTypesList> types) const = 0;

	/**
	 * @brief Duplicate the current object.
	 *
	 * @return New object
	 */
	virtual std::shared_ptr<Data> duplicate() const = 0;

	/**
	 * @brief Get the data type.
	 *
	 * @return The type
	 */
	virtual std::shared_ptr<const DataType> getType() const = 0;

	/**
	 * @brief Copy the data value.
	 *
	 * @param  data  The data to copy
	 *
	 * @return True on success, false otherwise
	 */
	virtual bool copy(std::shared_ptr<Data> data) = 0;

 protected:
	bool is_set;
};

bool operator== (const Data &v1, const Data &v2);
bool operator!= (const Data &v1, const Data &v2);

}

#endif // OPENSAND_CONF_DATA_H
