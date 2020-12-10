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
 * @file Configuration.h
 * @brief Functions to help MetaModel and DataModel (un)serialization.
 */

#ifndef OPENSAND_CONF_CONFIGURATION_HPP
#define OPENSAND_CONF_CONFIGURATION_HPP

#include <memory>
#include <string>

#include "MetaModel.h"
#include "DataModel.h"


namespace OpenSANDConf
{
	/**
	 * @brief Write a XSD file from a model.
	 *
	 * @param  model     The model to write
	 * @param  filepath  The filepath to write
	 *
	 * @return True on success, false otherwise
	 */
	bool toXSD(std::shared_ptr<MetaModel> model, const std::string &filepath);

	/**
	 * @brief Read a XSD file to generate a new model.
	 *
	 * @param  filepath  The filepath to read
	 *
	 * @return The new generated model from XSD on success, nullptr otherwise
	 */
  std::shared_ptr<MetaModel> fromXSD(const std::string &filepath);

	/**
	 * @brief Write a XML file from a datamodel
	 *
	 * @param  datamodel  The datamodel to write
	 * @param  filepath  The filepath to write
	 *
	 * @return True on success, false otherwise
	 */
	bool toXML(std::shared_ptr<DataModel> datamodel, const std::string &filepath);

	/**
	 * @brief Read a XML file to generate a new datamodel matching a model.
	 *
	 * @param  model     The model which the new datamodel will match to
	 * @param  filepath  The filepath to read
	 *
	 * @return The new generated datamodel from XML on success, nullptr otherwise
	 */
  std::shared_ptr<DataModel> fromXML(std::shared_ptr<MetaModel> model, const std::string &filepath);
}

#endif // OPENSAND_CONF_CONFIGURATION_HPP
