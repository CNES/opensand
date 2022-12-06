/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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
 * @author Aurelien Delrieu <adelrieu@toulouse.viveris.com>
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
#include "MessageEvent.h"
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


// ####### Block #######

TestBlock::TestBlock(const std::string &name):
	Block(name)
{
	LOG(this->log_rt, LEVEL_INFO, "Block %s created\n", name.c_str());
}


TestBlock::~TestBlock()
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
	((Downward *)this->downward)->setInputFd(pipefd[0]);
	((Upward *)this->upward)->setOutputFd(pipefd[1]);

	return true;
}


// ####### Upward #######

TestBlock::Upward::Upward(const std::string &name):
	Rt::Block::Upward{name},
	nbr_timeouts{0},
	output_fd{-1},
	last_written(64, '\0')
{
}


void TestBlock::Upward::setOutputFd(int32_t fd)
{
	this->output_fd = fd;
}


bool TestBlock::Upward::onInit(void)
{
	this->nbr_timeouts = 0;
	//timer event every 100ms
	this->addTimerEvent("test_timer", 100, true);
	return true;
}


bool TestBlock::Upward::onEvent(const Rt::Event* const event)
{
	std::string error;
	int res = 0;
	
	switch(event->getType())
	{
		case Rt::EventType::Timer:
		{
			// timer only on upward channel
			this->nbr_timeouts++;
			// test for duration
			if(this->nbr_timeouts > 10)
			{
				// that is 2 seconds, should be enough to read the file, so stop the application
				std::cout << "Stop test after 10 loops, pid = " << getpid() << std::endl;
				kill(getpid(), SIGTERM);
			}

			Rt::time_val_t elapsed_time = event->getTimeFromTrigger();
			long int elapsed_seconds = elapsed_time / 1000000,
			         elapsed_microseconds = elapsed_time % 1000000;
			sprintf(this->last_written.data(), "%ld.%06ld",
			        elapsed_seconds, elapsed_microseconds);

			res = write(this->output_fd, this->last_written.data(), this->last_written.size());
			if(res == -1)
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
				                    "cannot write on pipe");
			}
			std::cout << "Timer triggered in block: " << this->getName()
			          << "; value: " << this->last_written << std::endl;
			break;
		}

		case Rt::EventType::Message:
		{
			auto msg_event = static_cast<const Rt::MessageEvent*>(event);
			Rt::Ptr<Rt::Data> data = msg_event->getMessage<Rt::Data>();
			std::cout << "Data received from opposite channel in block: "
			          << this->getName() << "; data: " << data->c_str() << std::endl;

			std::string result(reinterpret_cast<const char*>(data->c_str()), data->size());
			if(this->last_written != result)
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
				                    "wrong data received '%s' instead of '%s'",
				                    data->c_str(), this->last_written.c_str());
			}
			this->last_written.clear();
			this->last_written.resize(this->last_written.capacity());
			break;
		}

		default:
			Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true, "unknown event");
			return false;
	}
	return true;
}


TestBlock::Upward::~Upward()
{
	if(this->output_fd >= 0)
	{
		close(this->output_fd);
	}
}


// ####### Downward #######

TestBlock::Downward::Downward(const std::string &name):
	Rt::Block::Downward{name},
	input_fd{-1}
{
}


void TestBlock::Downward::setInputFd(int32_t fd)
{
	this->input_fd = fd;
}


bool TestBlock::Downward::onInit(void)
{
	// high priority to be sure to read it before another timer
	this->addFileEvent("downward", this->input_fd, 64, 2);
	return true;
}


bool TestBlock::Downward::onEvent(const Rt::Event* const event)
{
	switch(event->getType())
	{
		case Rt::EventType::File:
		{
			auto file_event = static_cast<const Rt::FileEvent*>(event);
			auto data = Rt::make_ptr<Rt::Data>(file_event->getData());

			std::cout << "Data received on socket in block: " << this->getName()
			          << "; data: " << data->c_str() << std::endl;

			if(!this->shareMessage(std::move(data), 0))
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
				                    "unable to transmit data to opposite channel");
			}
			break;
		}
		default:
			Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true, "unknown event");
			return false;

	}
	return true;
}


TestBlock::Downward::~Downward()
{
}


// ####### Test #######

int main(int argc, char **argv)
{
	int ret = 0;
#if ENABLE_TCMALLOC
HeapLeakChecker heap_checker("test_block");
{
#endif
	std::string error;

	std::cout << "Launch test" << std::endl;

	Rt::Rt::createBlock<TestBlock>("test");
	
	std::cout << "Start loop, please wait..." << std::endl;
	Output::Get()->setLogLevel("", log_level_t::LEVEL_DEBUG);
	Output::Get()->configureTerminalOutput();
	Output::Get()->finalizeConfiguration();
	if(!Rt::Rt::run(true))
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
