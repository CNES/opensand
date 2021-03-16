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
 * @file DataParameter.h
 * @brief Represents a datamodel parameter
 */

#ifndef OPENSAND_CONF_DATA_PARAMETER_H
#define OPENSAND_CONF_DATA_PARAMETER_H

#include <memory>
#include <string>
#include <tuple>

#include "DataElement.h"
#include "Data.h"


namespace OpenSANDConf
{

class DataParameter;

/**
 * @brief Represents a datamodel parameter
 */
class DataParameter: public DataElement, public std::enable_shared_from_this<DataParameter>
{
 public:
	friend class MetaParameter;
	friend class DataElement;

	/**
	 * Destructor.
	 */
	virtual ~DataParameter();

	/**
	* @brief Get the parameter data.
	*
	* @return  The parameter's data
	*/
	std::shared_ptr<Data> getData() const;

 protected:
	/**
	 * @brief Constructor.
	 *
	 * @param  id      The identifier
	 * @param  parent  The parent path
	 * @param  data    The data
	 */
	DataParameter(const std::string &id, const std::string &parent, std::shared_ptr<Data> data);

	/**
	 * @brief Constructor by copy (cloning).
	 *
	 * @param  other  The object to copy
	 * @param  types  The types list
	 */
	DataParameter(const DataParameter &other, std::shared_ptr<DataTypesList> types);

	/**
	 * @brief Constructor by copy (duplication).
	 *
	 * @param  id      The identifier
	 * @param  parent  The parent path
	 * @param  other   The object to copy
	 */
	DataParameter(const std::string &id, const std::string &parent, const DataParameter &other);

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
	 * @brief Create a reference to the current object.
	 *
	 * @return Reference
	 */
	std::tuple<std::shared_ptr<const DataParameter>, std::shared_ptr<Data>> createReference() const;

	/**
	 * @brief Validate the data element.
	 *
	 * @return  True if the data element is valid, false otherwise
	 */
	virtual bool validate() const override;

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
	std::shared_ptr<Data> data;
};

}

#endif // OPENSAND_CONF_DATA_PARAMETER_H
