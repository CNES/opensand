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

#include <thread>

#include "BlockManager.h"
/*
#include "Block.h"
#include "RtChannelBase.h"
#include "RtEvent.h"
#include "RtMutex.h"
#include "Types.h"
*/


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
	/**
	 * @brief Creates and adds a block to the application.
	 *
	 * @tparam Bl       The block class
	 * @param name      The block name
	 * @return A pointer to the newly created block
	 */
	template <class Bl>
	static Bl *createBlock(const std::string &name);

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
	static Bl *createBlock(const std::string &name,
	                       Specific specific);

	/**
	 * @brief Connects two blocks
	 *
	 * @param upper     The upper block
	 * @param lower     The lower block
	 */
	template <class UpperBl, class LowerBl>
	static void connectBlocks(const UpperBl *upper, const LowerBl *lower);

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
	static void connectBlocks(const UpperBl *upper,
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
	static void connectBlocks(const UpperBl *upper,
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
	static void connectBlocks(const UpperBl *upper,
	                          const LowerBl *lower,
	                          typename LowerBl::Upward::DemuxKey up_key,
	                          typename UpperBl::Downward::DemuxKey down_key);

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
	static bool run(bool init = false);

	/**
	 * @param Internal error report
	 *
	 * @param name        The name of the block or element in which the error occured
	 * @param thread_id   Identifier of the reporting thread
	 * @param critical    Whether the application should be stopped
	 * @param msg_format  The error
	 */
	static void reportError(const std::string &name, std::thread::id thread_id,
	                        bool critical, const char *msg_format, ...);

	/**
	 * @brief Send a stop signal
	 *
	 * @param signal  The signal to raise
	 */
	static void stop();

 private:
	/// The block manager instance
	static BlockManager manager;
};


template <class Bl>
Bl *Rt::createBlock(const std::string &name)
{
	return Rt::manager.createBlock<Bl>(name);
}


template <class Bl, class Specific>
Bl *Rt::createBlock(const std::string &name, Specific specific)
{
	return Rt::manager.createBlock<Bl>(name, specific);
}


template <class UpperBl, class LowerBl>
void Rt::connectBlocks(const UpperBl *upper, const LowerBl *lower)
{
	Rt::manager.connectBlocks(upper, lower);
}


template <class UpperBl, class LowerBl>
void Rt::connectBlocks(const UpperBl *upper,
                       const LowerBl *lower,
                       typename UpperBl::Downward::DemuxKey down_key)
{
	Rt::manager.connectBlocks(upper, lower, down_key);
}


template <class UpperBl, class LowerBl>
void Rt::connectBlocks(const UpperBl *upper,
                       const LowerBl *lower,
                       typename LowerBl::Upward::DemuxKey up_key)
{
	Rt::manager.connectBlocks(upper, lower, up_key);
}


template <class UpperBl, class LowerBl>
void Rt::connectBlocks(const UpperBl *upper,
                       const LowerBl *lower,
                       typename LowerBl::Upward::DemuxKey up_key,
                       typename UpperBl::Downward::DemuxKey down_key)
{
	Rt::manager.connectBlocks(upper, lower, up_key, down_key);
}


#endif
