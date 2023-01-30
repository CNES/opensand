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

#include <pthread.h>

#include <opensand_output/Output.h>

#include "Block.h"
#include "RtFifo.h"
#include "Rt.h"
#include "RtChannelBase.h"
#include "RtChannel.h"


namespace Rt
{


std::shared_ptr<Fifo> BlockBase::createFifo()
{
	// Do we catch bad_alloc to return nullptr here?
	Fifo *fifo = new Fifo;
	auto fifo_ptr = std::shared_ptr<Fifo>{fifo};
	return fifo_ptr;
}


BlockBase::BlockBase(const std::string &name):
	name{name},
	initialized{false}
{
	// Output logs
	this->log_rt = Output::Get()->registerLog(LEVEL_WARNING, this->name + ".rt");
	this->log_init = Output::Get()->registerLog(LEVEL_WARNING, this->name + ".init");
	LOG(this->log_rt, LEVEL_INFO, "Block %s created\n", this->name.c_str());
}


std::string BlockBase::getName() const
{
	return this->name;
}


bool BlockBase::onInit() { return true; }


bool BlockBase::isInitialized() const
{
	return this->initialized;
}


void BlockBase::setInitialized()
{
	this->initialized = true;
}


bool BlockBase::start()
{
	//create upward thread
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: start upward channel\n", this->name.c_str());
	try
	{
		this->up_thread = this->initUpwardThread();
	}
	catch (const std::system_error& e)
	{
		Rt::Rt::reportError(this->name, std::this_thread::get_id(), true,
		                    "cannot start upward thread [%u: %s]", e.code(), e.what());
		return false;
	}
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: upward channel thread id %lu\n",
	    this->name.c_str(), this->up_thread.get_id());

	//create downward thread
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: start downward channel\n", this->name.c_str());
	try
	{
		this->down_thread = this->initDownwardThread();
	}
	catch (const std::system_error& e)
	{
		Rt::Rt::reportError(this->name, std::this_thread::get_id(), true,
		                    "cannot downward start thread [%u: %s]", e.code(), e.what());
		// TODO: avoid cancel here and find a way to let
		// the other thread terminate gracefully
		pthread_cancel(this->up_thread.native_handle());
		this->up_thread.join();
		return false;
	}
	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: downward channel thread id: %lu\n",
	    this->name.c_str(), this->down_thread.get_id());

	return true;
}

bool BlockBase::stop()
{
	bool status = true;

	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: stop channels\n", this->name.c_str());

	LOG(this->log_rt, LEVEL_INFO,
	    "Block %s: join channels\n", this->name.c_str());
	try
	{
		this->up_thread.join();
	}
	catch (const std::system_error& e)
	{
		Rt::Rt::reportError(this->name, std::this_thread::get_id(), false,
		                    "cannot join upward thread [%u: %s]", e.code(), e.what());
		status = false;
	}

	try
	{
		this->down_thread.join();
	}
	catch (const std::system_error& e)
	{
		Rt::Rt::reportError(this->name, std::this_thread::get_id(), false,
		                    "cannot join downward thread [%u: %s]", e.code(), e.what());
		status = false;
	}

	return status;
}


void BlockBase::reportError(const std::string& message) const
{
	Rt::Rt::reportError(this->name, std::this_thread::get_id(), true, message.c_str());
}


void BlockBase::reportSuccess(const std::string& message) const
{
	LOG(this->log_init, LEVEL_NOTICE, "%s", message.c_str());
}


};
