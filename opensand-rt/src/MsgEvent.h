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
/* $Id: MsgEvent.h,v 1.1.1.1 2013/03/28 16:30:46 cgaillardet Exp $ */

#ifndef MSGEVENT_H
#define MSGEVENT_H

#include <string>

#include "Event.h"

using std::string;


/**
  * @class MsgEvent
  * @brief Class describing a message event. Inherits from Event
  *
  *
  */

class MsgEvent: public Event
{
  public:
    /*
	 * Constructor
	 *
     * @param input_fd file descriptor of this event, default -1
     *
     * @param new_priority priority of the event, default 0
     *
     * @param data char pointer to raw data
     *
     * @param size size of data pointed
	 */
    MsgEvent(int32_t input_fd = -1, uint8_t new_priority = 6, unsigned char *data = NULL, uint16_t size = 0);
	~MsgEvent();


	/*
	 * GetData data pointer getter
	 *
	 * @return pointer to data
	 */
    unsigned char *GetData() {return this->data;};

	/*
	 * GetSize size getter
	 *
	 * @return data size
	 */
	uint16_t GetSize() {return this->size;};


	/*
	 * SetData data and size setter
	 *
	 * @param data pointer to data
     *
	 * @param size data length
	 */
    void SetData(unsigned char *data, uint16_t length);

  protected:

    /// data pointer
    unsigned char *data;

    /// data size
	uint16_t size;



};

#endif
