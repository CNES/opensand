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
 * @file Block.cpp
 * @author Cyrille GAILLARDET / <cgaillardet@toulouse.viveris.com>
 * @author Julien BERNARD / <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @brief  The block description
 *
 */

#include <csignal>
#include <stdlib.h>
#include <list>
#include <vector>
#include <sys/select.h>
#include <pthread.h>

#include "Block.h"
#include "Rt.h"
#include "RtChannelBase.h"
#include "RtChannel.h"
#include "RtChannelMux.h"
#include "RtChannelDemux.h"
#include "RtChannelMuxDemux.h"
#include "Types.h"

#include <opensand_output/Output.h>
#include <opensand_output/OutputLog.h>
#include <opensand_output/OutputEvent.h>


Block::Block(const std::string &name):
	name(name),
	initialized(false)
{
	// Output logs
	this->log_rt = Output::Get()->registerLog(LEVEL_WARNING, "%s.rt", this->name.c_str());
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, "%s.init", this->name.c_str());
	LOG(this->log_rt, LEVEL_INFO, "Block %s created\n", this->name.c_str());
}


Block::~Block()
{
  delete this->downward;
  this->downward = nullptr;

  delete this->upward;
  this->upward = nullptr;
}


bool Block::init(void)
{
	// initialize channels
	if(!this->upward->init())
	{
		return false;
	}
	if(!this->downward->init())
	{
		return false;
	}

	return true;
}


bool Block::initSpecific(void)
{
	// specific block initialization
	if(!this->onInit())
	{
		Rt::reportError(this->name, std::this_thread::get_id(),
		                true, "Block onInit failed");
		return false;
	}

	// initialize channels
	if(!this->upward->onInit())
	{
		Rt::reportError(this->name, std::this_thread::get_id(),
		                true, "Upward onInit failed");
		return false;
	}
	if(!this->downward->onInit())
	{
		Rt::reportError(this->name, std::this_thread::get_id(),
		                true, "Downward onInit failed");
		return false;
	}
	this->initialized = true;
	this->upward->setIsBlockInitialized(true);
	this->downward->setIsBlockInitialized(true);
	LOG(this->log_init, LEVEL_NOTICE,
	    "Block initialization complete\n");

	return true;
}


bool Block::onInit() { return true; }


bool Block::isInitialized(void)
{
	return this->initialized;
}


bool Block::start(void)
{
	//create upward thread
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: start upward channel\n", this->name.c_str());
  try {
	  this->up_thread = std::thread{&RtUpward::executeThread, this->upward};
  } catch (const std::system_error& e) {
		Rt::reportError(this->name, std::this_thread::get_id(), true,
		                "cannot start upward thread [%u: %s]", e.code(), e.what());
    return false;
  }
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: upward channel thread id %lu\n",
	    this->name.c_str(), this->up_thread.get_id());

	//create downward thread
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: start downward channel\n", this->name.c_str());
  try {
    this->down_thread = std::thread{&RtDownward::executeThread, this->downward};
  } catch (const std::system_error& e) {
		Rt::reportError(this->name, std::this_thread::get_id(), true,
		                "cannot downward start thread [%u: %s]", e.code(), e.what());
    pthread_cancel(this->up_thread.native_handle());
    this->up_thread.join();
		return false;
  }
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: downward channel thread id: %lu\n",
	    this->name.c_str(), this->down_thread.get_id());

	return true;
}

bool Block::stop()
{
	bool status = true;

	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: stop channels\n", this->name.c_str());
	// the process may be already killed as the may have caught the stop signal first
	// So, do not report an error
	pthread_cancel(this->up_thread.native_handle());
	pthread_cancel(this->down_thread.native_handle());

	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: join channels\n", this->name.c_str());
	try {
		this->up_thread.join();
	} catch (const std::system_error& e) {
		Rt::reportError(this->name, std::this_thread::get_id(), false,
		                "cannot join upward thread [%u: %s]", e.code(), e.what());
		status = false;
	}

	try {
		this->down_thread.join();
	} catch (const std::system_error& e) {
		Rt::reportError(this->name, std::this_thread::get_id(), false,
		                "cannot join downward thread [%u: %s]", e.code(), e.what());
		status = false;
	}

	return status;
}


RtChannelBase *Block::getUpwardChannel(void) const
{
	return this->upward;
}


RtChannelBase *Block::getDownwardChannel(void) const
{
	return this->downward;
}
