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
 * @file MetaTypesList.h
 * @brief Represents a list of meta types.
 */

#ifndef OPENSAND_CONF_META_TYPES_LIST_H
#define OPENSAND_CONF_META_TYPES_LIST_H

#include <memory>
#include <string>
#include <map>
#include <vector>


namespace OpenSANDConf
{
	class MetaType;
	class MetaEnumType;
	class DataTypesList;

	/**
	 * @brief Represents a list of types.
	 */
	class MetaTypesList
	{
	public:
		friend class MetaModel;

		/**
		 * @brief Desctructor.
		 */
		 ~MetaTypesList();

		/**
		 * @brief Compare to another object.
		 *
		 * @param  other  Object to compare to
		 *
		 * @return  True if objects are equals, false otherwise
		 */
		bool equal(const MetaTypesList &other) const;

		/**
		* @brief Get all types.
		*
		* @return  The all types
		*/
		std::vector<std::shared_ptr<MetaType>> getTypes() const;

		/**
		* @brief Get a type.
		*
		* @param  id  The type identifier
		*
		* @return  The type if found, nullptr otherwise
		*/
		std::shared_ptr<MetaType> getType(const std::string &id) const;

		/**
		* @brief Get enumeration types.
		*
		* @return  The enumeration types
		*/
		std::vector<std::shared_ptr<MetaEnumType>> getEnumTypes() const;

		/**
		* @brief Add a new enumeration
		*
		* @param  id           The enumeration identifier
		* @param  name         The enumeration name
		* @param  values       The enumeration values
		*
		* @return  The newly created enumeration on success, nullptr otherwise
		*/
		std::shared_ptr<MetaEnumType> addEnumType(const std::string &id, const std::string &name, const std::vector<std::string> &values);

		/**
		* @brief Add a new enumeration
		*
		* @param  id           The enumeration identifier
		* @param  name         The enumeration name
		* @param  values       The enumeration values
		* @param  description  The enumeration description
		*
		* @return  The newly created enumeration on success, nullptr otherwise
		*/
		std::shared_ptr<MetaEnumType> addEnumType(const std::string &id, const std::string &name, const std::vector<std::string> &values, const std::string &description);

	protected:
		/**
		 * @brief Constructor.
		 */
		MetaTypesList();

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		MetaTypesList(const MetaTypesList &other);

		/**
		 * @brief Clone the current meta types list
		 *
		 * @return The new meta types list
		 */
		std::shared_ptr<MetaTypesList> clone() const;

		/**
		 * @brief Create the data types list.
		 *
		 * @return The new data types list
		 */
		std::shared_ptr<DataTypesList> createData() const;
		
	private:
		std::map<std::string, std::shared_ptr<MetaType>> types;
		std::map<std::string, std::shared_ptr<MetaType>> enums;

		/**
		 * @brief Add a type to a types map.
		 *
		 * @param  types  The types map
		 * @param  type   The type to add
		 */
		static void addToMap(std::map<std::string, std::shared_ptr<MetaType>> &types, std::shared_ptr<MetaType> type);
	};
}

#endif // OPENSAND_CONF_META_TYPES_LIST_H
