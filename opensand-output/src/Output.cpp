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
 * OpenSAND is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file Output.cpp
 * @brief Methods of the Output static class, used by the application to
 *        interact with the output.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */

#include "Output.h"

#include <opensand_conf/uti_debug.h>


OutputInternal Output::instance;
pthread_mutex_t Output::mutex;

void Output::init(bool enabled, event_level_t min_level,
                  const char *sock_prefix)
{
	Output::acquireLock();
	instance.init(enabled, min_level, sock_prefix);
	Output::releaseLock();
}

Event *Output::registerEvent(const std::string &identifier,
                             event_level_t level)
{
	Event *evt;

	Output::acquireLock();
	evt = instance.registerEvent(identifier, level);
	Output::releaseLock();

	return evt;
}

Event *Output::registerEvent(event_level_t level,
                             const char *identifier, ...)
{
	Event *evt;
	char buf[1024];
	va_list args;
	
	va_start(args, identifier);

	vsnprintf(buf, sizeof(buf), identifier, args);

	va_end(args);

	evt = Output::registerEvent(buf, level);

	return evt;
}


bool Output::finishInit()
{
	bool ret;

	Output::acquireLock();
	ret = instance.finishInit();
	Output::releaseLock();

	return ret;
}

void Output::sendProbes()
{
	Output::acquireLock();
	instance.sendProbes();
	Output::releaseLock();
}

void Output::sendEvent(Event* event, const char* msg_format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	Output::acquireLock();
	instance.sendEvent(event, buf);
	Output::releaseLock();
}

Output::Output()
{
	if(pthread_mutex_init(&this->mutex, NULL) != 0)
	{
		UTI_ERROR("cannot initialize mutex\n");
		assert(0);
	}
}

Output::~Output()
{
	if(pthread_mutex_destroy(&this->mutex) != 0)
	{
		UTI_ERROR("cannot destroy mutex\n");
	}
}

void Output::setProbeState(uint8_t probe_id, bool enabled)
{
	Output::acquireLock();
	instance.setProbeState(probe_id, enabled);
	Output::releaseLock();
}

void Output::disable()
{
	Output::acquireLock();
	instance.disable();
	Output::releaseLock();
}

void Output::enable()
{
	Output::acquireLock();
	instance.enable();
	Output::releaseLock();
}

void Output::acquireLock()
{
	if(pthread_mutex_lock(&(mutex)) != 0)
	{
		UTI_ERROR("cannot acquire lock on output\n");
		assert(0);
	}
}

void Output::releaseLock()
{
	if(pthread_mutex_unlock(&(mutex)) != 0)
	{
		UTI_ERROR("cannot release lock on output\n");
		assert(0);
	}
}

