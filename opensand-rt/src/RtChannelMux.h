/*
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
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file RtChannelMux.h
 * @author Yohan SIMARD / <yohan.simard@viveris.fr>
 * @brief  A channel with N input fifos and 1 output fifo
 */

#ifndef RT_CHANNEL_MUX_H
#define RT_CHANNEL_MUX_H

#include <vector>

#include "RtChannelBase.h"


namespace Rt
{


/**
 * @class ChannelMux
 * @brief A channel with N input fifos and 1 output fifo.
 */
class ChannelMux: public ChannelBase
{
 public:
	ChannelMux(const std::string &name, const std::string &type);

	/**
	 * @brief Add a message in the next channel queue
	 * @warning The message shall not be reused in the channel after this call
	 *          because will be used in other blocks
	 *
	 * @param data  A pointer on the  message to enqueue
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool enqueueMessage(Ptr<void> data, uint8_t type);

	/**
	 * @brief Add a fifo of a previous channel
	 *
	 * @param fifo  The fifo
	 */
	void addPreviousFifo(std::shared_ptr<Fifo> &fifo);

	/**
	 * @brief Set the fifo of the next channel
	 *
	 * @param fifo  The fifo of the next channel
	 */
	void setNextFifo(std::shared_ptr<Fifo> &fifo);

 protected:
	bool initPreviousFifo() override;

 private:
	/// The fifos of the previous channels
	std::vector<std::shared_ptr<Fifo>> previous_fifos;
	/// The fifo of the next channel
	std::shared_ptr<Fifo> next_fifo;
};


};  // namespace Rt


#endif
