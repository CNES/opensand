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
 * @file BlockManager.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The manager that handles blocks
 *
 */

#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#include <vector>

#include "Block.h"
#include "TemplateHelper.h"


class OutputLog;


namespace Rt
{


/**
 * @class BlockManager
 * @brief Interface for operations on runtime library. Singleton.
 *
 */
class BlockManager
{
	friend class Rt;

 protected:
	BlockManager();
	~BlockManager();

	/**
	 * @brief Creates and adds a block to the application.
	 *
	 * @tparam Bl       The block class
	 * @param name      The block name
	 * @return A pointer to the newly created block
	 */
#if __cplusplus < 202002L
	template <class Bl>
#else
	template<IsBlock Bl>
#endif
	Bl& createBlock(const std::string &name);

	/**
	 * @brief Creates and adds a block to the application.
	 *
	 * @tparam Bl       The block class
	 * @param name      The block name
	 * @param specific  User defined data to pass to the
	 *                  constructor of the block
	 * @return A pointer to the newly created block
	 */
#if __cplusplus < 202002L
	template <class Bl, class Specific>
#else
	template<IsBlock Bl, class Specific>
#endif
	Bl& createBlock(const std::string &name, Specific specific);

	/**
	 * @brief Connects two blocks
	 *
	 * @param upper     The upper block
	 * @param lower     The lower block
	 */
#if __cplusplus < 202002L
	template <class UpperBl, class LowerBl>
#else
	template<SimpleUpper UpperBl, SimpleLower LowerBl>
#endif
	void connectBlocks(UpperBl& upper, LowerBl& lower);

	/**
	 * @brief Connects a multiplexer block to a simple block
	 *
	 * @param upper     The upper block, with a Mux upward channel
	 *                  and a Demux downward channel
	 * @param lower     The lower block
	 * @param down_key  The key to send messages from the upper block to
	 *                  the lower block
	 */
#if __cplusplus < 202002L
	template <class UpperBl, class LowerBl>
#else
	template<MultipleUpper UpperBl, SimpleLower LowerBl>
#endif
	void connectBlocks(UpperBl& upper, LowerBl& lower,
	                   typename UpperBl::ChannelDownward::DemuxKey down_key);

	/**
	 * @brief Connects a simple block to a multiplexer block
	 *
	 * @param upper     The upper block
	 * @param lower     The lower block, with a Demux upward channel
	 *                  and a Mux downward channel
	 * @param up_key    The key to send messages from the lower block to
	 *                  the upper block
	 */
#if __cplusplus < 202002L
	template <class UpperBl, class LowerBl>
#else
	template<SimpleUpper UpperBl, MultipleLower LowerBl>
#endif
	void connectBlocks(UpperBl& upper, LowerBl& lower,
	                   typename LowerBl::ChannelUpward::DemuxKey up_key);

	/**
	 * @brief Connects two multiplexer blocks
	 *
	 * @param upper     The upper block, with a Mux upward channel
	 *                  and a Demux downward channel
	 * @param lower     The lower block, with a Demux upward channel
	 *                  and a Mux downward channel
	 * @param up_key    The key to send messages from the lower block to
	 *                  the upper block
	 * @param down_key  The key to send messages from the upper block to
	 *                  the lower block
	 */
#if __cplusplus < 202002L
	template <class UpperBl, class LowerBl>
#else
	template<MultipleUpper UpperBl, MultipleLower LowerBl>
#endif
	void connectBlocks(UpperBl& upper, LowerBl& lower,
	                   typename LowerBl::ChannelUpward::DemuxKey up_key,
	                   typename UpperBl::ChannelDownward::DemuxKey down_key);

	/**
	 * @brief stops the application
	 *        Force kill if a thread don't stop
	 *
	 * @param signal  The received signal
	 */
	void stop();

	/**
	 * @brief Initialize the manager, creates and initialize blocks
	 *
	 * @return true if succesful, false otherwise
	 */
	bool init();

	/**
	 * @brief Start blocks
	 *
	 */
	bool start();

	/**
	 * @brief Internal error report
	 *
	 * @param msg        The error message
	 * @param critical   Whether the application should be stopped
	 */
	void reportError(const std::string& msg, bool critical);

	/**
	 * @brief Checks if threads and application are alive or should be stopped
	 *        Exits when application goes stopped
	 */
	void wait();

	/**
	 * @brief Check if something got really wrong in the process
	 *
	 * @return true if no fatal error occured, false otherwise
	 */
	bool getStatus();

	/// Output Log
	std::shared_ptr<OutputLog> log_rt;

 private:
	/// list of pointers to the blocks
	std::vector<std::unique_ptr<BlockBase>> block_list;

	/// check if we already tried to stop process
	bool stopped;

	/// whether a critical error was raised
	bool status;

	int stop_fd;
};


#if __cplusplus < 202002L
template <class Bl>
#else
template<IsBlock Bl>
#endif
Bl& BlockManager::createBlock(const std::string &name)
{
	auto block = new Bl{name};
	this->block_list.push_back(std::unique_ptr<Bl>{block});
	return *block;
}


#if __cplusplus < 202002L
template <class Bl, class Specific>
#else
template<IsBlock Bl, class Specific>
#endif
Bl& BlockManager::createBlock(const std::string &name, Specific specific)
{
	auto block = new Bl{name, specific};
	this->block_list.push_back(std::unique_ptr<Bl>{block});
	return *block;
}


struct ChannelsConnector
{
	template <class Sender, class Receiver, std::enable_if_t<has_one_input<Receiver>::value, bool> = true>
	static inline void connect(Sender &sender, Receiver &receiver)
	{
		auto fifo = BlockBase::createFifo();
		sender.setNextFifo(fifo);
		receiver.setPreviousFifo(fifo);
	}

	template <class Sender, class Receiver, std::enable_if_t<!has_one_input<Receiver>::value, bool> = true>
	static inline void connect(Sender &sender, Receiver &receiver)
	{
		auto fifo = BlockBase::createFifo();
		sender.setNextFifo(fifo);
		receiver.addPreviousFifo(fifo);
	}

	template <class Sender, class Receiver, std::enable_if_t<has_one_input<Receiver>::value, bool> = true>
	static inline std::void_t<typename Sender::DemuxKey> connect(Sender &sender, Receiver &receiver, typename Sender::DemuxKey key)
	{
		auto fifo = BlockBase::createFifo();
		sender.addNextFifo(key, fifo);
		receiver.setPreviousFifo(fifo);
	}

	template <class Sender, class Receiver, std::enable_if_t<!has_one_input<Receiver>::value, bool> = true>
	static inline std::void_t<typename Sender::DemuxKey> connect(Sender &sender, Receiver &receiver, typename Sender::DemuxKey key)
	{
		auto fifo = BlockBase::createFifo();
		sender.addNextFifo(key, fifo);
		receiver.addPreviousFifo(fifo);
	}
};


#if __cplusplus < 202002L
template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower)
{
	static_assert(has_one_input<typename UpperBl::ChannelUpward>::value);
	static_assert(has_one_output<typename UpperBl::ChannelDownward>::value);
	static_assert(has_one_output<typename LowerBl::ChannelUpward>::value);
	static_assert(has_one_input<typename LowerBl::ChannelDownward>::value);

	ChannelsConnector::connect(lower.upward, upper.upward);
	ChannelsConnector::connect(upper.downward, lower.downward);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower,
                                 typename UpperBl::ChannelDownward::DemuxKey down_key)
{
	static_assert(has_n_inputs<typename UpperBl::ChannelUpward>::value);
	static_assert(has_n_outputs<typename UpperBl::ChannelDownward>::value);
	static_assert(has_one_output<typename LowerBl::ChannelUpward>::value);
	static_assert(has_one_input<typename LowerBl::ChannelDownward>::value);

	ChannelsConnector::connect(lower.upward, upper.upward);
	ChannelsConnector::connect(upper.downward, lower.downward, down_key);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower,
                                 typename LowerBl::ChannelUpward::DemuxKey up_key)
{
	static_assert(has_one_input<typename UpperBl::ChannelUpward>::value);
	static_assert(has_one_output<typename UpperBl::ChannelDownward>::value);
	static_assert(has_n_outputs<typename LowerBl::ChannelUpward>::value);
	static_assert(has_n_inputs<typename LowerBl::ChannelDownward>::value);
	
	ChannelsConnector::connect(lower.upward, upper.upward, up_key);
	ChannelsConnector::connect(upper.downward, lower.downward);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower,
                                 typename LowerBl::ChannelUpward::DemuxKey up_key,
                                 typename UpperBl::ChannelDownward::DemuxKey down_key)
{
	static_assert(has_n_inputs<typename UpperBl::ChannelUpward>::value);
	static_assert(has_n_outputs<typename UpperBl::ChannelDownward>::value);
	static_assert(has_n_outputs<typename LowerBl::ChannelUpward>::value);
	static_assert(has_n_inputs<typename LowerBl::ChannelDownward>::value);

	ChannelsConnector::connect(lower.upward, upper.upward, up_key);
	ChannelsConnector::connect(upper.downward, lower.downward, down_key);
}
#else
template<SimpleUpper UpperBl, SimpleLower LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower)
{
	ChannelsConnector::connect(lower.upward, upper.upward);
	ChannelsConnector::connect(upper.downward, lower.downward);
}

template<MultipleUpper UpperBl, SimpleLower LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower,
                                 typename UpperBl::ChannelDownward::DemuxKey down_key)
{
	ChannelsConnector::connect(lower.upward, upper.upward);
	ChannelsConnector::connect(upper.downward, lower.downward, down_key);
}

template<SimpleUpper UpperBl, MultipleLower LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower,
                                 typename LowerBl::ChannelUpward::DemuxKey up_key)
{
	ChannelsConnector::connect(lower.upward, upper.upward, up_key);
	ChannelsConnector::connect(upper.downward, lower.downward);
}

template<MultipleUpper UpperBl, MultipleLower LowerBl>
void BlockManager::connectBlocks(UpperBl& upper, LowerBl& lower,
                                 typename LowerBl::ChannelUpward::DemuxKey up_key,
                                 typename UpperBl::ChannelDownward::DemuxKey down_key)
{
	ChannelsConnector::connect(lower.upward, upper.upward, up_key);
	ChannelsConnector::connect(upper.downward, lower.downward, down_key);
}
#endif


};  // namespace Rt


#endif
