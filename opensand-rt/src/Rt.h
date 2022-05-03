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
#include "RtChannelBase.h"
#include "RtEvent.h"
#include "RtMutex.h"
#include "Types.h"
#include "NetSocketEvent.h"
#include "MessageEvent.h"
#include "TimerEvent.h"
#include "TcpListenEvent.h"
#include "FileEvent.h"
#include "SignalEvent.h"


/**
 * @class Rt
 *
 * @brief The interface for opensand-rt
 */
class Rt
{
	friend class RtChannelBase;
	friend class SignalEvent;

  public:

	Rt();
	~Rt();

	/**
	 * @brief Creates and adds a block to the application
	 *        The block should be created from upper to lower
	 *
	 * @tparam Bl       The block class
	 * @tparam Up       The upward channel class
	 * @tparam Down     The downward channel class
	 * @param name      The name of the block
	 * @param upper     The upper block or NULL if none
	 * @return the block
	 */
	template<class Bl, class Up, class Down>
	static Block *createBlock(const string &name,
	                          Block *const upper = NULL);

	/**
	 * @brief Creates and adds a block to the application
	 *        The block should be created from upper to lower
	 *
	 * @tparam Bl       The block class
	 * @tparam Up       The upward channel class
	 * @tparam Down     The downward channel class
	 * @tparam T        The type of the specific parameter
	 * @param name      The name of the block
	 * @param upper     The upper block or NULL if none
	 * @param specific  User defined data
	 * @return the block
	 */
	template<class Bl, class Up, class Down, class T>
	static Block *createBlock(const string &name,
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
	static Block *createMuxBlock(const string &name,
	                             Specific &&...specific);

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
	static Block *createMuxedBlock(const string &name,
	                               Key key,
	                               Block *upper,
	                               Specific &&...specific);

	/**
	 * @brief Initialize the blocks
	 *
	 * @return true on success, false otherwise
	 */
	static bool init(void);

	/**
	 * @brief Start the blocks and checks if threads
	 *        and application are alive
	 *        Exits when application goes stopped
	 *
	 * @param init  whether the run do the initialization or not
	 * @return true on success, false otherwise
	 */
	static bool run(bool init=false);

	/**
	 * @param Internal error report
	 *
	 * @param name        The name of the block or element in which the error occured
	 * @param thread_id   Identifier of the reporting thread
	 * @param critical    Whether the application should be stopped
	 * @param msg_format  The error
	 */
	static void reportError(const string &name, pthread_t thread_id,
	                        bool critical, const char *msg_format, ...);

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
Block *Rt::createBlock(const string &name,
                       Block *const upper)
{
	return Rt::manager.createBlock<Bl, Up, Down>(name, upper);
}

template<class Bl, class Up, class Down, class T>
Block *Rt::createBlock(const string &name,
                       Block *const upper,
                       T specific)
{
	return Rt::manager.createBlock<Bl, Up, Down, T>(name, upper, specific);
}

template <class Bl, class Up, class Down, class... Specific>
Block *Rt::createMuxBlock(const string &name,
                          Specific &&...specific)
{
	return Rt::manager.createMuxBlock<Bl, Up, Down>(name, specific...);
}

template <class Bl, class Up, class Down, class Key, class... Specific>
Block *Rt::createMuxedBlock(const string &name,
                            Key key,
                            Block *upper,
                            Specific &&...specific)
{
	return Rt::manager.createMuxedBlock<Bl, Up, Down>(name, key, upper, specific...);
}

#endif
