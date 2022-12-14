/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file CodingTypes.h
 * @brief The coding types
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef CODING_TYPES_H
#define CODING_TYPES_H

#include <string>
#include <map>


/**
 * @class CodingTypes
 * @brief The coding types
 */
class CodingTypes
{
public:
	~CodingTypes();

	/**
	 * @brief Check a coding exists
	 *
	 * @param coding_label  The coding label
	 *
	 * @return  True if the label is managed, false otherwise
	 */
	static bool exist(std::string coding_label);

	/**
	 * @brief Get the default coding rate
	 *
	 * @return  The default coding rate
	 */
	static float getDefaultRate();

	/**
	 * @brief Get a coding rate
	 *
	 * @param coding_label  The coding label
	 *
	 * @return  The coding rate
	 */
	static float getRate(std::string coding_label);

private:
	float default_coding_rate;
	std::map<std::string, float> coding_rates;

	CodingTypes();
};

#endif

