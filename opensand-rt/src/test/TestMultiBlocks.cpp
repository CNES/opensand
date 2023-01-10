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


static Rt::Ptr<Rt::Data> read_msg(const Rt::MessageEvent& event, std::string name, std::string from)
{
	switch(event.getType())
	{
		case Rt::EventType::Message:
		{
			auto data = event.getMessage<Rt::Data>();
			std::cout << "Block " << name << ": " << data->size()
			          << " bytes of data received from "
			          << from << " block" << std::endl;
			return data;
		}

		default:
			Rt::Rt::reportError(name, std::this_thread::get_id(), true,
			                    "unknown event: %u", event.getType());
	}

	return Rt::make_ptr<Rt::Data>();
}


/*
 * Top Block:
 *  - downward: read file (NetSocketEvent) and transmit it to lower block
 *  - upward: read message from lower block (MessageEvent) and compare to file
 */

TopBlock::TopBlock(const std::string &name, std::string file):
	Rt::Block<TopBlock, std::string>{name, file}
{
}


Rt::UpwardChannel<TopBlock>::UpwardChannel(const std::string &name, std::string):
	Channels::Upward<UpwardChannel<TopBlock>>{name}
{
}


bool Rt::UpwardChannel<TopBlock>::onEvent(const MessageEvent& event)
{
	auto data = read_msg(event, this->getName(), "lower");
	if(!data)
	{
		return false;
	}

	// send data to opposite channel to be compared with original
	if(!this->shareMessage(std::move(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "unable to transmit data to opposite channel");

		return false;
	}

	return true;
}


Rt::DownwardChannel<TopBlock>::DownwardChannel(const std::string &name, std::string file):
	Channels::Downward<DownwardChannel<TopBlock>>{name},
	input_file{file},
	input_fd{-1},
	last_written{}
{
}


Rt::DownwardChannel<TopBlock>::~DownwardChannel()
{
	if(this->input_fd >= 0)
	{
		close(this->input_fd);
	}
}


bool Rt::DownwardChannel<TopBlock>::onInit()
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


bool Rt::DownwardChannel<TopBlock>::onEvent(const Event& event)
{
	Rt::reportError(this->getName(), std::this_thread::get_id(), true,
	                "unknown event: %u", event.getType());
	return false;
}


bool Rt::DownwardChannel<TopBlock>::onEvent(const FileEvent& event)
{
	std::size_t size = event.getSize();
	if(size == 0)
	{
		// EOF stop process
		sleep(1);
		std::cout << "EOF: kill process" << std::endl;
		kill(getpid(), SIGTERM);
		return true;
	}

	auto data = make_ptr<Data>(event.getData());
	// keep data in order to compare on the opposite block
	Data buffer{*data};
	std::cout << "Block " << this->getName() << ": " << buffer.size()
	          << " bytes of data received on net socket" << std::endl;
	this->last_written.push(buffer);

	// wait in order to receive data on the opposite block and compare it
	// this also allow testing multithreading as this thread is paused
	// while other should handle the data

	// transmit to lower layer
	if(!this->enqueueMessage(std::move(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "cannot send data to lower block");
	}
	sleep(1);
	return true;
}


bool Rt::DownwardChannel<TopBlock>::onEvent(const MessageEvent& event)
{
	// received from opposite channel to compare
	auto data = event.getMessage<Data>();

	// compare data
	if(this->last_written.empty())
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "nothing to compare with data received '%s'",
		                data.get());
		return false;
	}

	Data buffer = this->last_written.front();
	this->last_written.pop();
	if(buffer != *data)
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "wrong data received '%s' instead of '%s'",
		                data->c_str(), buffer.c_str());
		return false;
	}

	std::cout << "LOOP: MATCH" << std::endl;
	return true;
}


/*
 * Middle Block:
 *  - downward/upward: read message (MessageEvent) and transmit to following block
 */

MiddleBlock::MiddleBlock(const std::string &name):
	Rt::Block<MiddleBlock>(name)
{
}


Rt::UpwardChannel<MiddleBlock>::UpwardChannel(const std::string &name):
	Channels::Upward<UpwardChannel<MiddleBlock>>{name}
{
}


bool Rt::UpwardChannel<MiddleBlock>::onEvent(const MessageEvent& event)
{
	auto data = read_msg(event, this->getName(), "lower");
	if(!data)
	{
		return false;
	}

	// transmit to upper layer
	if (!this->enqueueMessage(std::move(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "cannot send data to upper block");
		return false;
	}
	return true;
}


Rt::DownwardChannel<MiddleBlock>::DownwardChannel(const std::string &name):
	Channels::Downward<DownwardChannel<MiddleBlock>>{name}
{
}


bool Rt::DownwardChannel<MiddleBlock>::onEvent(const MessageEvent& event)
{
	auto data = read_msg(event, this->getName(), "upper");
	if(!data)
	{
		return false;
	}

	// transmit to lower layer
	if (!this->enqueueMessage(std::move(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "cannot send data to lower block");
		return false;
	}
	return true;
}


/*
 * Bottom Block:
 *  - downward: read message from upper block (MessageEvent) and write on pipe
 *  - upward: read on pipe (NetSocketEvent) and transmit it to upper block
 */

BottomBlock::BottomBlock(const std::string &name):
	Rt::Block<BottomBlock>{name}
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
	
	this->upward.setInputFd(pipefd[0]);
	this->downward.setOutputFd(pipefd[1]);

	return true;
}


Rt::UpwardChannel<BottomBlock>::UpwardChannel(const std::string &name):
	Channels::Upward<UpwardChannel<BottomBlock>>{name},
	input_fd{-1}
{
}


Rt::UpwardChannel<BottomBlock>::~UpwardChannel()
{
	if (this->input_fd >= 0)
	{
		close(this->input_fd);
	}
}


void Rt::UpwardChannel<BottomBlock>::setInputFd(int32_t fd)
{
	this->input_fd = fd;
}


bool Rt::UpwardChannel<BottomBlock>::onInit(void)
{
	if(this->input_fd < 0)
	{
		return false;
	}
	// high priority to be sure to read it before another timer
	this->addNetSocketEvent("bottom_upward", this->input_fd, 1000, 2);
	
	return true;
}


bool Rt::UpwardChannel<BottomBlock>::onEvent(const Event& event)
{
	Rt::reportError(this->getName(), std::this_thread::get_id(), true,
	                "unknown event %u", event.getType());
	return false;
}


bool Rt::UpwardChannel<BottomBlock>::onEvent(const NetSocketEvent& event)
{
	std::size_t size = event.getSize();
	auto data = make_ptr<Data>(event.getData());
	std::cout << "Block " << this->getName() << ": " << size
	          << " bytes of data received on net socket" << std::endl;

	// transmit to upper layer
	if (!this->enqueueMessage(std::move(data), 0))
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
		                "cannot send data to upper block");
		return false;
	}

	return true;
}


Rt::DownwardChannel<BottomBlock>::DownwardChannel(const std::string &name):
	Channels::Downward<DownwardChannel<BottomBlock>>{name},
	output_fd{-1}
{
}


Rt::DownwardChannel<BottomBlock>::~DownwardChannel()
{
	if(this->output_fd >= 0)
	{
		close(this->output_fd);
	}
}


void Rt::DownwardChannel<BottomBlock>::setOutputFd(int32_t fd)
{
	this->output_fd = fd;
}


bool Rt::DownwardChannel<BottomBlock>::onInit(void)
{
	if(this->output_fd < 0)
	{
		return false;
	}
	return true;
}


bool Rt::DownwardChannel<BottomBlock>::onEvent(const MessageEvent& event)
{
	auto data = read_msg(event, this->getName(), "upper");
	if(!data)
	{
		return false;
	}

	// write on pipe for opposite channel
	int res = write(this->output_fd, data->data(), data->size());
	if(res == -1)
	{
		Rt::reportError(this->getName(), std::this_thread::get_id(), true,
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
		std::string argument(*argv);

		if(argument == "-h" || argument == "--help")
		{
			/* print help */
			usage();
			return 1;
		}
		else if(argument == "-i")
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

	auto& top = Rt::Rt::createBlock<TopBlock>("top", input_file);
	auto& middle = Rt::Rt::createBlock<MiddleBlock>("middle");
	auto& bottom = Rt::Rt::createBlock<BottomBlock>("bottom");
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
