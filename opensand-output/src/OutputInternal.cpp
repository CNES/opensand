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
 * @file OutputInternal.cpp
 * @brief Class used to hold internal output variables and methods.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Alban FRICOT <africot@toulouse.viveris.com>
 */


#include "OutputInternal.h"

#include "Output.h"
#include "CommandThread.h"

#include <vector>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <algorithm>

#define TIMEOUT 6



// TODO store a state (registered or not) for each probe and log
// and when we try to send it if not registered, then register it
// and any other pending probe/log
// 
// TODO separate log output and default log

OutputInternal::OutputInternal():
	enable_collector(false),
	initializing(true),
	enable_logs(true),
	enable_syslog(true),
	enable_stdlog(false),
	probes(),
	logs(),
	default_log(NULL),
	log(NULL),
	levels(),
	specific(),
	blocked(0),
	mutex("Output")
{
}

OutputInternal::~OutputInternal()
{
	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		delete this->probes[i];
	}

	for(size_t i = 0 ; i < this->logs.size() ; i++)
	{
		delete this->logs[i];
	}
}

OutputEvent *OutputInternal::registerEvent(const string &identifier)
{
	this->mutex.acquireLock();
	uint8_t new_id = this->logs.size();
	OutputEvent *event = new OutputEvent(new_id, identifier);

	this->logs.push_back(event);
	this->mutex.releaseLock();

	this->sendLog(this->log, LEVEL_DEBUG,
	              "Registering event %s with id %d\n",
	              identifier.c_str(), new_id);
	// single registration if process is already started
	if(this->collectorEnabled() && !this->sendRegister(event))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Failed to register new event %s\n",
		              identifier.c_str());
	}

	return event;
}

OutputLog *OutputInternal::registerLog(log_level_t display_level,
                                       const string &name)
{
	this->mutex.acquireLock();
	uint8_t new_id = this->logs.size();
	vector<OutputLog *>::iterator it;
	// if this log already exist do not create a new one
	// and keep higher level
	for(it = this->logs.begin(); it != this->logs.end(); ++it)
	{
		if((*it)->getName() == name)
		{
			this->mutex.releaseLock();
			// logs functions are protected by a mutex in log
			(*it)->setDisplayLevel(std::max(display_level,
			                                (*it)->getDisplayLevel()));
			return *it;
		}
	}
	OutputLog *log = new OutputLog(new_id, display_level, name);
	this->checkLogLevel(log);
	this->logs.push_back(log);
	this->mutex.releaseLock();

	this->sendLog(this->log, LEVEL_DEBUG,
	              "Registering log %s with id %u\n", name.c_str(), new_id);
	// single registration if process is already started
	if(this->collectorEnabled() && !this->sendRegister(log))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Failed to register new log %s\n", name.c_str());
	}

	return log;
}

uint8_t OutputInternal::getBaseProbeId(BaseProbe *probe) const
{
	return probe->id;	
}

uint8_t OutputInternal::getStorageTypeId(BaseProbe *probe) const
{
	return (uint8_t)probe->getDataType();
}

string OutputInternal::getLogName(const OutputLog *log) const
{
	return log->getName();
}

uint8_t OutputInternal::getLogId(const OutputLog *log) const
{
	return log->id;	
}

uint16_t OutputInternal::getValueCount(BaseProbe *probe) const
{
	return probe->values_count;	
}

const int *OutputInternal::getColors() const
{
	return OutputLog::colors;	
}

const char **OutputInternal::getLevels() const
{
	return OutputLog::levels;
}

void OutputInternal::sendLog(log_level_t log_level, 
                             const string &message_text)
{
	if(!this->default_log)
	{
		if(log_level > LEVEL_WARNING)
		{
			return;
		}
	}
	this->sendLog(this->default_log, log_level, message_text);
}


void OutputInternal::sendLog(const OutputLog *log,
                             log_level_t log_level, 
                             const char *msg_format, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg_format);

	vsnprintf(buf, sizeof(buf), msg_format, args);

	va_end(args);

	if(log)
	{
		this->sendLog(log, log_level, string(buf));
	}
	else
	{
		this->sendLog(log_level, string(buf));
	}
}


void OutputInternal::setProbeState(uint8_t probe_id, bool enabled)
{
	this->sendLog(this->log, LEVEL_INFO,
	              "%s probe %s\n",
	              enabled ? "Enabling" : "Disabling",
	              this->probes[probe_id]->getName().c_str());

	OutputLock lock(this->mutex); // take lock
	this->probes[probe_id]->enabled = enabled;
}

void OutputInternal::setLogLevel(uint8_t log_id, log_level_t level)
{
	this->sendLog(this->log, LEVEL_INFO, "log %s level %u\n",
	              this->logs[log_id]->getName().c_str(),
	              level);

	this->logs[log_id]->setDisplayLevel(level);
}


void OutputInternal::disableCollector(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_collector = false;
}

void OutputInternal::enableCollector(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_collector = true;
}

void OutputInternal::disableLogs(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_logs = false;
}

void OutputInternal::enableLogs(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_logs = true;
}

void OutputInternal::disableSyslog(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_syslog = false;
}

void OutputInternal::enableSyslog(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_syslog = true;
}

void OutputInternal::enableStdlog(void)
{
	OutputLock lock(this->mutex); // take lock
	this->enable_stdlog = true;
}

bool OutputInternal::collectorEnabled(void) const
{
	OutputLock lock(this->mutex);
	return this->enable_collector;
}

bool OutputInternal::logsEnabled(void) const
{
	OutputLock lock(this->mutex);
	return this->enable_logs;
}

bool OutputInternal::syslogEnabled(void) const
{
	OutputLock lock(this->mutex);
	return this->enable_syslog;
}

bool OutputInternal::stdlogEnabled(void) const
{
	OutputLock lock(this->mutex);
	return this->enable_stdlog;
}

bool OutputInternal::isInitializing(void) const
{
	OutputLock lock(this->mutex);
	return this->initializing;
}

void OutputInternal::setInitializing(bool val)
{
	OutputLock lock(this->mutex);
	this->initializing = val;
}

void OutputInternal::setLevels(const map<string, log_level_t> &levels,
                               const map<string, log_level_t> &specific)
{
	this->levels = levels;
	this->specific = specific;
	vector<OutputLog *>::iterator log_it;
	this->mutex.acquireLock();
	for(log_it = this->logs.begin(); log_it != this->logs.end(); ++log_it)
	{
		this->checkLogLevel(*log_it);
	}
	this->mutex.releaseLock();
}

void OutputInternal::checkLogLevel(OutputLog *log)
{
	map<string, log_level_t>::iterator lvl_it;
	for(lvl_it = this->levels.begin(); lvl_it != this->levels.end(); ++lvl_it)
	{
		string name = (*lvl_it).first;
		string log_name = log->getName();
		std::transform(log_name.begin(), log_name.end(),
		               log_name.begin(), ::tolower);
		// first check if this is an init log as its override others names
		if(name == "init" && (log_name.find(name) != std::string::npos))
		{
			log->setDisplayLevel((*lvl_it).second);
			break;
		}
		// then check if the name beginning of the log is in levels
		if(log_name.compare(0, name.size(), name) == 0)
		{
			log->setDisplayLevel((*lvl_it).second);
			// continue for init checking
		}
	}
	for(lvl_it = this->specific.begin(); lvl_it != this->specific.end(); ++lvl_it)
	{
		string name = (*lvl_it).first;
		string log_name = log->getName();
		std::transform(log_name.begin(), log_name.end(),
		               log_name.begin(), ::tolower);
		// check if the log name matches a part of the user defined log
		if((log_name.find(name) != std::string::npos))
		{
			log->setDisplayLevel((*lvl_it).second);
			// stop once we get a match
			break;
		}
	}
}
