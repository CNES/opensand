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
 * @file MetaEnumType.h
 * @brief Represents a meta enumeration type
 */

#ifndef OPENSAND_CONF_META_ENUM_TYPE_H
#define OPENSAND_CONF_META_ENUM_TYPE_H

#include <memory>
#include <string>
#include <vector>

#include "MetaValueType.h"
#include "BaseEnum.h"


namespace OpenSANDConf
{
	/**
	 * @brief String type with limited values
	 */
	class MetaEnumType: public MetaValueType<std::string>, public BaseEnum
	{
	public:
		friend class MetaTypesList;

		/**
		 * @brief Destructor.
		 */
		virtual ~MetaEnumType();

	protected:
		/**
		 * @brief Constructor
		 *
		 * @param  id           The identifier
		 * @param  name         The name
		 * @param  description  The description
		 * @param  values       The enumeration values
		 */
		MetaEnumType(const std::string &id, const std::string &name, const std::string &description, const std::vector<std::string> &values);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		MetaEnumType(const MetaEnumType &other);

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

#endif // OPENSAND_CONF_META_ENUM_TYPE_H
