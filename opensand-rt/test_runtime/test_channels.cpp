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
/* $Id: test_channels.cpp,v 1.1.1.1 2013/04/10 9:32:18 cgaillardet Exp $ */

/****** System Includes ******/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


/****** Application Includes ******/
#include "test_channels.h"

TimerChannel::TimerChannel(int32_t file_in_fd, int32_t timer_out_fd, int32_t file_out_fd)
{
    this->timeouts = 0;
    this->timer_output_fd= timer_out_fd;
    this->socket_output_fd= file_out_fd;
    this->file_input_fd = file_in_fd;

}

TimerChannelB::TimerChannelB(int32_t file_in_fd, int32_t timer_out_fd, int32_t file_out_fd):
TimerChannel::TimerChannel(file_in_fd, timer_out_fd, file_out_fd)
{
}

bool TimerChannel::CustomInit(void)
{
    //timer event every 100ms
    this->AddTimerEvent(100);
	this->AddNetSocketEvent(this->file_input_fd);
    return true;
}


bool TimerChannel::OnEvent(Event* event)
{
    string error;
    char time_as_string[200];
    char second_conversion_buffer[21];
    char microsecond_conversion_buffer[21];
    timeval trigger_time;
    int res=0;

    if (event->GetType() == Timer)
    {
        this->timeouts++;
        trigger_time = event->GetLifetime();
        // timeval is a pain to print as its types are time_t and suseconds_t
        // both being "arithmetic type" and system dependant
        // if 32bits, it is subject to year 2038 bug


        if (sizeof(time_t) == 4) //32 bits
        {
            //convert seconds to char*
            snprintf(second_conversion_buffer,10,"%03ld",trigger_time.tv_sec);

            //convert microseconds to char*
            snprintf(microsecond_conversion_buffer,10,"%03ld",trigger_time.tv_usec);

            snprintf(time_as_string, 199 ,"Triggered at %s,%s\n",
                     second_conversion_buffer,
                     microsecond_conversion_buffer);
        }
        else if (sizeof(time_t) == 8) //64 bits
        {
            //convert seconds to char*
            snprintf(second_conversion_buffer,20,"%03ld",trigger_time.tv_sec);

            //convert microseconds to char*
            snprintf(microsecond_conversion_buffer,20,"%03ld",trigger_time.tv_usec);

            snprintf(time_as_string, 199 ,"Triggered at %s,%s\n",
                     second_conversion_buffer,
                     microsecond_conversion_buffer);

        }
        else // should not happen, error
        {
           //abort test
            error=" time_t size not 32 nor 64 bits";
            BlockMgr::ReportError(0,true,error);
        }

        res = write(this->GetTimerOutputFd(), time_as_string,strlen(time_as_string));
        if (res == -1)
       	{
       		error = "write error on timer log file";
			::BlockMgr::ReportError(pthread_self(),true, error);        		
		}
        printf("Timer triggered in thread %lu. value : %s\n",pthread_self(),time_as_string);

        // test for duration
        if (this->timeouts >10 )
        {
            // that is 2 seconds, should be enough to read the file, so stop the application
            printf("TIMEOUT REACHED\n");
            this->alive = false;
        }
    }
    else if (event->GetType() == NetSocket)
    {
        //printf("OnEventNetSocket, thread %lu\n",pthread_self());
        res = write(this->GetSocketOutputFd(), ((NetSocketEvent *)event)->GetData(), ((NetSocketEvent *)event)->GetSize());
    }
    else //spurious
    {
       //abort test
        error="event not of time NetSocket or Timer";
        BlockMgr::ReportError(0,true,error);

    }
return (0);
}


TimerChannel::~TimerChannel()
{

}


int main(int argc, char **argv)
{

    int32_t backward_fd[3];
    int32_t forward_fd[3];
    string error;

    printf("START\n");

    //create 2 output file, 1 that will contain timer outputs, 1 that will contain standard output from block
    backward_fd[0]= open("./timer_out_backward.txt",O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP);
    if (backward_fd[0] < 0)
    {
        //abort test
        error="cannot open output timer file";
        BlockMgr::ReportError(0,true,error);
    }


    backward_fd[1]= open("./file_out_backward.txt",O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP);

   if (backward_fd[1] < 0)
    {
        //abort test
        error="cannot open output network file";
        BlockMgr::ReportError(0,true,error);
    }


    backward_fd[2] = open("./file_in_backward.txt",O_RDONLY);

    if (backward_fd[2]  < 0)
    {
        //abort test
        error="cannot open input file";
        BlockMgr::ReportError(0,true,error);
    }


    //create 2 output file, 1 that will contain timer outputs, 1 that will contain standard output from block
    forward_fd[0]= open("./timer_out_forward.txt",O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP);
    if (forward_fd[0] < 0)
    {
        //abort test
        error="cannot open output timer file";
        BlockMgr::ReportError(0,true,error);
    }


    forward_fd[1]= open("./file_out_forward.txt",O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP);

   if (forward_fd[1] < 0)
    {
        //abort test
        error="cannot open output network file";
        BlockMgr::ReportError(0,true,error);
    }


    forward_fd[2] = open("./file_in_forward.txt",O_RDONLY);

    if (forward_fd[2]  < 0)
    {
        //abort test
        error="cannot open input file";
        BlockMgr::ReportError(0,true,error);
    }

    printf("FILES DONE\n");

    TimerChannelB channel_backward(backward_fd[2],backward_fd[0], backward_fd[1]);
    TimerChannel channel_forward(forward_fd[2],forward_fd[0], forward_fd[1]);

    printf("CHANNELS CREATED\n");

    BlockMgr::GetInstance()->CreateBlock((Channel*)&channel_backward, &channel_forward,true);

    printf("BLOCK CREATED\n");

    BlockMgr::GetInstance()->Init();

    printf("INIT OVER\n");

    BlockMgr::GetInstance()->Start();

    printf("STARTED\n");

    BlockMgr::GetInstance()->RunLoop();

  //  printf("ENDED, closing files\n");
    //wait a little bit
    sleep(3);
    //close files :
    close (backward_fd[0]);
    close (backward_fd[1]);
    close (backward_fd[2]);
    close (forward_fd[0]);
    close (forward_fd[1]);
    close (forward_fd[2]);

}
