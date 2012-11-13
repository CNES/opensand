/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
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
 * @file AttenuationModel.h
 * @brief AttenuationModel
 * @author Santiago PENA <santiago.penaluque@cnes.fr>
 */

#ifndef ATTENUATION_MODEL_H
#define ATTENUATION_MODEL_H

#include <string>
#include <map>


using std::string;

/**
 * @class AttenuationModel
 * @brief AttenuationModel
 */
class AttenuationModel
{

	protected:

		/* The model current attenuation */
		double attenuation;

		/*The counter of time */
		int time_counter;

		/* AttenuationModel type (Triangular/OnOff) */
		string attenuation_model_mode;

		/* Granularity*/
		int granularity;

	public:

		/**
		 * @brief Build the attenuation_model
		 *
		 * @param attenuation_model_mode the attenuation model type
		 * @param granularity            the attenuation model granularity
		 */
		AttenuationModel(string attenuation_model_mode, int granularity);

		/**
		 * @brief Destroy the attenuation model
		 */
		~AttenuationModel();

		/**
		 * @brief initialize the attenuation model
		 *
		 * @param param the attenuation model parameters
		 * @return true on success, false otherwise
		 */
		virtual bool init(std::map<string, string> param) = 0;

		/**
		 * @brief Get the attenuation model type
		 */
		string getAttenuationModelMode();

		/**
		 * @brief Set the attenuation model mode
		 *
		 * @param attenuation_model_mode  the attenuation model Mode
		 */
		void setAttenuationModelMode(string attenuation_model_mode);

		/**
		 * @brief Get the model current attenuation
		 */
		double getAttenuation();

		/**
		 * @brief Set the attenuation model current attenuation
		 *
		 * @param attenuation the model attenuation
		 */
		void setAttenuation(double attenuation);

		/**
		 * @brief Get the attenuation model time counter
		 */
		int getTimeCounter();

		/**
		 * @brief Set the attenuation model counter
		 * @param counter the attenuation model counter
		 */
		void setTimeCounter(int time_counter);

		/**
		 * @brief Get the Granularity
		 */
		int getGranularity();

		/**
		 * @brief Set the the Granularity
		 *
		 * @param  the Granularity
		 */
		void setGranularity(int granularity);

		/**
		 * @brief Set the attenuation model current attenuation according to time
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool computeAttenuation() = 0;

		/**
		 * @brief update the attenuation model current attenuation and the time counter
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool updateAttenuationModel() = 0;

};

#endif
