/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
 * @file Block.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The block description
 *
 */

#ifndef BLOCK_H
#define BLOCK_H

#include "Channel.h"
#include "Types.h"
#include "Event.h"


#include <stdlib.h>
#include <string>
#include <list>
#include <vector>
#include <sys/select.h>


#define FIFO_SIZE 3

class Channel;

using std::string;


/**
 * @class Block
 * @brief describes a block
 *
 * upper block and lower block are absolute;
 * upward channel processes data from lower block to upper block.
 * downward channel processes data from lower block to upper block.
 *
 */
class Block
{
	friend class Channel;
	friend class BlockManager;

  public:

	/**
	 * @brief Block constructor
	 *
	 * @param name  The name of the block
	 */
	Block(const string &name);
	virtual ~Block();


	/**
	 * @brief Initialize the block
	 *
	 * @return true on success, false otherwise
	 */
	// TODO once all will be done in channel, the part initialized
	// here should be transmitted to channel constructor
	virtual bool onInit(void) = 0;


	// TODO remove following functions
	/**
	 * @brief Process a downward event in block
	 * @warning Be careful, at the moment this function can be called
	 *          by to thread
	 *
	 * @param event  The event received in block
	 * @return true if event was correctly processed, false otherwise
	 */
	virtual bool onDownwardEvent(const Event *const event) = 0;

	/**
	 * @brief Process an upward event in block
	 * @warning Be careful, at the moment this function can be called
	 *          by to thread
	 *
	 * @param event  The event received in block
	 * @return true if event was correctly processed, false otherwise
	 */
	virtual bool onUpwardEvent(const Event *const event) = 0;

	/**
	 * @brief Send a message to upper block
	 * @warning The message shall not be reused in the block after this call
	 *          because will be used in upper blocks
	 *
	 * @param message  The message to send to upper block
	 * @return true on success, false otherwise
	 */
	bool sendUp(void *message);

	/**
	 * @brief Send a message to lower block
	 * @warning The message shall not be reused in the block after this call
	 *          because will be used in upper blocks
	 *          TODO find a way to prevent that
	 *
	 * @param message  IN: The message to send to lower block
	 *                 OUT: NULL
	 * @return true on success, false otherwise
	 */
	bool sendDown(void *message);
	// end TODO

	/**
	 * @brief Get the name of the block
	 *
	 * @return the name of the block
	 */
	string getName(void) const {return this->name;};

	/**
	 * @class Upward channel
	 *        With this class we are able to define Upward channel
	 *        functions in Block
	 */
	class Upward: public Channel
	{
	  public:
		Upward(Block &bl):
			Channel(bl, upward_chan)
		{};
	};

	/**
	 * @class Downward channel
	 *        With this class we are able to define Upward channel
	 *        functions in Block
	 */
	class Downward: public Channel
	{
	  public:
		Downward(Block &bl):
			Channel(bl, downward_chan)
		{};
	};


  protected:

	/**
	 * @brief Check whether the block is initialized
	 *
	 * @return true if the block is initialized, false otherwise
	 */
	bool isInitialized(void);

	/**
	 * @brief Initernal block initialization, will call onInit on
	 *        block and channels
	 *
	 * @return true on success, false otherwise
	 */
	bool init(void);

	/**
	 * @brief start the channel threads
	 *
	 * @return true on success, false otherwise
	 */
    bool start(void);

	/*
	 * @brief Stop the channel threads and call block destructor
	 *
	 * @param signal  The received signal
	 * @return true on success, false otherwise
	 */
	bool stop(int signal);

	/**
	 * @brief Handle an event received from a channel
	 *
	 * @param event  The event
	 * @param chan   The type of channel
	 * @return true on success, false otherwise
	 */
	// TODO remove once onEvent will be in channel
	 bool processEvent(const Event *const event, chan_type_t chan);

	/**
	 * @brief Get the upward channel
	 *
	 * @return the upward channel
	 */
	Channel *getUpwardChannel(void) const;

	/**
	 * @brief Get the downward channel
	 *
	 * @return the downward channel
	 */
	Channel *getDownwardChannel(void) const;

	/// The upward channel
	Upward *upward;
	/// The downward channel
	Downward *downward;

	/// The upward channel thread
	pthread_t up_thread_id;
	/// The downward channel thread
	pthread_t down_thread_id;

	/// The name of the block
	const string name;

	/// Whether the block is initialized
	bool initialized;

  private:
#ifdef DEBUG_BLOCK_MUTEX
	pthread_mutex_t *block_mutex; /// mutex to separate channel processing
#endif
};

// TODO malloc/new hook !!

#endif

