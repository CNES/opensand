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
/* $Id: Channel.cpp,v 1.1.1.1 2013/04/08 8:02:31 cgaillardet Exp $ */

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h> //snprintf
#include <signal.h> //pthread_sigmask
#include "Channel.h"

#define MAGICSTARTREADWORD "GO"
#define MAGICUNLOCKWORD "NOTFULL"
#define READBLOCKSIZE (2000)




Channel::Channel(uint8_t max_message) :
    pipe_to_next(-1),
    pipe_from_next(-1),
    pipe_to_previous(-1),
    pipe_from_previous(-1),
    max_input_fd (-1),
    max_output_fd(-1),
    previous_channel(NULL),
    next_channel(NULL),
    alive(false),
    paused(true)
{
    if (max_message < 3 )
    {
        max_message = 3;
    }
    this->max_message_size = max_message;

    pthread_mutex_init(&(this->mutex),NULL);

}

void Channel::AddTimerEvent(uint32_t duration_ms, uint8_t priority, bool auto_rearm)
{
    TimerEvent *event = new TimerEvent(duration_ms,priority, auto_rearm, false);
    this->waiting_for_events.push_back((Event*) event );
    AddInputFd(event->GetFd());
}

void Channel::AddNetSocketEvent(int32_t fd, uint8_t priority)
{
    NetSocketEvent *event = new NetSocketEvent(fd,priority);
    this->waiting_for_events.push_back((Event*) event );
    AddInputFd(event->GetFd());
}

void Channel::AddSignalEvent(sigset_t signal_mask, uint8_t priority)
{
    SignalEvent *event = new SignalEvent(signal_mask,priority);
    this->waiting_for_events.push_back((Event*) event);

    AddInputFd(event->GetFd());
}

#ifdef DEBUG_BLOCK_MUTEX
bool Channel::Init(pthread_mutex_t *block_mutex)
{
this->block_mutex =block_mutex;
#else
bool Channel::Init(void)
{
#endif


    sigset_t blocked_signals;
    //pipes are created when this method is called
    bool res = false;
    MsgEvent *message_ready;
    pthread_attr_t attr; // thread attribute

    this->alive = true;
    // set thread detachstate attribute to DETACHED
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    //create thread
    if (0 == pthread_create(&(this->thread_id), &attr, &Channel::StartThread,(void *)this))
    {
        //block all signals by default
        sigfillset (&blocked_signals);
        pthread_sigmask(SIG_SETMASK,&blocked_signals,NULL);


        //create a msg event with pipe_from_previous as FD
        //add it to this->waiting_for_events
        res = true;
        if (this->pipe_from_previous != -1)
        {
            message_ready = new MsgEvent(this->pipe_from_previous);
            if (message_ready != NULL)
            {
                this->waiting_for_events.push_back(message_ready);
                AddInputFd(this->pipe_from_previous);

            }
            else
            {
                res= false;

            }
        }

    }

    return res;
}


 void Channel::AddInputFd(int32_t fd)
 {
     if (fd > this->max_input_fd)
     {
         this->max_input_fd = fd;
     }
     FD_SET(fd, &(this->input_fd_set));
 }





 void Channel::AddOutputFd(int32_t fd)
 {
     if (fd > this->max_output_fd)
     {
         this->max_output_fd = fd;
     }
     FD_SET(fd, &(this->output_fd_set));
 }




void Channel::SetPipeToNext(int32_t fd)
{
    this->pipe_to_next = fd;
    this->AddOutputFd(fd);
}

void Channel::SetPipeFromNext(int32_t fd)
{
    this->pipe_from_next = fd;
    this->AddInputFd(fd);
}

void Channel::SetPipeToPrevious(int32_t fd)
{
    this->pipe_to_previous = fd;
    this->AddOutputFd(fd);
}

void Channel::SetPipeFromPrevious(int32_t fd)
{
    this->pipe_from_previous = fd;
    this->AddInputFd(fd);
}

void Channel::Pause (void)
{
    this->paused= true;
}

void Channel::Start(void)
{
    this->paused = false;
}

void * Channel::StartThread(void *pthis)
{
    ((Channel *)pthis)->ExecuteThread();

}


void Channel::EnqueueMessage(MsgEvent *new_message, int32_t pipe_to_wait)
{
    char error_buffer[250];
	fd_set blocking;
	int32_t resultat;
    int32_t read_data_size;
    string error;
	uint8_t magic_word_size_from_next = strlen(MAGICUNLOCKWORD);
    char pipe_from_next_buffer[magic_word_size_from_next+1];

	if (this->message_list.size() == this->max_message_size)
	{ //cant enqueue, so select() on the pipe_from_next fd ...
	    //unless this is the last channel.
	    //if it is the last channel, wait instead of blocking
	    if (this->pipe_from_next == -1)
	    {
	        while(this->message_list.size() == this->max_message_size);
	    }
	    else
	    {
            FD_ZERO(&blocking);
            FD_SET(pipe_to_wait, &blocking);
            resultat = select(pipe_to_wait + 1, &blocking, NULL,NULL,NULL);

            //verify and clear the pipe
            read_data_size =read(pipe_to_wait,pipe_from_next_buffer,magic_word_size_from_next);
            pipe_from_next_buffer[read_data_size] = 0;
            if ((read_data_size != magic_word_size_from_next)|| ( strcmp(pipe_from_next_buffer, MAGICUNLOCKWORD) != 0 ))
            {

                snprintf(error_buffer,249,"pipe from next buffer does not contain the magic word. Content: =%s=, size=%u, expected size=%u\n",pipe_from_next_buffer,strlen(pipe_from_next_buffer),magic_word_size_from_next);
                error=error_buffer;
                ::BlockMgr::ReportError(this->thread_id, true, error);

            }
		}
	}
	// now we can enqueue
	//Critical section - Waiting for a new event
	if (pthread_mutex_lock(&(this->mutex))== -1) // should never happen - if it does, all we can do is coredump
	{
        error = "Mutex lock failure";
        ::BlockMgr::ReportError(this->thread_id, true, error);

	}
	this->message_list.push_back(new_message);

	pthread_mutex_unlock(&(this->mutex));
}




// priority comparisaon
bool compare_priority (Event *first,Event *second)
{
	return (first->GetPriority() < second->GetPriority());
}



void Channel::SendEnqueuedSignal(void)
{
    string error;
    int32_t res = 0;
    fd_set write_fd_set;


    if (this->pipe_to_next != -1)
    {
        FD_ZERO(&write_fd_set);
        FD_SET(this->pipe_to_next,&write_fd_set);
        select(this->pipe_to_next+1,NULL,&write_fd_set,NULL,NULL);
        res = write(this->pipe_to_next,MAGICSTARTREADWORD,strlen(MAGICSTARTREADWORD));
        if (res != strlen(MAGICSTARTREADWORD))
        {
          error = "Magic word write on pipe to next failure";
        ::BlockMgr::ReportError(this->thread_id, true, error);
        }
    }



}


/**
 * Execute the thread - it's the thread body
 *
*/
void Channel::ExecuteThread(void)
{
    int32_t resSelect = 0 ;
	int32_t nfds = 0;
    int32_t resRecv = 0;
    fd_set current_input_fd_set;
    fd_set write_fd_set;
    int32_t read_data_size = 0;
    int32_t write_data_size = 0;
    uint64_t timerRead;
    string error;
    uint8_t magic_word_size_to_previous = strlen(MAGICUNLOCKWORD);
    char pipe_to_previous_buffer[magic_word_size_to_previous+1];
    uint8_t magic_word_size_from_previous = strlen(MAGICSTARTREADWORD);
	char pipe_from_previous_buffer[magic_word_size_from_previous+1];

    unsigned char signal_buffer[129]; //signal structure is always 128 bytes long

	unsigned char external_fd_buffer[READBLOCKSIZE + 1];
	list<Event *> priority_sorted_events;

    //first, wait until the thread is started
    while (this->paused == true);

   // start timers
    for (list<Event*>::iterator iter= this->waiting_for_events.begin(); iter!=this->waiting_for_events.end(); iter++)
    {
		if (((*iter) !=NULL) && ((*iter)->GetType() == Timer))
		{
			((TimerEvent *)(*iter))->Start();
		}
    }

	while (this->alive ==true)
	{

#ifdef DEBUG_BLOCK_MUTEX
        if (pthread_mutex_lock((this->block_mutex))== -1) // should never happen - if it does, all we can do is coredump
        {
            error = "Block mutex lock failure";
            ::BlockMgr::ReportError(this->thread_id, true, error);
        }
#endif

	    if (this->paused == false)
	    {
            current_input_fd_set = this->input_fd_set;
            nfds = this->max_input_fd ;

			// wait for any event
			resSelect= select(nfds +1,&current_input_fd_set,NULL,NULL,NULL);

			//unfortunately, FD_ISSET is the only usable thing
			priority_sorted_events.clear();

			//for each event
			for(list<Event* >::iterator iter = this->waiting_for_events.begin(); iter !=this->waiting_for_events.end(); iter++)
			{
				//is this event FD has raised
				if (FD_ISSET((*iter)->GetFd(), &current_input_fd_set) == true)
				{
					// check what kind of event it is
							//is it a signal ?
					if ((*iter)->GetType() == Signal) // signal
					{
						read_data_size = read((*iter)->GetFd(),signal_buffer,128); //signal structure is always 128 bytes
						if (read_data_size != 128)
						{
						 error = "Signal read is not 128 bytes";
                        ::BlockMgr::ReportError(this->thread_id, true, error);


						}
						((SignalEvent *)*iter)->SetData(signal_buffer,read_data_size);
						priority_sorted_events.push_back(*iter);
					}
					//is it a timer ?
					else if ((*iter)->GetType() == Timer)
					{
						//auto rearm ? if so rearm
						if (((TimerEvent *)(*iter))->IsAutoRearm() == true)
						{
							((TimerEvent *)(*iter))->Start();
						}
						else
						{//no auto rearm: disable
							((TimerEvent *)(*iter))->Disable();
						}
						priority_sorted_events.push_back(*iter);
					}
                    // is it a message ?
					else if ((*iter)->GetType() == Message) //pipe from previous, message ready
                    {
                        //Critical section - dequeue message
                        if (pthread_mutex_lock(&(this->mutex))== -1) // should never happen - if it does, all we can do is coredump
                        {
                           error = "Mutex lock failure";
                            ::BlockMgr::ReportError(this->thread_id, true, error);

                        }

                        //dequeue message and add it to the event received list
                        ((MsgEvent*)(*iter))->SetData(this->message_list.front()->GetData(), this->message_list.front()->GetSize());

                        //delete the message. Users have to "new" messages in OnEvent()
                        delete this->message_list.front();
                        this->message_list.pop_front();


                        if (this->message_list.size() + 1 == this->max_message_size )
                        {
                            FD_ZERO(&write_fd_set);
                            FD_SET(this->pipe_to_previous,&write_fd_set);
                            select(this->pipe_to_previous+1,NULL,&write_fd_set,NULL,NULL);
                            // write magic word on pipe to unlock previous thread
                            write_data_size = write(this->pipe_to_previous, MAGICUNLOCKWORD, strlen(MAGICUNLOCKWORD) );

                            if (write_data_size != strlen(MAGICUNLOCKWORD))
                            {
                                error = "Error writing magic unlock word to previous thread";
                                ::BlockMgr::ReportError(this->thread_id, true, error);
                            }

                        }
                        priority_sorted_events.push_back(*iter);

                        //read the pipe to clear it, should contain the magic word
                        read_data_size =read(this->pipe_from_previous,pipe_from_previous_buffer,strlen(MAGICSTARTREADWORD));
                        pipe_from_previous_buffer[read_data_size]=0;
                        if ((read_data_size != strlen(MAGICSTARTREADWORD))
                            || ( strcmp(pipe_from_previous_buffer, MAGICSTARTREADWORD) != 0 ))
                        {
                            pipe_from_previous_buffer[read_data_size] = 0;
                            error = "pipe from previous buffer does not contain the magic word. Content: ";
                            error.append(pipe_from_previous_buffer);
                            ::BlockMgr::ReportError(this->thread_id, true, error);
                        }
                        //end of critical section - dequeued message
                        pthread_mutex_unlock(&(this->mutex));



                    }
					// is it a standard FD ?
					else if ((*iter)->GetType() == NetSocket) // socket in
					{
                        read_data_size = read((*iter)->GetFd(),external_fd_buffer,READBLOCKSIZE);
                        external_fd_buffer[read_data_size] = 0;
						((NetSocketEvent *)* iter)->SetData(external_fd_buffer,read_data_size);
						priority_sorted_events.push_back(*iter);
					}
					else // spurious, report event, clear it
					{
                        read_data_size = read((*iter)->GetFd(),external_fd_buffer,READBLOCKSIZE);
                        //terminate string
                        external_fd_buffer[read_data_size]=0;

                        error = "unexpected FD raise, FD content: ";
                        error.append((char *)external_fd_buffer);
                        ::BlockMgr::ReportError(this->thread_id, false, error);
					}
				}

			}
			// TODO :sort the list according to priority, it does not work
			priority_sorted_events.sort(compare_priority);


			//call OnEvent on each event

			for(list<Event *>::iterator iter = priority_sorted_events.begin(); iter != priority_sorted_events.end();iter++)
			{
                this->OnEvent(*iter);
			}

#ifdef DEBUG_BLOCK_MUTEX
			pthread_mutex_unlock(&(this->mutex));
#endif
		}
	}
}




Channel::~Channel()
{
    //close pipes
    if (this->pipe_to_next!= -1)
    {
     close(pipe_to_next);
    }

    if (this->pipe_from_next != -1)
    {
        close (pipe_from_next);
    }

    if (this->pipe_to_previous != -1)
    {
        close(pipe_to_previous);
    }
    if (this->pipe_from_previous != -1)
    {
        close(pipe_from_previous);
    }
    // delete current enqueued messages
    for (list<MsgEvent*>::iterator iter= this->message_list.begin(); iter!=this->message_list.end(); iter++)
    {
       if ((*iter) != NULL)
       {
        delete (*iter);
       }

    }

    // delete all events
    for (list<Event *>::iterator iter= this->waiting_for_events.begin(); iter!=this->waiting_for_events.end(); iter++)
    {
        if ((*iter) != NULL)
        {
            if ((*iter)->GetType() == Message)
            {
                delete ((MsgEvent*)(*iter));
            }
            else if ((*iter)->GetType() == Message)
            {
                delete ((TimerEvent*)(*iter));
            }
            else if ((*iter)->GetType() == Message)
            {
                delete ((NetSocketEvent*)(*iter));
            }
            else if ((*iter)->GetType() == Message)
            {
                delete ((SignalEvent*)(*iter));
            }
            else // undefined, should not happen
            {

            }
        }
    }
    this->alive = false;
	//Delete all resources
	pthread_mutex_destroy(&(this->mutex));
}

