/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
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
 * @file PhyChannel.h
 * @brief Physical Layer Channel
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 */

#ifndef PHY_CHANNEL_H
#define PHY_CHANNEL_H

#include "PhysicalLayerPlugin.h"
#include "FmtDefinitionTable.h"
#include "DelayFifo.h"
#include "DvbFrame.h"
#include "NetContainer.h"

#include <opensand_rt/Rt.h>
#include <opensand_output/OutputLog.h>

#include <string>


using std::string;

/**
 * @class PhyChannel
 * @brief Physical Layer Channel
 */
class PhyChannel
{
 protected:

	/// Output logs
	OutputLog *log_channel;

	/// The channel status
	bool status;

	/// Clear Sky Conditions (best C/N in clear-sky conditions)
	unsigned int clear_sky_condition;

	/// AttenuationModels
	AttenuationModelPlugin *attenuation_model;

	/** Minimal Conditions (minimun C/N to have QEF communications)
	 *  of global link (i.e. considering the Modcod scheme)
	 */
	MinimalConditionPlugin *minimal_condition;

	/// Error Insertion object : defines who error will be introduced
	ErrorInsertionPlugin *error_insertion;

	/// Period of channel(s) attenuation update (ms)
	time_ms_t refresh_period_ms;

	/// Timer id for attenuation update
	event_id_t att_timer;

	/// Whether this is the satellite
	bool is_sat;

	/// The satellite delay model
	SatDelayPlugin *satdelay;

	/// The timer to check if there's a new item ready in FIFO
	event_id_t fifo_timer;

	/// The timer to update the satellite delay
	// TODO: this is unused on one of the two channels (same for probe_delay)
	event_id_t delay_timer;

	// TODO: satellite physical channels don't use this FIFO, it should be removed
	// from here. Possible solution: channels from terminals extend the satellite's,
	// and not the way around.
	/// the FIFO that implements the delay
	DelayFifo delay_fifo;

	/**
	 * @brief Update the conditions of the communication model
	 *        (attenuation, propagation model, waveforms-modcod)
	 *
	 * @return true on success, false otherwise
	 */
	bool update();

	/**
	 * @brief get the total C/N of the link according to the uplink C/N
	 *        carried in the T_DVB_PHY structure and the downlink C/N
	 *        computed from clear sky conditions and attenuation
	 *
	 * @param dvb_frame  The uplink DVB frame
	 *
	 * @return the total C/N
	 */
	double getTotalCN(DvbFrame *dvb_frame);

	/*
	 * @brief Inserts the C/N value of the Channel in a given T_DVB_PHY
	 *        structure
	 *
	 * @param dvb_frame  The current frame
	 */
	void addSegmentCN(DvbFrame *dvb_frame);

	/*
	 * @brief Update the Minimal Condition attribute when a msg is received
	 *
	 * @param dvb_frame The DVB frame
	 * @return true on success, false otherwise
	 */
	bool updateMinimalCondition(DvbFrame *dvb_frame);

	/**
	 * @brief Determine if a Packet shall be corrupted or not
	 *        depending on the attenuation_model conditions
	 *
	 * @param cn_total  The total C/N of the link
	 * @return true if it must be corrupted, false otherwise
	 */
	bool isToBeModifiedPacket(double cn_total);

	/**
	 * @brief Corrupt a package with error bits
	 *
	 * @param dvb_frame the frame to be modified
	 */
	void modifyPacket(DvbFrame *dvb_frame);


	/**
	 * Process the attenuation for a DVB frame
	 *
	 * @param dvb_frame The DVB frame to send
	 * @return Whether the DVB frame was successfully sent or not
	 */
	virtual bool processAttenuation(DvbFrame *dvb_frame) = 0;

	/**
	 * @brief handle the FIFO timer
	 *
	 * @return true if success, false on error
	 */
	virtual bool handleFifoTimer() = 0;

	/**
	 * @brief push dvb_frame into delay FIFO
	 *
	 * @param data the container with the dvb_frame
	 * @delay delay the delay to wait
	 * @return true on succes, false otherwise
	 */
	bool pushInFifo(NetContainer *data, time_ms_t delay);

	/// probes
	Probe<float> *probe_attenuation;
	Probe<float> *probe_clear_sky_condition;
	Probe<float> *probe_minimal_condition;
	Probe<float> *probe_total_cn;
	Probe<int> *probe_drops;
	Probe<int> *probe_delay;
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
