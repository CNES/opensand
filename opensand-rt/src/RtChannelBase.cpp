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
 */

#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <set>

#include <opensand_output/Output.h>

#include "RtChannelBase.h"
#include "Rt.h"
#include "RtFifo.h"
#include "RtEvent.h"
#include "FileEvent.h"
#include "MessageEvent.h"
#include "NetSocketEvent.h"
#include "SignalEvent.h"
#include "TcpListenEvent.h"
#include "TimerEvent.h"
#include "RtCommunicate.h"

#ifdef TIME_REPORTS
	#include <numeric>
	#include <algorithm>
#endif


namespace Rt
{


// TODO pointer on onEventUp/Down
ChannelBase::ChannelBase(const std::string &name, const std::string &type):
	log_init{nullptr},
	log_rt{nullptr},
	log_receive{nullptr},
	log_send{nullptr},
	channel_name{name},
	channel_type{type},
	block_initialized{false},
	in_opp_fifo{nullptr},
	out_opp_fifo{nullptr},
	stop_fd{-1},
	w_sel_break{-1},
	r_sel_break{-1}
{
	FD_ZERO(&(this->input_fd_set));
}


ChannelBase::~ChannelBase()
{
	close(this->w_sel_break);
	close(this->r_sel_break);
#ifdef TIME_REPORTS
	this->getDurationsStatistics();
#endif
}


bool ChannelBase::onInit()
{
	return true;
}


bool ChannelBase::onEvent(const Event& event)
{
	return false;
}


bool ChannelBase::shareMessage(Ptr<void> data, uint8_t type)
{
	Message m{std::move(data)};
	m.type = type;
	return this->pushMessage(this->out_opp_fifo, std::move(m));
}


bool ChannelBase::init(int stop_fd)
{
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

	// register the signal mask for stop
	this->stop_fd = stop_fd;
	FD_SET(this->stop_fd, &(this->input_fd_set));

	LOG(this->log_init, LEVEL_INFO,
	    "Starting initialization\n");

	// pipe used to break select when a new event is received
	int32_t pipefd[2];
	if(pipe(pipefd) != 0)
	{
		this->reportError(true, "cannot initialize pipe\n");
		return false;
	}
	this->r_sel_break = pipefd[0];
	this->w_sel_break = pipefd[1];
	FD_SET(this->r_sel_break, &(this->input_fd_set));

	// initialize fifos and create associated messages
	if(!this->in_opp_fifo || !this->in_opp_fifo->init())
	{
		this->reportError(true, "cannot initialize opposite fifo\n");
		return false;
	}

	if(!this->addMessageEvent(this->in_opp_fifo, 4, true))
	{
		this->reportError(true, "cannot create opposite message event\n");
		return false;
	}

	if (!initPreviousFifo())
	{
		return false;
	}
	return true;
}


void ChannelBase::setIsBlockInitialized(bool initialized)
{
	this->block_initialized = initialized;
}


int32_t ChannelBase::addTimerEvent(const std::string &name,
                                   double duration_ms,
                                   bool auto_rearm,
                                   bool start,
                                   uint8_t priority)
{
	std::unique_ptr<TimerEvent> event;

	try {
		event.reset(new TimerEvent(name, duration_ms, auto_rearm, start, priority));
	} catch (std::bad_alloc&) {
		this->reportError(true, "cannot create timer event\n");
		return -1;
	}

	int32_t event_fd = event->getFd();
	if (!this->addEvent(std::move(event)))
	{
		return -1;
	}

	return event_fd;
}


int32_t ChannelBase::addTcpListenEvent(const std::string &name,
                                         int32_t fd,
                                         size_t max_size,
                                         uint8_t priority)
{
	std::unique_ptr<TcpListenEvent> event;
	
	try {
		event.reset(new TcpListenEvent(name, fd, max_size, priority));
	} catch (std::bad_alloc&) {
		this->reportError(true, "cannot create file event\n");
		return -1;
	}

	int32_t event_fd = event->getFd();
	if (!this->addEvent(std::move(event)))
	{
		return -1;
	}

	return event_fd;
}


int32_t ChannelBase::addFileEvent(const std::string &name,
                                  int32_t fd,
                                  size_t max_size,
                                  uint8_t priority)
{
	std::unique_ptr<FileEvent> event;
	
	try {
		event.reset(new FileEvent(name, fd, max_size, priority));
	} catch (std::bad_alloc&) {
		this->reportError(true, "cannot create file event\n");
		return -1;
	}

	int32_t event_fd = event->getFd();
	if (!this->addEvent(std::move(event)))
	{
		return -1;
	}

	return event_fd;
}


int32_t ChannelBase::addNetSocketEvent(const std::string &name,
                                       int32_t fd,
                                       size_t max_size,
                                       uint8_t priority)
{
	std::unique_ptr<NetSocketEvent> event;
	
	try {
		event.reset(new NetSocketEvent(name, fd, max_size, priority));
	} catch (std::bad_alloc&) {
		this->reportError(true, "cannot create net socket event\n");
		return -1;
	}

	int32_t event_fd = event->getFd();
	if (!this->addEvent(std::move(event)))
	{
		return -1;
	}

	return event_fd;
}


int32_t ChannelBase::addSignalEvent(const std::string &name,
                                    sigset_t signal_mask,
                                    uint8_t priority)
{
	std::unique_ptr<SignalEvent> event;
	
	try {
		event.reset(new SignalEvent(name, signal_mask, priority));
	} catch (std::bad_alloc&) {
		this->reportError(true, "cannot create signal event\n");
		return -1;
	}

	int32_t event_fd = event->getFd();
	if (!this->addEvent(std::move(event)))
	{
		return -1;
	}

	return event_fd;
}


bool ChannelBase::addMessageEvent(std::shared_ptr<Fifo> &out_fifo,
                                  uint8_t priority,
                                  bool opposite)
{
	std::string name = this->channel_type;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	if(opposite)
	{
		name += "_opposite";
	}

	std::unique_ptr<MessageEvent> event;
  
	try {
		event.reset(new MessageEvent(out_fifo, name, out_fifo->getSigFd(), priority));
	} catch (const std::bad_alloc&) {
		this->reportError(true, "cannot create message event\n");
		return false;
	}

	return this->addEvent(std::move(event));
}


bool ChannelBase::addEvent(std::unique_ptr<Event> event)
{
	if(this->events.find(event->getFd()) != this->events.end())
	{
		this->reportError(true, "duplicated fd\n");
		return false;
	}
	this->new_events.push_back(std::move(event));

	// break the select loop
	if (!check_write(this->w_sel_break))
	{
		LOG(this->log_rt, LEVEL_ERROR,
		    "failed to break select upon a new "
		    "event reception\n");
	}

#ifdef TIME_REPORTS
	this->durations[event->getName()] = std::vector<double>();
#endif

	return true;
}


void ChannelBase::updateEvents(void)
{
	// add new events
	for(auto &&new_event: new_events)
	{
		LOG(this->log_rt, LEVEL_INFO,
		    "Add new event \"%s\" in list\n",
		    new_event->getName().c_str());
		// add fd to set
		auto fd = new_event->getFd();
		FD_SET(fd, &(this->input_fd_set));
		// add fd to map
		this->events[fd] = std::move(new_event);
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
			    it->second->getName().c_str());
			// remove fd from set
			FD_CLR(it->first, &(this->input_fd_set));
			// remove fd from map
			this->events.erase(it);
		}
	}
	this->removed_events.clear();
}


TimerEvent *ChannelBase::getTimer(event_id_t id)
{
	Event *event = nullptr;

	auto it = this->events.find(id);
	if(it == this->events.end())
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "event not found, search in new events\n");

		// check in new events
		for(auto &&new_event: new_events)
		{
			if(*new_event == id)
			{
				LOG(this->log_rt, LEVEL_DEBUG,
				    "event found in new events\n");
				event = new_event.get();
				goto found;
			}
		}

		this->reportError(false, "cannot find timer\n");
		return nullptr;
	}
	else
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "Timer found\n");
		event = it->second.get();
	}

found:
	TimerEvent *timer_event = dynamic_cast<TimerEvent *>(event);
	if(!timer_event || timer_event->getType() != EventType::Timer)
	{
		this->reportError(false, "cannot start event that is not a timer\n");
		return nullptr;
	}

	return timer_event;
}


bool ChannelBase::startTimer(event_id_t id)
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


bool ChannelBase::setDuration(event_id_t id, double new_duration)
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


bool ChannelBase::raiseTimer(event_id_t id)
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


void ChannelBase::removeEvent(event_id_t id)
{
	this->removed_events.push_back(id);
}


void ChannelBase::executeThread(void)
{
	int32_t number_fd;
	int32_t handled;
	fd_set readfds;

	/*
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
	timeval timeout{0, 500};
	*/

	// sort the list according to priority
	static const auto eventSorter = [](const Event* e1, const Event* e2) { return (*e1) < (*e2); };
	std::set<Event *, decltype(eventSorter)> priority_sorted_events(eventSorter);

	while(true)
	{
		handled = 0;
		
		// get the new events for the next loop
		this->updateEvents();
		priority_sorted_events.clear();
		readfds = this->input_fd_set;

		// wait for any event
		// we need a timeout in order to refresh event list
		event_id_t max_fd = this->events.rbegin()->first;
		number_fd = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
		if(number_fd < 0)
		{
			this->reportError(true, "select failed: [%u: %s]\n", errno, strerror(errno));
		}

		// unfortunately, FD_ISSET is the only usable thing
		if(FD_ISSET(this->stop_fd, &readfds))
		{
			// we have to stop
			LOG(this->log_rt, LEVEL_INFO,
				"stop signal received\n");
			std::cout << "Stop signal received in " << this->channel_name << " (" << this->channel_type << ")\n";
			return;
		}

		// check for select break
		if(FD_ISSET(this->r_sel_break, &readfds))
		{
			if (!check_read(this->r_sel_break))
			{
				LOG(this->log_rt, LEVEL_ERROR,
				    "failed to read in pipe");
			}
			handled++;
		}

		// handle each event
		for(auto &&[key, event]: events)
		{
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
				if(event->getType() == EventType::Signal)
				{
					// this is the only case where it is critical as
					// stop event is a signal
					this->reportError(true, "unable to handle signal event\n");
					return;
				}
				this->reportError(false, "unable to handle event\n");
				// ignore this event
				continue;
			}
			priority_sorted_events.insert(event.get());
		}

		// call processEvent on each event
		for(auto &&event: priority_sorted_events)
		{
			const std::string event_name = event->getName();
			event->setTriggerTime();
			LOG(this->log_rt, LEVEL_DEBUG, "event received (%s)",
			    event_name.c_str());
			if(!this->handleEvent(event))
			{
				LOG(this->log_rt, LEVEL_ERROR,
				    "failed to process event %s\n",
				    event_name.c_str());
			}
#ifdef TIME_REPORTS
			time_val_t time = event->getTimeFromTrigger();
			this->durations[event_name].push_back(time);
#endif
		}
	}
}

void ChannelBase::reportError(bool critical, const char *msg_format, ...)
{
	char msg[512];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(msg, 512, msg_format, args);

	va_end(args);

	Rt::reportError(this->channel_name, std::this_thread::get_id(), critical, msg);
};


void ChannelBase::setOppositeFifo(std::shared_ptr<Fifo> &in_fifo, std::shared_ptr<Fifo> &out_fifo)
{
	this->in_opp_fifo = in_fifo;
	this->out_opp_fifo = out_fifo;
}


bool ChannelBase::initSingleFifo(std::shared_ptr<Fifo> &fifo)
{
	if (fifo)
	{
		if (!fifo->init())
		{
			this->reportError(true, "cannot initialize previous fifo\n");
			return false;
		}
		if (!this->addMessageEvent(fifo))
		{
			this->reportError(true, "cannot create previous message event\n");
			return false;
		}
	}
	return true;
}


bool ChannelBase::pushMessage(std::shared_ptr<Fifo> &out_fifo, Message message)
{
	if (out_fifo == nullptr)
	{
		LOG(this->log_send, LEVEL_ERROR, "Tried to send a message through a null FIFO");
		return false;
	}

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

	if(!out_fifo->push(std::move(message)))
	{
		this->reportError(false, "cannot push data in fifo for next block\n");
		return false;
	}

	return true;
}

#ifdef TIME_REPORTS
void ChannelBase::getDurationsStatistics(void) const
{
  std::shared_ptr<OutputEvent> event = Output::Get()->registerEvent("Time Report");
	for(auto &&duration_pair: durations)
	{
		std::vector<double> duration = duration_pair.second;
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


};
