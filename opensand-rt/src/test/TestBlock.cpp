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
 *
 * @file TestBlock.cpp
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @brief This test check that we can raise a timer on a channel then
 *        write on a socket that will be monitored by the opposite channel
 *        and transmit back data to the initial channel in order to
 *        compare it
 *
 *        The NetSocketEvent should be raised directly after timer and
 *        contain the same data
 *
 *  +------------------------------+
 *  | +----------+   +-----------+ |
 *  | |          |   |           | |
 *  | | compare<-+---+-----+     | |
 *  | |          |   |     |     | |
 *  | |          |   |     |     | |
 *  | |  timer   |   | NetSocket | |
 *  | |    |     |   |     ^     | |
 *  | |    |     |   |     |     | |
 *  | |    |     |   |     |     | |
 *  | +----+-----+   +-----+-----+ |
 *  |      +---------------+       |
 *  +------------------------------+
 *
 */


#include "TestBlock.h"

#include "Rt.h"
#include "TimerEvent.h"
#include "NetSocketEvent.h"

#include <opensand_output/Output.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <signal.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if ENABLE_TCMALLOC
#include <heap-checker.h>
#endif


TestBlock::TestBlock(const string &name):
	Block(name)
{
}

bool TestBlock::onInit()
{
	int32_t pipefd[2];

	if(pipe(pipefd) != 0)
	{
		std::cerr << "error when opening pipe between upward and "
		          << "downward channels" << std::endl;
		return false;
	}
	this->input_fd = pipefd[0];
	this->output_fd = pipefd[1];

	this->nbr_timeouts = 0;
	//timer event every 100ms
	this->upward->addTimerEvent("test_timer", 100, true);

	// high priority to be sure to read it before another timer
	this->downward->addFileEvent("downward", this->input_fd, 64, 2);
	return true;
}

bool TestBlock::onUpwardEvent(const RtEvent *const event)
{
	string error;
	timeval elapsed_time;
	int res = 0;

	switch(event->getType())
	{
		case evt_timer:
			// timer only on upward channel
			this->nbr_timeouts++;
			elapsed_time = ((TimerEvent *)event)->getTimeFromTrigger();
			sprintf(this->last_written, "%ld.%06ld",
					elapsed_time.tv_sec, elapsed_time.tv_usec);

			res = write(this->output_fd,
			            this->last_written, strlen(this->last_written));
			if(res == -1)
			{
				Rt::reportError(this->name, pthread_self(), true,
				                "cannot write on pipe");
			}
			std::cout << "Timer triggered in block: " << this->name
			          << "; value: " << this->last_written << std::endl;
			fflush(stdout);

			// test for duration
			if(this->nbr_timeouts > 10)
			{
				// that is 2 seconds, should be enough to read the file, so stop the application
				std::cout << "Stop test after 10 loops, pid = " << getpid() << std::endl;
				kill(getpid(), SIGTERM);
			}
			break;

		case evt_message:
		{
			size_t length = ((MessageEvent *)event)->getLength();
			char *data = (char *)((MessageEvent *)event)->getData();
			std::cout << "Data received from opposite channel in block: "
			          << this->name << "; data: " << data << std::endl;
			fflush(stdout);

			if(strncmp(this->last_written, (char*)data, length))
			{
				Rt::reportError(this->name, pthread_self(), true,
				                "wrong data received '%s' instead of '%s'",
				                data, this->last_written);
			}
			bzero(this->last_written, 64);
			free(data);
		}
		break;

		default:
			Rt::reportError(this->name, pthread_self(), true, "unknown event");
			return false;
	}
	return true;
}

bool TestBlock::onDownwardEvent(const RtEvent *const event)
{
	char *data;
	size_t size;
	switch(event->getType())
	{
		case evt_file:
			size = ((FileEvent *)event)->getSize();
			data = (char *)((FileEvent *)event)->getData();
			std::cout << "Data received on socket in block: " << this->name
			          << "; data: " << data << std::endl;
			fflush(stdout);

			if(!this->downward->shareMessage((void **)&data, size))
			{
				Rt::reportError(this->name, pthread_self(), true,
				                "unable to transmit data to opposite channel");
			}
			break;

		default:
			Rt::reportError(this->name, pthread_self(), true, "unknown event");
			return false;

	}
	return true;
}

TestBlock::~TestBlock()
{
	close(this->output_fd);
}


int main(int argc, char **argv)
{
	int ret = 0;
#if ENABLE_TCMALLOC
HeapLeakChecker heap_checker("test_block");
{
#endif
	Output::init(false);
	Output::enableStdlog();
	string error;

	std::cout << "Launch test" << std::endl;

	Rt::createBlock<TestBlock, TestBlock::RtUpward,
	                TestBlock::RtDownward>("test");

	std::cout << "Start loop, please wait..." << std::endl;
	Output::finishInit();
	if(!Rt::run(true))
	{
		ret = 1;
		std::cerr << "Unable to run" << std::endl;
	}
	else
	{
		std::cout << "Successfull" << std::endl;
	}

#if ENABLE_TCMALLOC
}
if(!heap_checker.NoLeaks()) assert(NULL == "heap memory leak");
#endif

	return ret;
}
