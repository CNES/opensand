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

#include <memory>
#include <string>
#include <thread>


class RtEvent;
class RtChannelBase;
class RtChannel;
class RtChannelMux;
template<typename Key> class RtChannelDemux;
template<typename Key> class RtChannelMuxDemux;
class OutputLog;
class OutputEvent;


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
	 */
	Block(const std::string &name);

	virtual ~Block();

	/// The upward channel
	RtChannelBase *upward;
	/// The downward channel
	RtChannelBase *downward;

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
		RtUpwardBase(const std::string &name):
			ChannelType(name, "Upward")
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
		RtDownwardBase(const std::string &name):
			ChannelType(name, "Downward")
		{};
	};

 public:
	/// An upward channel with 1 input and 1 output
	using RtUpward = RtUpwardBase<RtChannel>;
	/// An upward channel with N inputs and 1 output
	using RtUpwardMux = RtUpwardBase<RtChannelMux>;
	/// An upward channel with 1 input and N outputs
	template <typename Key>
	using RtUpwardDemux = RtUpwardBase<RtChannelDemux<Key>>;
	/// An upward channel with N inputs and N outputs
	template <typename Key>
	using RtUpwardMuxDemux = RtUpwardBase<RtChannelMuxDemux<Key>>;

	/// A downward channel with 1 inputs and 1 outputs
	using RtDownward = RtDownwardBase<RtChannel>;
	/// A downward channel with N inputs and 1 outputs
	using RtDownwardMux = RtDownwardBase<RtChannelMux>;
	/// A downward channel with 1 inputs and N outputs
	template <typename Key>
	using RtDownwardDemux = RtDownwardBase<RtChannelDemux<Key>>;
	/// A downward channel with N inputs and N outputs
	template <typename Key>
	using RtDownwardMuxDemux = RtDownwardBase<RtChannelMuxDemux<Key>>;

 protected:
	/**
	 * @brief Initialize the block
	 *
	 * @warning Do not do anything else than basic initialization here,
	 *          as it is realized before channel initialization
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool onInit();
	
	/**
	 * @brief Get the name of the block
	 *
	 * @return the name of the block
	 */
	std::string getName(void) const { return this->name; };

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
	 * @return true on success, false otherwise
	 */
	bool stop(void);

	/// Output Log
	std::shared_ptr<OutputLog> log_rt;
	std::shared_ptr<OutputLog> log_init;

	/// The name of the block
	const std::string name;

 private:
	/// The upward channel thread
  std::thread up_thread;
	/// The downward channel thread
  std::thread down_thread;

	/// Whether the block is initialized
	bool initialized;

	/// The event for block initialization
	std::shared_ptr<OutputEvent> event_init;
};

// TODO malloc/new hook !!

#endif
