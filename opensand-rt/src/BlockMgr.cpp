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
/* $Id: BlockMgr.cpp,v 1.1.1.1 2013/04/02 11:42:27 cgaillardet Exp $ */


#include "BlockMgr.h"

#include <signal.h>
#include <iostream>



// Initialisation du singleton à NULL
BlockMgr *BlockMgr::singleton = NULL;


Block* BlockMgr::CreateBlock(Channel *backward, Channel *forward, bool b_first)
{
	Block *block = NULL;

	if((forward != NULL) || (backward != NULL))
	{
		block = new Block (backward,forward);
		this->block_list.push_back(block);

		if(true == b_first)
		{
			this->first_block = block;


		}
	}
	return block;
}


void BlockMgr::SetBlockHierarchy(Block *block, Block *backward_block,
                                 Block *forward_block)
{
	block->SetForwardAddress(forward_block);
	block->SetbackwardAddress(backward_block);
}



void BlockMgr::Stop(bool b_hard)
{
	if(true == b_hard) // send a stop signal to each block
	{
		raise(SIGSTOP);
	}
	else // stop each block
	{
		for(list<Block *>::iterator iter= this->block_list.begin();
		    iter != this->block_list.end(); ++iter)
		{
			if(*iter !=NULL)
			{
				(*iter)->Stop();
			}
		}
	}

}


bool BlockMgr::Init(void)
{
	Block *current_block_address= this->first_block;
	Block *previous_block_address = NULL;
	int32_t pipe_fd_from_previous_block[2];
	int32_t pfd[2];
	bool has_first = false;
	bool has_last = false;
	bool result = true;

	// [0] contains backward direction, [1] contains forward direction
	pipe_fd_from_previous_block[0] = -1;
	pipe_fd_from_previous_block[1] = -1;

	//first, create every pipe and check blocks hierarchy
	while(current_block_address != NULL && result == true &&
	      has_last == false)
	{
		// is it the first block ?
		if((has_first == false) &&
		   (current_block_address->GetBackwardAddress() == NULL))
		{
			//the first block has no previous
			if(current_block_address->GetForwardAddress() != NULL)
			{
				//create pipe from first to second block (backward)
				if(pipe(pfd) == -1)
				{
					result = false;
				}

				current_block_address->GetBackwardChannel()->SetPipeToPrevious(pfd[0]);
				pipe_fd_from_previous_block[0] = pfd[0];

				//create pipe from first to second block (forward)
				if(pipe(pfd) == -1)
				{
					result = false;
				}

				current_block_address->GetForwardChannel()->SetPipeToNext(pfd[0]);
				pipe_fd_from_previous_block[1] = pfd[1];

				previous_block_address = current_block_address;
				current_block_address = previous_block_address->GetForwardAddress();
			}
			else // if it is also the last block, no pipe required
			{
				has_last = true;
			}
			has_first = true;
		}

		//is it a last block ?
		else if(current_block_address->GetForwardAddress() == NULL)
		{
			if((pipe_fd_from_previous_block[0] <0) || (pipe_fd_from_previous_block[1] <0))
			{
				result = false;
			}

			current_block_address->GetBackwardChannel()->SetPipeFromNext(pipe_fd_from_previous_block[0]);
			current_block_address->GetForwardChannel()->SetPipeFromPrevious(pipe_fd_from_previous_block[1]);

			//create pipe to previous block (backward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}

			current_block_address->GetBackwardChannel()->SetPipeToNext(pfd[0]);
			previous_block_address->GetBackwardChannel()->SetPipeFromPrevious(pfd[1]);


			//create pipe to previous block (forward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}

			current_block_address->GetForwardChannel()->SetPipeToPrevious(pfd[0]);
			previous_block_address->GetForwardChannel()->SetPipeFromNext(pfd[1]);
			has_last = true;
		}
		else //it is a middle block
		{
			if((pipe_fd_from_previous_block[0] <0) || (pipe_fd_from_previous_block[1] <0))
			{
				result = false;
			}

			current_block_address->GetBackwardChannel()->SetPipeFromNext(pipe_fd_from_previous_block[0]);
			current_block_address->GetForwardChannel()->SetPipeFromPrevious(pipe_fd_from_previous_block[1]);

			//create pipe to previous (backward)

			if(pipe(pfd) == -1)
			{
				result = false;
			}

			current_block_address->GetBackwardChannel()->SetPipeToNext(pfd[0]);
			previous_block_address->GetBackwardChannel()->SetPipeFromPrevious(pfd[1]);



			//create pipe to previous (forward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}

			current_block_address->GetForwardChannel()->SetPipeToPrevious(pfd[0]);
			previous_block_address->GetForwardChannel()->SetPipeFromNext(pfd[1]);

			//create pipe to next (backward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}
			current_block_address->GetBackwardChannel()->SetPipeToPrevious(pfd[0]);
			pipe_fd_from_previous_block[0] =pfd[1];

			//create pipe to next (forward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}
			current_block_address->GetForwardChannel()->SetPipeToNext(pfd[0]);
			pipe_fd_from_previous_block[1] =pfd[1];

			previous_block_address = current_block_address;
			current_block_address = previous_block_address->GetForwardAddress();
		}
	}

	// if succesful until now, call each block init to init them and create their threads.
	// note: threads are not started yet
	if((result == true) && (has_first == true) && (has_last ==true))
	{
		for(list<Block*>::iterator iter = this->block_list.begin();
		    (result == true) && (iter !=this->block_list.end()); iter++)
		{
			result &= (*iter)->Init();
		}

	}
	return result;
}


BlockMgr* BlockMgr::GetInstance(void)
{
	if(BlockMgr::singleton == NULL)
	{
		BlockMgr::singleton =  new BlockMgr();
	}
	return BlockMgr::singleton;
}


void BlockMgr::Kill (void)
{
	if(BlockMgr::singleton != NULL)
	{

		delete BlockMgr::singleton;
		BlockMgr::singleton = NULL;
	}
}

void BlockMgr::ReportError(pthread_t thread_id, bool critical, string error)
{

	std::cout<<error<<std::endl;
	if(critical == true)
	{
		BlockMgr::GetInstance()->Stop();
	}
}

void BlockMgr::Start(void)
{
	//start all threads
	for(list<Block *>::iterator iter= this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		(*iter)->Start();
	}
}

void BlockMgr::Pause(void)
{
	//puts all threads to sleep
	for(list<Block *>::iterator iter= this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		(*iter)->Sleep();
	}

}

void BlockMgr::Resume(void)
{
	//wake all threads
	for(list<Block *>::iterator iter= this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		(*iter)->Wake();
	}

}

BlockMgr::~BlockMgr(void)
{

	for(list<Block*>::iterator iter = this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		delete (*iter);
	}

}

