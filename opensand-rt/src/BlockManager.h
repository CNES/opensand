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
 * @file BlockManager.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The manager that handles blocks
 *
 */

#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H


#include <list>
#include <string>

#include "Block.h"
#include "MessageEvent.h"


using std::string;
using std::list;

class Block;

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
	 * @param name   The block name
	 * @param upper  The upper block or NULL if none
	 * @return The block
	 */
	template<class Bl, class Up, class Down>
	Block *createBlock(const string &name, Block *const upper);

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
	 * @param Internal error report
	 *
	 * @param name       The name of the block or element in which the error occured
	 * @param thread_id  Identifier of the reporting thread
	 * @param critical   Whether the application should be stopped
	 * @param error      The error message
	 * @param val        The return value
	 */
	void reportError(const string &name, pthread_t thread_id,
	                 bool critical, string error, int val);

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


   private:

	/// list of pointers to the blocks
	list<Block *> block_list;

	/// check if we already tried to stop process
	bool stopped;

	/// whether a critical error was raised
	bool status;

};

template<class Bl, class Up, class Down>
Block *BlockManager::createBlock(const string &name, Block *const upper)
{
	Block *block = new Bl(name);
	Block::Upward *up = new Up(*block);
	Block::Downward *down = new Down(*block);

	block->upward = up;
	block->downward = down;

	if(upper)
	{
		Fifo *up_fifo = new Fifo();
		Fifo *down_fifo = new Fifo();

		// set upward fifo for upper block
		up->setNextFifo(up_fifo);
		upper->getUpwardChannel()->setFifo(up_fifo);

		// set downward fifo for block
		down->setFifo(down_fifo);
		upper->getDownwardChannel()->setNextFifo(down_fifo);
	}

	this->block_list.push_back(block);

	return block;
}


#endif


