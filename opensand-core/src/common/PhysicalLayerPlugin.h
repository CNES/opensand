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
 * @file PhysicalLayerPlugin.h
 * @brief Plugins for Physical Layer Minimal conditions, Nominal conditions,
 *        Error insertion and Attenuation models
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef PHYSICAL_LAYER_PLUGIN_H
#define PHYSICAL_LAYER_PLUGIN_H

#include "OpenSandPlugin.h"
#include "lib_dvb_rcs.h"

#include <string>
#include <map>


using std::string;


/**
 * @class AttenuationModel
 * @brief AttenuationModel
 */
class AttenuationModelPlugin: public OpenSandPlugin
{

	protected:

		/* The model current attenuation */
		double attenuation;

		/* Granularity*/
		int granularity;

	public:

		/**
		 * @brief AttenuationModelPlugin constructor
		 *
		 * @param granularity  the attenuation model granularity
		 */
		AttenuationModelPlugin(): OpenSandPlugin() {};

		/**
		 * @brief AttenuationModelPlugin destructor
		 */
		virtual ~AttenuationModelPlugin() {};

		/**
		 * @brief initialize the attenuation model
		 *
		 * @param granularity the attenuation granularity
		 * @param link        the link
		 * @return true on success, false otherwise
		 */
		virtual bool init(int granularity, string link) = 0;

		/**
		 * @brief Get the model current attenuation
		 */
		double getAttenuation() {return this->attenuation;};

		/**
		 * @brief Set the attenuation model current attenuation
		 *
		 * @param attenuation the model attenuation
		 */
		void setAttenuation(double attenuation) {this->attenuation = attenuation;};

		/**
		 * @brief update the attenuation model current attenuation
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool updateAttenuationModel() = 0;

};

/**
 * @class NominalCondition 
 * @brief Nominal Condition 
 */
class NominalConditionPlugin: public OpenSandPlugin
{

	protected:

		/* NominalCondition C/N in clear sky conditions */
		double nominal_cn;

	public:

		/**
		  @brief NominamConditionPlugin constructor
		 */
		NominalConditionPlugin(): OpenSandPlugin() {};

		/**
		 * @brief NominalConditionPlugin destructor
		 */
		virtual ~NominalConditionPlugin() {};

		/**
		 * @brief initialize the nominal condition
		 *
		 * @param link  the link
		 * @return true on success, false otherwise
		 */
		virtual bool init(string link) = 0;

		/**
		 * @brief Set the nominalCondition current Carrier to Noise ratio according to time
		 *
		 * @return the nominal C/N
		 */
		virtual double getNominalCN() {return this->nominal_cn;};

};


/**
 * @class Minimal Condition
 * @brief Minimal Condition only used on downlink
 */
class MinimalConditionPlugin: public OpenSandPlugin
{

	protected:

		/// MinimalCondition C/N in clear sky conditions
		double minimal_cn;

	public:

		/**
		 * @brief MinimalConditionPlugin constructor
		 */
		MinimalConditionPlugin(): OpenSandPlugin() {};

		/**
		 * @brief MinimalConditionPlugin destructor
		 */
		virtual ~MinimalConditionPlugin() {};

		/**
		 * @brief initialize the minimal condition
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool init() = 0;

		/**
		 * @brief Set the minimalCondition current Carrier to Noise ratio
		 *        according to time
		 *
		 * @param time the current time
		 */
		virtual double getMinimalCN() {return this->minimal_cn;};

		/**
		 * @brief Updates Thresold when a msg arrives to Channel
		 *        (when MODCOD mode: use BBFRAME modcod id) 
		 *
		 * @param hdr the BBFrame header
		 * @return true on success, false otherwise
		 */
		virtual bool updateThreshold(T_DVB_HDR *hdr) = 0;
};

/**
 * @class ErrorInsertion
 * @brief ErrorInsertion
 */
class ErrorInsertionPlugin: public OpenSandPlugin 
{
	public:

		/**
		 * @brief ErrorInsertionPlugin constructor
		 */
		ErrorInsertionPlugin(): OpenSandPlugin() {};

		/**
		 * @brief ErrorInsertionPlugin destructor
		 */
		virtual ~ErrorInsertionPlugin() {};

		/**
		 * @brief initialize the error insertion
		 *
		 * @return true on success, false otherwise
		 */
		virtual bool init() = 0;

		/**
		 * @brief Determine if a Packet shall be corrupted or not depending on
		 *        the attenuationModel conditions 
		 *
		 * @return true if it must be corrupted, false otherwise 
		 */
		virtual bool isToBeModifiedPacket(double cn_uplink,
		                                  double nominal_cn,
		                                  double attenuation,
		                                  double threshold_qef) = 0;

		/**
		 * @brief Corrupt a package with error bits 
		 *
		 * @param frame the packet to be modified 
		 */
		virtual void modifyPacket(T_DVB_META *frame, long length) = 0;

};


#endif
