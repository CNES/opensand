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
 * @file Fifo.h
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The fifo and signaling pipres for opensand-rt
 *         intra-block messages
 *
 */

#ifndef RT_FIFO_H
#define RT_FIFO_H

#include <queue>

#include "Types.h"
#include "RtMutex.h"


namespace Rt
{


/**
 * @class Fifo
 * @brief A fifo between two blocks
 */
class Fifo
{
 public:
	~Fifo();

 protected:
	friend class Channel;
	friend class ChannelBase;
	friend class ChannelMux;
	template <typename Key>
	friend class ChannelDemux;
	template <typename Key>
	friend class ChannelMuxDemux;
	friend class MessageEvent;
	friend class BlockManager;

	/**
	 * @brief Fifo constructor
	 *
	 */
	Fifo();

	/**
	 * @brief Initialize the fifo
	 *
	 * @return true on success, false otherwise
	 */
	bool init();
	
	/**
	 * @brief Add a new element in the fifo
	 * 
	 * @param message  the element to add in the fifo
	 * @return true on success, false otherwise
	 */
	bool push(Message message);
	
	/**
	 * @brief Access the first element and remove it from the queue
	 * 
	 * @param message  the first element in the fifo
	 * @return true on success, false otherwise
	 */
	bool pop(Message &message);
	
	/**
	 * 	@brief Get the file descriptor signaling data
	 * 	
	 * 	@return the read end of the pipe for data signaling
	 */
	int32_t getSigFd(void) const {return this->r_sig_pipe;};

 private:
	/// the queue
	std::queue<Message> fifo;

	/// The fifo size
	std::size_t max_size;
	
	/// The signaling pipe file descriptor for writing operations
	int32_t w_sig_pipe;
	
	/// The signaling pipe file descriptor for reading operations
	int32_t r_sig_pipe;
	
	/// The mutex on fifo access
	Mutex fifo_mutex;
	
	/// The mutex for fifo full (we need a semaphore here because it is
	//  lock and unlocked by different threads
	//  This semaphore is intialized with the fifo size
	Semaphore fifo_size_sem;
};


};  // namespace Rt


#endif
