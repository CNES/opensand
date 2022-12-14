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
 * @file TestMultiBlocks.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Aurelien Delrieu <adelrieu@toulouse.viveris.com>
 * @brief This test check that we can read a file on a channel then
 *        transmit content to lower block, the bottom block transmit it
 *        to the following channel that will forward it to the top and
 *        compare output
 *
 *
 *        file
 *          |
 *  +-------+-----------------------+
 *  | +-----+-----+   +-----------+ |
 *  | |     |     |   |  compare  | |
 *  | |     |     |Top|     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----|-----+   +-----+-----+ |
 *  +-------|---------------+-------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----+-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     |    Middle     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  +-------|-----------------------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----|-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     |    Bottom     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  |       +---------------+       |
 *  +-------------------------------+
 */

#include "TestMultiBlocks.h"

#include "Rt.h"
#include "MessageEvent.h"

#include <opensand_output/Output.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if ENABLE_TCMALLOC
#include <heap-checker.h>
#endif

/**
 * @brief Print usage of the test application
 */
static void usage(void)
{
	std::cerr << "Test multi blocks: test the opensand rt library" << std::endl
	          << "usage: test_multi_blocks -i input_file" << std::endl;
}


static char *read_msg(const MessageEvent *const event, std::string name, std::string from)
{
	switch(event->getType())
	{
		case EventType::Message:
		{
			char *data = static_cast<char *>(event->getData());
			data[event->getLength()] = '\0';
			std::cout << "Block " << name << ": " << strlen(data)
			          << " bytes of data received from "
			          << from << " block" << std::endl;
			return data;
		}

		default:
			Rt::reportError(name, std::this_thread::get_id(), true,
			                "unknown event: %u", event->getType());
	}

	return nullptr;
}


/*
 * Top Block:
 *  - downward: read file (NetSocketEvent) and transmit it to lower block
 *  - upward: read message from lower block (MessageEvent) and compare to file
 */

TopBlock::TopBlock(const std::string &name, std::string file):
	Block(name)
{
}

TopBlock::~TopBlock()
{
}

bool TopBlock::Upward::onEvent(const RtEvent *const event)
{
	char *data = read_msg(static_cast<const MessageEvent *>(event), this->getName(), "lower");
	if(!data)
	{
		return false;
	}
	            
	// send data to opposite channel to be compared with original
	if(!this->shareMessage((void **)&data, strlen(data)))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "unable to transmit data to opposite channel");

    delete [] data;
		return false;
	}

	return true;
}

TopBlock::Downward::~Downward()
{
	if(this->input_fd >= 0)
	{
		close(this->input_fd);
	}
}

bool TopBlock::Downward::onInit()
{
	this->input_fd = open(this->input_file.c_str(), O_RDONLY);
	if(this->input_fd < 0)
	{
		//abort test
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "cannot open input file \"%s\": %s",
		                this->input_file.c_str(), strerror(errno));
		return false;
	}
	// high priority to be sure to read it before another timer
	this->addFileEvent("top_downward", this->input_fd, 1000);
	return true;
}

bool TopBlock::Downward::onEvent(const RtEvent *const event)
{
	size_t size;
	char *data;
	std::string buffer;
	switch(event->getType())
	{
    case EventType::File:
			size = ((NetSocketEvent *)event)->getSize();
			if(size == 0)
			{
				// EOF stop process
				sleep(1);
				std::cout << "EOF: kill process" << std::endl;
				kill(getpid(), SIGTERM);
				break;
			}
			data =  (char *)((NetSocketEvent *)event)->getData();
			std::cout << "Block " << this->getName() << ": " << strlen(data)
			          << " bytes of data received on net socket" << std::endl;
			fflush(stdout);
			// keep data in order to compare on the opposite block
			buffer.assign(data);
			this->last_written.push(buffer);
			//strncpy(this->last_written, data, size);
			//this->last_written[size] = '\0';

			// wait in order to receive data on the opposite block and compare it
			// this also allow testing multithreading as this thread is paused
			// while other should handle the data


			// transmit to lower layer
			if(!this->enqueueMessage((void **)&data, strlen(data), 0))
			{
				Rt::reportError(this->getName(), std::this_thread::get_id(), true,
				                "cannot send data to lower block");
			}
			sleep(1);
			break;
			
    case EventType::Message:
			// received from opposite channel to compare
			data = (char *)((MessageEvent *)event)->getData();
			size = ((MessageEvent *)event)->getLength();
			data[size] = '\0';
			
			// compare data
			if(this->last_written.empty())
			{
				Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                        "nothing to compare with data received '%s'",
		                        data);
		        free(data);
		        return false;
			}
			buffer = this->last_written.front();
			this->last_written.pop();
			if(strncmp(data, buffer.c_str(), size))
			{
				Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                        "wrong data received '%s' instead of '%s'",
		                        data, buffer.c_str());
		        free(data);
		        return false;
			}
			std::cout << "LOOP: MATCH" << std::endl;
			free(data);
			break;

		default:
			Rt::reportError(this->getName(), std::this_thread::get_id(), true,
			                "unknown event: %u", event->getType());
			return false;

	}
	return true;
}

/*
 * Middle Block:
 *  - downward/upward: read message (MessageEvent) and transmit to following block
 */

MiddleBlock::MiddleBlock(const std::string &name):
	Block(name)
{
}

bool MiddleBlock::Upward::onEvent(const RtEvent *const event)
{
	char *data = read_msg(static_cast<const MessageEvent *>(event), this->getName(), "lower");
	if(!data)
	{
		return false;
	}

	// transmit to upper layer
	if (!this->enqueueMessage((void **)&data, strlen(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true, "cannot send data to upper block");
	}
	return true;
}

bool MiddleBlock::Downward::onEvent(const RtEvent *const event)
{
	char *data = read_msg(static_cast<const MessageEvent *>(event), this->getName(), "upper");
	if(!data)
	{
		return false;
	}

	// transmit to lower layer
	if (!this->enqueueMessage((void **)(&data), strlen(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true, "cannot send data to lower block");
	}
	return true;
}

MiddleBlock::~MiddleBlock()
{
}

/*
 * Bottom Block:
 *  - downward: read message from upper block (MessageEvent) and write on pipe
 *  - upward: read on pipe (NetSocketEvent) and transmit it to upper block
 */

BottomBlock::BottomBlock(const std::string &name):
	Block(name)
{
}

BottomBlock::~BottomBlock()
{
}

bool BottomBlock::onInit()
{
	int32_t pipefd[2];

	if(pipe(pipefd) != 0)
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "error when opening pipe between upward and"
		                "downward channels");
		return false;
	}
	
	((Upward *)this->upward)->setInputFd(pipefd[0]);
	((Downward *)this->downward)->setOutputFd(pipefd[1]);

	return true;
}

BottomBlock::Upward::~Upward()
{
}

void BottomBlock::Upward::setInputFd(int32_t fd)
{
	this->input_fd = fd;
}

bool BottomBlock::Upward::onInit(void)
{
	if(this->input_fd < 0)
	{
		return false;
	}
	// high priority to be sure to read it before another timer
	this->addFileEvent("bottom_upward", this->input_fd, 1000, 2);
	
	return true;
}

bool BottomBlock::Upward::onEvent(const RtEvent *const event)
{
	char *data;
	size_t size;
	switch(event->getType())
	{
    case EventType::File:
			size = ((NetSocketEvent *)event)->getSize();
			data = (char *)((NetSocketEvent *)event)->getData();
			std::cout << "Block " << this->getName() << ": " << size
			          << " bytes of data received on net socket" << std::endl;

			// transmit to upper layer
			if (!this->enqueueMessage((void **)&data, strlen(data), 0))
			{
				Rt::reportError(this->getName(), std::this_thread::get_id(), true, "cannot send data to upper block");
			}
			break;

		default:
			Rt::reportError(this->getName(), std::this_thread::get_id(), true,
			                "unknown event %u", event->getType());
			return false;

	}
	return true;
}

BottomBlock::Downward::~Downward()
{
	if(this->output_fd >= 0)
	{
		close(this->output_fd);
		// input fd closed in NetSocketEvent
	}
}

void BottomBlock::Downward::setOutputFd(int32_t fd)
{
	this->output_fd = fd;
}

bool BottomBlock::Downward::onInit(void)
{
	if(this->output_fd < 0)
	{
		return false;
	}
	return true;
}

bool BottomBlock::Downward::onEvent(const RtEvent *const event)
{
	int res = 0;
	char *data = read_msg(static_cast<const MessageEvent *>(event), this->getName(), "upper");
	if(!data)
	{
		return false;
	}

	// write on pipe for opposite channel
	res = write(this->output_fd,
	            data, strlen(data));
	if(res == -1)
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "cannot write on pipe");
    delete [] data;
		return false;
	}

  delete [] data;
	return true;
}

int main(int argc, char **argv)
{
	int ret = 0;
#if ENABLE_TCMALLOC
HeapLeakChecker heap_checker("test_multi_blocks");
{
#endif
	std::string error;
	std::string input_file;
	int args_used;

	/* parse program arguments, print the help message in case of failure */
	if(argc <= 1 || argc > 3)
	{
		usage();
		return 1;
	}

	for(argc--, argv++; argc > 0; argc -= args_used, argv += args_used)
	{
		args_used = 1;

		if(!strcmp(*argv, "-h") || !strcmp(*argv, "--help"))
		{
			/* print help */
			usage();
			return 1;
		}
		else if(!strcmp(*argv, "-i"))
		{
			/* get the name of the file where the configuration is stored */
			input_file = argv[1];
			args_used++;
		}
		else
		{
			usage();
			return 1;
		}
	}

	std::cout << "Launch test" << std::endl;

	auto top = Rt::createBlock<TopBlock>("top", input_file);
	auto middle = Rt::createBlock<MiddleBlock>("middle");
	auto bottom = Rt::createBlock<BottomBlock>("bottom");
	Rt::connectBlocks(top, middle);
	Rt::connectBlocks(middle, bottom);

	std::cout << "Start loop, please wait..." << std::endl;
	Output::Get()->finalizeConfiguration();
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
