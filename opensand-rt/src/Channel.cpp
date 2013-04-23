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
 * @file Channel.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#include "Channel.h"

#include "Block.h"
#include "Rt.h"

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sstream>
#include <iostream>

#define SIG_STRUCT_SIZE 128

using std::ostringstream;

// TODO pointer on onEventUp/Down and remove chan and add name
Channel::Channel(Block &bl, chan_type_t chan):
	block(bl),
	chan(chan),
	fifo(NULL),
	max_input_fd(-1),
	stop_fd(-1)
{
	FD_ZERO(&(this->input_fd_set));
}

Channel::~Channel()
{
	// delete all events
	for(list<Event *>::iterator iter = this->events.begin();
	    iter != this->events.end(); ++iter)
	{
		if(*iter == NULL)
		{
			continue;
		}
		delete(*iter);
	}
	if(this->fifo)
	{
		delete this->fifo;
	}
}

bool Channel::enqueueMessage(void *message)
{
	if(!this->next_fifo->push(message))
	{
		this->reportError(false,
		                  "cannot push data in fifo for next block");
		return false;
	}
	return true;
}


bool Channel::init(void)
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

	// start timers
	for(list<Event *>::iterator iter = this->events.begin();
	    iter != this->events.end(); ++iter)
	{
		if((*iter) != NULL && (*iter)->getType() == evt_timer)
		{
			((TimerEvent *)(*iter))->start();
		}
	}

	return true;
}

int32_t Channel::addTimerEvent(const string &name,
                               uint32_t duration_ms,
                               uint8_t priority,
                               bool auto_rearm)
{
	TimerEvent *event = new TimerEvent(name, duration_ms, 
	                                   priority, auto_rearm, false);
	if(!event)
	{
		this->reportError(true, "cannot create timer event");
	}
	this->events.push_back((Event *)event);
	this->addInputFd(event->getFd());
	return event->getFd();
}

int32_t Channel::addNetSocketEvent(int32_t fd, uint8_t priority)
{
	string name = this->block.getName();
	name += "/net_socket";
	NetSocketEvent *event = new NetSocketEvent(name,
	                                           fd, priority);
	if(!event)
	{
		this->reportError(true, "cannot create net socket event");
	}
	this->events.push_back((Event *)event);
	this->addInputFd(event->getFd());
	return event->getFd();
}

int32_t Channel::addSignalEvent(const string &name,
                                sigset_t signal_mask,
                                uint8_t priority)
{
	SignalEvent *event = new SignalEvent(name, signal_mask, priority);
	if(!event)
	{
		this->reportError(true, "cannot create signal event");
	}
	this->events.push_back((Event *)event);
	this->addInputFd(event->getFd());
	return event->getFd();
}

void Channel::addMessageEvent(uint8_t priority)
{
	string name = "downward";
	if(this->chan == upward_chan)
	{
		name = "upward";
	}

	MessageEvent *event = new MessageEvent(name, this->fifo->getSigFd(),
	                                       priority);
	if(!event)
	{
		this->reportError(true, "cannot create message event");
	}
	this->events.push_back((Event *)event);
	this->addInputFd(this->fifo->getSigFd());
}

void Channel::addInputFd(int32_t fd)
{
	if(fd > this->max_input_fd)
	{
		this->max_input_fd = fd;
	}
	FD_SET(fd, &(this->input_fd_set));
}

void *Channel::startThread(void *pthis)
{
	((Channel *)pthis)->executeThread();

	return NULL;
}

bool Channel::processEvent(const Event *const event)
{
	std::cout << "Channel " << this->chan << ": event received: "
	          << event->getName() << std::endl;
	return this->block.processEvent(event, this->chan);
};


void Channel::executeThread(void)
{
	while(true)
	{
		int32_t nfds = 0;
		fd_set current_input_fd_set;
		int32_t number_fd = 0;
		int32_t handled = 0;

		list<Event *> priority_sorted_events;

		current_input_fd_set = this->input_fd_set;
		nfds = this->max_input_fd ;

		// wait for any event
		number_fd = select(nfds + 1, &current_input_fd_set, NULL, NULL, NULL);
		if(number_fd < 0)
		{
			ostringstream error;

			this->reportError(true, "select failed:", errno);
		}
		// unfortunately, FD_ISSET is the only usable thing
		priority_sorted_events.clear();

		// for each event, stop 
		for(list<Event *>::iterator iter = this->events.begin();
			iter != this->events.end(); ++iter)
		{
			if(handled >= number_fd)
			{
				// all events treated, no need to continue the loop
				break;
			}
			// is this event FD has raised
			if(!FD_ISSET((*iter)->getFd(), &current_input_fd_set))
			{
				continue;
			}
			handled++;

			// fd is set
			switch((*iter)->getType())
			{
				case evt_signal:
					if(!this->handleSignalEvent((SignalEvent *)*(iter)))
					{
						// this is the only case where it is critical as
						// stop event is a signal
						std::cout << "Channel " << this->chan
						          << ": stop after signal event" << std::endl;
						pthread_exit(0);
					}
					priority_sorted_events.push_back(*iter);
					break;

				case evt_timer:
					if(!this->handleTimerEvent((TimerEvent *)(*iter)))
					{
						continue;
					}
					priority_sorted_events.push_back(*iter);
					break;

				case evt_message:
					if(!this->handleMessageEvent((MessageEvent *)(*iter)))
					{
						continue;
					}
					priority_sorted_events.push_back(*iter);
					break;

				case evt_net_socket:
					if(!this->handleNetSocketEvent((NetSocketEvent *)(*iter)))
					{
						continue;
					}
					priority_sorted_events.push_back(*iter);
					break;

				// TODO custom !
				default:
					this->handleUnknownEvent(*iter);
			}
		}
		// sort the list according to priority
		// TODO check that
		priority_sorted_events.sort();

		// call processEvent on each event
		for(list<Event *>::iterator iter = priority_sorted_events.begin();
			iter != priority_sorted_events.end(); ++iter)
		{
			(*iter)->setCreationTime();
			this->processEvent(*iter);
		}
	}
}


bool Channel::handleSignalEvent(SignalEvent *event)
{
	struct signalfd_siginfo info;
	int rlen;

	// signal structure size is constant
	rlen = read(event->getFd(), &info, sizeof(struct signalfd_siginfo));
	if(rlen != sizeof(struct signalfd_siginfo))
	{
		this->reportError(true, "cannot read signal", ((rlen < 0) ? errno : 0));
		return false;
	}
	if(this->stop_fd == event->getFd())
	{
		// we have to stop
		std::cout << "Channel " << this->chan
		          << ": stop signal received: " << info.ssi_signo <<  std::endl;
		return false;
	}
	event->setSignalInfo(info);
	return true;
}

bool Channel::handleTimerEvent(TimerEvent *event)
{
	// auto rearm ? if so rearm
	// TODO is it not automatic ?
	if(event->isAutoRearm())
	{
		event->start();
	}
	else
	{
		//no auto rearm: disable
		event->disable();
	}
	return true;
}


bool Channel::handleMessageEvent(MessageEvent *event)
{
	unsigned char data[strlen(MAGIC_WORD)];
	int ret;

	// read the pipe to clear it, and check that if contains
	// the correct signaling
	ret = read(event->getFd(), // <=> this->fifo->getFd()
	            data,
	            strlen(MAGIC_WORD));
	if(ret != strlen(MAGIC_WORD) ||
	   strncmp((char *)data, MAGIC_WORD, strlen(MAGIC_WORD)) != 0)
	{
		ostringstream error;
		error << "pipe signaling message from previous block contain wrong data: "
		      << data;
		this->reportError(false, error.str(), ((ret < 0) ? errno : 0));
		return false;
	}

	// set the event content
	event->setMessage(this->fifo->pop());
	return true;
}


bool Channel::handleNetSocketEvent(NetSocketEvent *event)
{
	unsigned char data[MAX_SOCK_SIZE];
	size_t size;

	size = read(event->getFd(), data, MAX_SOCK_SIZE);
	if(size < 0)
	{
		ostringstream error;
		this->reportError(false, "unable to read on socket", errno);
		return false;
	}
	event->setData(data, size);

	return true;
}

void Channel::handleUnknownEvent(Event *event)
{
	ostringstream error;
	char data[MAX_SOCK_SIZE + 1];
	size_t size;

	size = read(event->getFd(), data, MAX_SOCK_SIZE);
	if(size < 0)
	{
		this->reportError(false, "unable to read unknown event", errno);
		return;
	}
	data[size] = '\0';
	error << "unknown event received: name = " << event->getName()
	      << " data = " << data;
	this->reportError(false, error.str());
}

void Channel::reportError(bool critical, string error, int val)
{
	Rt::reportError(this->block.getName(), pthread_self(),
	                critical, error, val);
};

