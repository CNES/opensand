/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file NominalCondition.h
 * @brief Nominal C/N for clear-sky conditions
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef NOMINAL_CONDITION_H
#define NOMINAL_CONDITION_H 


#include <string>
#include <map>

using std::string;

/**
 * @class NominalCondition 
 * @brief Nominal Condition 
 */
class NominalCondition
{

	protected:
		/* NominalCondition mode */
		string nominal_condition_mode;

		/* NominalCondition C/N in clear sky conditions */
		double nominal_cn;

	public:

		/**
		  @brief Build the nominalCondition
		 *
		 * @param cste  the nominalCondition cste
		 */
		NominalCondition(string nominal_condition_mode);

		/**
		 * @brief Destroy the nominalCondition
		 */
		~NominalCondition();

		/**
		 * @brief initialize the nominal condition
		 *
		 * @param param The minimal nominal condition parameters
		 * @return true on success, false otherwise
		 */
		virtual bool init(std::map<string, string> param) = 0;

		/**
		 * @brief Get the nominalCondition type
		 */
		string getNominalConditionMode();

		/**
		 * @brief Set the nominalCondition mode
		 *
		 * @param nominal_condition_mode  the nominalCondition Mode
		 */
		void setNominalConditionMode(string nominal_condition_mode);

		/**
		 * @brief Set the nominalCondition current Carrier to Noise ratio according to time
		 * @param time the current time
		 */
		virtual double getNominalCN() = 0;

};

#endif
