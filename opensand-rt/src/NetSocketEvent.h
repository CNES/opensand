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
/* $Id: NetSocketEvent.h,v 1.1.1.1 2013/03/28 16:37:30 cgaillardet Exp $ */



#ifndef NETSOCKETEVENT_H
#define NETSOCKETFDEVENT_H

#include "Types.h"
#include "Event.h"

#include <sys/time.h>

#define MAX_DATA_IN_FD_EVENT 65535

class NetSocketEvent:public Event
{

  public:
	NetSocketEvent(int32_t currentFd, uint8_t priority = 8);
	~NetSocketEvent();
	void SetData(char *data, uint32_t size);
	uint32_t GetSize(){return this->size;};
	char *GetDataPtr(){return this->data;};

  protected:
	char *data;
	uint32_t size;


};

#endif
