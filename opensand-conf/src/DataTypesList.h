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
 * @file DataTypesList.h
 * @brief Represents a list of data types.
 */

#ifndef OPENSAND_CONF_DATA_TYPES_LIST_H
#define OPENSAND_CONF_DATA_TYPES_LIST_H

#include <memory>
#include <string>
#include <map>


namespace OpenSANDConf
{
	class DataType;
	class DataEnumType;

	/**
	 * @brief Represents a list of types.
	 */
	class DataTypesList
	{
	public:
		friend class MetaTypesList;
		friend class DataModel;

		/**
		 * @brief Desctructor.
		 */
		 ~DataTypesList();

		/**
		 * @brief Compare to another object.
		 *
		 * @param  other  Object to compare to
		 *
		 * @return  True if objects are equals, false otherwise
		 */
		bool equal(const DataTypesList &other) const;

		friend bool operator== (const DataTypesList &v1, const DataTypesList &v2);
		friend bool operator!= (const DataTypesList &v1, const DataTypesList &v2);

		/**
		* @brief Get a type.
		*
		* @param  id  The type identifier
		*
		* @return  The type if found, nullptr otherwise
		*/
		std::shared_ptr<DataType> getType(const std::string &id) const;

	protected:
		/**
		 * @brief Constructor.
		 */
		DataTypesList();

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		DataTypesList(const DataTypesList &other);

		/**
		 * @brief Clone the current object.
		 *
		 * @return Cloned object
		 */
		std::shared_ptr<DataTypesList> clone() const;

		/**
		 * @brief Add a new type.
		 *
		 * @param  type  The type to add
		 *
		 * @return True on success, false otherwise
		 */
		bool addType(std::shared_ptr<DataType> type);

	private:
		std::map<std::string, std::shared_ptr<DataType>> types;
	};

	bool operator== (const DataTypesList &v1, const DataTypesList &v2);
	bool operator!= (const DataTypesList &v1, const DataTypesList &v2);
}

#endif // OPENSAND_CONF_DATA_TYPES_LIST_H
