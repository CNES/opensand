/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 CNES
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
 * @file MinimalCondition.h
 * @brief Minimal C/N necessary for QEF transmissions
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef MINIMAL_CONDITION_H
#define MINIMAL_CONDITION_H

#include "OpenSandFrames.h"

#include <string>
#include <map>

using std::string;

/**
 * @class Minimal Condition
 * @brief Minimal Condition
 */
class MinimalCondition
{

	protected:
		/// MinimalCondition mode
		string minimal_condition_mode;

		/// MinimalCondition C/N in clear sky conditions
		double minimal_cn;

	public:

		/**
		 * @brief Build the minimalCondition
		 *
		 * @param cste  the minimalCondition cste
		 */
		MinimalCondition(string minimalMode);

		/**
		 * @brief Destroy the minimalCondition
		 */
		~MinimalCondition();

		/**
		 * @brief initialize the minimal condition
		 *
		 * @param param The minimal condition parameters
		 * @return true on success, false otherwise
		 */
		virtual bool init(std::map<string, string> param) = 0;

		/**
		 * @brief Get the minimalCondition type
		 */
		string getMinimalConditionMode();

		/**
		 * @brief Set the minimalCondition mode
		 *
		 * @param minimal_condition_mode  the minimal condition Mode
		 */
		void setMinimalConditionMode(string minimal_condition_mode);

		/**
		 * @brief Set the minimalCondition current Carrier to Noise ratio
		 *        according to time
		 *
		 * @param time the current time
		 */
		virtual double getMinimalCN() = 0;

		/**
		 * @brief Updates Thresold when a msg arrives to Channel
		 *        (when MODCOD mode: use BBFRAME modcod id) 
		 *
		 * @param hdr the BBFrame header
		 * @return true on success, false otherwise
		 */
		virtual bool updateThreshold(T_DVB_HDR *hdr) = 0;

		/**
		 * @brief Set the minimalCondition current Carrier to Noise ratio 
		 */
		virtual void setMinimalCN(double minimal_cn) = 0;
};

#endif
