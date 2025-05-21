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
 * @file BaseEnum.h
 * @brief Represents the base of enumeration.
 */

#ifndef OPENSAND_CONF_BASE_ENUM_H
#define OPENSAND_CONF_BASE_ENUM_H

#include <string>
#include <vector>


namespace OpenSANDConf
{
	/**
	 * @brief Base of enumeration
	 */
	class BaseEnum
	{
	public:
		/**
		 * @brief Destructor.
		 */
		virtual ~BaseEnum();

		/**
		 * @brief Get the enumeration values
		 *
		 * @return  The enumeration values
		 */
		const std::vector<std::string> &getValues() const;
		std::vector<std::string> &getMutableValues();
	protected:
		/**
		 * @brief Constructor
		 *
		 * @param values       The enumeration values
		 */
		BaseEnum(const std::vector<std::string> &values);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		BaseEnum(const BaseEnum &other);

	public:
		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const BaseEnum &other) const;

	private:
    std::vector<std::string> values;
	};
}

#endif // OPENSAND_CONF_BASE_ENUM_H
