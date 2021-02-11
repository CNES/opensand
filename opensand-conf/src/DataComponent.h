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
 * @file DataComponent.h
 * @brief Represents a datamodel component
 *        (holds a list of components, lists and parameters).
 */

#ifndef OPENSAND_CONF_DATA_COMPONENT_H
#define OPENSAND_CONF_DATA_COMPONENT_H

#include <memory>
#include <string>

#include "DataContainer.h"
#include "DataList.h"
#include "DataParameter.h"


namespace OpenSANDConf
{

/**
 * @brief Represents a datamodel component
 *        (holds a list of components, lists and parameters).
 */
class DataComponent: public DataContainer
{
 public:
	friend class MetaComponent;
	friend class DataList;
	friend class DataModel;

	using DataContainer::getItems;
	using DataContainer::getItem;

	/**
	 * @brief Destructor.
	 */
	virtual ~DataComponent();

	/**
	 * @brief Get an identified parameter.
	 *
	 * @param  id  The identifier
	 *
	 * @return  The parameter if found, nullptr otherwise
	 */
	std::shared_ptr<DataParameter> getParameter(const std::string &id) const;

	/**
	 * @brief Get an identified component.
	 *
	 * @param  id  The identifier
	 *
	 * @return  The component if found, nullptr otherwise
	 */
	std::shared_ptr<DataComponent> getComponent(const std::string &id) const;

	/**
	 * @brief Get an identified list.
	 *
	 * @param  id  The identifier
	 *
	 * @return  The list if found, nullptr otherwise
	 */
	std::shared_ptr<DataList> getList(const std::string &id) const;

 protected:
	/**
	 * @brief Construtor.
	 *
	 * @param  id      The identifier
	 * @param  parent  The parent path
	 */
	DataComponent(const std::string &id, const std::string &parent);

	/**
	 * @brief Constructor by copy (cloning).
	 *
	 * @param  other  The object to copy
	 * @param  types  The types list
	 */
	DataComponent(const DataComponent &other, std::shared_ptr<DataTypesList> types);

	/**
	 * @brief Constructor by copy (duplication).
	 *
	 * @param  id      The new identifier
	 * @param  parent  The parent path
	 * @param  other   The object to copy
	 */
	DataComponent(const std::string &id, const std::string &parent, const DataComponent &other);

	/**
	 * @brief Add a new item to the component.
	 *
	 * @param  item  The item to add
	 */
	void addItem(std::shared_ptr<DataElement> item);

	/**
	 * @brief Clone the current object.
	 *
	 * @param  types  The types list
	 *
	 * @return The cloned object
	 */
	virtual std::shared_ptr<DataElement> clone(std::shared_ptr<DataTypesList> types) const override;

	/**
	 * @brief Duplicate the current object.
	 *
	 * @param  id      The new identifier
	 * @param  parent  The parent path
	 *
	 * @return The duplicated object
	 */
	virtual std::shared_ptr<DataElement> duplicateObject(const std::string &id, const std::string &parent) const override;

 public:
	/**
	 * @brief Compare to another element
	 *
	 * @param  other  Element to compare to
	 *
	 * @return  True if elements are equals, false otherwise
	 */
	virtual bool equal(const DataElement &other) const override;
};

}

#endif // OPENSAND_CONF_DATA_COMPONENT_H
