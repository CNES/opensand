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


class BlockMgr {


public:
    Block* CreateBlock(Channel *backward, Channel *forward, bool first = false);
    void SetBlockHierarchy(Block *block, Block *backward_block, Block *forward_block);
    void Stop(bool hard = false);
    bool Init(void);
    void Start(void);
    void Pause(void);
    void Resume(void);
	static BlockMgr* GetInstance(void);
    static void Kill (void);
    static void ReportError(pthread_t thread_id, bool critical, string error="");
private:

    static BlockMgr *singleton;

    list<Block *> block_list;
    Block * first_block;

    BlockMgr();
    ~BlockMgr();

};



#endif


