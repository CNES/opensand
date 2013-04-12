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
/* $Id: Block.h,v 1.1.1.1 2013/04/03 11:37:12 cgaillardet Exp $ */

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "Block.h"

Block::Block(Channel* backward, Channel* forward) :
    backward(backward),
    forward(forward),
	previous_block(NULL),
	next_block(NULL)
{
#ifdef DEBUG_BLOCK_MUTEX
    pthread_mutex_init(&(this->mutex),NULL);
#endif
}

bool Block::Init(void)
{
    bool res = true;

#ifdef DEBUG_BLOCK_MUTEX
   	res = forward->Init(&this->mutex);
	res = res && backward->Init(&this->mutex);
#else

	res = forward->Init();
	res = res && backward->Init();
#endif

	res = res && forward->CustomInit();
	res = res && backward->CustomInit();

    return res;
}

void Block::Pause(void)
{
    backward->Pause();
	forward->Pause();
}

void Block::Start(void)
{
    backward->Start();
	forward->Start();
}

void Block::Stop(void)
{
    this->~Block();
}

Block::~Block()
{
    if (this->backward != NULL)
    {
        delete this->backward;
    }

    if (this->forward != NULL)
    {
        delete this->forward;
    }


}
