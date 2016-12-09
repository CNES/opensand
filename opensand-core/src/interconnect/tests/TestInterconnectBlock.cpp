/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file TestInterconnectBlock.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.fr>
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
 *  | |     | Interconnect  |     | |
 *  | |     |   Downward    |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  +-------|-----------------------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----+-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     | Interconnect  |     | |
 *  | |     |    Upward     |     | |
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

#include "TestInterconnectBlock.h"

#include "opensand_rt/Rt.h"

#include <opensand_output/Output.h>
#include <TestBlockInterconnectUpward.h>
#include <TestBlockInterconnectDownward.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <Config.h>

#if ENABLE_TCMALLOC
#include <heap-checker.h>
#endif

using std::ostringstream;

static char *read_msg(const MessageEvent *const event, string name, string from, size_t &length)
{
    char *data;
    switch(event->getType())
    {
        case evt_message:
            length = event->getLength();
            data = (char *)calloc(length, sizeof(char));
            memcpy(data, event->getData(), length);
        
            if (DEBUG)
            {
                std::cout << "Block " << name << ": " << length
                          << " bytes of data received from "
                          << from << " block" << std::endl;
                fflush(stdout);
            }
            free(event->getData());
            break;

        default:
            Rt::reportError(name, pthread_self(), true,
                            "unknown event: %u", event->getType());
            return NULL;

    }
    return data;
}

/*
 * Compare contents of file a and file_b.
 * @param file_a : fd of open first file
 * @param file_b : fd of open second file
 * @return size of file if contents are identical, else 0.
 */

long int compare_files(int32_t file_a, int32_t file_b)
{
    char buf_a[1024], buf_b[1024];
    ssize_t read_a, read_b;
    long int filesize = 0;

    if (file_a < 0 || file_b < 0)
        return 0;
    // set file offset to zero
    lseek(file_a, 0, SEEK_SET);
    lseek(file_b, 0, SEEK_SET);

    while( (read_a = read(file_a, buf_a, 32)) > 0)
    {
        read_b = read(file_b, buf_b, 32);
        if(read_a != read_b)
            return 0;
        if(memcmp(buf_a, buf_b, read_a) != 0)
            return 0;
        filesize += read_a;
    }
    return filesize;
}

/*
 * Top Block:
 *  - downward: read file (NetSocketEvent) and transmit it to lower block
 *  - upward: read message from lower block (MessageEvent) and compare to file
 */

TopBlock::TopBlock(const string &name, struct top_specific spec):
	Block(name),
    must_wait(spec.must_wait),
	input_file(spec.input_file),
    output_file(spec.output_file),
    start_t(0),
    end_t(0)
{
}

bool TopBlock::onInit()
{
    // Open input file
	this->input_fd = open(this->input_file.c_str(), O_RDONLY);
	if(this->input_fd < 0)
	{
		//abort test
		Rt::reportError(this->name, pthread_self(), true,
		                "cannot open input file %s: %s",
		                input_file.c_str(), strerror(errno));
		return false;
	}
    // Open output file
    this->output_fd = open(this->output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                           S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(this->output_fd < 0)
    {
        // abort test
        Rt::reportError(this->name, pthread_self(), true,
                        "cannot open output file %s: %s",
                        output_file.c_str(), strerror(errno));
        return false;
    }
	// high priority to be sure to read it before another timer
    if (!this->must_wait)
	    this->downward->addFileEvent("top_downward", this->input_fd, 1000);
	return true;
}

void TopBlock::startReading()
{
    this->downward->addFileEvent("top_downward", this->input_fd, 1000);
}

bool TopBlock::onDownwardEvent(const RtEvent *const event)
{
    return ((Downward *)this->downward)->onEvent(event);
}

bool TopBlock::Downward::onEvent(const RtEvent *const event)
{
	char *data;
	size_t size;
    long int filesize;
    double total_t;
    double speed;
	switch(event->getType())
	{
		case evt_file:
            if(!((TopBlock *)this->block)->getStartT())
                ((TopBlock *)this->block)->setStartT(clock());
			size = ((NetSocketEvent *)event)->getSize();
			if(size == 0)
			{
                // EOF, compare input and output file
                // wait for other end to finish storing
                // (should use proper mutex)
                sleep(5);
                ((TopBlock *)this->block)->saveOutput();           

                if ( (filesize = compare_files(((TopBlock *)this->block)->getInputFd(), 
                                               ((TopBlock *)this->block)->getOutputFd())) )
                {
                    total_t = (((TopBlock *)this->block)->getEndT() -
                              ((TopBlock *)this->block)->getStartT()) / (double) CLOCKS_PER_SEC;
                    speed = filesize * 8 / total_t;
                    speed /= 1024;
                    speed /= 1024;
                    std::cout << "FILES MATCH!" << std::endl;
                    std::cout << "Total bytes transmitted: " << filesize << std::endl;
                    std::cout << "Total time in sec: " << total_t << std::endl;
                    std::cout << "Speed in Mbps: " << speed << std::endl;
                    std::cerr << filesize << "," << total_t << "," << speed << std::endl;
                }
                else
                {
                    std::cout << "ERROR: FILES DON'T MATCH" << std::endl;
                }
				// stop process
				sleep(1);
				//std::cout << "EOF: kill process" << std::endl;
				kill(getpid(), SIGTERM);
				break;
			}
			data =  (char *)((NetSocketEvent *)event)->getData();
            if (DEBUG)
			    std::cout << "Block " << ((TopBlock *)this->block)->getName() << ": " << size
			              << " bytes of data read from file" << std::endl;
			fflush(stdout);

			// transmit to lower layer
			if(!this->enqueueMessage((void **)&data, size))
			{
				Rt::reportError(((TopBlock *)this->block)->getName(), pthread_self(), true,
				                "cannot send data to lower block");
			}
			break;

		default:
			Rt::reportError(((TopBlock *)this->block)->getName(), pthread_self(), true,
			                "unknown event: %u", event->getType());
			return false;

	}
	return true;
}

bool TopBlock::onUpwardEvent(const RtEvent *const event)
{
    return ((Upward *)this->upward)->onEvent(event);
}

bool TopBlock::Upward::onEvent(const RtEvent *const event)
{
    size_t length;
    ssize_t bytes_written;

	char *data = read_msg((MessageEvent *)event,
                          ((TopBlock *)this->block)->getName(),
                          "lower", length);
	if(!data)
	{
		return false;
	}
    ((TopBlock *)this->block)->setEndT(clock());
    // write data to output file
    bytes_written = ((TopBlock *)this->block)->writeOutput((const void *) data, length);
    if ((bytes_written > 0) && ((size_t) bytes_written < length))
    {
        Rt::reportError(((TopBlock *)this->block)->getName(), pthread_self(), true,
                        "could not store data in output_file");
        return false;
    }
	free(data);
	return true;
}

TopBlock::~TopBlock()
{
    if(this->input_fd)
	    close(this->input_fd);
    if(this->output_fd)
        close(this->output_fd);
}

/*
 * Write bytes to output file
 */
ssize_t TopBlock::writeOutput(const void * buf, size_t length)
{
    return write(this->output_fd, buf, length);
}

/*
 * Close output file (save), and reopen as readonly
 */
void TopBlock::saveOutput()
{
    if(this->output_fd > 0)
    {
        close(this->output_fd);
        this->output_fd = open(this->output_file.c_str(), O_RDONLY);
    }
}


/*
 * Bottom Block:
 *  - downward: read message from upper block (MessageEvent) and write on pipe
 *  - upward: read on pipe (NetSocketEvent) and transmit it to upper block
 */

BottomBlock::BottomBlock(const string &name):
	Block(name)
{
}

bool BottomBlock::onInit()
{
	int32_t pipefd[2];

	if(pipe(pipefd) != 0)
	{
		Rt::reportError(this->name, pthread_self(), true,
		                "error when opening pipe between upward and"
		                "downward channels");
		return false;
	}
	this->input_fd = pipefd[0];
	this->output_fd = pipefd[1];

	// high priority to be sure to read it before another timer
	this->upward->addFileEvent("bottom_upward", this->input_fd, 1000, 2);
	return true;
}

bool BottomBlock::onDownwardEvent(const RtEvent *const event)
{
    return ((Downward *)this->downward)->onEvent(event);
}

bool BottomBlock::Downward::onEvent(const RtEvent *const event)
{
    size_t length;
	int res = 0;
	char *data = read_msg((MessageEvent *)event,
                          ((BottomBlock *)this->block)->getName(),
                          "upper", length);
	if(!data)
	{
		return false;
	}

	// write on pipe for opposite channel
	res = write(((BottomBlock *)this->block)->output_fd,
	            data, length);
	if(res == -1)
	{
		Rt::reportError(((BottomBlock *)this->block)->getName(), pthread_self(), true,
		                "cannot write on pipe");
		free(data);
		return false;
	}
	free(data);
	return true;
}

bool BottomBlock::onUpwardEvent(const RtEvent *const event)
{
    return ((Upward *)this->upward)->onEvent(event);
}

bool BottomBlock::Upward::onEvent(const RtEvent *const event)
{
	char *data;
	size_t size;
	switch(event->getType())
	{
		case evt_file:
			size = ((NetSocketEvent *)event)->getSize();
			data = (char *)((NetSocketEvent *)event)->getData();
			if (DEBUG)
                std::cout << "Block " << ((BottomBlock *)this->block)->getName() << ": " << size
			              << " bytes of data received on net socket" << std::endl;
			fflush(stdout);

			// transmit to upper layer
			if(!this->enqueueMessage((void **)&data, size))
			{
				Rt::reportError(((BottomBlock *)this->block)->getName(), pthread_self(), true, "cannot send data to upper block");
			}
			break;

		default:
			Rt::reportError(((BottomBlock *)this->block)->getName(), pthread_self(), true,
			                "unknown event %u", event->getType());
			return false;

	}
	return true;
}

BottomBlock::~BottomBlock()
{
	close(this->output_fd);
	// input fd closed in NetSocketEvent
}

