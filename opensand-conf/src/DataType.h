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
 * @file DataType.h
 * @brief Represents a data type
 */

#ifndef OPENSAND_CONF_DATA_TYPE_H
#define OPENSAND_CONF_DATA_TYPE_H

#include <memory>
#include <string>

#include "BaseElement.h"


namespace OpenSANDConf
{
	class Data;

	/**
	 * @brief Generic data type
	 */
	class DataType: public BaseElement
	{
	public:
		friend class MetaParameter;
		friend class MetaElement;
		friend class DataParameter;
		friend class DataTypesList;

		/**
		 * @brief Destructor.
		 */
		virtual ~DataType();

		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const DataType &other) const;

		friend bool operator== (const DataType &v1, const DataType &v2);
		friend bool operator!= (const DataType &v1, const DataType &v2);
  
	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  id  The identifier
		 */
		DataType(const std::string &id);

		/**
		 * @brief Clone the current object.
		 *
		 * @return The cloned object
		 */
		virtual std::shared_ptr<DataType> clone() const = 0;

		/**
		 * @brief Create a data
		 *
		 * @return New data
		 */
		virtual std::shared_ptr<Data> createData() const = 0;
	};

	bool operator== (const DataType &v1, const DataType &v2);
	bool operator!= (const DataType &v1, const DataType &v2);
}

#endif // OPENSAND_CONF_DATA_TYPE_H
