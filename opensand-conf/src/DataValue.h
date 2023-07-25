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
 * @file DataValue.h
 * @brief Represents a specialized data value
 */

#ifndef OPENSAND_CONF_DATA_VALUE_H
#define OPENSAND_CONF_DATA_VALUE_H

#include <memory>
#include <string>
#include <sstream>

#include "Data.h"
#include "DataValueType.h"
#include "DataTypesList.h"


namespace OpenSANDConf
{
	template <typename T>
	class DataValueType;

	template <typename T>
	class DataValue: public Data
	{
	public:
		friend class DataValueType<T>;

		/**
		 * @brief Destructor.
		 */
		virtual ~DataValue();

		/**
		 * @brief Get the data value as string.
		 *
		 * @return  Data value as string
		 */
		virtual std::string toString() const;

		/**
		 * @brief Set the data value from string.
		 *
		 * @param  val  Data value as string
		 *
		 * @return  True on success, False otherwise
		 */
		virtual bool fromString(std::string val);

		/**
		 * @brief Get the data value.
		 *
		 * @return  Data value
		 */
		virtual T get() const;

		/**
		 * @brief Set the data value.
		 *
		 * @param  val  Data value
		 *
		 * @return  True on success, False otherwise
		 */
		virtual bool set(T val);

	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  type  The data type
		 */
		DataValue(std::weak_ptr<const DataValueType<T>> type);

		/**
		 * @brief Clone the current object.
		 *
		 * @param  types  The new types list
		 *
		 * @return New object
		 */
		virtual std::shared_ptr<Data> clone(std::shared_ptr<DataTypesList> types) const;

		/**
		 * @brief Duplicate the current object.
		 *
		 * @return New object
		 */
		virtual std::shared_ptr<Data> duplicate() const;

		/**
		 * @brief Get the data type.
		 *
		 * @return The type
		 */
		virtual std::shared_ptr<const DataType> getType() const override;

		/**
		 * @brief Copy the data value.
		 *
		 * @param  data  The data to copy
		 *
		 * @return True on success, false otherwise
		 */
		virtual bool copy(std::shared_ptr<Data> data) override;

	public:
		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const Data &other) const override;

	private:
		std::weak_ptr<const DataValueType<T>> type;
		T value;
	};
}

template <typename T>
OpenSANDConf::DataValue<T>::DataValue(std::weak_ptr<const DataValueType<T>> type):
	Data(),
	type(type)
{
}

template <typename T>
OpenSANDConf::DataValue<T>::~DataValue()
{
}

template <typename T>
std::shared_ptr<OpenSANDConf::Data> OpenSANDConf::DataValue<T>::clone(std::shared_ptr<DataTypesList> types) const
{
	auto type = std::static_pointer_cast<DataValueType<T>>(types->getType(this->type.lock()->getId()));
	if(type == nullptr)
	{
		return nullptr;
	}
	auto data = std::shared_ptr<OpenSANDConf::DataValue<T>>(new OpenSANDConf::DataValue<T>(type));
	if(this->isSet())
	{
		data->set(this->get());
	}
	return data;
}

template <typename T>
std::shared_ptr<OpenSANDConf::Data> OpenSANDConf::DataValue<T>::duplicate() const
{
	auto data = std::shared_ptr<OpenSANDConf::DataValue<T>>(new OpenSANDConf::DataValue<T>(this->type));
	if(this->isSet())
	{
		data->set(this->get());
	}
	return data;
}

template <typename T>
std::shared_ptr<const OpenSANDConf::DataType> OpenSANDConf::DataValue<T>::getType() const
{
	return this->type.lock();
}

template <typename T>
bool OpenSANDConf::DataValue<T>::copy(std::shared_ptr<Data> data)
{
	auto original = std::dynamic_pointer_cast<OpenSANDConf::DataValue<T>>(data);
	if(original == nullptr)
	{
		return false;
	}
	this->set(original->get());
	return true;
}

template <typename T>
std::string OpenSANDConf::DataValue<T>::toString() const
{
	std::stringstream ss;
	if(this->is_set)
	{
		ss << std::fixed << std::boolalpha << this->value;
	}
	return ss.str();
}

template <typename T>
bool OpenSANDConf::DataValue<T>::fromString(std::string val)
{
	std::stringstream ss(val);
	T tmp;
	ss >> std::boolalpha >> tmp;
	return (!ss.fail()) && this->set(tmp);
}

template <typename T>
T OpenSANDConf::DataValue<T>::get() const
{
	return this->value;
}

template <typename T>
bool OpenSANDConf::DataValue<T>::set(T val)
{
	if(!this->type.lock()->check(val))
	{
		return false;
	}
	this->value = val;
	this->is_set = true;
	return true;
}

template <typename T>
bool OpenSANDConf::DataValue<T>::equal(const OpenSANDConf::Data &other) const
{
	auto elt = dynamic_cast<const OpenSANDConf::DataValue<T> *>(&other);
	if(elt == nullptr || !this->OpenSANDConf::Data::equal(*elt))
	{
		return false;
	}
	{
		auto thistype = this->type.lock();
		auto othertype = elt->type.lock();
		if(*thistype != *othertype)
		{
			return false;
		}
	}
	return this->is_set ? this->value == elt->value : true;
}

template <>
bool OpenSANDConf::DataValue<std::string>::fromString(std::string val);

template <>
bool OpenSANDConf::DataValue<uint8_t>::fromString(std::string val);

template <>
bool OpenSANDConf::DataValue<int8_t>::fromString(std::string val);

template <>
std::string OpenSANDConf::DataValue<uint8_t>::toString() const;

template <>
std::string OpenSANDConf::DataValue<int8_t>::toString() const;

#endif // OPENSAND_CONF_DATA_VALUE_H
