/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 Viveris Technologies
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
 * @file DataValueType.h
 * @brief Represents a specialized data type
 */

#ifndef OPENSAND_CONF_DATA_VALUE_TYPE_H
#define OPENSAND_CONF_DATA_VALUE_TYPE_H

#include <memory>
#include <string>

#include "DataType.h"
#include "DataValue.h"
#include "DataTypesList.h"

/** unused macro to avoid compilation warning with unused parameters. */
#ifdef __GNUC__
#  define UNUSED(x) x __attribute__((unused))
#elif __LCLINT__
#  define UNUSED(x) /*@unused@*/ x
#else               /* !__GNUC__ && !__LCLINT__ */
#  define UNUSED(x) x
#endif              /* !__GNUC__ && !__LCLINT__ */


namespace OpenSANDConf
{
	template <typename T>
	class DataValue;

	template <typename T>
	class MetaValueType;

	template <typename T>
	class DataValueType;

	/**
	 * @brief Generic data type
	 */
	template <typename T>
	class DataValueType: public std::enable_shared_from_this<DataValueType<T>>, public DataType
	{
	public:
		friend class MetaValueType<T>;

		/**
		 * @brief Destructor.
		 */
		virtual ~DataValueType();

	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  id  The identifier
		 */
		DataValueType(const std::string &id);

		/**
		 * @brief Clone the current object.
		 *
		 * @return The cloned object
		 */
		virtual std::shared_ptr<DataType> clone() const override;

		/**
		 * @brief Create a data.
		 *
		 * @return New data
		 */
		virtual std::shared_ptr<Data> createData() const override;

	public:
		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const DataType &other) const override;

		/**
		 * @brief Check a value matches the current type
		 *
		 * @param  value  The value to check
		 *
		 * @return  True if the data matches the current type, false otherwise
		 */
		virtual bool check(T value) const;
	};
}

template <typename T>
OpenSANDConf::DataValueType<T>::DataValueType(const std::string &id):
	OpenSANDConf::DataType(id)
{
}

template <typename T>
OpenSANDConf::DataValueType<T>::~DataValueType()
{
}

template <typename T>
std::shared_ptr<OpenSANDConf::DataType> OpenSANDConf::DataValueType<T>::clone() const
{
	return std::shared_ptr<OpenSANDConf::DataValueType<T>>(new OpenSANDConf::DataValueType<T>(this->getId()));
}

template <typename T>
std::shared_ptr<OpenSANDConf::Data> OpenSANDConf::DataValueType<T>::createData() const
{
	return std::shared_ptr<OpenSANDConf::DataValue<T>>(new OpenSANDConf::DataValue<T>(this->shared_from_this()));
}

template <typename T>
bool OpenSANDConf::DataValueType<T>::equal(const OpenSANDConf::DataType &other) const
{
	const OpenSANDConf::DataValueType<T> *elt = dynamic_cast<const OpenSANDConf::DataValueType<T> *>(&other);
	return elt != nullptr && this->OpenSANDConf::DataType::equal(*elt);
}

template <typename T>
bool OpenSANDConf::DataValueType<T>::check(T UNUSED(value)) const
{
	return true;
}

#endif // OPENSAND_CONF_DATA_VALUE_TYPE_H
