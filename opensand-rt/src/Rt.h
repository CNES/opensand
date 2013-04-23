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
 * @file Rt.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  High level interface for opensand-rt
 *
 */


#ifndef OPENSAND_RT_H
#define OPENSAND_RT_H


#include "BlockManager.h"
#include "Block.h"

class BlockManager;
class Block;

class Rt
{
	friend class Channel;
	friend class SignalEvent;

  public:

	Rt();
	~Rt();

	/**
	 * @brief Creates and adds a block to the application
	 *        The block should be created from upper to lower
	 *
	 * @param name   The name of the block
	 * @param upper  The upper block or NULL if none
	 * @return the block
	 */
	template<class Bl, class Up, class Down>
	static Block *createBlock(const string &name, Block *const upper = NULL);

	/**
	 * @brief Start the blocks and checks if threads
	 *        and application are alive
	 *        Exits when application goes stopped
	 *
	 * @return true on success, false otherwise
	 */
	static bool run(void);

	/**
	 * @param Internal error report
	 *
	 * @param name       The name of the block or element in which the error occured
	 * @param thread_id  Identifier of the reporting thread
	 * @param critical   Whether the application should be stopped
	 * @param val        The error return value (0 for none)
	 * @param error      The error message
	 */
	static void reportError(const string &name, pthread_t thread_id,
	                        bool critical, string error, int val = 0);

	/**
	 * @brief Send a stop signal
	 *
	 * @param signal  The signal to raise
	 */
	static void stop(int signal);

   private:

	/// The block manager instance
	static BlockManager manager;

};

template<class Bl, class Up, class Down>
Block *Rt::createBlock(const string &name, Block *const upper)
{
	return Rt::manager.createBlock<Bl, Up, Down>(name, upper);
}

#endif


