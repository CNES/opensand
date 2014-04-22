/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 * @file RtChannel.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#ifndef RT_CHANNEL_H
#define RT_CHANNEL_H

#include "Types.h"
#include "TimerEvent.h"

#include <opensand_output/OutputLog.h>

#include <stdlib.h>
#include <string>
#include <map>
#include <list>
#include <sys/select.h>


class Block;
class RtFifo;
class RtEvent;

using std::list;
using std::map;
using std::string;

//#define TIME_REPORTS

/**
 * @class RtChannel
 * @brief describes a single channel
 *
 * channel direction is always relative to its position.
 * Its next channel is the channel it will send data.
 * Its previous channel is the channel it will receive data from.
 *
 */
class RtChannel
{
	friend class Block;
	friend class BlockManager;

  protected:

	/// Output Log
	OutputLog *log_init;
	OutputLog *log_rt;
	OutputLog *log_receive;
	OutputLog *log_send;

	/// The bloc containing channel
	// TODO remove
	Block *block;

	/**
	 * @brief Channel Constructor
	 *
	 * @param bl    The block containing this channel (TODO remove)
	 * @param chan  The channel type
	 *
	 */
	RtChannel(Block *const bl, chan_type_t chan);

	/**
	 * @brief Channel Constructor
	 *
	 * @param bl        The block containing this channel (TODO remove)
	 * @param chan      The channel type
	 * @tparam specific  User defined data
	 *
	 */
	template<class T>
	RtChannel(Block *const bl, chan_type_t chan, T specific);

	virtual ~RtChannel();

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


  public:

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
	int32_t addTimerEvent(const string &name,
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
	int32_t addNetSocketEvent(const string &name,
	                          int32_t fd,
	                          size_t max_size = MAX_SOCK_SIZE,
	                          uint8_t priority = 3);

	/**
	 * @brief Add a file event to the channel
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor
	 * @param max_size  The maximum data size
	 * @param priority  The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addFileEvent(const string &name,
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
	int32_t addSignalEvent(const string &name,
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
	 * @brief Trigger a timer immediately
	 *        In fact, we set the minimum time and start it
	 *        because there is no way to trigger it manually
	 *
	 * @param id  The timer id
	 * @return true on success, false otherwise
	 */
	bool raiseTimer(event_id_t id);

	/**
	 * @brief Add a message in the next channel queue
	 * @warning The message shall not be reused in the channel after this call
	 *          because will be used in other blocks
	 *
	 * @param data  IN: A pointer on the  message to enqueue
	 *              OUT: NULL
	 * @param size  The size of data in message
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool enqueueMessage(void **data, size_t size=0, uint8_t type=0);

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

	/**
	 * @brief Set the fifo for previous channel message
	 *
	 * @param fifo  The fifo
	 */
	void setPreviousFifo(RtFifo *fifo);

	/**
	 * @brief Set the fifo for next channel
	 *
	 * @param fifo  The fifo of the next channel
	 */
	void setNextFifo(RtFifo *fifo);

	/**
	 * @brief Set the fifois for opposite channel (in the same block)
	 *
	 * @param in_fifo   The fifo for incoming messages
	 * @param out_fifo  The fifo for outgoing messages
	 */
	void setOppositeFifo(RtFifo *in_fifo, RtFifo *out_fifo);

	/**
	 * @brief Start the channel thread
	 *
	 * @param pthis  pointer to the channel
	 *
	 */
	static void *startThread(void *pthis);

	/**
	 * @brief Get the current timeval
	 *
	 * @return the current time
	 */
	clock_t getCurrentTime(void);

#ifdef TIME_REPORTS
	/// statistics about events durations (in us)
	map<string, list<double> > durations;

	/**
	 * @brief print statistics on events durations
	 */
	void getDurationsStatistics(void) const;
#endif

  private:

	/// the block direction
	chan_type_t chan;

	/// events that are currently monitored by the channel thread
	map<event_id_t, RtEvent *> events;

	/// the list of new events (used to avoid updates inside the loop)
	list<RtEvent *> new_events;

	/// the list of removed event id
	list<event_id_t> removed_events;

	/// The fifo of the channel for messages from previous channel
	RtFifo *previous_fifo;
	/// The fifo on the next channel
	RtFifo *next_fifo;
	/// The fifo for incoming messages from opposite channel
	RtFifo *in_opp_fifo;
	/// The fifo for outgoing messages to opposite channel
	RtFifo *out_opp_fifo;

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
	 * @brief Add a message  event to the channel
	 *
	 * @param fifo      The fifo for the messages
	 * @param priority  The priority of the event (small for high priority)
	 * @param opposite  Whether this is a message for opposite channels
	 * @return true on success, false otherwise
	 */
	bool addMessageEvent(RtFifo *fifo, uint8_t priority=6, bool opposite=false);

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
	bool addEvent(RtEvent *event);

	/**
	 * @brief Update the events map with the new received event
	 *        We need to do that in order to avoid modifying the event
	 *        map while itering on it
	 *
	 */
	void updateEvents(void);

	/**
	 * @brief Update the maximum input fd after event removal
	 *
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
	 * @param Process an event
	 *
	 * @param event  The event
	 * @return true on success, false otherwise
	 */
	bool processEvent(const RtEvent *const event);
	// TODO replace with onEvent

	/**
	 * @brief Get a timer
	 *
	 * @param id  The timer id
	 * @return the timer  on success, NULL otherwise
	 */
	TimerEvent *getTimer(event_id_t id);

	/**
	 * @brief Push a message in another channel fifo
	 *
	 * @param fifo  The fifo
	 * @param data  IN: A pointer on the  message to enqueue
	 *              OUT: NULL
	 * @param size  The size of data in message
	 * @param type  The type of message
	 * @return true on success, false otherwise
	 */
	bool pushMessage(RtFifo *fifo, void **data, size_t size, uint8_t type=0);

};

template<class T>
RtChannel::RtChannel(Block *const bl, chan_type_t chan, T specific):
	log_init(NULL),
	log_rt(NULL),
	log_receive(NULL),
	log_send(NULL),
	block(bl),
	chan(chan),
	previous_fifo(NULL),
	in_opp_fifo(NULL),
	max_input_fd(-1),
	stop_fd(-1),
	w_sel_break(-1),
	r_sel_break(-1)
{
	FD_ZERO(&(this->input_fd_set));
};

#endif

