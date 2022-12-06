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
 * @file BlockManager.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The manager that handles blocks
 *
 */

#include <unistd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <syslog.h>
#include <cstring>

#include <cxxabi.h>
#include <execinfo.h>

#include <opensand_output/Output.h>

#include "BlockManager.h"
#include "Rt.h"
#include "RtChannelBase.h"
#include "RtFifo.h"


namespace Rt
{


// taken from http://oroboro.com/stack-trace-on-crash/
static inline void print_stack(unsigned int max_frames = 63)
{
	// storage array for stack trace address data
	void *addrlist[max_frames+1];
	// retrieve current stack addresses
	uint32_t addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void *));

	if(addrlen == 0)
	{
		return;
	}
	syslog(LEVEL_CRITICAL, "stack trace:\n");

	// resolve addresses into strings containing "filename(function+address)",
	// Actually it will be ## program address function + offset
	// this array must be free()-ed
 
	// create readable strings to each frame.
	char **symbollist = backtrace_symbols(addrlist, addrlen);
	
	size_t funcnamesize = 1024;
	char funcname[1024];

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (unsigned int i = 4; i < addrlen; i++)
	{
		char *begin_name   = NULL;
		char *begin_offset = NULL;
		char *end_offset   = NULL;

		// find parentheses and +address offset surrounding the mangled name
		// ./module(function+0x15c) [0x8048a6d]
		for(char *p = symbollist[i]; *p; ++p)
		{
			if(*p == '(')
			{
				begin_name = p;
			}
			else if(*p == '+')
			{
				begin_offset = p;
			}
			else if(*p == ')' && (begin_offset || begin_name))
			{
				end_offset = p;
			}
		}

		if(begin_name && end_offset && (begin_name < end_offset))
		{
			*begin_name++ = '\0';
			*end_offset++ = '\0';
			if(begin_offset)
			{
				*begin_offset++ = '\0';
			}

			// mangled name is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset). now apply
			// __cxa_demangle():

			int status = 0;
			char *ret = abi::__cxa_demangle(begin_name, funcname,
			                                &funcnamesize, &status);
			char *fname = begin_name;
			if(status == 0)
			{
				fname = ret;
			}

			if(begin_offset)
			{
				syslog(LEVEL_CRITICAL, "  %-30s ( %-40s  + %-6s) %s\n",
				       symbollist[i], fname, begin_offset, end_offset );
			}
			else
			{
				syslog(LEVEL_CRITICAL, "  %-30s ( %-40s    %-6s) %s\n",
				       symbollist[i], fname, "", end_offset );
			}
		}
		else
		{
			// couldn't parse the line? print the whole line.
			syslog(LEVEL_CRITICAL, " %-40s\n", symbollist[i]);
		}
	}

	free(symbollist);
}


static void crash_handler(int sig)
{
	syslog(LEVEL_CRITICAL, "Crash with signal %d: %s\n", sig, strsignal(sig));
	signal(sig, SIG_DFL);
	print_stack();
	closelog();
	// raise signal to get a core dump
	kill(getpid(), sig);
	exit(-42);
}


BlockManager::BlockManager():
	stopped(false),
	status(true)
{
}


BlockManager::~BlockManager()
{
	for(auto &&block: block_list)
	{
		delete block;
	}
}


void BlockManager::stop(void)
{
	if(this->stopped)
	{
		LOG(this->log_rt, LEVEL_INFO,
		    "already tried to stop process\n");
		return;
	}

	// avoid calling many times stop, we may have loop else
	this->stopped = true;
	for(auto &&block: block_list)
	{
		if(block != nullptr)
		{
			block->stop();
		}
	}
}


bool BlockManager::init(void)
{
	// TODO use that in debug mode only => option in configure.ac
	// core dumps may be disallowed by parent of this process; change that
	//struct rlimit core_limits;
	//core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
	//setrlimit(RLIMIT_CORE, &core_limits);

	// Output log
	this->log_rt = Output::Get()->registerLog(LEVEL_WARNING, "Rt");

	for(auto &&block: block_list)
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "Initializing block %s.",
		    block->getName().c_str());
		if(block->isInitialized())
		{
			LOG(this->log_rt, LEVEL_NOTICE,
			    "Block %s already initialized...",
			    block->getName().c_str());
			continue;
		}

		if(!block->init())
		{
			// only return false, the block init function should call
			// report error with critical to true
			return false;
		}
		LOG(this->log_rt, LEVEL_NOTICE,
		    "Block %s initialized.",
		    block->getName().c_str());
	}

	for(auto &&block: block_list)
	{
		LOG(this->log_rt, LEVEL_DEBUG,
		    "Initializing specifics of block %s.",
		    block->getName().c_str());
		if(block->isInitialized())
		{
			LOG(this->log_rt, LEVEL_NOTICE,
			    "Block %s already initialized...",
			    block->getName().c_str());
		}
		if(!block->initSpecific())
		{
			// only return false, the block initSpecific function should call
			// report error with critical to true
			return false;
		}
		LOG(this->log_rt, LEVEL_NOTICE,
		    "Block %s initialized its specifics.",
		    block->getName().c_str());
	}

	return true;
}


void BlockManager::reportError(const std::string& msg, bool critical)
{
	if(critical)
	{
		LOG(this->log_rt, LEVEL_CRITICAL, "%s", msg.c_str());
		// stop process to signal something goes wrong
		this->status = false;
		kill(getpid(), SIGTERM);
	}
	else
	{
		LOG(this->log_rt, LEVEL_ERROR, "%s", msg.c_str());
	}
}


bool BlockManager::start(void)
{
	//start all threads
	for(auto &&block: block_list)
	{
		if(!block->isInitialized())
		{
			Rt::reportError("manager", std::this_thread::get_id(),
			                true, "block not initialized");
			return false;
		}
		if(!block->start())
		{
			Rt::reportError("manager", std::this_thread::get_id(),
			                true, "block does not start");
			return false;
		}
	}
	return true;
}


void BlockManager::wait(void)
{
	sigset_t signal_mask;
	fd_set fds;
	int fd = -1; 
	int ret;

	sigset_t blocked_signals;
	
	signal(SIGSEGV, crash_handler);
	signal(SIGABRT, crash_handler);

	//block all signals
	sigfillset(&blocked_signals);
	ret = pthread_sigmask(SIG_SETMASK, &blocked_signals, NULL);
	if(ret == -1)
	{
		Rt::reportError("manager", std::this_thread::get_id(),
		                true, "error setting signal mask");
		this->status = false;
	}

	// TODO handle SIGSTOP in threads because it breaks select
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGINT);
	sigaddset(&signal_mask, SIGQUIT);
	sigaddset(&signal_mask, SIGTERM);
	fd = signalfd(-1, &signal_mask, 0);

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select(fd + 1, &fds, NULL, NULL, NULL);
	if(ret == -1 || !FD_ISSET(fd, &fds))
	{
		Rt::reportError("manager", std::this_thread::get_id(),
		                true, "select error");
		this->status = false;
	}
	else if(ret)
	{
		struct signalfd_siginfo fdsi;
		auto rlen = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
		if(rlen != sizeof(struct signalfd_siginfo))
		{
			Rt::reportError("manager", std::this_thread::get_id(),
			                true, "cannot read signal");
			this->status = false;
		}
		LOG(this->log_rt, LEVEL_INFO,
		    "signal received: %d\n", fdsi.ssi_signo);
		this->stop();
	}
}


bool BlockManager::getStatus()
{
	return this->status;
}


void BlockManager::setupBlock(Block *block, ChannelBase *upward, ChannelBase *downward)
{
	block->upward = upward;
	block->downward = downward;

	auto up_opp_fifo = std::shared_ptr<Fifo>{new Fifo()};
	auto down_opp_fifo = std::shared_ptr<Fifo>{new Fifo()};

	upward->setOppositeFifo(up_opp_fifo, down_opp_fifo);
	downward->setOppositeFifo(down_opp_fifo, up_opp_fifo);

	this->block_list.push_back(block);
}


bool BlockManager::checkConnectedBlocks(const Block *upper, const Block *lower)
{
	if (!upper)
	{
		LOG(log_rt, LEVEL_ERROR, "Upper block to connect is null");
		return false;
	}
	if (!lower)
	{
		LOG(log_rt, LEVEL_ERROR, "Lower block to connect is null");
		return false;
	}
	return true;
}


std::shared_ptr<Fifo> BlockManager::createFifo()
{
	// Do we catch bad_alloc to return nullptr here?
	return std::shared_ptr<Fifo>{new Fifo()};
}


};
