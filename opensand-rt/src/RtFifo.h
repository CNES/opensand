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

#include "Types.h"
#include "RtMutex.h"

#include <queue>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#define MAGIC_WORD "GO"

using std::queue;

/**
 * @class RtFifo
 * @brief A fifo between two blocks
 */
class RtFifo
{
  protected:

	friend class RtChannel;
	friend class MessageEvent;
	friend class BlockManager;

	/**
	 * @brief Fifo constructor
	 *
	 */
	RtFifo();
	~RtFifo();

	/**
	 * @brief Initialize the fifo
	 *
	 * @return true on success, false otherwise
	 */
	bool init();
	
	/**
	 * @brief Add a new element in the fifo
	 * 
	 * @param the data part of the element to add in the fifo
	 * @param the size of the element to add in the fifo
	 * @return true on success, false otherwise
	 */
	bool push(void *data, size_t size, uint8_t type);
	
	/**
	 * @brief Access the first element but do not delete it
	 * 
	 * @param elem  the first element in the fifo
	 * @return true on success, false otherwise
	 */
	bool pop(rt_msg_t &message);
	
	/**
	 * 	@brief Get the file descriptor signaling data
	 * 	
	 * 	@return the read end of the pipe for data signaling
	 */
	int32_t getSigFd(void) const {return this->r_sig_pipe;};

  private:

	/// the queue
	queue<rt_msg_t> fifo;
  
	/// The fifo size
	size_t max_size;
	
	/// The signaling pipe file descriptor for writing operations
	int32_t w_sig_pipe;
	
	/// The signaling pipe file descriptor for reading operations
	int32_t r_sig_pipe;
	
	/// The mutex on fifo access
	RtMutex fifo_mutex;
	
	/// The mutex for fifo full (we need a semaphore here because it is
	//  lock and unlocked by different threads
	//  This semaphore is intialized with the fifo size
	sem_t fifo_size_sem;
};

#endif

