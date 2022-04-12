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
 * @file MetaElement.h
 * @brief Base class of all metamodel elements.
 */

#ifndef OPENSAND_CONF_META_ELEMENT_H
#define OPENSAND_CONF_META_ELEMENT_H

#include <memory>
#include <string>
#include <utility>

#include "NamedElement.h"
#include "MetaTypesList.h"


namespace OpenSANDConf
{

class TypeList;
class DataElement;
class MetaParameter;
class DataType;
class Data;

/**
 * @brief Base class of all metamodel elements.
 */
class MetaElement: public NamedElement
{
public:
	friend class MetaContainer;
	friend class MetaModel;

	/**
	 * @brief Destructor.
	 */
	virtual ~MetaElement();

	/**
	 * @brief Get the path.
	 *
	 * @return  The path
	 */
	std::string getPath() const;

	/**
	 * @brief Get the advanced status.
	 *
	 * @return  The advanced status
	 */
	bool isAdvanced() const;

	/**
	 * @brief Set the advanced status.
	 *
	 * @param  advanced  The advanced status
	 */
	void setAdvanced(bool advanced);

	/**
	 * @brief Get the read-only status.
	 *
	 * @return	The read-only status
	 */
	bool isReadOnly() const;

	/**
	 * @brief Set the read-only status.
	 *
	 * @param readOnly	The read-only status
	 */
	void setReadOnly(bool readOnly);

	/**
	 * @brief Get the reference parameter.
	 *
	 * @return The target parameter
	 */
	std::shared_ptr<MetaParameter> getReferenceTarget() const;

	/**
	 * @brief Get the expected data of the reference parameter.
	 *
	 * @return The expected data
	 */
	std::shared_ptr<Data> getReferenceData() const;

	/**
	 * @brief Compare to another element
	 *
	 * @param  other  Element to compare to
	 *
	 * @return  True if elements are equals, false otherwise
	 */
	virtual bool equal(const MetaElement &other) const;

	friend bool operator== (const MetaElement &v1, const MetaElement &v2);
	friend bool operator!= (const MetaElement &v1, const MetaElement &v2);

protected:
	/**
	 * @brief Constructor.
	 *
	 * @param  id           The identifier
	 * @param  parent       The parent path
	 * @param  name         The name
	 * @param  description  The description
	 */
	MetaElement(const std::string &id, const std::string &parent, const std::string &name, const std::string &description);

	/**
	 * @brief Constructor by copy.
	 *
	 * @param  other  The object to copy
	 */
	MetaElement(const MetaElement &other);

	/**
	 * @brief Clone the current object.
	 *
	 * @param  types  The types list
	 *
	 * @return The cloned object
	 */
	virtual std::shared_ptr<MetaElement> clone(std::weak_ptr<const MetaTypesList> types) const = 0;

	/**
	 * @brief Create a datamodel element.
	 *
	 * @param  types  The types list
	 *
	 * @return  The new datamodel element if succeeds, nullptr otherwise
	 */
	virtual std::shared_ptr<DataElement> createData(std::shared_ptr<DataTypesList> types) const = 0;

	/**
	 * @brief Get the parent path.
	 *
	 * @return  The parent path
	 */
	const std::string &getParentPath() const;

	/**
	 * @brief Specify a reference to a parameter value.
	 *
	 * @param  target  The target parameter
	 *
	 * @return True on success, false otherwise
	 */
	void setReference(std::shared_ptr<MetaParameter> target);

private:
	std::string parent;
	bool advanced;
	bool readOnly;
	std::tuple<std::shared_ptr<MetaParameter>, std::shared_ptr<Data>, std::shared_ptr<DataType>> reference;

	/**
	 * @brief Get an item by path.
	 *
	 * @param  root  The root element
	 * @param  path  The item's path
	 *
	 * @return  The item if found, nullptr otherwise
	 */
	static std::shared_ptr<MetaElement> getItemFromRoot(std::shared_ptr<MetaElement> root, const std::string &path);
};

bool operator== (const MetaElement &v1, const MetaElement &v2);
bool operator!= (const MetaElement &v1, const MetaElement &v2);

}

#endif // OPENSAND_CONF_META_ELEMENT_H
