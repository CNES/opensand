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

#include "RtChannelBase.h"
#include "RtFifo.h"
#include <opensand_output/Output.h>
#include <unordered_map>

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
    
	// Inherit base constructors
	using RtChannelBase::RtChannelBase;

	~RtChannelDemux() override;

	/**
	 * @brief Add a message in the next channel queue
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
	 * @brief Set the fifo for previous channel
	 *
	 * @param fifo  The fifo of the previous channel
	 */
	void setPreviousFifo(RtFifo *fifo);

	/**
	 * @brief Add a fifo for next channel message
	 *
	 * @param key  The key that will be mapped to this fifo
	 * @param fifo  The fifo
	 */
	void addNextFifo(Key key, RtFifo *fifo);

  protected:
	bool initPreviousFifo() override;

  private:
	/// The fifo of the channel for messages from previous channel
	RtFifo *previous_fifo = nullptr;
	/// The fifos on the next channel
	std::unordered_map<Key, RtFifo *> next_fifos{};
};

template <typename Key>
RtChannelDemux<Key>::~RtChannelDemux()
{
	if (this->previous_fifo)
	{
		delete this->previous_fifo;
	}
}

template <typename Key>
bool RtChannelDemux<Key>::initPreviousFifo()
{
	if (this->previous_fifo)
	{
		if (!this->previous_fifo->init())
		{
			this->reportError(true, "cannot initialize previous fifo\n");
			return false;
		}
		if (!this->addMessageEvent(this->previous_fifo))
		{
			this->reportError(true, "cannot create previous message event\n");
			return false;
		}
	}
	return true;
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
void RtChannelDemux<Key>::addNextFifo(Key key, RtFifo *fifo)
{
	bool actually_inserted = this->next_fifos.emplace(key, fifo).second;
	if (!actually_inserted) {
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot add next FIFO: a FIFO already exists with this key");
	}
};

template <typename Key>
void RtChannelDemux<Key>::setPreviousFifo(RtFifo *fifo)
{
	this->previous_fifo = fifo;
};

#endif
