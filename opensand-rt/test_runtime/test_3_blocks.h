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





class InputChannel : public Channel
{

    public:
    InputChannel(int32_t input_fd);


    bool OnEvent(Event * event);
    bool CustomInit(void);

    protected:
    int32_t file_input_fd;
    private:
};

class InputChannelF : InputChannel
{
    public:
    InputChannelF(int32_t input_fd);

};
class InputChannelB : InputChannel
{
    public:
    InputChannelB(int32_t input_fd);


};


class MiddleChannel : public Channel
{

    public:
    MiddleChannel();

    bool OnEvent(Event * event);
    bool CustomInit(void);

    protected:

    private:
};

class MiddleChannelF : MiddleChannel
{};
class MiddleChannelB : MiddleChannel {};

class OutputChannel : public Channel
{

    public:
    OutputChannel(int32_t output_fd);

    bool OnEvent(Event * event);
    bool CustomInit(void);

    int32_t GetSocketOutputFd(void) { return this->file_output_fd;};
    protected:
    int32_t file_output_fd;
    private:
};

class OutputChannelF:OutputChannel
{
    public:
    OutputChannelF(int32_t output_fd);

};
class OutputChannelB:OutputChannel
{
    public:
    OutputChannelB(int32_t output_fd);

};
