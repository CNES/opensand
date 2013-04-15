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
#include <stdio.h>


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
    //set the block previous and next block
	block->SetForwardAddress(forward_block);
	block->SetbackwardAddress(backward_block);

    if (backward_block != NULL)
    {
        //set the link from the forward channel to its previous channel (from the previous block)
        block->GetForwardChannel()->SetPreviousChannel( backward_block->GetForwardChannel());
        //set the link from the backward channel to its next channel (from the previous block, as backward channel goes reverse)
        block->GetBackwardChannel()->SetNextChannel(backward_block->GetBackwardChannel());
    }
    else
    {
        //this happens on the first block
        block->GetForwardChannel()->SetPreviousChannel(NULL);
        block->GetBackwardChannel()->SetNextChannel(NULL);
    }

    if (forward_block != NULL)
    {
        //set the link from the forward channel to its next channel (from the next block)
        block->GetForwardChannel()->SetNextChannel(forward_block->GetForwardChannel());
        //set the link from the backward channel to its previous channel (from the next block, as backward channel goes reverse)
        block->GetBackwardChannel()->SetPreviousChannel(forward_block->GetBackwardChannel());

    }
    else
    {
        //this happens on the last block
        block->GetForwardChannel()->SetNextChannel(NULL);
        block->GetBackwardChannel()->SetPreviousChannel(NULL);
    }

}



void BlockMgr::Stop(bool hard)
{
    this->alive = false;
    if (hard == true) // send a stop signal to each block
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
        //yes :the first block has no previous
            //is it the last bloc too ?
			if(current_block_address->GetForwardAddress() != NULL)
            { //no; it has a next
				//create pipe from first to second block (backward)
				if(pipe(pfd) == -1)
				{
					result = false;
				}
                current_block_address->GetBackwardChannel()->SetPipeToPrevious(pfd[1]);
				pipe_fd_from_previous_block[0] = pfd[0];

				//create pipe from first to second block (forward)
				if(pipe(pfd) == -1)
				{
					result = false;
				}
                current_block_address->GetForwardChannel()->SetPipeToNext(pfd[1]);
                pipe_fd_from_previous_block[1] = pfd[0];

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
            current_block_address->GetBackwardChannel()->SetPipeToNext(pfd[1]);
            previous_block_address->GetBackwardChannel()->SetPipeFromPrevious(pfd[0]);


			//create pipe to previous block (forward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}
            current_block_address->GetForwardChannel()->SetPipeToPrevious(pfd[1]);
            previous_block_address->GetForwardChannel()->SetPipeFromNext(pfd[0]);
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
            current_block_address->GetBackwardChannel()->SetPipeToNext(pfd[1]);
            previous_block_address->GetBackwardChannel()->SetPipeFromPrevious(pfd[0]);



			//create pipe to previous (forward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}
            current_block_address->GetForwardChannel()->SetPipeToPrevious(pfd[1]);
            previous_block_address->GetForwardChannel()->SetPipeFromNext(pfd[0]);

			//create pipe to next (backward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}
            current_block_address->GetBackwardChannel()->SetPipeToPrevious(pfd[1]);
            pipe_fd_from_previous_block[0] =pfd[0];

			//create pipe to next (forward)
			if(pipe(pfd) == -1)
			{
				result = false;
			}
            current_block_address->GetForwardChannel()->SetPipeToNext(pfd[1]);
            pipe_fd_from_previous_block[1] =pfd[0];

			previous_block_address = current_block_address;
			current_block_address = previous_block_address->GetForwardAddress();
		}
	}

	// if succesful until now, call each block init to init them and create their threads.
	// note: threads are not started yet
	if((result == true) && (has_first == true) && (has_last ==true))
	{
//uncomment this for pipe association printing
//        int i=0;
//end uncomment
		for(list<Block*>::iterator iter = this->block_list.begin(); (result == true) && (iter !=this->block_list.end()); iter++)
		{
		
//uncomment this for pipe association printing
//            i++;
//          printf("bloc %i \nforward to previous %i - forward to next %i\n",i, (*iter)->GetForwardChannel()->GetPipeToPrevious(),(*iter)->GetForwardChannel()->GetPipeToNext());
//            printf("forward from previous %i - forward from next %i\n", (*iter)->GetForwardChannel()->GetPipeFromPrevious(),(*iter)->GetForwardChannel()->GetPipeFromNext());
//            printf("backward to previous %i - backward to next %i\n", (*iter)->GetBackwardChannel()->GetPipeToPrevious(),(*iter)->GetBackwardChannel()->GetPipeToNext());
//            printf("backward from previous %i - backward from next %i\n", (*iter)->GetBackwardChannel()->GetPipeFromPrevious(),(*iter)->GetBackwardChannel()->GetPipeFromNext());
//end uncomment

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
        (*iter)->Pause();
	}

}

void BlockMgr::Resume(void)
{
	//wake all threads
	for(list<Block *>::iterator iter= this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
        (*iter)->Start();
	}

}

BlockMgr::~BlockMgr(void)
{
	for(list<Block*>::iterator iter = this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		delete (*iter);
	}
    delete BlockMgr::singleton;
}


BlockMgr::BlockMgr() : alive(true)
{
    //block all signals
    sigset_t blocked_signals;
    sigfillset (&blocked_signals);
    pthread_sigmask(SIG_SETMASK,&blocked_signals,NULL);
}


void BlockMgr::RunLoop(void)
{
    while (this->alive == true)
    {
        for(list<Block*>::iterator iter = this->block_list.begin(); iter != this->block_list.end(); iter++)
        {
            if (((*iter)->backward == NULL) ||
                ((*iter)->forward == NULL) ||
                ((*iter)->backward->IsAlive() == false ) ||
                ((*iter)->forward->IsAlive() == false ))
            {
                    this->alive = false;
            }

        }
    }
}


