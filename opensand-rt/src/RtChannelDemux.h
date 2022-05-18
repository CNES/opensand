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
 * @file RtChannelDemux.h
 * @author Yohan SIMARD / <yohan.simard@viveris.fr>
 * @brief  A channel with 1 input fifo and N output fifos
 *
 */

#ifndef RT_CHANNEL_DEMUX_H
#define RT_CHANNEL_DEMUX_H

#include <unordered_map>

#include <opensand_output/Output.h>

#include "RtChannelBase.h"
#include "RtFifo.h"


/**
 * @class RtChannelDemux
 * @brief A channel with 1 input fifo and N output fifos.
 *        The output fifo is selected with a key when
 *        enqueuing a message.
 * @tparam Key the type used to select the output fifo.
 *             Should be cheap to copy (int, enum, etc.)
 */
template <typename Key>
class RtChannelDemux: public RtChannelBase
{
 public:
	using DemuxKey = Key;

	RtChannelDemux(const std::string &name, const std::string &type);

	/**
	 * @brief Add a message in the next channel fifo mapped to key
	 * @warning The message shall not be reused in the channel after this call
	 *          because will be used in other blocks
	 *
	 * @param key   The key to select which fifo to use
	 * @param data  IN: A pointer on the  message to enqueue
	 *              OUT: NULL
	 * @param size  The size of data in message
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool enqueueMessage(Key key, void **data, size_t size = 0, uint8_t type = 0);

	/**
	 * @brief Set the fifo of the previous channel
	 *
	 * @param fifo  The fifo of the previous channel
	 */
	void setPreviousFifo(std::shared_ptr<RtFifo> &fifo);

	/**
	 * @brief Add a fifo of a next channel
	 *
	 * @param key  The key that will be mapped to this fifo
	 * @param fifo  The fifo
	 */
	void addNextFifo(Key key, std::shared_ptr<RtFifo> &fifo);

 protected:
	bool initPreviousFifo() override;

 private:
	/// The fifo of the previous channel
  std::shared_ptr<RtFifo> previous_fifo;
	/// The fifos of the next channels
	std::unordered_map<Key, std::shared_ptr<RtFifo>> next_fifos;
};


template <typename Key>
RtChannelDemux<Key>::RtChannelDemux(const std::string &name, const std::string &type):
  RtChannelBase{name, type},
  previous_fifo{nullptr},
  next_fifos{}
{
}


template <typename Key>
bool RtChannelDemux<Key>::initPreviousFifo()
{
  return this->initSingleFifo(this->previous_fifo);
}


template <typename Key>
bool RtChannelDemux<Key>::enqueueMessage(Key key, void **data, size_t size, uint8_t type)
{
	auto fifo_it = next_fifos.find(key);
	if (fifo_it == next_fifos.end())
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Cannot enqueue message: no FIFO found for this key");
		return false;
	}
	auto fifo = fifo_it->second;
	return this->pushMessage(fifo, data, size, type);
}


template <typename Key>
void RtChannelDemux<Key>::addNextFifo(Key key, std::shared_ptr<RtFifo> &fifo)
{
	bool actually_inserted = this->next_fifos.emplace(key, fifo).second;
	if (!actually_inserted) {
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add next FIFO: a FIFO already exists with this key");
	}
};


template <typename Key>
void RtChannelDemux<Key>::setPreviousFifo(std::shared_ptr<RtFifo> &fifo)
{
	this->previous_fifo = fifo;
};


#endif
