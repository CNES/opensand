/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file DataContainer.h
 * @brief Base class of all datamodel containers.
 */

#ifndef OPENSAND_CONF_DATA_CONTAINER_H
#define OPENSAND_CONF_DATA_CONTAINER_H

#include <memory>
#include <string>
#include <vector>

#include "DataElement.h"
#include "DataTypesList.h"


namespace OpenSANDConf
{

class DataContainer: public DataElement
{
 public:
	friend class MetaContainer;
	friend class DataList;
	friend class DataElement;
	friend class DataModel;

	/**
	 * @brief Destructor.
	 */
	virtual ~DataContainer();

	/**
	 * @brief Compare to another element
	 *
	 * @param  other  Element to compare to
	 *
	 * @return  True if elements are equals, false otherwise
	 */
	virtual bool equal(const DataElement &other) const override;

 protected:
	/**
	 * @brief Constructor.
	 *
	 * @param  id      The identifier
	 * @param  parent  The parent path
	 */
	DataContainer(const std::string &id, const std::string &parent);

	/**
	 * @brief Contructor by copy (cloning).
	 *
	 * @param  other  The object to copy
	 * @param  types  The types list
	 */
	DataContainer(const DataContainer &other, std::shared_ptr<DataTypesList> types);

	/**
	 * @brief Constructor by copy (duplication).
	 *
	 * @param  id      The new identifier
	 * @param  parent  The parent path
	 * @param  other   The object to copy
	 */
	DataContainer(const std::string &id, const std::string &parent, const DataContainer &other);

	/**
	 * @brief Clone the current object.
	 *
	 * @param  types  The types list
	 *
	 * @return The cloned object
	 */
	virtual std::shared_ptr<DataElement> clone(std::shared_ptr<DataTypesList> types) const = 0;

	/**
	 * @brief Duplicate the current object.
	 *
	 * @param  id      The new identifier
	 * @param  parent  The parent path
	 *
	 * @return The duplicated object
	 */
	virtual std::shared_ptr<DataElement> duplicateObject(const std::string &id, const std::string &parent) const = 0;

	/**
	 * @brief Duplicate the reference to another object.
	 *
	 * @param  copy  The object to set a copy of the reference
	 *
	 * @return True on success, false otherwise
	 */
	virtual bool duplicateReference(std::shared_ptr<DataElement> copy) const override;

	/**
	 * @brief Validate the datamodel element.
	 *
	 * @return  True if the element is valid, false otherwise
	 */
	virtual bool validate() const override;

	/**
	 * @brief Get the items.
	 *
	 * @return  The items
	 */
	virtual const std::vector<std::shared_ptr<DataElement>> &getItems() const;

	/**
	 * @brief Get an identified item.
	 *
	 * @param  id  The identifier
	 *
	 * @return  The item if found, nullptr otherwise
	 */
	virtual std::shared_ptr<DataElement> getItem(std::string id) const;

	/**
	 * @brief Add an item.
	 *
	 * @param  item  The item to add
	 */
	void addItem(std::shared_ptr<DataElement> item);

	/**
	 * @brief Clear the container from its items.
	 */
	void clearItems();

 private:
	std::vector<std::shared_ptr<DataElement>> items;
};

}

#endif // OPENSAND_CONF_DATA_CONTAINER_H
