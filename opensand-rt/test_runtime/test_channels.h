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
/* $Id: test_channels.h,v 1.1.1.1 2013/04/10 9:29:52 cgaillardet Exp $ */

/****** System Includes ******/


/****** Application Includes ******/
#include "../src/Channel.h"

class TimerChannel : public Channel
{

    public:

    TimerChannel(int32_t file_in_fd, int32_t timer_out_fd, int32_t file_out_fd);
    ~TimerChannel();
    bool OnEvent(Event * event);
    bool CustomInit(void);

    void SetTimerOutputFd(int32_t fd) {this->timer_output_fd = fd;};
    void SetSocketOutputFd(int32_t fd) {this->socket_output_fd = fd;};

    int32_t GetTimerOutputFd(void) { return this->timer_output_fd;};
    int32_t GetSocketOutputFd(void) { return this->socket_output_fd;};

    uint32_t timeouts;

    protected:

    int32_t timer_output_fd ;
    int32_t file_input_fd;
    int32_t socket_output_fd;


    private:
};


class TimerChannelB: public TimerChannel
{
    public:
    TimerChannelB(int32_t file_in_fd, int32_t timer_out_fd, int32_t file_out_fd);
}
;
