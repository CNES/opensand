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

/**
 * @file RtChannel.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#include "RtChannel.h"

#include "Rt.h"
#include "Block.h"
#include "RtFifo.h"
#include "RtEvent.h"
#include "MessageEvent.h"
#include "TimerEvent.h"
#include "NetSocketEvent.h"
#include "SignalEvent.h"

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <iostream>
#include <stdarg.h>

#define SIG_STRUCT_SIZE 128

using std::ostringstream;


// TODO pointer on onEventUp/Down and remove chan and add name
RtChannel::RtChannel(Block &bl, chan_type_t chan):
	block(bl),
	chan(chan),
	fifo(NULL),
	max_input_fd(-1),
	stop_fd(-1)
{
	FD_ZERO(&(this->input_fd_set));
}

RtChannel::~RtChannel()
{
	// delete all events
	this->updateEvents(); // update to also clear new events
	for(map<event_id_t, RtEvent *>::iterator iter = this->events.begin();
	    iter != this->events.end(); ++iter)
	{
		if((*iter).second == NULL)
		{
			continue;
		}
		delete((*iter).second);
	}
	this->events.clear();
	if(this->fifo)
	{
		delete this->fifo;
	}
}

bool RtChannel::enqueueMessage(void *data, size_t size, uint8_t type)
{
	if(!this->next_fifo->push(data, size, type))
	{
		this->reportError(false,
		                  "cannot push data in fifo for next block");
		return false;
	}
	return true;
}


bool RtChannel::init(void)
{
	sigset_t signal_mask;

	std::cout << "Channel " << this->chan << ": init" << std::endl;
	if(!this->onInit())
	{
		this->reportError(true,
		                  "custom channel initialization failed");
		return false;
	}

	// create the signal mask for stop (highest priority)
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGINT);
	sigaddset(&signal_mask, SIGQUIT);
	sigaddset(&signal_mask, SIGTERM);
	this->stop_fd = this->addSignalEvent("stop", signal_mask, 0);

	// initialize fifo and create associated message
	if(this->fifo)
	{
		if(!this->fifo->init())
		{
			this->reportError(true,
			                  "cannot initialize fifo");
			return false;
		}
		this->addMessageEvent();
	}

	return true;
}

int32_t RtChannel::addTimerEvent(const string &name,
                               uint32_t duration_ms,
                               bool auto_rearm,
                               bool start,
                               uint8_t priority)
{
	TimerEvent *event = new TimerEvent(name, duration_ms,
	                                   auto_rearm, start,
	                                   priority);
	if(!event)
	{
		this->reportError(true, "cannot create timer event");
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannel::addNetSocketEvent(const string &name,
                                   int32_t fd,
                                   uint8_t priority)
{
	NetSocketEvent *event = new NetSocketEvent(name,
	                                           fd, priority);
	if(!event)
	{
		this->reportError(true, "cannot create net socket event");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannel::addSignalEvent(const string &name,
                                sigset_t signal_mask,
                                uint8_t priority)
{
	SignalEvent *event = new SignalEvent(name, signal_mask, priority);
	if(!event)
	{
		this->reportError(true, "cannot create signal event");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

void RtChannel::addMessageEvent(uint8_t priority)
{
	string name = "downward";
	if(this->chan == upward_chan)
	{
		name = "upward";
	}

	MessageEvent *event = new MessageEvent(this->fifo, name,
	                                       this->fifo->getSigFd(),
	                                       priority);
	if(!event)
	{
		this->reportError(true, "cannot create message event");
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return;
	}
}

bool RtChannel::addEvent(RtEvent *event)
{
	if(this->events[event->getFd()])
	{
		this->reportError(true, "duplicated fd");
		return false;
	}
	this->new_events.push_back(event);

	this->addInputFd(event->getFd());
	
	return true;
}

void RtChannel::updateEvents(void)
{
	
	// add new events
	for(list<RtEvent *>::iterator iter = this->new_events.begin();
		iter != this->new_events.end(); ++iter)
	{
		this->events[(*iter)->getFd()] = *iter;
	}
	this->new_events.clear();
	
	// remove old events
	for(list<event_id_t>::iterator iter = this->removed_events.begin();
		iter != this->removed_events.end(); ++iter)
	{
		map<event_id_t, RtEvent *>::iterator it;
		
		it = this->events.find(*iter);
		if(it != this->events.end())
		{
			delete (*it).second;
			this->events.erase(it);
		}
	}
	this->removed_events.clear();
}

bool RtChannel::startTimer(event_id_t id)
{
	map<event_id_t, RtEvent *>::iterator it;
	RtEvent *event = NULL;
	
	it = this->events.find(id);
	if(it == this->events.end())
	{
		bool found = false;
		// check in new events
		for(list<RtEvent *>::iterator iter = this->new_events.begin();
			iter != this->new_events.end(); ++iter)
		{
			if(*(*iter) == id)
			{
				found = true;
				event = *iter;
				break;
			}
		}
		if(!found)
		{
			this->reportError(false, "cannot find timer");
			return false;
		}
	}
	else
	{
		event = (*it).second;
	}
	
	if(event->getType() != evt_timer)
	{
		this->reportError(false, "cannot start event that is not a timer");
		return false;
	}
	((TimerEvent *)event)->start();
	
	return true;
}

void RtChannel::addInputFd(int32_t fd)
{
	if(fd > this->max_input_fd)
	{
		this->max_input_fd = fd;
	}
	FD_SET(fd, &(this->input_fd_set));
}

void RtChannel::removeEvent(event_id_t id)
{
	this->removed_events.push_back(id);
}

void *RtChannel::startThread(void *pthis)
{
	((RtChannel *)pthis)->executeThread();

	return NULL;
}

bool RtChannel::processEvent(const RtEvent *const event)
{
	std::cout << "Channel " << this->chan << ": event received: "
	          << event->getName() << std::endl;
	return this->block.processEvent(event, this->chan);
};


void RtChannel::executeThread(void)
{
	while(true)
	{
		int32_t nfds = 0;
		fd_set current_input_fd_set;
		int32_t number_fd = 0;
		int32_t handled = 0;

		list<RtEvent *> priority_sorted_events;
		
		// get the new events for the next loop
		this->updateEvents();
		current_input_fd_set = this->input_fd_set;
		nfds = this->max_input_fd ;

		// wait for any event
		number_fd = select(nfds + 1, &current_input_fd_set, NULL, NULL, NULL);
		if(number_fd < 0)
		{
			this->reportError(true, "select failed: [%u: %s]", errno, strerror(errno));
		}
		// unfortunately, FD_ISSET is the only usable thing
		priority_sorted_events.clear();

		// for each event, stop
		for(map<event_id_t, RtEvent *>::iterator iter = this->events.begin();
			iter != this->events.end(); ++iter)
		{
			RtEvent *event = (*iter).second;
			if(handled >= number_fd)
			{
				// all events treated, no need to continue the loop
				break;
			}
			// if this event FD has raised
			if(!FD_ISSET(event->getFd(), &current_input_fd_set))
			{
				continue;
			}
			handled++;

			// fd is set
			if(!event->handle())
			{
				if(event->getType() == evt_signal)
				{
					// this is the only case where it is critical as
					// stop event is a signal
					this->reportError(true, "unable to handle signal event");
					pthread_exit(NULL);
				}
				this->reportError(false, "unable to handle event");
				// ignore this event
				continue;
			}
			priority_sorted_events.push_back(event);
			if(*event == this->stop_fd)
			{
				// we have to stop
				std::cout << "Channel " << this->chan
				          << ": stop signal received: "
				          << ((SignalEvent *)event)->getTriggerInfo().ssi_signo
				          << std::endl;
				pthread_exit(NULL);
			}
		}
		// sort the list according to priority
		// TODO check that
		priority_sorted_events.sort();

		// call processEvent on each event
		for(list<RtEvent *>::iterator iter = priority_sorted_events.begin();
			iter != priority_sorted_events.end(); ++iter)
		{
			(*iter)->setCreationTime();
			this->processEvent(*iter);
		}
	}
}

void RtChannel::reportError(bool critical, const char *msg_format, ...)
{
	char msg[512];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(msg, 512, msg_format, args);

	va_end(args);

	Rt::reportError(this->block.getName(), pthread_self(),
	                critical, msg);
};

void RtChannel::setFifo(RtFifo *fifo)
{
	this->fifo = fifo;
};

void RtChannel::setFifoSize(uint8_t size)
{
	this->fifo->resize(size);
};

void RtChannel::setNextFifo(RtFifo *fifo)
{
	this->next_fifo = fifo;
};

