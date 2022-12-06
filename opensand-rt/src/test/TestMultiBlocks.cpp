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


static Rt::Ptr<Rt::Data> read_msg(const Rt::MessageEvent* const event, std::string name, std::string from)
{
	switch(event->getType())
	{
		case Rt::EventType::Message:
		{
			auto data = event->getMessage<Rt::Data>();
			std::cout << "Block " << name << ": " << data->size()
			          << " bytes of data received from "
			          << from << " block" << std::endl;
			return data;
		}

		default:
			Rt::Rt::reportError(name, std::this_thread::get_id(), true,
			                    "unknown event: %u", event->getType());
	}

	return Rt::make_ptr<Rt::Data>();
}


/*
 * Top Block:
 *  - downward: read file (NetSocketEvent) and transmit it to lower block
 *  - upward: read message from lower block (MessageEvent) and compare to file
 */

TopBlock::TopBlock(const std::string &name, std::string file):
	Rt::Block(name)
{
}

TopBlock::~TopBlock()
{
}

TopBlock::Upward::Upward(const std::string &name, std::string file):
	Rt::Block::Upward{name}
{
}

bool TopBlock::Upward::onEvent(const Rt::Event* const event)
{
	auto data = read_msg(static_cast<const Rt::MessageEvent*>(event), this->getName(), "lower");
	if(!data)
	{
		return false;
	}
	            
	// send data to opposite channel to be compared with original
	if(!this->shareMessage(std::move(data), 0))
	{
		Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                    "unable to transmit data to opposite channel");

		return false;
	}

	return true;
}

TopBlock::Downward::Downward(const std::string &name, std::string file):
	Rt::Block::Downward{name},
	input_file{file},
	input_fd{-1},
	last_written{}
{
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
		Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                    "cannot open input file \"%s\": %s",
		                    this->input_file.c_str(), strerror(errno));
		return false;
	}
	// high priority to be sure to read it before another timer
	this->addFileEvent("top_downward", this->input_fd, 1000);
	return true;
}

bool TopBlock::Downward::onEvent(const Rt::Event* const event)
{
	switch(event->getType())
	{
		case Rt::EventType::File:
		{
			auto file_event = static_cast<const Rt::FileEvent*>(event);
			std::size_t size = file_event->getSize();
			if(size == 0)
			{
				// EOF stop process
				sleep(1);
				std::cout << "EOF: kill process" << std::endl;
				kill(getpid(), SIGTERM);
				break;
			}
			auto data = Rt::make_ptr<Rt::Data>(file_event->getData());
			// keep data in order to compare on the opposite block
			Rt::Data buffer{*data};
			std::cout << "Block " << this->getName() << ": " << buffer.size()
			          << " bytes of data received on net socket" << std::endl;
			this->last_written.push(buffer);

			// wait in order to receive data on the opposite block and compare it
			// this also allow testing multithreading as this thread is paused
			// while other should handle the data

			// transmit to lower layer
			if(!this->enqueueMessage(std::move(data), 0))
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
				                    "cannot send data to lower block");
			}
			sleep(1);
			break;
		}
		case Rt::EventType::Message:
		{
			// received from opposite channel to compare
			auto message = static_cast<const Rt::MessageEvent*>(event);
			auto data = message->getMessage<Rt::Data>();
			
			// compare data
			if(this->last_written.empty())
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                            "nothing to compare with data received '%s'",
		                            data.get());
		        return false;
			}
			Rt::Data buffer = this->last_written.front();
			this->last_written.pop();
			if(buffer != *data)
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                            "wrong data received '%s' instead of '%s'",
		                            data->c_str(), buffer.c_str());
		        return false;
			}
			std::cout << "LOOP: MATCH" << std::endl;
			break;
		}
		default:
			Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
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

MiddleBlock::Upward::Upward(const std::string &name):
	Rt::Block::Upward{name}
{
}

bool MiddleBlock::Upward::onEvent(const Rt::Event* const event)
{
	auto data = read_msg(static_cast<const Rt::MessageEvent*>(event), this->getName(), "lower");
	if(!data)
	{
		return false;
	}

	// transmit to upper layer
	if (!this->enqueueMessage(std::move(data), 0))
	{
		Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                    "cannot send data to upper block");
		return false;
	}
	return true;
}

MiddleBlock::Downward::Downward(const std::string &name):
	Rt::Block::Downward{name}
{
}

bool MiddleBlock::Downward::onEvent(const Rt::Event* const event)
{
	auto data = read_msg(static_cast<const Rt::MessageEvent*>(event), this->getName(), "upper");
	if(!data)
	{
		return false;
	}

	// transmit to lower layer
	if (!this->enqueueMessage(std::move(data), 0))
	{
		Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                    "cannot send data to lower block");
		return false;
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
		Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                    "error when opening pipe between upward and"
		                    "downward channels");
		return false;
	}
	
	((Upward *)this->upward)->setInputFd(pipefd[0]);
	((Downward *)this->downward)->setOutputFd(pipefd[1]);

	return true;
}

BottomBlock::Upward::Upward(const std::string &name):
	Rt::Block::Upward{name},
	input_fd{-1}
{
}

BottomBlock::Upward::~Upward()
{
	if (this->input_fd >= 0)
	{
		close(this->input_fd);
	}
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
	this->addNetSocketEvent("bottom_upward", this->input_fd, 1000, 2);
	
	return true;
}

bool BottomBlock::Upward::onEvent(const Rt::Event* const event)
{
	switch(event->getType())
	{
		case Rt::EventType::NetSocket:
		{
			auto sock_event = static_cast<const Rt::NetSocketEvent*>(event);
			std::size_t size = sock_event->getSize();
			auto data = Rt::make_ptr<Rt::Data>(sock_event->getData());
			std::cout << "Block " << this->getName() << ": " << size
			          << " bytes of data received on net socket" << std::endl;

			// transmit to upper layer
			if (!this->enqueueMessage(std::move(data), 0))
			{
				Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
				                    "cannot send data to upper block");
				return false;
			}
			break;
		}
		default:
			Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
			                    "unknown event %u", event->getType());
			return false;

	}
	return true;
}

BottomBlock::Downward::Downward(const std::string &name):
	Rt::Block::Downward{name},
	output_fd{-1}
{
}

BottomBlock::Downward::~Downward()
{
	if(this->output_fd >= 0)
	{
		close(this->output_fd);
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

bool BottomBlock::Downward::onEvent(const Rt::Event* const event)
{
	auto data = read_msg(static_cast<const Rt::MessageEvent*>(event), this->getName(), "upper");
	if(!data)
	{
		return false;
	}

	// write on pipe for opposite channel
	int res = write(this->output_fd, data->data(), data->size());
	if(res == -1)
	{
		Rt::Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                    "cannot write on pipe");
		return false;
	}

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

	auto top = Rt::Rt::createBlock<TopBlock>("top", input_file);
	auto middle = Rt::Rt::createBlock<MiddleBlock>("middle");
	auto bottom = Rt::Rt::createBlock<BottomBlock>("bottom");
	Rt::Rt::connectBlocks(top, middle);
	Rt::Rt::connectBlocks(middle, bottom);

	std::cout << "Start loop, please wait..." << std::endl;
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
