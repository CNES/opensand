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
 * @file NamedElement.h
 * @brief Base class of all described elements.
 */

#ifndef OPENSAND_CONF_NAMED_ELEMENT_H
#define OPENSAND_CONF_NAMED_ELEMENT_H

#include <string>

#include "BaseElement.h"


namespace OpenSANDConf
{
	/**
	 * @brief Base class of all described elements.
	 */
	class NamedElement: public BaseElement
	{
	public:
		/**
		 * @brief Destructor.
		 */
		virtual ~NamedElement();

		/**
		 * @brief Get the named element's name.
		 *
		 * @return  The named element's name
		 */
		const std::string &getName() const;

		/**
		 * @brief Get the named element's description.
		 *
		 * @return The named element's description
		 */
		const std::string &getDescription() const;

		/**
		 * @brief Set the named element's description.
		 *
		 * @param description The new description
		 */
		void setDescription(const std::string& description);

	protected:
		/**
		 * @brief Constructor.
		 *
		 * @param  id           The identifier
		 * @param  name         The name
		 * @param  description  The description
		 */
		NamedElement(const std::string &id, const std::string &name, const std::string &description);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		NamedElement(const NamedElement &other);

		/**
		 * @brief Compare to another element
		 *
		 * @param  other  Element to compare to
		 *
		 * @return  True if elements are equals, false otherwise
		 */
		virtual bool equal(const NamedElement &other) const;

	private:
    std::string name;
    std::string description;
	};
}

#endif // OPENSAND_CONF_NAMED_ELEMENT_H
