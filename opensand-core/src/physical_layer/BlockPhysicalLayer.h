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
 * @file BlockPhysicalLayer.h
 * @brief A DVB physical layer block
 * @author Santiago PENA  <santiago.penaluque@cnes.fr>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 *
 * This block modifies the DVB frames sent/received on satellite terminals
 * depending on emulated physical conditions of the up and downlink
 *
 */

#ifndef BLOCK_PHYSICAL_LAYER_H
#define BLOCK_PHYSICAL_LAYER_H


#include <string>
#include <map>

#include <opensand_rt/Block.h>
#include <opensand_rt/RtChannel.h>

#include "OpenSandCore.h"
#include "GroundPhysicalChannel.h"


template<typename> class Probe;
class AttenuationHandler;


/**
 * @class Upward
 * @brief Ground Upward Physical Layer Channel
 */
template<>
class Rt::UpwardChannel<class BlockPhysicalLayer>: public GroundPhysicalChannel, public Channels::Upward<UpwardChannel<BlockPhysicalLayer>>
{
 private:
	/// Probes
	std::shared_ptr<Probe<float>> probe_total_cn = nullptr;

 protected:
	/// The attenuation process
	AttenuationHandler *attenuation_hdl;

	/**
	 * @brief Forward the frame to the next channel
	 *
	 * @param dvb_frame  the DVB frame to forward
	 *
	 * @return true on success, false otherwise
	 */
	bool forwardPacket(Ptr<DvbFrame> dvb_frame) override;

	/**
	 * @brief Get the C/N fot the current DVB frame
	 *
	 * @param dvb_frame  the current DVB frame
	 *
	 * @return the current C/N
	 */
	double getCn(DvbFrame &dvb_frame) const;

public:
	/**
	 * @brief Constructor of the ground upward physical channel
	 *
	 * @param name    the name of the channel
	 * @param config  the config of the block
	 */
	UpwardChannel(const std::string &name, PhyLayerConfig config);

	/**
	 * @brief Destroy the Channel
	 */
	virtual ~UpwardChannel();

	/**
	 * @brief Initialize the ground upward physical channel
	 *
	 * @return true on success, false otherwise
	 */
	bool onInit() override;

	/**
	 * @brief Event processing
	 *
	 * @param event  the event to process
	 *
	 * @return true on success, false otherwise
	 */
	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const TimerEvent &event) override;
	bool onEvent(const MessageEvent &event) override;
};


/**
 * @class Downward
 * @brief Ground Downward Physical Layer Channel
 */
template<>
class Rt::DownwardChannel<class BlockPhysicalLayer>: public GroundPhysicalChannel, public Channels::Downward<DownwardChannel<BlockPhysicalLayer>>
{
 private:
	/// Probes
	std::shared_ptr<Probe<int>> probe_delay = nullptr;

 protected:
	/// Event
	event_id_t delay_update_timer;

	/**
	 * @brief Update the delay
	 *
	 * @return true on success, false otherwise
	 */
	bool updateDelay();

	/**
	 * @brief Forward the frame to the next channel
	 *
	 * @param dvb_frame  the DVB frame to forward
	 *
	 * @return true on success, false otherwise
	 */
	bool forwardPacket(Ptr<DvbFrame> dvb_frame) override;

	/**
	 * @brief Prepare the frame
	 *
	 * @param dvb_frame  the DVB frame to forward
	 */
	void preparePacket(DvbFrame &dvb_frame);

 public:
	/**
	 * @brief Constructor of the ground downward physical channel
	 *
	 * @param name    the name of the channel
	 * @param config  the config of the block
	 */
	DownwardChannel(const std::string &name, PhyLayerConfig config);

	/**
	 * @brief Initialize the ground downward physical channel
	 *
	 * @return true on success, false otherwise
	 */
	bool onInit() override;

	/**
	 * @brief Event processing
	 *
	 * @param event  the event to process
	 *
	 * @return true on success, false otherwise
	 */
	using ChannelBase::onEvent;
	bool onEvent(const Event &event) override;
	bool onEvent(const TimerEvent &event) override;
	bool onEvent(const MessageEvent &event) override;
};


/**
 * @class BlockPhysicalLayer
 * @brief Basic DVB PhysicalLayer block
 */
class BlockPhysicalLayer: public Rt::Block<BlockPhysicalLayer, PhyLayerConfig>
{
 public:
	/**
	 * Build a physical layer block
	 *
	 * @param name    The name of the block
	 * @param config  The config of the block
	 */
	BlockPhysicalLayer(const std::string &name, PhyLayerConfig config);

	static void generateConfiguration();

 protected:
	// initialization method
	bool onInit() override;

	/// The terminal mac_id
	tal_id_t mac_id;

	/// The satellite delay for this terminal
	SatDelayPlugin *satdelay = nullptr;
};


#endif
