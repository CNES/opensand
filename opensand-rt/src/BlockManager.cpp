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

/**
 * @file BlockManager.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @brief  The manager that handles blocks
 *
 */

#include "BlockManager.h"
#include "RtFifo.h"
#include "Rt.h"

#include <opensand_conf/uti_debug.h>

#include <signal.h>
#include <cstdio>
#include <cstring>
#include <sys/signalfd.h>

Event *BlockManager::critical_evt = NULL;

BlockManager::BlockManager():
	stopped(false),
	status(true)
{
	BlockManager::critical_evt = Output::registerEvent("critical", LEVEL_ERROR);
}


BlockManager::~BlockManager()
{
	for(list<Block *>::const_iterator iter = this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		delete (*iter);
	}
}

void BlockManager::stop(int signal)
{
	if(this->stopped)
	{
		UTI_DEBUG("already tried to stop process\n");
		return;
	}
	for(list<Block *>::iterator iter = this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		if(*iter != NULL)
		{
			if(!(*iter)->stop(signal))
			{
				(*iter)->stop(SIGKILL);
			}
		}
	}
	// avoid calling many times stop, we may have loop else
	this->stopped = true;
}

bool BlockManager::init(void)
{
	for(list<Block*>::iterator iter = this->block_list.begin();
	    iter != this->block_list.end(); iter++)
	{
		if(!(*iter)->init())
		{
			// only return false, the block init function should call
			// report error with critical to true
			return false;
		}
	}
	return true;
}


void BlockManager::reportError(const char *msg, bool critical)
{
	UTI_ERROR("%s", msg);

	if(critical == true)
	{
		Output::sendEvent(BlockManager::critical_evt, "CRITICAL: %s", msg);
		std::cerr << "FATAL: stop process" << std::endl;
		// stop process to signal something goes wrong
		this->status = false;
		this->stop(SIGTERM);
	}
}

bool BlockManager::start(void)
{
	//start all threads
	for(list<Block *>::iterator iter = this->block_list.begin();
	    iter != this->block_list.end(); ++iter)
	{
		if(!(*iter)->isInitialized())
		{
			Rt::reportError("manager", pthread_self(),
			                true, "block not initialized");
			return false;
		}
		if(!(*iter)->start())
		{
			Rt::reportError("manager", pthread_self(),
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

	//block all signals
	sigfillset(&blocked_signals);
	ret = pthread_sigmask(SIG_SETMASK, &blocked_signals, NULL);
	if(ret == -1)
	{
		Rt::reportError("manager", pthread_self(),
		                true, "error setting signal mask");
		this->status = false;
	}

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
		Rt::reportError("manager", pthread_self(),
		                true, "select error");
		this->status = false;
	}
	else if(ret)
	{
		struct signalfd_siginfo fdsi;
		int rlen;
		rlen = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
		if(rlen != sizeof(struct signalfd_siginfo))
		{
			Rt::reportError("manager", pthread_self(),
			                true, "cannot read signal");
			this->status = false;
		}
		UTI_DEBUG("signal received: %d\n", fdsi.ssi_signo);
		this->stop(fdsi.ssi_signo);
	}
}

bool BlockManager::getStatus()
{
	return this->status;
}
