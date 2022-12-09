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
 * @file RtChannel.h
 * @author Yohan SIMARD / <yohan.simard@viveris.fr>
 * @brief  A simple channel with 1 input fifo and 1 output fifo
 */

#ifndef RT_CHANNEL_H
#define RT_CHANNEL_H

#include "RtChannelBase.h"


/**
 * @class RtChannel
 * @brief A simple channel with 1 input fifo and 1 output fifo
 */
class RtChannel: public RtChannelBase
{
 public:
	RtChannel(const std::string &name, const std::string &type);

	/**
	 * @brief Add a message in the next channel fifo
	 * @warning The message shall not be reused in the channel after this call
	 *          because will be used in other blocks
	 *
	 * @param data  IN: A pointer on the  message to enqueue
	 *              OUT: NULL
	 * @param size  The size of data in message
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool enqueueMessage(void **data, size_t size, uint8_t type);

	/**
	 * @brief Set the fifo of the previous channel
	 *
	 * @param fifo  The fifo
	 */
	void setPreviousFifo(std::shared_ptr<RtFifo> &fifo);

	/**
	 * @brief Set the fifo of the next channel
	 *
	 * @param fifo  The fifo of the next channel
	 */
	void setNextFifo(std::shared_ptr<RtFifo> &fifo);

 protected:
	bool initPreviousFifo() override;

 private:
	/// The fifo of the previous channel
  std::shared_ptr<RtFifo> previous_fifo;
	/// The fifo of the next channel
  std::shared_ptr<RtFifo> next_fifo;
};


#endif
