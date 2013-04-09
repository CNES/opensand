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
/* $Id: Channel.h,v 1.1.1.1 2013/04/08 07:57:49 cgaillardet Exp $ */


#ifndef CHANNEL_H
#define CHANNEL_H

#include "Types.h"
#include "Block.h"
#include "MsgEvent.h"
#include "TimerEvent.h"
#include "NetSocketEvent.h"
#include "SignalEvent.h"


#include <stdlib.h>
#include  <string>
#include <list>
#include <vector>
#include <sys/select.h>


class Block;


using std::list;
using std::vector;

class Channel
{
  public:

	Channel(uint8_t max_message= 3);

	virtual ~Channel();

	void EnqueueMessage(MsgEvent *message,int32_t pipe_fd_from_next);

#ifdef DEBUG_BLOCK_MUTEX
	bool Init(mutex *block_mutex);
#else
	bool Init(void);
#endif

	bool Sleep(void);
	bool Wake(void);
	bool Start(void);

	uint32_t GetDuration(void);

	void SetPipeToNext(int32_t fd);
	void SetPipeFromNext(int32_t fd);
	void SetPipeToPrevious(int32_t fd);
	void SetPipeFromPrevious(int32_t fd);


	DirectionThreadState GetState(void) { return state;};

	virtual bool OnEvent(Event * event) = 0;
	virtual bool CustomInit(void) = 0;

	static void * StartThread(void *);


  protected:

	void AddTimerEvent(uint32_t duration_ms,
	                   uint8_t priority=2,
	                   bool auto_rearm = true);
	void AddNetSocketEvent(int32_t fd = -1, uint8_t priority=3);
	void AddSignalFdEvent(sigset_t signal_mask, uint8_t priority=1);

	list<MsgEvent *> message_list;
	uint8_t max_message_size;

	list<Event *> waiting_for_events;

	DirectionThreadState state;

	int32_t pipe_to_next;
	int32_t pipe_from_next;
	int32_t pipe_to_previous;
	int32_t pipe_from_previous;

	int32_t max_input_fd;
	int32_t max_output_fd;

	fd_set input_fd_set;
	fd_set output_fd_set;

	uint32_t duration_ms;

	Block *previous_block;
	Block *next_block;


private:

	void ExecuteThread(void);
	void AddInputFd(int32_t fd);
	void AddOutputFd(int32_t fd);

#ifdef DEBUG_BLOCK_MUTEX
	pthead_mutx_t *block_mutex;
#endif

	pthread_t thread_id; //Thread ID
	pthread_mutex_t mutex; //Mutex for critical section
	pthread_cond_t cond; //Condition for critical section


};

#endif

