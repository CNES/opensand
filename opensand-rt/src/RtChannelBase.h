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
 * @file RtChannelBase.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#ifndef RT_CHANNEL_BASE_H
#define RT_CHANNEL_BASE_H

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "Types.h"
#include "TimerEvent.h"


class Block;
class RtFifo;
class RtEvent;
class OutputLog;


//#define TIME_REPORTS

/**
 * @class RtChannelBase
 * @brief Base for all channel classes
 *
 * channel direction is always relative to its position.
 * Its next channel is the channel it will send data.
 * Its previous channel is the channel it will receive data from.
 *
 */
class RtChannelBase
{
	friend class Block;
	friend class BlockManager;

 protected:
	/// Output Log
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_rt;
	std::shared_ptr<OutputLog> log_receive;
	std::shared_ptr<OutputLog> log_send;

	/**
	 * @brief Channel Constructor
	 *
	 * @param name       The name of the block channel
	 * @param type       The type of the block channel (upward or downward)
	 *
	 */
	RtChannelBase(const std::string &name, const std::string &type);

	virtual ~RtChannelBase();

	/**
	 * @brief Initialize the channel
	 *        Can be use to initialize elements specific to a channel
	 *
	 * @warning: called at the end of initialization, if you have to do some
	 *           processing, you can do them here
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool onInit(void) {return true;};

	/**
	 * @param Process an event
	 *
	 * @param event  The event
	 * @return true on success, false otherwise
	 */
	virtual bool onEvent(const RtEvent *const event) = 0;

 public:
	/**
	 * @brief Get the channel name
	 * 
	 * @return channel name
	 */
	std::string getName() { return this->channel_name; }
	
	/**
	 * @brief Add a timer event to the channel
	 *
	 * @param name         The name of the timer
	 * @param duration_ms  The duration of the timer (ms)
	 * @param auto_rearm   Whether the timer will get rearmed after processing
	 * @param start        Whether the timer will start after being created
	 * @param priority     The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addTimerEvent(const std::string &name,
	                      double duration_ms,
	                      bool auto_rearm = true,
	                      bool start = true,
	                      uint8_t priority = 2);

	/**
	 * @brief Add a net socket event to the channel
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor
	 * @param max_size  The maximum data size
	 * @param priority  The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addNetSocketEvent(const std::string &name,
	                          int32_t fd,
	                          size_t max_size = MAX_SOCK_SIZE,
	                          uint8_t priority = 3);

	/**
	 * @brief Add a tcp listen event to the channel
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor
	 * @param max_size  The maximum data size
	 * @param priority  The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addTcpListenEvent(const std::string &name,
	                          int32_t fd,
	                          size_t max_size = MAX_SOCK_SIZE,
	                          uint8_t priority = 4);

	/**
	 * @brief Add a file event to the channel
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor
	 * @param max_size  The maximum data size
	 * @param priority  The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addFileEvent(const std::string &name,
	                     int32_t fd,
	                     size_t max_size = MAX_SOCK_SIZE,
	                     uint8_t priority = 4);

	/**
	 * @brief Add a signal event to the channel
	 *
	 * @param name         The name of the event
	 * @param signal_mask  Mask containing all the signals that trigger this event
	 * @param priority     The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addSignalEvent(const std::string &name,
	                       sigset_t signal_mask,
	                       uint8_t priority = 1);

	/**
	 * @brief Internal error report
	 *
	 * @param critical    Whether the application should be stopped
	 * @param msg_format  The error message
	 */
	void reportError(bool critical, const char *msg_format, ...);

	/**
	 * @brief Remove an event
	 *
	 * @param id  The event id
	 */
	void removeEvent(event_id_t id);

	/**
	 * @brief Start a timer
	 *
	 * @param id  The timer id
	 * @return true on success, false otherwise
	 */
	bool startTimer(event_id_t id);

	/**
	 * @brief set a new duration
	 *
	 * @param id  The timer id
	 * @param new_duration    the new duration
	 * @return true on success, false otherwise
	 */
	bool setDuration(event_id_t id, double new_duration);

	/**
	 * @brief Trigger a timer immediately
	 *        In fact, we set the minimum time and start it
	 *        because there is no way to trigger it manually
	 *
	 * @param id  The timer id
	 * @return true on success, false otherwise
	 */
	bool raiseTimer(event_id_t id);

	/**
	 * @brief Transmit a message to the opposite channel (in the same block)
	 *
	 * @param data  IN: A pointer on the  message to enqueue
	 *              OUT: NULL
	 * @param size  The size of data in message
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool shareMessage(void **data, size_t size=0, uint8_t type=0);

 protected:
	/**
	 * @brief Internal channel initialization
	 *        Call specific onInit function
	 *
	 * @return true on success, false otherwise
	 */
	bool init(void);

	virtual bool initPreviousFifo() = 0;

	bool initSingleFifo(std::shared_ptr<RtFifo> &fifo);

	/**
	 * @brief Set the block initialization status
	 * 
	 * @param initialized  Th block initialization status
	 */
	void setIsBlockInitialized(bool initialized);
	
	/**
	 * @brief Set the fifos for opposite channel (in the same block)
	 *
	 * @param in_fifo   The fifo for incoming messages
	 * @param out_fifo  The fifo for outgoing messages
	 */
	void setOppositeFifo(std::shared_ptr<RtFifo> &in_fifo, std::shared_ptr<RtFifo> &out_fifo);

	/**
	 * @brief Add a message  event to the channel
	 *
	 * @param fifo      The fifo for the messages
	 * @param priority  The priority of the event (small for high priority)
	 * @param opposite  Whether this is a message for opposite channels
	 * @return true on success, false otherwise
	 */
	bool addMessageEvent(std::shared_ptr<RtFifo> &fifo, uint8_t priority = 6, bool opposite = false);

	/**
	 * @brief Push a message in another channel fifo
	 *
	 * @param fifo  The fifo
	 * @param data  IN: A pointer on the message to enqueue
	 *              OUT: NULL
	 * @param size  The size of data in message
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool pushMessage(std::shared_ptr<RtFifo> &fifo, void **data, size_t size, uint8_t type = 0);

#ifdef TIME_REPORTS
	/// statistics about events durations (in us)
	std::map<std::string, std::vector<double> > durations;

	/**
	 * @brief print statistics on events durations
	 */
	void getDurationsStatistics(void) const;
#endif

 private:
	/// name of the block channel
	std::string channel_name;
	
	/// type of the block channel (upward or downward)
	std::string channel_type;
	
	bool block_initialized;
	
	/// events that are currently monitored by the channel thread
	std::map<event_id_t, std::unique_ptr<RtEvent>> events;

	/// the list of new events (used to avoid updates inside the loop)
	std::vector<std::unique_ptr<RtEvent>> new_events;

	/// the list of removed event id
	std::vector<event_id_t> removed_events;

	/// The fifo for incoming messages from opposite channel
	std::shared_ptr<RtFifo> in_opp_fifo;
	/// The fifo for outgoing messages to opposite channel
	std::shared_ptr<RtFifo> out_opp_fifo;

	/// contains the highest FD of input events
	int32_t max_input_fd;

	/// fd_set containing monitored input FDs
	fd_set input_fd_set;

	/// fd o the stop signal event
	int32_t stop_fd;

	/// fd used to write on a pipe that  breaks select when an event is created
	int32_t w_sel_break;
	/// fd used in select to break when an event is created
	int32_t r_sel_break;

	/**
	 * @brief the loop
	 *
	 */
	void executeThread(void);

	/**
	 * @brief Add an event in event map
	 *
	 * @param event  The event
	 * @return true on success, false otherwise
	 */
	bool addEvent(std::unique_ptr<RtEvent> event);

	/**
	 * @brief Update the events map with the new received event
	 *        We need to do that in order to avoid modifying the event
	 *        map while itering on it
	 */
	void updateEvents(void);

	/**
	 * @brief Update the maximum input fd after event removal
	 */
	void updateMaxFd(void);

	/**
	 * @brief Add a fd to input_fd_set
	 *        Should be called each time the channel got a new event
	 *
	 * @param fd  The file descriptor to monitor
	 */
	void addInputFd(int32_t fd);

	/**
	 * @brief Get a timer
	 *
	 * @param id  The timer id
	 * @return the timer  on success, NULL otherwise
	 */
	TimerEvent *getTimer(event_id_t id);
};


#endif
