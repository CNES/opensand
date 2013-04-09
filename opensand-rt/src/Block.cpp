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

#include "Block.h"



Block::Block(Channel* backward, Channel* forward) :
    backward(backward),
    forward(forward),
	previous_block(NULL),
	next_block(NULL)
{

}

bool Block::Init(void)
{
    bool res = false;

#ifdef DEBUG_BLOCK_MUTEX
	res = forward->Init(&this->mutex) &&  backward->Init(&this->mutex);
#endif
	res = forward->Init() &&  backward->Init();
	res &= forward->CustomInit() &&  backward->CustomInit();
    return res;
}

bool Block::Sleep(void)
{
    bool res = false;
	res = forward->Sleep() &&  backward->Sleep();
    return res;
}

bool Block::Wake(void)
{
    bool res = false;
	res = forward->Wake() &&  backward->Wake();
    return res;
}

bool Block::Start(void)
{
    bool res = false;
	res = forward->Start() &&  backward->Start();
    return res;
}

void * Block::StartThread(void *pthis)
{
    forward->StartThread(pthis);
	backward->StartThread(pthis);
}

void Block::Stop(void)
{
    this->~Block();
}

Block::~Block()
{
    if (this->forward != NULL)
    {
        delete this->forward;
    }

    if (this->backward != NULL)
    {
        delete this->backward;
    }

}
