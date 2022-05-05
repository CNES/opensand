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

#include "Block.h"
#include "MessageEvent.h"
#include "RtFifo.h"

#include <opensand_output/OutputLog.h>

#include "TemplateHelper.h"
#include <list>
#include <string>

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
	template <class Bl>
	Bl *createBlock(const std::string &name)
	{
		auto *block = new Bl(name);
		auto *upward = new typename Bl::Upward(name);
		auto *downward = new typename Bl::Downward(name);
		setupBlock(block, upward, downward);
		return block;
	}

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
	Bl *createBlock(const std::string &name,
	                Specific specific);

	/**
	 * @brief Connects two blocks
	 *
	 * @param upper     The upper block
	 * @param upper     The lower block
	 */
	template <class UpperBl, class LowerBl>
	void connectBlocks(const UpperBl *upper,
	                   const LowerBl *lower);

	/**
	 * @brief Connects a multiplexer block to a simple block
	 *
	 * @param upper     The upper block, with a Mux upward channel
	 *                  and a Demux downward channel
	 * @param upper     The lower block
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
	 * @param upper     The lower block, with a Demux upward channel
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
	 * @param upper     The lower block, with a Demux upward channel
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
	 * @brief stops the application
	 *        Force kill if a thread don't stop
	 *
	 * @param signal  The received signal
	 */
	void stop(int signal);

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
	void reportError(const char *msg, bool critical);

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
	void setupBlock(Block *block, RtChannelBase *upward, RtChannelBase *downward);

	/// list of pointers to the blocks
	std::list<Block *> block_list;

	/// check if we already tried to stop process
	bool stopped;

	/// whether a critical error was raised
	bool status;
};

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
	static_assert(has_one_input<typename LowerBl::Upward>::value);
	static_assert(has_one_output<typename LowerBl::Downward>::value);

	if (!upper || !lower)
	{
		LOG(log_rt, LEVEL_ERROR, "One of the blocks to connect is null");
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	// Should never fail if all blocks are initialized using the createBlock() function
	assert(lower_upward && lower_downward && upper_upward && upper_downward && "Incoherent types of blocks");

	RtFifo *up_fifo = new RtFifo();
	RtFifo *down_fifo = new RtFifo();

	// connect upward fifo to both blocks
	lower_upward->setNextFifo(up_fifo);
	upper_upward->setPreviousFifo(up_fifo);

	// connect downward fifo to both blocks
	lower_downward->setPreviousFifo(down_fifo);
	upper_downward->setNextFifo(down_fifo);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper,
                                 const LowerBl *lower,
                                 typename UpperBl::Downward::DemuxKey down_key)
{
	static_assert(has_n_inputs<typename UpperBl::Upward>::value);
	static_assert(has_n_outputs<typename UpperBl::Downward>::value);
	static_assert(has_one_input<typename LowerBl::Upward>::value);
	static_assert(has_one_output<typename LowerBl::Downward>::value);

	if (!upper || !lower)
	{
		LOG(log_rt, LEVEL_ERROR, "One of the blocks to connect is null");
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	// Should never fail if all blocks are initialized using the createBlock() function
	assert(lower_upward && lower_downward && upper_upward && upper_downward && "Incoherent types of blocks");

	RtFifo *up_fifo = new RtFifo();
	RtFifo *down_fifo = new RtFifo();

	// connect upward fifo to both blocks
	lower_upward->setNextFifo(up_fifo);
	upper_upward->addPreviousFifo(up_fifo);

	// connect downward fifo to both blocks
	lower_downward->setPreviousFifo(down_fifo);
	upper_downward->addNextFifo(down_key, down_fifo);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper,
                                 const LowerBl *lower,
                                 typename LowerBl::Upward::DemuxKey up_key)
{
	static_assert(has_one_input<typename UpperBl::Upward>::value);
	static_assert(has_one_output<typename UpperBl::Downward>::value);
	static_assert(has_n_inputs<typename LowerBl::Upward>::value);
	static_assert(has_n_outputs<typename LowerBl::Downward>::value);
	
	if (!upper || !lower)
	{
		LOG(log_rt, LEVEL_ERROR, "One of the blocks to connect is null");
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	// Should never fail if all blocks are initialized using the createBlock() function
	assert(lower_upward && lower_downward && upper_upward && upper_downward && "Incoherent types of blocks");

	RtFifo *up_fifo = new RtFifo();
	RtFifo *down_fifo = new RtFifo();

	// connect upward fifo to both blocks
	lower_upward->addNextFifo(up_key, up_fifo);
	upper_upward->setPreviousFifo(up_fifo);

	// connect downward fifo to both blocks
	lower_downward->addPreviousFifo(down_fifo);
	upper_downward->setNextFifo(down_fifo);
}

template <class UpperBl, class LowerBl>
void BlockManager::connectBlocks(const UpperBl *upper,
                                 const LowerBl *lower,
                                 typename LowerBl::Upward::DemuxKey up_key,
                                 typename UpperBl::Downward::DemuxKey down_key)
{
	static_assert(has_n_inputs<typename UpperBl::Upward>::value);
	static_assert(has_n_outputs<typename UpperBl::Downward>::value);
	static_assert(has_n_inputs<typename LowerBl::Upward>::value);
	static_assert(has_n_outputs<typename LowerBl::Downward>::value);

	if (!upper || !lower)
	{
		LOG(log_rt, LEVEL_ERROR, "One of the blocks to connect is null");
		return;
	}

	auto lower_upward = dynamic_cast<typename LowerBl::Upward *>(lower->upward);
	auto lower_downward = dynamic_cast<typename LowerBl::Downward *>(lower->downward);
	auto upper_upward = dynamic_cast<typename UpperBl::Upward *>(upper->upward);
	auto upper_downward = dynamic_cast<typename UpperBl::Downward *>(upper->downward);

	// Should never fail if all blocks are initialized using the createBlock() function
	assert(lower_upward && lower_downward && upper_upward && upper_downward && "Incoherent types of blocks");

	RtFifo *up_fifo = new RtFifo();
	RtFifo *down_fifo = new RtFifo();

	// connect upward fifo to both blocks
	lower_upward->addNextFifo(up_key, up_fifo);
	upper_upward->addPreviousFifo(up_fifo);

	// connect downward fifo to both blocks
	lower_downward->addPreviousFifo(down_fifo);
	upper_downward->addNextFifo(down_key, down_fifo);
}

#endif
