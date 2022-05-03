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
 * @file Block.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The block description
 *
 */

#ifndef BLOCK_H
#define BLOCK_H

#include "RtChannelBase.h"
#include "RtChannel.h"
#include "RtChannelMux.h"
#include "RtChannelDemux.h"
#include "Types.h"


#include <stdlib.h>
#include <string>
#include <list>
#include <vector>
#include <sys/select.h>

#include <opensand_output/OutputLog.h>
#include <opensand_output/OutputEvent.h>

class RtEvent;

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
	friend class RtChannel;
	friend class BlockManager;

  public:

	/**
	 * @brief Block constructor
	 *
	 * @param name      The name of the block
	 * @param specific  Specific block parameters
	 */
	Block(const string &name, void *specific = NULL);

	virtual ~Block();

  private:
	/**
	 * @class Upward channel
	 *        With this class we are able to define Upward channel
	 *        functions in Block
	 */
	template <typename ChannelType>
	class RtUpwardBase: public ChannelType
	{
	  public:
		RtUpwardBase(const string &name):
			ChannelType(name, "Upward")
		{};

		template<class T>
		RtUpwardBase(const string &name, T specific):
			ChannelType(name, "Upward", specific)
		{};	
	};

	/**
	 * @class Downward channel
	 *        With this class we are able to define Downward channel
	 *        functions in Block
	 */
	template <typename ChannelType>
	class RtDownwardBase: public ChannelType
	{
	  public:
		RtDownwardBase(const string &name):
			ChannelType(name, "Downward")
		{};

		template<class T>
		RtDownwardBase(const string &name, T specific):
			ChannelType(name, "Downward", specific)
		{};
	};
	
  public:
	using RtUpward = RtUpwardBase<RtChannel>;
	using RtUpwardMux = RtUpwardBase<RtChannelMux>;
	template <typename Key>
	using RtUpwardDemux = RtUpwardBase<RtChannelDemux<Key>>;
	using RtDownward = RtDownwardBase<RtChannel>;
	using RtDownwardMux = RtDownwardBase<RtChannelMux>;
	template <typename Key>
	using RtDownwardDemux = RtDownwardBase<RtChannelDemux<Key>>;

  protected:

	/**
	 * @brief Initialize the block
	 *
	 * @warning Do not do anything else than basic initialization here,
	 *          as it is realized before channel initialization
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool onInit(void) = 0;
	
	/**
	 * @brief Get the name of the block
	 *
	 * @return the name of the block
	 */
	string getName(void) const {return this->name;};

	/**
	 * @brief Check whether the block is initialized
	 *
	 * @return true if the block is initialized, false otherwise
	 */
	bool isInitialized(void);

	/**
	 * @brief Internal block initialization
	 *
	 * @return true on success, false otherwise
	 */
	bool init(void);

	/**
	 * @brief Specific block and channels initialization
	 *        Call onInit methods
	 *
	 * @return true on success, false otherwise
	 */
	bool initSpecific(void);

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
	 * @brief Get the upward channel
	 *
	 * @return the upward channel
	 */
	RtChannelBase *getUpwardChannel(void) const;

	/**
	 * @brief Get the downward channel
	 *
	 * @return the downward channel
	 */
	RtChannelBase *getDownwardChannel(void) const;

	/// Output Log
	std::shared_ptr<OutputLog> log_rt;
	std::shared_ptr<OutputLog> log_init;

	/// The upward channel
	RtChannelBase *upward;
	/// The downward channel
	RtChannelBase *downward;

	/// The name of the block
	const string name;

  private:

	/// The upward channel thread
	pthread_t up_thread_id;
	/// The downward channel thread
	pthread_t down_thread_id;

	/// Whether the block is initialized
	bool initialized;

	/// The event for block initialization
	std::shared_ptr<OutputEvent> event_init;
};

// TODO malloc/new hook !!

#endif

