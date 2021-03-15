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
 * @file DataList.h
 * @brief Represents a datamodel list
 *        (holds list items following a pattern).
 */

#ifndef OPENSAND_CONF_DATA_LIST_H
#define OPENSAND_CONF_DATA_LIST_H

#include <memory>
#include <string>

#include "DataContainer.h"


namespace OpenSANDConf
{

class DataComponent;

class DataList: public DataContainer
{
 public:
	friend class MetaList;
	friend class DataModel;
	friend class DataElement;

	using DataContainer::getItems;
	using DataContainer::getItem;

	/**
	 * @brief Destructor.
	 */
	virtual ~DataList();

	/**
	 * @brief Add an item to the list.
	 *
	 * @return  The newly added item
	 */
	std::shared_ptr<DataComponent> addItem();

	/**
	 * @brief Clear the list from its items.
	 */
	void clearItems();

 protected:
	/**
	 * @brief Constructor.
	 *
	 * @param  id           The identifier
	 * @param  parent       The parent path
	 * @param  pattern      The pattern
	 * @param  types  The types list
	 */
	DataList(const std::string &id, const std::string &parent, std::shared_ptr<DataComponent> pattern, std::shared_ptr<DataTypesList> types);

	/**
	 * @brief Constructor by copy (cloning).
	 *
	 * @param  other  The object to copy
	 * @param  types  The types list
	 */
	DataList(const DataList &other, std::shared_ptr<DataTypesList> types);
	/**
	 * @brief Constructor by copy (duplication.
	 *
	 * @param  id      The identifier
	 * @param  parent  The parent path
	 * @param  other   The object to copy
	 */
	DataList(const std::string &id, const std::string &parent, const DataList &other);

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

	/**
	 * @brief Duplicate the reference to another object.
	 *
	 * @param  copy  The object to set a copy of the reference
	 *
	 * @return True on success, false otherwise
	 */
	virtual bool duplicateReference(std::shared_ptr<DataElement> copy) const override;

	/**
	* @brief Get the list's pattern.
	*
	* @return  The list's pattern
	*/
	std::shared_ptr<DataComponent> getPattern() const;

 public:
	/**
	 * @brief Compare to another element
	 *
	 * @param  other  Element to compare to
	 *
	 * @return  True if elements are equals, false otherwise
	 */
	virtual bool equal(const DataElement &other) const override;

 private:
	std::shared_ptr<DataComponent> pattern;
	std::shared_ptr<DataTypesList> types;
};

}

#endif // OPENSAND_CONF_DATA_LIST_H
