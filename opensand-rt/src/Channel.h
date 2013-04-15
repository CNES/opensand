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


/**
  * @class Channel
  * @brief describes a single channel
  *
  * channel direction is always relative to its position.
  * Its next channel is the channel it will send data.
  * Its previous channel is the channel it will receive data from.
  *
  */

class Channel {

public:

	/*
	 * Constructor
	 *
	 * @param max_message is the size of the message fifo (default 3)
     *
	 */

	Channel(uint8_t max_message= 3);

	virtual ~Channel();


	/*
	 * EnqueueMessage adds a message to its queue.
	 * This method is supposed to be called from its previous channel
	 *
	 * @param message message to enqueue
	 * @param pipe_to_wait FD of the pipe it should wait from if fifo is full
     *
	 */
    void EnqueueMessage(MsgEvent *message, int32_t pipe_to_wait);



	/*
	 * SendEnqueuedSignal writes to the next channel pipe to signal it has enqueued
	 * This method is called in its own channel, after calling the EnqueueMessage from the next channel
	 *
	 */
    void SendEnqueuedSignal(void);

	/*
	 * GetNextChannel getter for next channel, ie the channel data will be sent to
	 *
     * @return address of the next channel
	 */
    Channel* GetNextChannel(void) {return this->next_channel;};

	/*
	 * GetPreviousChannel getter for previous channel, ie the channel data will come from
	 *
     * @return address of the previous channel
	 */
    Channel* GetPreviousChannel(void) {return this->previous_channel;};

    /*
	 * SetNextChannel setter for next channel, ie the channel data will be sent to
	 *
     * @param chan pointer to next channel
	 */
    void SetNextChannel(Channel *chan) { this->next_channel = chan;};

    /*
	 * SetPreviousChannel setter for previous channel, ie the channel data will come from
	 *
     * @param chan pointer to previous channel
	 */
    void SetPreviousChannel(Channel *chan) {this->previous_channel = chan;};


   /*
	 * Init initialize the channel
	 *
	 */

#ifdef DEBUG_BLOCK_MUTEX
    bool Init(pthread_mutex_t *block_mutex);
#else
	bool Init(void);
#endif


     /*
	 * Pause  pauses the processing
	 *
	 */
    void Pause(void);

    /*
	 * Start  Start / resume processing
	 *
	 */
    void Start(void);


    /*
	 * Stop stops the thread, leading to ending the application
	 *
	 */
    void Stop(void) { this->alive = false;};


     /*
	 * SetPipeToNext setter for pipe_to_next
	 *
     * @param fd file descriptor to write when a message has been enqueued
	 */
	void SetPipeToNext(int32_t fd);

     /*
	 * SetPipeFromNext setter for pipe_from_next
	 *
     * @param fd file descriptor to read when next channel message fifo is not full anymore
	 */
	void SetPipeFromNext(int32_t fd);

     /*
	 * SetPipeToPrevious setter for pipe_to_previous
	 *
     * @param fd file descriptor to write when own message fifo is not full anymore
	 */
	void SetPipeToPrevious(int32_t fd);

     /*
	 * SetPipeFromPrevious setter for pipe_from_previous
	 *
     * @param fd file descriptor to read when previous channel has enqueued a message
	 */
	void SetPipeFromPrevious(int32_t fd);

      /*
	 * GetPipeToNext getter for pipe_to_next
	 *
     * @return file descriptor to write when a message has been enqueued
	 */
    int32_t GetPipeToNext(void){return this->pipe_to_next;};

      /*
	 * GetPipeFromNext getter for pipe_from_next
	 *
     * @return file descriptor to read when next channel message fifo is not full anymore
	 */
    int32_t GetPipeFromNext(void){return this->pipe_from_next;};


     /*
	 * GetPipeToPrevious setter for pipe_to_previous
	 *
     * @return file descriptor to write when own message fifo is not full anymore
	 */
    int32_t GetPipeToPrevious(void){return this->pipe_to_previous;};


     /*
	 * GetPipeFromPrevious setter for pipe_from_previous
	 *
     * @return fd file descriptor to read when previous channel has enqueued a message
	 */
    int32_t GetPipeFromPrevious(void){return this->pipe_from_previous;};


     /*
	 * IsPaused
	 *
     * @return true if thread is paused, false if running
	 */
    bool IsPaused(void) { return paused;};

    /*
	 * IsAlive
	 *
     * @return true if thread is alive, false if not or asked to terminate
	 */
    bool IsAlive(void) {return this->alive;};


     /*
	 * CustomInit virtual method developers have to implement
	 *
	 * it contains custom definitions and is the method where events have to be added
	 */

	virtual bool CustomInit(void) = 0;

     /*
	 * OnEvent virtual method developers have to implement
	 *
	 * it contains Event processing and is called on every event happening
	 *
	 * @param event pointer to event that just happened
	 */
    virtual bool OnEvent(Event * event) = 0;



     /*
	 * StartThread method called to start the thread
	 *
	 * it contains a last init, then enters a loop that detects events and call OnEvent on them
	 *
	 * @param pointer to the class
	 *
	 */
	static void * StartThread(void *);


  protected:


    /*
	 * AddTimerEvent adds a timer event to the channel
	 *
	 *
	 * @param duration_ms duration of timer
	 *
     * @param priority by default 2. the smaller the higher
	 *
	 * @param auto_rearm if ture, will get rearmed when detected (not when expired)
	 *
	 */
    void AddTimerEvent(uint32_t duration_ms, uint8_t priority=2, bool auto_rearm = true);

    /*
	 * AddNetSocketEvent adds a net socket event to the channel
	 *
	 *
	 * @param fd file descriptor to monitor
	 *
     * @param priority by default 3. the smaller the higher
	 *
	 *
	 */

	void AddNetSocketEvent(int32_t fd = -1, uint8_t priority=3);

    /*
	 * AddSignalEvent adds a signal event to the channel
	 *
	 *
	 * @param signal_mask mask containing all the signals that trigger this event
	 *
     * @param priority by default 1. the smaller the higher
	 *
	 *
	 */
	void AddSignalEvent(sigset_t signal_mask, uint8_t priority=1);


    /// message fifo
	list<MsgEvent *> message_list;

    /// max size of the message fifo
	uint8_t max_message_size;

    /// thread boolean telling if it is alive or not. If set to false, it ends the thread
    bool alive;

    /// events that are currently monitored by the channel thread
	list<Event *> waiting_for_events;

    /// thread boolean telling if it should run or not.
    bool paused;


    /// pipe FD to write for the next channel
	int32_t pipe_to_next;
    /// pipe FD to read for the next channel
	int32_t pipe_from_next;
    /// pipe FD to write for the previous channel
	int32_t pipe_to_previous;
    /// pipe FD to read from the previous channel
	int32_t pipe_from_previous;

    /// contains the highest FD of waiting_for_events
	int32_t max_input_fd;

    /// contains the highest FD of output events
	int32_t max_output_fd;

    /// fd_set containing monitored FDs
	fd_set input_fd_set;

    /// fd_set containing output FDs
	fd_set output_fd_set;

    /// duration of the last thread loop (not implemented)
	uint32_t duration_ms;

    /// pointer to previous channel (channel that this one receives data)
    Channel *previous_channel;

    /// pointer to next channel (channel that this one sends data to)
    Channel *next_channel;


private:

    /*
	 * ExecuteThread core thread method
	 *
	 */
	void ExecuteThread(void);


    /*
	 * AddInputFd adds an fd to input_fd_set, used internally when events added
	 *
	 * @param fd file descriptor to monitor
	 */
	void AddInputFd(int32_t fd);


    /*
	 * AddOnputFd adds an fd to output_fd_se
	 *
	 * @param fd file descriptor to add
	 */
	void AddOutputFd(int32_t fd);

#ifdef DEBUG_BLOCK_MUTEX
    pthread_mutex_t *block_mutex;
#endif

    /// the thread
	pthread_t thread_id;
    /// channel mutex used for message fifo
	pthread_mutex_t mutex;

};

#endif

