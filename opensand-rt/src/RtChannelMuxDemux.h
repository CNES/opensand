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
 * @file RtChannelMuxDemux.h
 * @author Yohan SIMARD / <yohan.simard@viveris.fr>
 * @brief  A channel with 1 input fifo and N output fifos
 *
 */

#ifndef RT_CHANNEL_MUXDEMUX_H
#define RT_CHANNEL_MUXDEMUX_H

#include <vector>
#include <unordered_map>

#include <opensand_output/Output.h>

#include "RtChannelBase.h"
#include "RtFifo.h"


namespace Rt
{


/**
 * @class ChannelMuxDemux
 * @brief A channel with N input fifos and N output fifos.
 *        The output fifo is selected with a key when
 *        enqueuing a message.
 * @tparam Key the type used to select the output fifo.
 *             Should be cheap to copy (int, enum, etc.)
 */
template <typename Key>
class ChannelMuxDemux: public ChannelBase
{
 public:
	using DemuxKey = Key;

	ChannelMuxDemux(const std::string &name, const std::string &type);

	/**
	 * @brief Add a message in the next channel fifo mapped to key
	 * @warning The message shall not be reused in the channel after this call
	 *          because will be used in other blocks
	 *
	 * @param key   The key to select which fifo to use
	 * @param data  A pointer on the  message to enqueue
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool enqueueMessage(Key key, Ptr<void> data, uint8_t type);

	/**
	 * @brief Add a fifo of a previous channel
	 *
	 * @param fifo  The fifo of the previous channel
	 */
	void addPreviousFifo(std::shared_ptr<Fifo> &fifo);

	/**
	 * @brief Add a fifo of a next channel
	 *
	 * @param key   The key that will be mapped to this fifo
	 * @param fifo  The fifo
	 */
	void addNextFifo(Key key, std::shared_ptr<Fifo> &fifo);

  protected:
	bool initPreviousFifo() override;

  private:
	/// The fifos of the previous channels
	std::vector<std::shared_ptr<Fifo>> previous_fifos;
	/// The fifos of the next channels
	std::unordered_map<Key, std::shared_ptr<Fifo>> next_fifos;
};


template <typename Key>
ChannelMuxDemux<Key>::ChannelMuxDemux(const std::string &name, const std::string &type):
	ChannelBase{name, type},
	previous_fifos{},
	next_fifos{}
{
}


template <typename Key>
bool ChannelMuxDemux<Key>::initPreviousFifo()
{
	bool success = true;
	for (auto&& fifo : this->previous_fifos)
		success = success && this->initSingleFifo(fifo);

	return success;
}


template <typename Key>
bool ChannelMuxDemux<Key>::enqueueMessage(Key key, Ptr<void> data, uint8_t type)
{
	auto fifo_it = next_fifos.find(key);
	if (fifo_it == next_fifos.end())
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Cannot enqueue message: no FIFO found for this key");
		return false;
	}
	auto fifo = fifo_it->second;

	Message m{std::move(data)};
	m.type = type;
	return this->pushMessage(fifo, std::move(m));
}


template <typename Key>
void ChannelMuxDemux<Key>::addNextFifo(Key key, std::shared_ptr<Fifo> &fifo)
{
	bool actually_inserted = this->next_fifos.emplace(key, fifo).second;
	if (!actually_inserted)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add next FIFO: a FIFO already exists for this key");
	}
}


template <typename Key>
void ChannelMuxDemux<Key>::addPreviousFifo(std::shared_ptr<Fifo> &fifo)
{
	this->previous_fifos.push_back(fifo);
}


};  // namespace Rt


#endif
