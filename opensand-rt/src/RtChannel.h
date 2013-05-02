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
 * @file RtChannel.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#ifndef RT_CHANNEL_H
#define RT_CHANNEL_H

#include "Types.h"

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

	/// The bloc containing channel
	// TODO remove
	Block &block;

	/**
	 * @brief Channel Constructor
	 *
	 * @param bl    The block containing this channel (TODO remove)
	 * @param chan  The channel type
     *
	 */
	RtChannel(Block &bl, chan_type_t chan);


  public:

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
	                      uint32_t duration_ms,
	                      bool auto_rearm = true,
	                      bool start = true,
	                      uint8_t priority = 2);

	/**
	 * @brief Add a net socket event to the channel
	 *
	 * @param name	    The name of the event
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
	 * @param priority  The priority of the event (small for high priority)
	 * @return the event id on success, -1 otherwise
	 */
	int32_t addFileEvent(const string &name,
	                     int32_t fd,
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

  protected:

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
	bool enqueueMessage(void **data, size_t size, uint8_t type = 0);

	/**
	 * @brief Internal channel initialization
	 *        Call specific onInit function
	 *
	 * @return true on success, false otherwise
	 */
	bool init(void);

	/**
	 * @brief Set the channel fifo
	 *
	 * @param fifo  The fifo
	 */
	void setFifo(RtFifo *fifo);

	/**
	 * @brief Update the fifo size
	 *
	 * @param fifo_size  The new fifo size
	 */
	void setFifoSize(uint8_t size);

	/**
	 * @brief Set the fifo for next channl
	 *
	 * @param fifo  The fifo of the next channel
	 */
	void setNextFifo(RtFifo *fifo);

	/**
	 * @brief Start the channel thread
	 *
	 * @param pthis  pointer to the channel
	 *
	 */
	static void *startThread(void *pthis);

  private:

	/// the block direction
	chan_type_t chan;

	/// events that are currently monitored by the channel thread
	map<event_id_t, RtEvent *> events;

	/// the list of new events (used to avoid updates inside the loop)
	list<RtEvent *> new_events;

	/// the list of removed event id
	list<event_id_t> removed_events;

	/// The fifo of the channel
	RtFifo *fifo;
	/// The fifo on the next channel
	RtFifo *next_fifo;

	/// contains the highest FD of input events
	int32_t max_input_fd;

	/// fd_set containing monitored input FDs
	fd_set input_fd_set;

	/// fd o the stop signal event
	int32_t stop_fd;

	/**
	 * @brief Add a message  event to the channel
	 *
	 * @param signal_mask  Mask containing all the signals that trigger this event
	 * @param priority     The priority of the event (small for high priority)
	 */
	void addMessageEvent(uint8_t priority = 6);

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

};

#endif

