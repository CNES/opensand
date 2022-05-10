/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @file RtChannelBase.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#include "RtChannelBase.h"

#include "Rt.h"

#include <opensand_output/Output.h>

#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#ifdef TIME_REPORTS
	#include <numeric>
	#include <algorithm>
#endif

#define SIG_STRUCT_SIZE 128

// TODO pointer on onEventUp/Down
RtChannelBase::RtChannelBase(const std::string &name, const std::string &type):
	log_init(NULL),
	log_rt(NULL),
	log_receive(NULL),
	log_send(NULL),
	channel_name(name),
	channel_type(type),
	block_initialized(false),
	in_opp_fifo(NULL),
	max_input_fd(-1),
	stop_fd(-1),
	w_sel_break(-1),
	r_sel_break(-1)
{
	FD_ZERO(&(this->input_fd_set));
}

RtChannelBase::~RtChannelBase()
{
	// delete all events
	this->updateEvents(); // update to also clear new events
	for(auto &&event: events)
	{
		if(event.second == NULL)
		{
			continue;
		}
		delete(event.second);
	}
	this->events.clear();
	delete this->in_opp_fifo;
	close(this->w_sel_break);
	close(this->r_sel_break);
#ifdef TIME_REPORTS
	this->getDurationsStatistics();
#endif
}

bool RtChannelBase::shareMessage(void **data, size_t size, uint8_t type)
{
	return this->pushMessage(this->out_opp_fifo, data, size, type);
}

bool RtChannelBase::init(void)
{
	sigset_t signal_mask;
	int32_t pipefd[2];

	// Output Log
	this->log_rt = Output::Get()->registerLog(LEVEL_WARNING, "%s.%s.rt",
	                                   this->channel_name.c_str(),
	                                   this->channel_type.c_str());
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "%s.%s.init",
                                     this->channel_name.c_str(),
                                     this->channel_type.c_str());
	this->log_receive = Output::Get()->registerLog(LEVEL_WARNING, "%s.%s.receive",
                                     this->channel_name.c_str(),
                                     this->channel_type.c_str());
	this->log_send = Output::Get()->registerLog(LEVEL_WARNING, "%s.%s.send",
	                                   this->channel_name.c_str(),
	                                   this->channel_type.c_str());

	LOG(this->log_init, LEVEL_INFO,
	    "Starting initialization\n");

	// pipe used to break select when a new event is received
	if(pipe(pipefd) != 0)
	{
		this->reportError(true,
		                  "cannot initialize pipe\n");
		return false;
	}
	this->r_sel_break = pipefd[0];
	this->w_sel_break = pipefd[1];
	this->addInputFd(this->r_sel_break);

	// create the signal mask for stop (highest priority)
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGINT);
	sigaddset(&signal_mask, SIGQUIT);
	sigaddset(&signal_mask, SIGTERM);
	this->stop_fd = this->addSignalEvent("stop", signal_mask, 0);

	// initialize fifos and create associated messages
	if(!this->in_opp_fifo->init())
	{
		this->reportError(true,
		                  "cannot initialize opposite fifo\n");
		return false;
	}
	if(!this->addMessageEvent(this->in_opp_fifo, 4, true))
	{
		this->reportError(true,
		                  "cannot create opposite message event\n");
		return false;
	}
	if (!initPreviousFifo())
	{
		return false;
	}
	return true;
}

void RtChannelBase::setIsBlockInitialized(bool initialized)
{
	this->block_initialized = initialized;
}

int32_t RtChannelBase::addTimerEvent(const std::string &name,
                                 double duration_ms,
                                 bool auto_rearm,
                                 bool start,
                                 uint8_t priority)
{
	TimerEvent *event = new TimerEvent(name, duration_ms,
	                                   auto_rearm, start,
	                                   priority);
	if(!event)
	{
		this->reportError(true, "cannot create timer event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannelBase::addTcpListenEvent(const std::string &name,
                                     int32_t fd,
                                     size_t max_size,
                                     uint8_t priority)
{
	TcpListenEvent *event = new TcpListenEvent(name,
	                                           fd,
	                                           max_size,
	                                           priority);
	if(!event)
	{
		this->reportError(true, "cannot create file event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannelBase::addFileEvent(const std::string &name,
                                int32_t fd,
                                size_t max_size,
                                uint8_t priority)
{
	FileEvent *event = new FileEvent(name,
	                                 fd,
	                                 max_size,
	                                 priority);
	if(!event)
	{
		this->reportError(true, "cannot create file event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannelBase::addNetSocketEvent(const std::string &name,
                                     int32_t fd,
                                     size_t max_size,
                                     uint8_t priority)
{
	NetSocketEvent *event = new NetSocketEvent(name,
	                                           fd,
	                                           max_size,
	                                           priority);
	if(!event)
	{
		this->reportError(true, "cannot create net socket event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

int32_t RtChannelBase::addSignalEvent(const std::string &name,
                                  sigset_t signal_mask,
                                  uint8_t priority)
{
	SignalEvent *event = new SignalEvent(name, signal_mask, priority);
	if(!event)
	{
		this->reportError(true, "cannot create signal event\n");
		return -1;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return -1;
	}

	return event->getFd();
}

bool RtChannelBase::addMessageEvent(RtFifo *out_fifo,
                                uint8_t priority,
                                bool opposite)
{
	MessageEvent *event;
	
	std::string name = this->channel_type;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	
	if(opposite)
	{
		name += "_opposite";
	}
	event = new MessageEvent(out_fifo, name,
	                         out_fifo->getSigFd(),
	                         priority);
	if(!event)
	{
		this->reportError(true, "cannot create message event\n");
		return false;
	}
	if(!this->addEvent((RtEvent *)event))
	{
		return false;
	}

	return true;
}

bool RtChannelBase::addEvent(RtEvent *event)
{
	auto it = this->events.find(event->getFd());
	if(it != this->events.end())
	{
		this->reportError(true, "duplicated fd\n");
		return false;
	}
	this->new_events.push_back(event);

	// break the select loop
	if(write(this->w_sel_break,
	         MAGIC_WORD,
	         strlen(MAGIC_WORD)) != strlen(MAGIC_WORD))
	{
		LOG(this->log_rt, LEVEL_ERROR,
		    "failed to break select upon a new "
		    "event reception\n");
	}

#ifdef TIME_REPORTS
	this->durations[event->getName()] = list<double>();
#endif

	return true;
}

void RtChannelBase::updateEvents(void)
{
	// add new events
	for(auto &&new_event: new_events)
	{
		LOG(this->log_rt, LEVEL_INFO,
		    "Add new event \"%s\" in list\n",
		    new_event->getName().c_str());
		this->addInputFd(new_event->getFd());
		this->events[new_event->getFd()] = new_event;
	}
	this->new_events.clear();

	// remove old events
	for(auto &&removed_event: removed_events)
	{
		auto it = this->events.find(removed_event);
		if(it != this->events.end())
		{
			LOG(this->log_rt, LEVEL_INFO,
			    "Remove event \"%s\" from list\n",
			    (*it).second->getName().c_str());
			// remove fd from set
			FD_CLR((*it).first, &(this->input_fd_set));
			if((*it).first == this->max_input_fd)
			{
				this->updateMaxFd();
			}
			// remove fd from map
			delete (*it).second;
			this->events.erase(it);
		}
	}
	this->removed_events.clear();
}

void RtChannelBase::updateMaxFd(void)
{
	this->max_input_fd = 0;
	// update the greater fd
	for(auto &&event: events)
	{
		if(event.first > this->max_input_fd)
		{
			this->max_input_fd = event.first;
		}
	}

}

TimerEvent *RtChannelBase::getTimer(event_id_t id)
{
	RtEvent *event = NULL;

	auto it = this->events.find(id);
	if(it == this->events.end())
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "event not found, search in new events\n");
		bool found = false;
		// check in new events
		for(auto &&new_event: new_events)
		{
			if(*new_event == id)
			{
				LOG(this->log_rt, LEVEL_DEBUG,
				    "event found in new events\n");
				found = true;
				event = new_event;
				break;
			}
		}
		if(!found)
		{
			this->reportError(false, "cannot find timer\n");
			return NULL;
		}
	}
	else
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "Timer found\n");
		event = (*it).second;
	}
	if(event && event->getType() != evt_timer)
	{
		this->reportError(false, "cannot start event that is not a timer\n");
		return NULL;
	}

	return (TimerEvent *)event;
}

bool RtChannelBase::startTimer(event_id_t id)
{
	TimerEvent *event = this->getTimer(id);
	if(!event)
	{
		this->reportError(false, "cannot find timer: should not happend here\n");
		return false;
	}

	event->start();

	return true;
}

bool RtChannelBase::setDuration(event_id_t id, double new_duration)
{
	TimerEvent *event = this->getTimer(id);
	if(!event)
	{
		this->reportError(false, "cannot find timer: should not happend here\n");
		return false;
	}

	event->setDuration(new_duration);

	return true;
}

bool RtChannelBase::raiseTimer(event_id_t id)
{
	TimerEvent *event = this->getTimer(id);
	if(!event)
	{
		this->reportError(false, "cannot find timer: should not happend here\n");
		return false;
	}

	event->raise();

	return true;
}

void RtChannelBase::addInputFd(int32_t fd)
{
	if(fd > this->max_input_fd)
	{
		this->max_input_fd = fd;
	}
	FD_SET(fd, &(this->input_fd_set));
}

void RtChannelBase::removeEvent(event_id_t id)
{
	this->removed_events.push_back(id);
}

void *RtChannelBase::startThread(void *pthis)
{
	((RtChannelBase *)pthis)->executeThread();

	return NULL;
}

void RtChannelBase::executeThread(void)
{
	int32_t number_fd;
	int32_t handled;
	fd_set readfds;

	std::list<RtEvent *> priority_sorted_events;

	while(true)
	{
		handled = 0;
		
		// get the new events for the next loop
		this->updateEvents();
		readfds = this->input_fd_set;

		// wait for any event
		// we need a timeout in order to refresh event list
		number_fd = select(this->max_input_fd + 1, &readfds, NULL, NULL, NULL);
		if(number_fd < 0)
		{
			this->reportError(true, "select failed: [%u: %s]\n", errno, strerror(errno));
		}
		// unfortunately, FD_ISSET is the only usable thing
		priority_sorted_events.clear();

		// check for select break
		if(FD_ISSET(this->r_sel_break, &readfds))
		{
			unsigned char data[strlen(MAGIC_WORD)];
			if(read(this->r_sel_break, data, strlen(MAGIC_WORD)) < 0)
			{
				LOG(this->log_rt, LEVEL_ERROR,
				    "failed to read in pipe");
			}
			handled++;
		}

		// handle each event
		for(auto &&event_pair: events)
		{
			RtEvent *event = event_pair.second;
			if(handled >= number_fd)
			{
				// all events treated, no need to continue the loop
				break;
			}
			// if this event FD has raised
			if(!FD_ISSET(event->getFd(), &readfds))
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
					this->reportError(true, "unable to handle signal event\n");
					pthread_exit(NULL);
				}
				this->reportError(false, "unable to handle event\n");
				// ignore this event
				continue;
			}
			priority_sorted_events.push_back(event);
			if(*event == this->stop_fd)
			{
				// we have to stop
				LOG(this->log_rt, LEVEL_INFO,
				    "stop signal received\n");
				pthread_exit(NULL);
			}
		}
		// sort the list according to priority
		priority_sorted_events.sort(RtEvent::compareEvents);

		// call processEvent on each event
		for(auto &&event: priority_sorted_events)
		{
			event->setTriggerTime();
			LOG(this->log_rt, LEVEL_DEBUG, "event received (%s)",
			    event->getName().c_str());
			if(!this->onEvent(event))
			{
				LOG(this->log_rt, LEVEL_ERROR,
				    "failed to process event %s\n",
				    event->getName().c_str());
			}
#ifdef TIME_REPORTS
			timeval time = (*iter)->getTimeFromTrigger();
			double val = time.tv_sec * 1000000L + time.tv_usec;
			this->durations[(*iter)->getName()].push_back(val);
#endif
		}
	}
}

void RtChannelBase::reportError(bool critical, const char *msg_format, ...)
{
	char msg[512];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(msg, 512, msg_format, args);

	va_end(args);

	Rt::reportError(this->channel_name, pthread_self(),
	                critical, msg);
};


void RtChannelBase::setOppositeFifo(RtFifo *in_fifo, RtFifo *out_fifo)
{
	this->in_opp_fifo = in_fifo;
	this->out_opp_fifo = out_fifo;
};

bool RtChannelBase::pushMessage(RtFifo *out_fifo, void **data, size_t size, uint8_t type)
{
	bool success = true;

	// check that block is initialized (i.e. we are in event processing)
	if(!this->block_initialized)
	{
		LOG(this->log_send, LEVEL_NOTICE,
		    "Be careful, some message are sent while process are not "
		    "started. If too many messages are sent we may block because "
		    "fifo is full\n");
		// FIXME we could separate onInit into a static initialization and an
		//       initialization when threads are started
	}

	if(!out_fifo->push(*data, size, type))
	{
		this->reportError(false,
		                  "cannot push data in fifo for next block\n");
		success = false;
	}

	// be sure that the pointer won't be used anymore
	*data = NULL;
	return success;
}

#ifdef TIME_REPORTS
void RtChannelBase::getDurationsStatistics(void) const
{
  std::shared_ptr<OutputEvent> event = Output::Get()->registerEvent("Time Report");
	for(auto &&duration_pair: durations)
	{
		std::list<double> duration = duration_pair.second;
		if(duration.empty())
		{
			continue;
		}
		double sum = std::accumulate(duration.begin(),
		                             duration.end(), 0.0);
		double max = *std::max_element(duration.begin(),
		                               duration.end());
		double min = *std::min_element(duration.begin(),
		                               duration.end());
		double mean = sum / duration.size();

		Output::Get()->sendEvent(event,
		                  "[%s:%s] Event %s: mean = %.2f us, max = %d us, "
		                  "min = %d us, total = %.2f ms\n",
		                  this->channel_name.c_str(),
		                  this->channel_type.c_str(),
		                  duration_pair.first.c_str(), mean, int(max), int(min), sum / 1000);
	}
}
#endif
