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
 * @file DataEnumType.h
 * @brief Represents a data enumeration type
 */

#ifndef OPENSAND_CONF_DATA_ENUM_TYPE_H
#define OPENSAND_CONF_DATA_ENUM_TYPE_H

#include <memory>
#include <string>
#include <vector>

#include "DataValueType.h"
#include "BaseEnum.h"


namespace OpenSANDConf
{
	class MetaEnumType;

	/**
	 * @brief String type with limited values
	 */
	class DataEnumType: public DataValueType<std::string>, public BaseEnum
	{
	public:
		friend class MetaEnumType;

		/**
		 * @brief Destructor.
		 */
		virtual ~DataEnumType();

	protected:
		/**
		 * @brief Constructor
		 *
		 * @param  id      The identifier
		 * @param  values  The enumeration values
		 */
		DataEnumType(const std::string &id, const std::vector<std::string> &values);

		/**
		 * @brief Clone the current object.
		 *
		 * @return The cloned object
		 */
		virtual std::shared_ptr<DataType> clone() const override;

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
		virtual bool check(std::string value) const override;
	};
}

#endif // OPENSAND_CONF_DATA_ENUM_TYPE_H
