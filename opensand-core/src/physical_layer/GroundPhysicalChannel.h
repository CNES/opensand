/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
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
 * @file GroundPhysicalChannel.h
 * @brief Ground Physical Layer Channel
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 * @author Aurélien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef GROUND_PHYSICAL_CHANNEL_H
#define GROUND_PHYSICAL_CHANNEL_H

#include "PhysicalLayerPlugin.h"
#include "DelayFifo.h"
#include "DvbFrame.h"

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>
#include <opensand_rt/Types.h>


class NetContainer;


struct PhyLayerConfig
{
	tal_id_t mac_id;
	spot_id_t spot_id;
	Component entity_type;
};

/**
 * @class GroundPhysicalChannel
 * @brief Ground Physical Layer Channel
 */
class GroundPhysicalChannel
{
private:
	/// AttenuationModels
	AttenuationModelPlugin *attenuation_model;

	/// Clear Sky Conditions (best C/N in clear-sky conditions)
	double clear_sky_condition;

	/// The FIFO that implements the delay
	DelayFifo delay_fifo;

	/// Probes
	std::shared_ptr<Probe<float>> probe_attenuation = nullptr;
	std::shared_ptr<Probe<float>> probe_clear_sky_condition = nullptr;

protected:
	/// The terminal or gateway id
	tal_id_t mac_id;

	Component entity_type;
	spot_id_t spot_id;

	/// Logs
	std::shared_ptr<OutputLog> log_event = nullptr;
	std::shared_ptr<OutputLog> log_channel = nullptr;

	/// The satellite delay model
	SatDelayPlugin *satdelay_model = nullptr;

	/// Events
	event_id_t attenuation_update_timer;
	event_id_t fifo_timer;

	/**
	 * @brief Constructor of the ground physical channel
	 *
	 * @param config  the block config
	 */
	GroundPhysicalChannel(PhyLayerConfig config);

	/**
	 * @brief Initialize the ground physical channel
	 *
	 * @param upward_channel    whether the channel is going upward or downward
	 *
	 * @return true on success, false otherwise
	 */
	bool initGround(bool upward_channel, RtChannelBase *channel, std::shared_ptr<OutputLog> log_init);

	/**
	 * @brief Update the attenuation
	 *
	 * @return true on success, false otherwise
	 */
	bool updateAttenuation();

	/**
	 * @brief Get the current C/N value
	 * 
	 * @return the C/N value
	 */
	double getCurrentCn() const;

	/**
	 * @brief Push a packet in the FIFO to be delayed
	 *
	 * @param pkt  the packet to delay
	 * @return true on succes, false otherwise
	 */
	bool pushPacket(NetContainer *pkt);

	/**
	 * @brief Check there is at least one ready packet in FIFO at current time, then forward it
	 *
	 * @param current_time  the current time
	 *
	 * @return true if there is one, false otherwise
	 */
	bool forwardReadyPackets();

	/**
	 * @brief Forward the frame to the next channel
	 *
	 * @param dvb_frame  the DVB frame to forward
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool forwardPacket(DvbFrame *dvb_frame) = 0;

public:
	virtual ~GroundPhysicalChannel() = default;

	static void generateConfiguration();

	/**
	 * @brief Set the shared satellite delay plugin
	 *
	 * @param satdelay  the satellite delay plugin
	 */
	void setSatDelay(SatDelayPlugin *satdelay);

	/**
	 * @brief Compute the total C/N of the link according to the uplink C/N
	 *        and the downlink C/N
	 *
	 * @param up_cn    the uplink C/N value
	 * @param down_cn  the downlink C/N value
	 *
	 * @return the total C/N value
	 */
	static double computeTotalCn(double up_cn, double down_cn);
};


#endif
