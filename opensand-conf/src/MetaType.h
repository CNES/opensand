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
 * @file MetaType.h
 * @brief Represents a meta type
 */

#ifndef OPENSAND_CONF_META_TYPE_H
#define OPENSAND_CONF_META_TYPE_H

#include <memory>
#include <string>

#include "NamedElement.h"
#include "DataType.h"


namespace OpenSANDConf
{
	class DataType;

	/**
	 * @brief Generic meta type
	 */
	class MetaType: public NamedElement
	{
	public:
		friend class MetaTypesList;
		friend class MetaElement;
		friend class MetaParameter;

		/**
		 * @brief Destructor.
		 */
		virtual ~MetaType();

		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const MetaType &other) const;

		friend bool operator== (const MetaType &v1, const MetaType &v2);
		friend bool operator!= (const MetaType &v1, const MetaType &v2);

	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  id           The identifier
		 * @param  name         The name
		 * @param  description  The description
		 */
		MetaType(const std::string &id, const std::string &name, const std::string &description);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		MetaType(const MetaType &other);

		/**
		 * @brief Clone the current object.
		 *
		 * @return The cloned object
		 */
		virtual std::shared_ptr<MetaType> clone() const = 0;

		/**
		 * @brief Create a data type
		 *
		 * @return new data type
		 */
		virtual std::shared_ptr<DataType> createData() const = 0;
	};

	bool operator== (const MetaType &v1, const MetaType &v2);
	bool operator!= (const MetaType &v1, const MetaType &v2);
}

#endif // OPENSAND_CONF_META_TYPE_H
