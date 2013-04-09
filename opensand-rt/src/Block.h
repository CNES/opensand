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


class Block
{
  public:
	Block(Channel* backward, Channel* forward);
	~Block();

	Block *GetBackwardAddress(void){return previous_block;};
	Block *GetForwardAddress(void) {return next_block;};

	Channel *GetBackwardChannel(void){return backward;};
	Channel *GetForwardChannel(void) {return forward;};

  protected:

	bool CreateTimer(uint32_t duration_ms, bool auto_rearm);
	bool Init(void);
	bool Sleep(void);
	bool Wake(void);
	bool Start(void);

	void Stop(void);
	void * StartThread(void *pthis);

	void SetbackwardAddress(Block* previous){previous_block = previous;};
	void SetForwardAddress(Block* next){next_block = next;};

	Channel *backward;
	Channel *forward;

	Block *previous_block;
	Block *next_block;


  private:
#ifdef DEBUG_BLOCK_MUTEX
	pthread_mutex_t mutex; //Mutex for critical section
#endif
	friend class BlockMgr; // allow the block manager to call protected methods
};

#endif

