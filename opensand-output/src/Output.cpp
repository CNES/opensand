/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
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
 * @author Fabrice Hobaya <fhobaya@toulouse.viveris.com>
 */

#include "Output.h"

OutputOpensand Output::opensand_instance;

OutputInternal *Output::instance = NULL;

bool Output::init(bool enabled, const char *sock_prefix)
{
	if (instance)
	{
		return false;
	}
	
	instance = &(Output::opensand_instance);
	return instance->init(enabled, sock_prefix);
}

bool Output::initExt(bool enabled, const char *path)
{
	if (instance)
	{
		return false;
	}
	
	handle = dlopen(path, RTLD_LAZY);	
	if(!handle)
	{
		fputs (dlerror(), stderr);
		exit(1);
	}

	isntance = handle->create();

	return true;
}

bool Output::close()
{ 
	if((!instance) || (instance == &(Output::opensand_instance)))
	{
		return false;
	}
	
	handle->destroy();
	dlclose(handle);
}


OutputEvent *Output::registerEvent(const string &identifier)
{
	return instance->registerEvent(identifier);
}

OutputLog *Output::registerLog(log_level_t display_level,
                               const string &name)
{
	return instance->registerLog(display_level, name);
}

OutputEvent *Output::registerEvent(const char *identifier, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, identifier);

	vsnprintf(buf, sizeof(buf), identifier, args);

	va_end(args);

	return Output::registerEvent(string(buf));
}

OutputLog *Output::registerLog(log_level_t default_display_level,
                               const char* name, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, name);

	vsnprintf(buf, sizeof(buf), name, args);

	va_end(args);

	return Output::registerLog(default_display_level, string(buf));
}

bool Output::finishInit(void)
{
	return instance->finishInit();
}

void Output::sendProbes(void)
{
	instance->sendProbes();
}

void Output::sendEvent(OutputEvent* event,
                       const char* msg_format, ...)
{
	char buf[1024];
	va_list args;
	assert(event != NULL);
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	instance->sendLog(event, LEVEL_EVENT, string(buf));
}


void Output::sendLog(const OutputLog *log,
                     log_level_t log_level,
                     const char *msg_format, ...)
{
	char buf[1024];
	va_list args;
	assert(log != NULL);
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	instance->sendLog(log, log_level, buf);
}

void Output::sendLog(log_level_t log_level,
                     const char *msg_format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	instance->sendLog(log_level, string(buf));
}

Output::Output()
{
}

Output::~Output()
{
}

void Output::setProbeState(uint8_t probe_id, bool enabled)
{
	instance->setProbeState(probe_id, enabled);
}

void Output::setLogLevel(uint8_t log_id, log_level_t level)
{
	instance->setLogLevel(log_id, level);
}

void Output::disableCollector(void)
{
	instance->disableCollector();
}

void Output::enableCollector(void)
{
	instance->enableCollector();
}

void Output::disableLogs(void)
{
	instance->disableLogs();
}

void Output::enableLogs(void)
{
	instance->enableLogs();
}

void Output::disableSyslog(void)
{
	instance->disableSyslog();
}

void Output::enableSyslog(void)
{
	instance->enableSyslog();
}

void Output::enableStdlog(void)
{
	instance->enableStdlog();
}

void Output::setLevels(const map<string, log_level_t> &levels,
                       const map<string, log_level_t> &specific)
{
	instance->setLevels(levels, specific);
}
