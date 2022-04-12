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
 * @file DataModel.h
 * @brief Represents a datamodel.
 */

#ifndef OPENSAND_CONF_DATA_MODEL_H
#define OPENSAND_CONF_DATA_MODEL_H

#include <memory>
#include <string>

#include "DataTypesList.h"
#include "DataType.h"
#include "DataValueType.h"
#include "DataEnumType.h"

#include "DataComponent.h"


namespace OpenSANDConf
{

class DataTypesList;
class DataComponent;

/**
 * @brief Represents a datamodel.
 */
class DataModel
{
 public:
	friend class MetaModel;

	/**
	 * @brief Constructor by copy.
	 *
	 * @param  other  The object to copy
	 */
	DataModel(const DataModel &other);

	/**
	 * @brief Desctructor.
	 */
	~DataModel();

	/**
	 * @brief Clone the current model.
	 *
	 * @return  The newly cloned model
	 */
	std::shared_ptr<DataModel> clone() const;

	/**
	 * @brief Validate the datamodel.
	 *
	 * @return True if the datamodel is valid, false otherwise
	 */
	bool validate() const;

	/**
	 * @brief Compare to another object.
	 *
	 * @param  other  Object to compare to
	 *
	 * @return  True if objects are equals, false otherwise
	 */
	bool equal(const DataModel &other) const;

	/**
	 * @brief Get the version.
	 *
	 * @return  The version
	 */
	const std::string &getVersion() const;

	/**
	 * @brief Get the root component.
	 *
	 * @return  The root component
	 */
	std::shared_ptr<DataComponent> getRoot() const;

	/**
	 * @brief Get an item by path.
	 *
	 * @param  path  The item's path
	 *
	 * @return  The item if found, nullptr otherwise
	 */
	std::shared_ptr<DataElement> getItemByPath(const std::string &path) const;

	friend bool operator== (const DataModel &v1, const DataModel &v2);
	friend bool operator!= (const DataModel &v1, const DataModel &v2);

 protected:
	/**
	 * @brief Constructor.
	 *
	 * @param  version  The version
	 * @param  types    The data types list
	 * @param  root     The root data component
	 */
	DataModel(const std::string &version, std::shared_ptr<DataTypesList> types, std::shared_ptr<DataComponent> root);

	/**
	 * @brief Get an item by meta path (list pattern can be returned).
	 *
	 * @param  path  The item's path
	 *
	 * @return  The item if found, nullptr otherwise
	 */
	std::shared_ptr<DataElement> getItemByMetaPath(const std::string &path) const;

 private:
	std::string version;
	std::shared_ptr<DataTypesList> types;
	std::shared_ptr<DataComponent> root;
};

bool operator== (const DataModel &v1, const DataModel &v2);
bool operator!= (const DataModel &v1, const DataModel &v2);

}

#endif // OPENSAND_CONF_DATA_MODEL_H
