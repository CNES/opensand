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
 * @file Channel.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The channel included in blocks
 *
 */

#ifndef CHANNEL_H
#define CHANNEL_H

//#include "Block.h"
#include "Event.h"
#include "Types.h"
#include "MessageEvent.h"
#include "TimerEvent.h"
#include "NetSocketEvent.h"
#include "SignalEvent.h"
#include "Fifo.h"


#include <stdlib.h>
#include <string>
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
class Channel
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
	Channel(Block &bl, chan_type_t chan);


  public:

	virtual ~Channel();

	/**
	 * @brief Initialize the channel
	 *        Can be use to initialize elements specific to a channel
	 * 
	 * @return true on success, false otherwise
	 */
	virtual bool onInit(void) {return true;};

	/**
	 * @brief Add a timer event to the channel
	 *
	 * @param name         The name of the timer
	 * @param duration_ms  The duration of the timer (ms)
	 * @param priority     The priority of the event (small for high priority)
	 * @param auto_rearm   Whteher the timer will get rearmed after processing
	 * @return the event id
	 */
	int32_t addTimerEvent(const string &name,
	                      uint32_t duration_ms,
	                      uint8_t priority = 2,
	                      bool auto_rearm = true);

	/**
	 * @brief Add a net socket event to the channel
	 *
	 * @param fd       The file descriptor to monitor
	 * @param priority The priority of the event (small for high priority)
	 * @return the event id
	 */
	int32_t addNetSocketEvent(int32_t fd, uint8_t priority = 3);

	/**
	 * @brief Add a signal event to the channel
	 *
	 * @param name         The name of the event
	 * @param signal_mask  Mask containing all the signals that trigger this event
	 * @param priority     The priority of the event (small for high priority)
	 * @return the event id
	 */
	int32_t addSignalEvent(const string &name, sigset_t signal_mask, uint8_t priority = 1);

  protected:

	/**
	 * @brief Add a message in the next channel queue
	 *
	 * @param message  The message to enqueue
	 * @return true on success, false otherwise
	 */
	bool enqueueMessage(void *message);

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
	void setFifo(Fifo *fifo) {this->fifo = fifo;};

	/**
	 * @brief Update the fifo size
	 *
	 * @param fifo_size  The new fifo size
	 */
	void setFifoSize(uint8_t size) {this->fifo->resize(size);};

	/**
	 * @brief Set the fifo for next channl
	 *
	 * @param fifo  The fifo of the next channel
	 */
	void setNextFifo(Fifo *fifo) {this->next_fifo = fifo;};

	/*
	 * @brief Start the channel thread
	 *
	 * @return true on success, false otherwise
	 */
	bool start(void);

	/*
	 * @brief stop the channel thread
	 *
	 * @param signal  The received signal
	 * @return true on success, false otherwise
	 */
	bool stop(int signal);

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
	list<Event *> events;

	/// The fifo of the channel
	Fifo *fifo;
	/// The fifo on the next channel
	Fifo *next_fifo;

	/// contains the highest FD of input events
	int32_t max_input_fd;

	/// fd_set containing monitored input FDs
	fd_set input_fd_set;

	/// fd o the stop signal event
	int32_t stop_fd;

	/**
	 * @param Internal error report
	 *
	 * @param critical   Whether the application should be stopped
	 * @param error      The error message
	 * @param val        The return error code
	 */
	void reportError(bool critical, string error, int val = 0);

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
	bool processEvent(const Event *const event);
	// TODO replace with onEvent

};

#endif

