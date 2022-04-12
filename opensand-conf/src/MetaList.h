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
 * @file MetaList.h
 * @brief Represents a metamodel list
 *        (holds list items following a pattern).
 */

#ifndef OPENSAND_CONF_META_LIST_H
#define OPENSAND_CONF_META_LIST_H

#include <memory>
#include <string>

#include "MetaContainer.h"


namespace OpenSANDConf
{
	class MetaComponent;

	class MetaList: public MetaContainer
	{
	public:
		friend class MetaComponent;

		/**
		 * @brief Destructor.
		 */
		virtual ~MetaList();

		/**
		* @brief Get the list's pattern.
		*
		* @return  The list's pattern
		*/
    std::shared_ptr<MetaComponent> getPattern() const;

	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  id           The identifier
		 * @param  parent       The parent path
		 * @param  name         The name
		 * @param  description  The description
		 * @param  pattern      The pattern
		 * @param  types        The types list
		 */
		MetaList(const std::string &id, const std::string &parent, const std::string &name, const std::string &description, std::shared_ptr<MetaComponent> pattern, std::weak_ptr<const MetaTypesList> types);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 * @param  types  The types list
		 */
		MetaList(const MetaList &other, std::weak_ptr<const MetaTypesList> types);

		/**
		 * @brief Clone the current object.
		 *
		 * @param  types  The types list
		 *
		 * @return The cloned object
		 */
		virtual std::shared_ptr<MetaElement> clone(std::weak_ptr<const MetaTypesList> types) const override;

		/**
		 * @brief Create a datamodel element.
		 *
		 * @param  types  The types list
		 *
		 * @return  The new datamodel element if succeeds, nullptr otherwise
		 */
		virtual std::shared_ptr<DataElement> createData(std::shared_ptr<DataTypesList> types) const override;

	public:
		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const MetaElement &other) const override;
	};
}

#endif // OPENSAND_CONF_META_LIST_H
