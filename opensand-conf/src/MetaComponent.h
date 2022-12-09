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
 * @file MetaComponent.h
 * @brief Represents a generic metamodel component
 *        (holds a list of components, lists and parameters).
 */

#ifndef OPENSAND_CONF_META_COMPONENT_H
#define OPENSAND_CONF_META_COMPONENT_H

#include <memory>
#include <string>

#include "MetaContainer.h"
#include "MetaParameter.h"
#include "MetaList.h"
#include "MetaType.h"
#include "MetaEnumType.h"


namespace OpenSANDConf
{
	/**
	 * @brief Represents a generic metamodel component
	 *        (holds a list of components, lists and parameters).
	 */
	class MetaComponent: public MetaContainer
	{
	public:
		friend class MetaModel;
		friend class MetaList;

		using MetaContainer::getItems;
		using MetaContainer::getItem;

		/**
		 * @brief Destructor.
		 */
		virtual ~MetaComponent();

		/**
		 * @brief Get an identified parameter.
		 *
		 * @param  id  The identifier
		 *
		 * @return  The parameter if found, nullptr otherwise
		 */
		std::shared_ptr<MetaParameter> getParameter(const std::string &id) const;

		/**
		 * @brief Get an identified component.
		 *
		 * @param  id  The identifier
		 *
		 * @return  The component if found, nullptr otherwise
		 */
		std::shared_ptr<MetaComponent> getComponent(const std::string &id) const;

		/**
		 * @brief Get an identified list.
		 *
		 * @param  id  The identifier
		 *
		 * @return  The list if found, nullptr otherwise
		 */
		std::shared_ptr<MetaList> getList(const std::string &id) const;

		/**
		* @brief Add a new component to the component.
		*
		* @param  id           The component identifier
		* @param  name         The component name
		*
		* @return  The newly created component on success, nullptr otherwise
		*/
		std::shared_ptr<MetaComponent> addComponent(const std::string &id, const std::string &name);

		/**
		* @brief Add a new component to the component.
		*
		* @param  id           The component identifier
		* @param  name         The component name
		* @param  description  The component description
		*
		* @return  The newly created component on success, nullptr otherwise
		*/
		std::shared_ptr<MetaComponent> addComponent(const std::string &id, const std::string &name, const std::string &description);

		/**
		* @brief Add a new component to the component if it does not exist,
		*        or returns the existing one.
		*
		* @param  id           The component identifier
		* @param  name         The component name
		* @param  description  The component description
		*
		* @return  The newly created component or the existing one on success, nullptr otherwise
		*/
		std::shared_ptr<MetaComponent> getOrCreateComponent(const std::string &id, const std::string &name, const std::string &description = "");

		/**
		* @brief Add a new list to the component.
		*
		* @param  id                   The list identifier
		* @param  name                 The list name
		* @param  pattern_name         The list pattern name
		*
		* @return  The newly created list on success, nullptr otherwise
		*/
		std::shared_ptr<MetaList> addList(const std::string &id, const std::string &name, const std::string &pattern_name);

		/**
		* @brief Add a new list to the component.
		*
		* @param  id                   The list identifier
		* @param  name                 The list name
		* @param  pattern_name         The list pattern name
		* @param  description          The list description
		*
		* @return  The newly created list on success, nullptr otherwise
		*/
		std::shared_ptr<MetaList> addList(const std::string &id, const std::string &name, const std::string &pattern_name, const std::string &description);

		/**
		* @brief Add a new list to the component.
		*
		* @param  id                   The list identifier
		* @param  name                 The list name
		* @param  pattern_name         The list pattern name
		* @param  description          The list description
		* @param  pattern_description  The list pattern description
		*
		* @return  The newly created list on success, nullptr otherwise
		*/
		std::shared_ptr<MetaList> addList(const std::string &id, const std::string &name, const std::string &pattern_name, const std::string &description, const std::string &pattern_description);

		/**
		* @brief Add a new list to the component if it does not exist,
		*        or return the existing one.
		*
		* @param  id                   The list identifier
		* @param  name                 The list name
		* @param  pattern_name         The list pattern name
		* @param  description          The list description
		* @param  pattern_description  The list pattern description
		*
		* @return  The newly created list or the existing one on success, nullptr otherwise
		*/
		std::shared_ptr<MetaList> getOrCreateList(const std::string &id, const std::string &name, const std::string &pattern_name, const std::string &description = "", const std::string &pattern_description = "");

		/**
		* @brief Add a new parameter to the component.
		*
		* @param   id           The parameter identifier 
		* @param   name         The parameter name
		* @param   type         The parameter type
		*
		* @return  The newly created parameter on success, nullptr otherwise
		*/
		std::shared_ptr<MetaParameter> addParameter(const std::string &id, const std::string &name, std::shared_ptr<MetaType> type);

		/**
		* @brief Add a new parameter to the component.
		*
		* @param   id           The parameter identifier 
		* @param   name         The parameter name
		* @param   type         The parameter type
		* @param   description  The parameter description
		*
		* @return  The newly created parameter on success, nullptr otherwise
		*/
		std::shared_ptr<MetaParameter> addParameter(const std::string &id, const std::string &name, std::shared_ptr<MetaType> type, const std::string &description);

		/**
		* @brief Add a new parameter to the component if it does not exist,
		*        or return the existing one
		*
		* @param   id           The parameter identifier 
		* @param   name         The parameter name
		* @param   type         The parameter type
		* @param   description  The parameter description
		*
		* @return  The newly created parameter on success, nullptr otherwise
		*/
		std::shared_ptr<MetaParameter> getOrCreateParameter(const std::string &id, const std::string &name, std::shared_ptr<MetaType> type, const std::string &description = "");

	protected:
		/**
		 * @brief Construtor.
		 *
		 * @param  id           The identifier
		 * @param  parent       The parent path
		 * @param  name         The name
		 * @param  description  The description
		 * @param  types        The types list
		 */
		MetaComponent(const std::string &id, const std::string &parent, const std::string &name, const std::string &description, std::weak_ptr<const MetaTypesList> types);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 * @param  types  The types list
		 */
		MetaComponent(const MetaComponent &other, std::weak_ptr<const MetaTypesList> types);

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

#endif // OPENSAND_CONF_META_COMPONENT_H
