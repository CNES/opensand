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

#include "Event.h"


class MsgEvent : public Event
{
    public:
    MsgEvent(int32_t fd = -1, uint8_t new_priority = 6, char *data = NULL, uint16_t size = 0);
    ~MsgEvent();

    char *GetData() {return this->data;};
    uint16_t GetSize() {return this->size;};
    void SetData(char *data, uint32_t length);

    protected:

    char *data;
    uint16_t size;
  private:



};

#endif
