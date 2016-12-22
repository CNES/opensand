/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file Triangular.h
 * @brief Triangular
 * @author Fatima LAHMOUAD <fatima.lahmouad@etu.enseeiht.fr>
 */

#ifndef TRIANGULAR_ATTENUATION_MODEL_H
#define TRIANGULAR_ATTENUATION_MODEL_H


#include "PhysicalLayerPlugin.h"

#include <opensand_conf/ConfigurationFile.h>

#include <string>

using std::string;

/**
 * @class Triangular
 * @brief Triangular
 */
class Triangular: public AttenuationModelPlugin
{

	private:

		/** The Triangular slope */
		double slope;

		/** The Triangular period:
		 *  period * refresh_period_ms/1000 = period in seconds */
		int period;

		int duration_counter;

		map<string, ConfigurationList> config_section_map;

	public:

		/**
		 * @brief Build a Triangular
		 */
		Triangular();

		/**
		 *@brief Destroy the Triangular
		 */
		~Triangular();


		bool init(time_ms_t refresh_period_ms, string link);

		bool updateAttenuationModel();
};

CREATE(Triangular, attenuation_plugin, "Triangular");

#endif
