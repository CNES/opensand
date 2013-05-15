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
 * @file PhyChannel.h
 * @brief Physical Layer Channel
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef PHY_CHANNEL_H
#define PHY_CHANNEL_H

#include "PhysicalLayerPlugin.h"
#include "ModcodDefinitionTable.h"

#include <opensand_rt/Rt.h>

#include <string>


using std::string;

/**
 * @class PhyChannel
 * @brief Physical Layer Channel
 */
class PhyChannel
{
 protected:

	/// The channel status
	bool status;

	/// AttenuationModels
	AttenuationModelPlugin *attenuation_model;

	/** Nominal Conditions (best C/N in clear-sky conditions) of global
	 *  link (considering RF equipments,location,frequencies,coding)
	 */
	NominalConditionPlugin *nominal_condition;

	/** Minimal Conditions (minimun C/N to have QEF communications)
	 *  of global link (i.e. considering the Modcod scheme)
	 */
	MinimalConditionPlugin *minimal_condition;

	/// Error Insertion object : defines who error will be introduced
	ErrorInsertionPlugin *error_insertion;

	/// Period of channel(s) attenuation update (ms)
	time_ms_t granularity;

	/**
	 * @brief Get the Channel type
	 *
	 * @return the channel mode (File, On/Off, ...)
	 */
	string getMode();

	/**
	 * @brief Set the Channel mode
	 *
	 * @param channel_mode  the Channel Mode
	 */
	void setMode(string channel_mode);

	/**
	 * @brief Update the conditions of the communication model
	 *        (attenuation, propagation model, waveforms-modcod)
	 *
	 * @return true on success, false otherwise
	 */
	bool update();

	/*
	 * @brief Inserts the C/N value of the Channel in a given T_DVB_PHY
	 *        structure
	 *
	 * @param frame the packet to be modified
	 */
	void addSegmentCN(T_DVB_PHY *phy_frame);

	/*
	 * @brief Modify the C/N value of the Channel in a given T_DVB_PHY
	 *        structure with the combination of the current and previous C/N
	 *        values
	 *
	 * @param phy_frame the physical frame
	 */
	void modifySegmentCN(T_DVB_PHY *phy_frame);

	/*
	 * @brief Update the Minimal Condition attribute when a msg is received
	 *
	 * @param hdr The DVB header
	 * @return true on success, false otherwise
	 */
	bool updateMinimalCondition(T_DVB_HDR *hdr);

	/**
	 * @brief Determine if a Packet shall be corrupted or not
	 *        depending on the attenuation_model conditions
	 *
	 * @return true if it must be corrupted, false otherwise
	 */
	bool isToBeModifiedPacket(double CN_uplink);

	/**
	 * @brief Corrupt a package with error bits
	 *
	 * @param frame the packet to be modified
	 */
	void modifyPacket(T_DVB_META *frame, long length);


 public:

	/**
	 * @brief Build the Channel
	 */
	PhyChannel();

	/**
	 * @brief Destroy the Channel
	 */
	~PhyChannel();

};

#endif
