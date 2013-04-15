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
/* $Id: BlockMgr.h,v 1.1.1.1 2013/03/28 11:29:21 cgaillardet Exp $ */


#ifndef BLOCKMGR_H
#define BLOCKMGR_H


#include  <list>
#include <string>

#include "Block.h"
#include "Channel.h"
#include "MsgEvent.h"


using std::string;
using std::list;

class Channel;
class Block;


/**
  * @class BlockMgr
  * @brief Interface for operations on runtime library. Singleton.
  *
  *
  */

class BlockMgr {


public:

	/*
	 * CreateBlock creates and adds a block to the application.
	 * Developers must create two channels before calling that method
	 *
	 * @param backward pointer to the block backward channel
	 * @param forward pointer to the block forward channel
	 * @param first boolean to signal this will be the first block in the hierarchy
	 *
	 * @return address of the newly created block
	 *
	 */
	Block* CreateBlock(Channel *backward, Channel *forward, bool first = false);

     /*
	 * SetBlockHierarchy sets block hierarchy
	 *
	 * @param block pointer to the block to be set
	 * @param backward_block pointer to the first parameter previous block
	 * @param first forward_block pointer to the first parameter next block
	 *
	 */
    void SetBlockHierarchy(Block *block, Block *backward_block, Block *forward_block);


     /*
	 * Stop stops the application. when parameter is true, use SIGSTOP (brutal)
	 * otherwise ask each block to stop itself. Default is soft stop.
     *
	 * @param hard boolean: true: hard stop, false: soft (cleaner) stop.
	 *
	 */
	void Stop(bool hard = false);

     /*
	 * Init ititialize the manager, creates block pipes if needed then ask each block to init itself
     *
	 * @return true if succesful, false otherwise
	 */
	bool Init(void);

     /*
	 * Start calls each block Start() method
     *
	 */
	void Start(void);

     /*
	 * Pause calls each block Pause() method
     *
	 */
	void Pause(void);

     /*
	 * Resume calls each block Start() method.
	 * Currently does the same as Start() but may differ later
     *
	 */
	void Resume(void);


     /*
	 * GetInstance getter for the singleton. Creates the manager if it does not exist
     *
     * @return pointer to the block manager
	 */
	static BlockMgr* GetInstance(void);


     /*
	 * Kill destroys the singleton
     *
	 */
	static void Kill (void);


     /*
	 * ReportError allows to report an error from anywhere
     *
     * @param thread_id identifier of the reporting thread
     * @param critical stops the application if true
     * @param error string containing the error message to show
	 */
    static void ReportError(pthread_t thread_id, bool critical, string error="");

     /*
	 * IsAlive getter for the alive boolean
     *
     * @return true if application is alive, false if terminating
	 */
    bool IsAlive (void) {return this->alive;};

     /*
	 * RunLoop checks if threads and application are alive or should be stopped
     * exits only when application goes stopped.
	 */
    void RunLoop(void);
   private:

    /// block manager singleton
	static BlockMgr *singleton;

    /// boolean storing application state
    bool alive;

    /// list of pointers to the application blocks
	list<Block *> block_list;

    /// pointer to the first block of the application, for fast access
	Block *first_block;

	BlockMgr();
	~BlockMgr();

};



#endif


