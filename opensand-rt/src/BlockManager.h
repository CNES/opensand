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


class Fifo;


/**
 * @class BlockManager
 * @brief Interface for operations on runtime library. Singleton.
 *
 */
class BlockManager
{
	friend class Rt;

 public:
	static std::shared_ptr<Fifo> createFifo();

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
	template <class Bl>
	Bl *createBlock(const std::string &name);

	/**
	 * @brief Creates and adds a block to the application.
	 *
	 * @tparam Bl       The block class
	 * @param name      The block name
	 * @param specific  User defined data to pass to the
	 *                  constructor of the block
	 * @return A pointer to the newly created block
	 */
	template <class Bl, class Specific>
	Bl *createBlock(const std::string &name, Specific specific);

	/**
	 * @brief Connects two blocks
	 *
	 * @param upper     The upper block
	 * @param lower     The lower block
	 */
	template <class UpperBl, class LowerBl>
	void connectBlocks(const UpperBl *upper, const LowerBl *lower);

	/**
	 * @brief Connects a multiplexer block to a simple block
	 *
	 * @param upper     The upper block, with a Mux upward channel
	 *                  and a Demux downward channel
	 * @param lower     The lower block
	 * @param down_key  The key to send messages from the upper block to
	 *                  the lower block
	 */
	template <class UpperBl, class LowerBl>
	void connectBlocks(const UpperBl *upper,
	                   const LowerBl *lower,
	                   typename UpperBl::Downward::DemuxKey down_key);

	/**
	 * @brief Connects a simple block to a multiplexer block
	 *
	 * @param upper     The upper block
	 * @param lower     The lower block, with a Demux upward channel
	 *                  and a Mux downward channel
	 * @param up_key    The key to send messages from the lower block to
	 *                  the upper block
	 */
	template <class UpperBl, class LowerBl>
	void connectBlocks(const UpperBl *upper,
	                   const LowerBl *lower,
	                   typename LowerBl::Upward::DemuxKey up_key);

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
	template <class UpperBl, class LowerBl>
	void connectBlocks(const UpperBl *upper,
	                   const LowerBl *lower,
	                   typename LowerBl::Upward::DemuxKey up_key,
	                   typename UpperBl::Downward::DemuxKey down_key);

	/**
	 * @brief Connects two channels together, bypasses usual safety-checks
	 *
	 * @param sender    The channel that will send data into the fifo
	 * @param receiver  The channel that will receive data through the fifo
	 */
	template <class SenderCh, class ReceiverCh>
	void connectChannels(SenderCh &sender, ReceiverCh &receiver);

	/**
	 * @brief Connects two channels together, bypasses usual safety-checks
	 *
	 * @param sender    The channel that will send data into the fifo
	 * @param receiver  The channel that will receive data through the fifo
	 * @param key       The key under which the receiver is known from
	 *                  the sender
	 */
	template <class SenderCh, class ReceiverCh>
	void connectChannels(SenderCh &sender, ReceiverCh &receiver, typename SenderCh::DemuxKey key);

	/**
	 * @brief stops the application
	 *        Force kill if a thread don't stop
	 *
	 * @param signal  The received signal
	 */
	void stop(void);

	/**
	 * @brief Initialize the manager, creates and initialize blocks
	 *
	 * @return true if succesful, false otherwise
	 */
	bool init(void);

	/**
	 * @brief Start blocks
	 *
	 */
	bool start(void);

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
	void wait(void);

	/**
	 * @brief Check if something got really wrong in the process
	 *
	 * @return true if no fatal error occured, false otherwise
	 */
	bool getStatus(void);

	/// Output Log
	std::shared_ptr<OutputLog> log_rt;

 private:
	void setupBlock(Block *block, ChannelBase *upward, ChannelBase *downward);

	bool checkConnectedBlocks(const Block *upper, const Block *lower);

	/// list of pointers to the blocks
	std::vector<Block *> block_list;

	/// check if we already tried to stop process
	bool stopped;

	/// whether a critical error was raised
	bool status;
};


template <class Bl>
Bl *BlockManager::createBlock(const std::string &name)
{
	auto *block = new Bl(name);
	auto *upward = new typename Bl::Upward(name);
	auto *downward = new typename Bl::Downward(name);
	setupBlock(block, upward, downward);
	return block;
}


template <class Bl, class Specific>
Bl *BlockManager::createBlock(const std::string &name,
                              Specific specific)
{
	auto *block = new Bl(name, specific);
	auto *upward = new typename Bl::Upward(name, specific);
	auto *downward = new typename Bl::Downward(name, specific);
	setupBlock(block, upward, downward);
	return block;
}


template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper, const LowerBl *lower)
{
	static_assert(has_one_input<typename UpperBl::Upward>::value);
	static_assert(has_one_output<typename UpperBl::Downward>::value);
	static_assert(has_one_output<typename LowerBl::Upward>::value);
	static_assert(has_one_input<typename LowerBl::Downward>::value);

	if (!this->checkConnectedBlocks(upper, lower))
	{
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	connectChannels(*lower_upward, *upper_upward);
	connectChannels(*upper_downward, *lower_downward);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper,
                                 const LowerBl *lower,
                                 typename UpperBl::Downward::DemuxKey down_key)
{
	static_assert(has_n_inputs<typename UpperBl::Upward>::value);
	static_assert(has_n_outputs<typename UpperBl::Downward>::value);
	static_assert(has_one_output<typename LowerBl::Upward>::value);
	static_assert(has_one_input<typename LowerBl::Downward>::value);

	if (!this->checkConnectedBlocks(upper, lower))
	{
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	connectChannels(*lower_upward, *upper_upward);
	connectChannels(*upper_downward, *lower_downward, down_key);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper,
                                 const LowerBl *lower,
                                 typename LowerBl::Upward::DemuxKey up_key)
{
	static_assert(has_one_input<typename UpperBl::Upward>::value);
	static_assert(has_one_output<typename UpperBl::Downward>::value);
	static_assert(has_n_outputs<typename LowerBl::Upward>::value);
	static_assert(has_n_inputs<typename LowerBl::Downward>::value);
	
	if (!this->checkConnectedBlocks(upper, lower))
	{
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	connectChannels(*lower_upward, *upper_upward, up_key);
	connectChannels(*upper_downward, *lower_downward);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper,
                                 const LowerBl *lower,
                                 typename LowerBl::Upward::DemuxKey up_key,
                                 typename UpperBl::Downward::DemuxKey down_key)
{
	static_assert(has_n_inputs<typename UpperBl::Upward>::value);
	static_assert(has_n_outputs<typename UpperBl::Downward>::value);
	static_assert(has_n_outputs<typename LowerBl::Upward>::value);
	static_assert(has_n_inputs<typename LowerBl::Downward>::value);

	if (!this->checkConnectedBlocks(upper, lower))
	{
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	connectChannels(*lower_upward, *upper_upward, up_key);
	connectChannels(*upper_downward, *lower_downward, down_key);
}


template <class SenderCh, class ReceiverCh, bool> struct ChannelsConnectorBase;


template <class SenderCh, class ReceiverCh>
struct ChannelsConnectorBase<SenderCh, ReceiverCh, true>
{
	static inline void connect(SenderCh &sender,
	                           ReceiverCh &receiver)
	{
		auto fifo = BlockManager::createFifo();
		sender.setNextFifo(fifo);
		receiver.setPreviousFifo(fifo);
	}
};


template <class SenderCh, class ReceiverCh>
struct ChannelsConnectorBase<SenderCh, ReceiverCh, false>
{
	static inline void connect(SenderCh &sender,
	                           ReceiverCh &receiver)
	{
		auto fifo = BlockManager::createFifo();
		sender.setNextFifo(fifo);
		receiver.addPreviousFifo(fifo);
	}
};


template <class SenderCh, class ReceiverCh, bool B, bool>
struct ChannelsConnector : public ChannelsConnectorBase<SenderCh, ReceiverCh, B>
{
};


template <class SenderCh, class ReceiverCh>
struct ChannelsConnector<SenderCh, ReceiverCh, true, true> : public ChannelsConnectorBase<SenderCh, ReceiverCh, true>
{
	static inline void connect(SenderCh &sender,
	                           ReceiverCh &receiver,
	                           typename SenderCh::DemuxKey key)
	{
		auto fifo = BlockManager::createFifo();
		sender.addNextFifo(key, fifo);
		receiver.setPreviousFifo(fifo);
	}
};


template <class SenderCh, class ReceiverCh>
struct ChannelsConnector<SenderCh, ReceiverCh, false, true> : public ChannelsConnectorBase<SenderCh, ReceiverCh, false>
{
	static inline void connect(SenderCh &sender,
	                           ReceiverCh &receiver,
	                           typename SenderCh::DemuxKey key)
	{
		auto fifo = BlockManager::createFifo();
		sender.addNextFifo(key, fifo);
		receiver.addPreviousFifo(fifo);
	}
};


template <class SenderCh, class ReceiverCh>
void BlockManager::connectChannels(SenderCh &sender,
                                   ReceiverCh &receiver)
{
	ChannelsConnector<SenderCh, ReceiverCh, has_one_input<ReceiverCh>::value, false>::connect(sender, receiver);
}


template <class SenderCh, class ReceiverCh>
void BlockManager::connectChannels(SenderCh &sender,
                                   ReceiverCh &receiver,
                                   typename SenderCh::DemuxKey key)
{
	ChannelsConnector<SenderCh, ReceiverCh, has_one_input<ReceiverCh>::value, true>::connect(sender, receiver, key);
}


};  // namespace Rt


#endif
