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

#include <list>
#include <string>

using std::string;
using std::list;


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
	 * @brief Creates and adds a block to the application
	 *        The block should be created from upper to lower
	 *
	 * @tparam Bl         The block class
	 * @tparam Up       The upward channel class
	 * @tparam Down     The downward channel class
	 * @param name      The block name
	 * @param upper     The upper block or NULL if none
	 * @return The block
	 */
	template<class Bl, class Up, class Down>
	Block *createBlock(const string &name,
	                   Block *const upper = NULL);

	/**
	 * @brief Creates and adds a block to the application
	 *        The block should be created from upper to lower
	 *
	 * @tparam Bl       The block class
	 * @tparam Up       The upward channel class
	 * @tparam Down     The downward channel class
	 * @tparam T        The type of the specific parameter
	 * @param name      The block name
	 * @param upper     The upper block or NULL if none
	 * @param specific  User defined data
	 * @return The block
	 */
	template<class Bl, class Up, class Down, class T>
	Block *createBlock(const string &name,
	                   Block *const upper,
	                   T specific);

	/**
	 * @brief Creates and adds a multiplexer block to the top of
	 *        the application.
	 *
	 * @tparam Bl       The block class
	 * @tparam Up       The upward channel class
	 * @tparam Down     The downward channel class
	 * @tparam Specific The type of the specific parameter, if any
	 * @param name      The block name
	 * @param specific  User defined data (optional)
	 * @return The block
	 */
	template <class Bl, class Up, class Down, class... Specific>
	Block *createMuxBlock(const string &name,
	                      Specific &&... specific);

	/**
	 * @brief Creates and adds a block to the application, below a
	 *        multiplexer block.
	 *
	 * @tparam Bl       The block class
	 * @tparam Up       The upward channel class
	 * @tparam Down     The downward channel class
	 * @tparam Specific The type of the specific parameter, if any
	 * @param name      The block name
	 * @param key       The key of this block, used to send messages 
	 *                  from the multiplexer to this block
	 * @param upper     The upper multiplexer block
	 * @param specific  User defined data (optional)
	 * @return The block
	 */
	template <class Bl, class Up, class Down, class Key, class... Specific>
	Block *createMuxedBlock(const string &name,
	                        Key key,
	                        Block *upper,
	                        Specific &&... specific);

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

	/// list of pointers to the blocks
	list<Block *> block_list;

	/// check if we already tried to stop process
	bool stopped;

	/// whether a critical error was raised
	bool status;
};

template<class Bl, class Up, class Down>
Block *BlockManager::createBlock(const string &name,
                                 Block *const upper)
{
	Block *block = new Bl(name);

	Up *up = new Up(name);
	Down *down = new Down(name);

	RtFifo *up_opp_fifo = new RtFifo();
	RtFifo *down_opp_fifo = new RtFifo();

	block->upward = up;
	block->downward = down;

	// set opposite fifo
	up->setOppositeFifo(up_opp_fifo, down_opp_fifo);
	down->setOppositeFifo(down_opp_fifo, up_opp_fifo);

	if(upper)
	{
		auto upper_up = dynamic_cast<RtChannel *>(upper->getUpwardChannel());
		auto upper_down = dynamic_cast<RtChannel *>(upper->getDownwardChannel());

		assert(upper_up && upper_down && "Incoherent types of blocks");

		RtFifo *up_fifo = new RtFifo();
		RtFifo *down_fifo = new RtFifo();

		// set upward fifo for upper block
		up->setNextFifo(up_fifo);
		upper_up->setPreviousFifo(up_fifo);

		// set downward fifo for block
		down->setPreviousFifo(down_fifo);
		upper_down->setNextFifo(down_fifo);
	}

	this->block_list.push_back(block);

	return block;
}

template<class Bl, class Up, class Down, class T>
Block *BlockManager::createBlock(const string &name,
                                 Block *const upper,
                                 T specific)
{
	Block *block = new Bl(name, specific);

	Up *up = new Up(name, specific);
	Down *down = new Down(name, specific);

	RtFifo *up_opp_fifo = new RtFifo();
	RtFifo *down_opp_fifo = new RtFifo();

	block->upward = up;
	block->downward = down;

	// set opposite fifo
	up->setOppositeFifo(up_opp_fifo, down_opp_fifo);
	down->setOppositeFifo(down_opp_fifo, up_opp_fifo);

	if(upper)
	{
		auto upper_up = dynamic_cast<RtChannel *>(upper->getUpwardChannel());
		auto upper_down = dynamic_cast<RtChannel *>(upper->getDownwardChannel());

		assert(upper_up && upper_down && "Incoherent types of blocks");

		RtFifo *up_fifo = new RtFifo();
		RtFifo *down_fifo = new RtFifo();

		// set upward fifo for upper block
		up->setNextFifo(up_fifo);
		upper_up->setPreviousFifo(up_fifo);

		// set downward fifo for block
		down->setPreviousFifo(down_fifo);
		upper_down->setNextFifo(down_fifo);
	}

	this->block_list.push_back(block);

	return block;
}

template <class Bl, class Up, class Down, class... Specific>
Block *BlockManager::createMuxBlock(const string &name,
                                    Specific &&...specific)
{
	static_assert(std::is_base_of<Block::RtUpwardMux, Up>::value, "Up must derive from RtUpwardMux");
	static_assert(std::is_base_of<Block::RtDownwardDemux<typename Down::DemuxKey>, Down>::value, "Down must derive from RtDownwardDemux");

	return createBlock<Bl, Up, Down>(name, nullptr, specific...);
}

template <class Bl, class Up, class Down, class Key, class... Specific>
Block *BlockManager::createMuxedBlock(const string &name,
                                    Key key,
                                    Block *upper,
                                    Specific &&...specific)
{
	assert(upper != nullptr && "A muxed block cannot be at the top");
	static_assert(std::is_base_of<RtChannel, Up>::value, "Up must derive from RtChannel");
	static_assert(std::is_base_of<RtChannel, Down>::value, "Down must derive from RtChannel");

	Block *block = createBlock<Bl, Up, Down>(name, nullptr, specific...);

	RtFifo *up_fifo = new RtFifo();
	RtFifo *down_fifo = new RtFifo();

	auto up = dynamic_cast<RtChannel *>(block->getUpwardChannel());
	auto down = dynamic_cast<RtChannel *>(block->getDownwardChannel());

	auto upper_up = dynamic_cast<RtChannelMux *>(upper->getUpwardChannel());
	auto upper_down = dynamic_cast<RtChannelDemux<Key> *>(upper->getDownwardChannel());
	
	assert(up && down && upper_up && upper_down && "Incoherent types of blocks");

	// set upward fifo for upper block
	up->setNextFifo(up_fifo);
	upper_up->addPreviousFifo(up_fifo);

	// set downward fifo for block
	down->setPreviousFifo(down_fifo);
	upper_down->addNextFifo(key, down_fifo);
	return block;
}

#endif


