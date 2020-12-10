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
 * @file MetaValueType.h
 * @brief Represents a specialized meta type
 */

#ifndef OPENSAND_CONF_META_VALUE_TYPE_H
#define OPENSAND_CONF_META_VALUE_TYPE_H

#include <memory>
#include <string>

#include "MetaType.h"
#include "DataValueType.h"


namespace OpenSANDConf
{
	/**
	 * @brief Generic data type
	 */
	template <typename T>
	class MetaValueType: public MetaType
	{
	public:
		friend class MetaTypesList;

		/**
		 * @brief Destructor.
		 */
		virtual ~MetaValueType();

	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  id            The identifier
		 * @param  name          The name
		 * @param  description   The description
		 */
		MetaValueType(const std::string &id, const std::string &name, const std::string &description);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		MetaValueType(const MetaValueType<T> &other);

		/**
		 * @brief Clone the current object.
		 *
		 * @return The cloned object
		 */
		virtual std::shared_ptr<MetaType> clone() const override;

		/**
		 * @brief Create a data type.
		 *
		 * @return New data type
		 */
		virtual std::shared_ptr<DataType> createData() const override;

	public:
		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const MetaType &other) const override;
	};
}

template <typename T>
OpenSANDConf::MetaValueType<T>::MetaValueType(
		const std::string &id,
		const std::string &name,
		const std::string &description):
	OpenSANDConf::MetaType(id, name, description)
{
}

template <typename T>
OpenSANDConf::MetaValueType<T>::MetaValueType(
		const OpenSANDConf::MetaValueType<T> &other):
	OpenSANDConf::MetaType(other)
{
}

template <typename T>
OpenSANDConf::MetaValueType<T>::~MetaValueType()
{
}

template <typename T>
std::shared_ptr<OpenSANDConf::MetaType> OpenSANDConf::MetaValueType<T>::clone() const
{
	return std::shared_ptr<OpenSANDConf::MetaValueType<T>>(new OpenSANDConf::MetaValueType<T>(*this));
}

template <typename T>
std::shared_ptr<OpenSANDConf::DataType> OpenSANDConf::MetaValueType<T>::createData() const
{
	return std::shared_ptr<OpenSANDConf::DataValueType<T>>(new OpenSANDConf::DataValueType<T>(this->getId()));
}

template <typename T>
bool OpenSANDConf::MetaValueType<T>::equal(const OpenSANDConf::MetaType &other) const
{
	const OpenSANDConf::MetaValueType<T> *elt = dynamic_cast<const OpenSANDConf::MetaValueType<T> *>(&other);
	return elt != nullptr && this->OpenSANDConf::MetaType::equal(*elt);
}
#endif // OPENSAND_CONF_META_VALUE_TYPE_H
