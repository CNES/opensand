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
 * @file FileEvent.h
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The event for message read on network socket, can also be used
 *         by any fd-like oject such as file
 *
 */


#ifndef FILE_EVENT_H
#define FILE_EVENT_H

#include "RtEvent.h"
#include "Types.h"

#include <sys/time.h>


/**
  * @class FileEvent
  * @brief Events describing data received on a nework socket
  *
  */
class FileEvent: public RtEvent
{

  public:

	/**
	 * @brief FileEvent constructor
	 *
	 * @param name      The name of the event
	 * @param fd        The file descriptor to monitor for the event
	 * @param priority  The priority of the event
	 */
	FileEvent(const string &name,
	          int32_t fd = -1,
	          size_t max_size = MAX_SOCK_SIZE,
	          uint8_t priority = 5,
	          event_type_t type = evt_file);
	~FileEvent();


	/**
	 * @brief Get the message content
	 *
	 * @return the data contained in the message
	 */
	 virtual unsigned char *getData(void);

	/*
	 * @brief Get the size of data in the message
	 *
	 * @return the size of data in the message
	 */
	size_t getSize(void) const {return this->size;};

	virtual bool handle(void);

  protected:

	/// The maximum size of received data
	size_t max_size;

	/// data pointer
	unsigned char *data;

	/// data size
	size_t size;


};

#endif
