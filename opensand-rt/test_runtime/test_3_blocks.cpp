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
#include <signal.h>


/****** Application Includes ******/
#include "test_3_blocks.h"



InputChannel::InputChannel(int32_t input_fd):file_input_fd(input_fd)
{
}


InputChannelB::InputChannelB(int32_t input_fd):InputChannel(input_fd)
{
}

InputChannelF::InputChannelF(int32_t input_fd):InputChannel(input_fd)
{
}

MiddleChannel::MiddleChannel()
{
}

OutputChannel::OutputChannel(int32_t output_fd):file_output_fd(output_fd)
{
}


OutputChannelF::OutputChannelF(int32_t output_fd):OutputChannel(output_fd)
{
}

OutputChannelB::OutputChannelB(int32_t output_fd):OutputChannel(output_fd)
{
}



bool InputChannel::CustomInit(void)
{
	this->AddNetSocketEvent(this->file_input_fd);
    return true;
}


bool MiddleChannel::CustomInit(void)
{
    sigset_t signal_mask;
    sigemptyset (&signal_mask);
    //SIGUSR1 used
    sigaddset(&signal_mask, SIGUSR1 );

    //add signal event
	this->AddSignalEvent(signal_mask);
    return true;
}



bool OutputChannel::CustomInit(void)
{
   return true;
}




bool InputChannel::OnEvent(Event *event)
 {
    bool resultat = true;
    MsgEvent *msg = NULL;

    if (event->GetType() == NetSocket)
    {
        if (((NetSocketEvent*)event)->GetSize() == 0)
        {
            //empty !
            //do nothing


        }
        else
        {
            printf("INPUT event thread %lu\n",pthread_self());
            //forward event
            msg =  new MsgEvent();
            msg->SetData(((NetSocketEvent*)event)->GetData(),((NetSocketEvent*)event)->GetSize());
            this->GetNextChannel()->EnqueueMessage(msg, this->GetPipeFromNext());
            this->SendEnqueuedSignal();
        }
    }
    else
    {
        resultat = false;
    }
    return resultat;
 }

bool MiddleChannel::OnEvent(Event *event)
 {
    bool resultat = true;
    MsgEvent *msg = NULL;


    if (event->GetType() == Message)
    {
        printf("MIDDLE message%lu\n",pthread_self());
        //forward event
        msg = new MsgEvent();

        msg->SetData(((MsgEvent*)event)->GetData(),((MsgEvent*)event)->GetSize());
        this->GetNextChannel()->EnqueueMessage(msg,this->GetPipeFromNext());

        printf("Send signal after enqueue\n");
        this->SendEnqueuedSignal();
    }
    else if (event->GetType() == Signal)
    {
        printf("SIGUSR1 signal received, stopping\n");
        this->Stop();
    }
    else
    {
        resultat = false;
    }
    return resultat;


 }

bool OutputChannel::OnEvent(Event *event)
 {
     bool resultat = true;
     int32_t res;
    fd_set sortie;
    if (event->GetType() == Message)
    {
        printf("OUTPUT event thread %lu\n",pthread_self());

        //wait for the file FD to be ready
        FD_ZERO(&sortie);
        FD_SET(this->GetSocketOutputFd(), &sortie);

        printf("Wait for socket out (%i) to be ready\n", this->GetSocketOutputFd());
        select(this->GetSocketOutputFd() + 1, NULL, &sortie, NULL,NULL);
        //write to file
        printf("socket out ready\n");
        res = write(this->GetSocketOutputFd(), ((MsgEvent *)event)->GetData(), ((MsgEvent *)event)->GetSize());
        printf("Write done\n");
        if (res != ((MsgEvent *)event)->GetSize())
        {
            printf("ERROR writing file, did %u instead of %u\n",res, ((MsgEvent *)event)->GetSize());
        }
    }
    else
    {
        resultat = false;
    }
    return resultat;
 }



int main(int argc, char **argv)
{

    int32_t backward_fd[2];
    int32_t forward_fd[2];
    Block *blocks[3];
    string error;

    printf("START\n");

    //create 1 output file that will contain standard output from block
    backward_fd[0]= open("./file_out_3blocks_backward.txt",O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP);
    if (backward_fd < 0)
    {
        //abort test
        error="cannot open output network file";
        BlockMgr::ReportError(0,true,error);
    }

  //create 1 output file that will contain standard output from block
    forward_fd[0]= open("./file_out_3blocks_forward.txt",O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP);
    if (forward_fd < 0)
    {
        //abort test
        error="cannot open output network file";
        BlockMgr::ReportError(0,true,error);
    }


    backward_fd[1] = open("./file_in_backward.txt",O_RDONLY);

    if (backward_fd[1]  < 0)
    {
        //abort test
        error="cannot open input file";
        BlockMgr::ReportError(0,true,error);
    }

    forward_fd[1] = open("./file_in_forward.txt",O_RDONLY);

    if (forward_fd[1]  < 0)
    {
        //abort test
        error="cannot open input file";
        BlockMgr::ReportError(0,true,error);
    }

    printf("FILES DONE\n");

    InputChannelB block3_backward(backward_fd[1]);
    InputChannelF block1_forward(forward_fd[1]);

    MiddleChannelB block2_backward;
    MiddleChannelF block2_forward;

    OutputChannelB block1_backward(backward_fd[0]);
    OutputChannelF block3_forward(forward_fd[0]);




    printf("CHANNELS CREATED\n");

    blocks[0] = BlockMgr::GetInstance()->CreateBlock((Channel *)&block1_backward, (Channel *)&block1_forward,true);
    blocks[1] = BlockMgr::GetInstance()->CreateBlock((Channel *)&block2_backward, (Channel *)&block2_forward);
    blocks[2] = BlockMgr::GetInstance()->CreateBlock((Channel *)&block3_backward, (Channel *)&block3_forward);


    printf("BLOCKS CREATED\n");

    BlockMgr::GetInstance()->SetBlockHierarchy(blocks[0], NULL, blocks[1]);
    BlockMgr::GetInstance()->SetBlockHierarchy(blocks[1], blocks[0], blocks[2]);
    BlockMgr::GetInstance()->SetBlockHierarchy(blocks[2], blocks[1], NULL);

    printf("HIERARCHY SET\n");

    BlockMgr::GetInstance()->Init();

    printf("INIT OVER\n");

    BlockMgr::GetInstance()->Start();

    printf("STARTED\n");

    BlockMgr::GetInstance()->RunLoop();


    //close files :
    close (backward_fd[0]);
    close (backward_fd[1]);
    close (forward_fd[0]);
    close (forward_fd[1]);




}
