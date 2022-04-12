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
 * @file DataElement.h
 * @brief Base class of all datamodel elements.
 */

#ifndef OPENSAND_CONF_DATA_ELEMENT_H
#define OPENSAND_CONF_DATA_ELEMENT_H

#include <memory>
#include <string>
#include <tuple>

#include "BaseElement.h"


namespace OpenSANDConf
{

class DataTypesList;
class DataParameter;
class Data;

/**
 * @brief Base class of all datamodel elements.
 */
class DataElement: public BaseElement
{
 public:
	friend class DataContainer;
	friend class DataList;
	friend class DataModel;
	friend class MetaModel;

	/**
	 * @brief Destructor.
	 */
	virtual ~DataElement();

	/**
	 * @brief Get the path.
	 *
	 * @return  The path
	 */
	std::string getPath() const;

	/**
	 * @brief Check the reference value matches the expected value.
	 *
	 * @return True is reference is ok, false otherwise
	 */
	bool checkReference() const;

	/**
	 * @brief Compare to another element
	 *
	 * @param  other  Element to compare to
	 *
	 * @return  True if elements are equals, false otherwise
	 */
	virtual bool equal(const DataElement &other) const;

	friend bool operator== (const DataElement &v1, const DataElement &v2);
	friend bool operator!= (const DataElement &v1, const DataElement &v2);

 protected:
	/**
	 * @brief Constructor.
	 *
	 * @param  id      The identifier
	 * @param  parent  The parent path
	 */
	DataElement(const std::string &id, const std::string &parent);

	/**
	 * @brief Constructor by copy (cloning).
	 *
	 * @param  other  The object to copy
	 */
	DataElement(const DataElement &other);

	/**
	 * @brief Clone the current object.
	 *
	 * @param  types  The data types list
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
	virtual bool duplicateReference(std::shared_ptr<DataElement> copy) const;

	/**
	 * @brief Duplicate the current object.
	 *
	 * @param  id      The new identifier
	 * @param  parent  The parent path
	 *
	 * @return The duplicated object
	 */
	std::shared_ptr<DataElement> duplicate(const std::string &id, const std::string &parent) const;

	/**
	 * @brief Specify a reference to a parameter value.
	 *
	 * @param  target  The target parameter
	 *
	 * @return True on success, false otherwise
	 */
	void setReference(std::shared_ptr<const DataParameter> target);

	/**
	 * @brief Get the reference parameter.
	 *
	 * @return The target parameter
	 */
	std::shared_ptr<const DataParameter> getReferenceTarget() const;

	/**
	 * @brief Get the expected data of the reference parameter.
	 *
	 * @return The expected data
	 */
	std::shared_ptr<Data> getReferenceData() const;

	/**
	 * @brief Validate the datamodel element.
	 *
	 * @return  True if the element is valid, false otherwise
	 */
	virtual bool validate() const = 0;

	/**
	 * @brief Get the parent path.
	 *
	 * @return  The parent path
	 */
	const std::string &getParentPath() const;

 private:
	std::string parent;
	std::tuple<std::shared_ptr<const DataParameter>, std::shared_ptr<Data>> reference;

	/**
	 * @brief Get an item by path.
	 *
	 * @param  root  The root element
	 * @param  path  The item's path
	 * @param  meta  True to enable list pattern returning
	 *
	 * @return  The item if found, nullptr otherwise
	 */
	static std::shared_ptr<DataElement> getItemFromRoot(std::shared_ptr<DataElement> root, const std::string &path, bool meta);
};

bool operator== (const DataElement &v1, const DataElement &v2);
bool operator!= (const DataElement &v1, const DataElement &v2);

}

#endif // OPENSAND_CONF_DATA_ELEMENT_H
