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
 * @file MetaModel.h
 * @brief Represents a metamodel.
 */

#ifndef OPENSAND_CONF_META_MODEL_H
#define OPENSAND_CONF_META_MODEL_H

#include <memory>
#include <string>

#include "MetaTypesList.h"
#include "MetaType.h"
#include "MetaValueType.h"
#include "MetaEnumType.h"

#include "MetaComponent.h"


namespace OpenSANDConf
{
	class DataModel;
	class MetaElement;
	class MetaParameter;

	class MetaModel
	{
	public:
		/**
		 * @brief Constructor.
		 *
		 * @param  version  The version
		 */
		MetaModel(const std::string &version);

		/**
		 * @brief Constructor by copy.
		 *
		 * @param  other  The object to copy
		 */
		MetaModel(const MetaModel &other);

		/**
		 * @brief Desctructor.
		 */
		 ~MetaModel();

		/**
		 * @brief Clone the current model.
		 *
		 * @return  The newly cloned model
		 */
		 std::shared_ptr<MetaModel> clone() const;

		/**
		 * @brief Create a datamodel.
		 *
		 * @return  The new datamodel if succeeds, nullptr otherwise
		 */
		 std::shared_ptr<DataModel> createData() const;

		/**
		 * @brief Specify an element reference to a parameter value.
		 *
		 * @param  element  The element to reference
		 * @param  target   The target parameter
		 *
		 * @return True on success, false otherwise
		 */
		bool setReference(std::shared_ptr<MetaElement> element, std::shared_ptr<MetaParameter> target) const;

		/**
		 * @brief Reset the element reference to a parameter value.
		 *
		 * @param  element  The element to reset reference
		 */
		void resetReference(std::shared_ptr<MetaElement> element) const;

		/**
		 * @brief Compare to another object.
		 *
		 * @param  other  Object to compare to
		 *
		 * @return  True if objects are equals, false otherwise
		 */
		bool equal(const MetaModel &other) const;

		/**
		 * @brief Get the version.
		 *
		 * @return  The version
		 */
		const std::string &getVersion() const;

		/**
		 * @brief Get the types list.
		 *
		 * @return  The types list
		 */
		std::shared_ptr<MetaTypesList> getTypesDefinition() const;

		/**
		 * @brief Get the root component.
		 *
		 * @return  The root component
		 */
		std::shared_ptr<MetaComponent> getRoot() const;

		/**
		 * @brief Get an item by path.
		 *
		 * @param  path  The item's path
		 *
		 * @return  The item if found, nullptr otherwise
		 */
		std::shared_ptr<MetaElement> getItemByPath(const std::string &path) const;

		friend bool operator== (const MetaModel &v1, const MetaModel &v2);
		friend bool operator!= (const MetaModel &v1, const MetaModel &v2);

	private:
		std::string version;
		std::shared_ptr<MetaTypesList> types;
		std::shared_ptr<MetaComponent> root;
	};

	bool operator== (const MetaModel &v1, const MetaModel &v2);
	bool operator!= (const MetaModel &v1, const MetaModel &v2);
}

#endif // OPENSAND_CONF_META_MODEL_H
