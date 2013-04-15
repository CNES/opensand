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
/* $Id: Block.h,v 1.1.1.1 2013/04/02 14:47:23 cgaillardet Exp $ */



#ifndef BLOCK_H
#define BLOCK_H

#include <stdlib.h>
#include  <string>
#include <list>
#include <vector>
#include <sys/select.h>

#include "Types.h"
#include "Channel.h"
#include "BlockMgr.h"

class BlockMgr; // for friendly declaration


/**
  * @class Block
  * @brief describes a block
  *
  * next block and previous block are absolute;
  * forward channel processes data from previous block to next block.
  * backward channel processes data from previous block to next block.
  *
  */
class Block
{
  public:

	/*
	 * Constructor
	 *
	 * @param backward pointer to the block backward channel
	 * @param forward pointer to the block forward channel
	 *
	 */

	Block(Channel* backward, Channel* forward);
	~Block();


	/*
	 * GetBackwardAddress
	 *
	 *
	 * @return address of the previous block if defined (otherwise NULL)
	 */
	Block *GetBackwardAddress(void){return previous_block;};

	/*
	 * GetForwardAddress
	 *
	 *
	 * @return address of the next block if defined (otherwise NULL)
	 */
	Block *GetForwardAddress(void) {return next_block;};

	/*
	 * GetBackwardChannel
	 *
	 *
	 * @return address of the backward going channel if defined (otherwise NULL)
	 */
	Channel *GetBackwardChannel(void){return backward;};


	/*
	 * GetForwardChannel
	 *
	 *
	 * @return address of the forward going channel if defined (otherwise NULL)
	 */
	Channel *GetForwardChannel(void) {return forward;};

  protected:

	/*
	 * Init calls Init() and CustomInit() of its defined channels
	 *
	 * @return true if all inits OK, false otherwise
	 */
	bool Init(void);

	/*
	 * Pause calls Pause() of its defined channels
	 *
	 */
    void Pause(void);

	/*
	 * Start calls Start() of its defined channels
	 *
	 */
    void Start(void);

	/*
	 * Stop calls the block destructor
	 *
	 */
	void Stop(void);


	/*
	 * SetbackwardAddress setter for previous block
	 *
	 * @param previous pointer to the previous block
	 *
	 */
	void SetbackwardAddress(Block* previous){previous_block = previous;};


	/*
	 * SetForwardAddress setter for next block
	 *
	 * @param previous pointer to the next block
	 *
	 */
	void SetForwardAddress(Block* next){next_block = next;};



    /// pointer to the forward channel, if the block has one.
	Channel *backward;
    /// pointer to the backward channel, if the block has one.
	Channel *forward;

    /// pointer to the previous block, if there is one.
	Block *previous_block;
    /// pointer to the next block, if there is one.
	Block *next_block;


  private:
#ifdef DEBUG_BLOCK_MUTEX
	pthread_mutex_t mutex; //Mutex for critical section
#endif
	friend class BlockMgr; // allow the block manager to call protected methods
};

#endif

