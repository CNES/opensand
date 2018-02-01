/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2018 TAS
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
 * @file OutputOpenbach.cpp
 * @brief Class used to hold openbach output variables and methods.
 * @author Alban Fricot <africot@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
 */


#include "OutputOpenbach.h"
#include <opensand_output/Output.h>
#include <opensand_output/OutputInternal.h>
#include <collectagent.h>

#include <vector>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <algorithm>
#include <chrono>

#define REGISTER_COLLECT_CONF_PATH "/var/run/sand-daemon/register_collect.conf"
#define PLATFORM_ID "OPENSAND_PLATFORM_ID"
#define TIMEOUT 6

map<log_level_t, int> OutputOpenbach::priority_map = {{LEVEL_DEBUG, 8}, {LEVEL_INFO, 7}, {LEVEL_NOTICE, 6}, {LEVEL_WARNING, 5}, {LEVEL_ERROR, 4}, {LEVEL_CRITICAL, 3}, {LEVEL_EVENT, 8}};

constexpr long double high_resolution_to_ms = 1000 * std::chrono::high_resolution_clock::period::num / (long double)(std::chrono::high_resolution_clock::period::den);

OutputOpenbach::OutputOpenbach(const char *entity)
{
	// Retrieve PLATFORM_ID env variable
	const char *platform_id = std::getenv(PLATFORM_ID);
	// If it exists, append to entity name
	if ((platform_id != NULL) && (strlen(platform_id) > 0))
	{
		char entity_name[2 + strlen(platform_id) + strlen(entity)];
		sprintf(entity_name, "%s.%s", platform_id, entity);
		this->entity = string(entity_name);
	}
	else
	{
		this->entity = string(entity);
	}
	// Set the JOB_NAME env variable, don't overwrite
	setenv("JOB_NAME", "opensand", 0);
}

OutputOpenbach::~OutputOpenbach()
{
	this->enable_collector = false;

	// close syslog
	closelog();
}

bool OutputOpenbach::init(bool enable_collector)
{
	FILE *fd_conf;
	string message;
	string conf_path = REGISTER_COLLECT_CONF_PATH;

	// Create register collect configuration file
	fd_conf = fopen(conf_path.c_str(), "w+");
	fprintf(fd_conf, "[default]\nstorage=true\nbroadcast=false\n");
	fclose(fd_conf);
	
	// Register to collect-agent
	if(!collect_agent::register_collect(conf_path))
	{
		this->sendLog(this->log, LEVEL_ERROR, "Register collect failed\n");
	}
	else 
	{		
		this->sendLog(this->log, LEVEL_INFO, "Register collect succeeded\n");
	}
	
	if(enable_collector)
	{
		this->enableCollector();
	}

	this->log = this->registerLog(LEVEL_WARNING, "output");
	this->default_log = this->registerLog(LEVEL_WARNING, "default");

	this->OutputInternal::sendLog(this->log, LEVEL_INFO, "Output initialization done (%s)\n",
	                              enable_collector ? "enabled" : "disabled");

	this->setInitializing(true);
	return true;
}

bool OutputOpenbach::finishInit(void)
{
	if(!this->collectorEnabled())
	{
		this->setInitializing(false);
		return true;
	}

	if(!this->isInitializing())
	{
		this->sendLog(this->log, LEVEL_ERROR, "Initialization already done\n");
		return true;
	}

	this->setInitializing(false);

	this->sendLog(this->log, LEVEL_INFO, "Output initialized\n");
	return true;
}


bool OutputOpenbach::sendRegister(BaseProbe *probe)
{
	return true;
}

bool OutputOpenbach::sendRegister(OutputLog *log)
{
	return true;
}

void OutputOpenbach::sendProbes(void)
{
	std::unordered_map<string, string> stats;

	long double timestamp;
	
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::duration dtn = t1.time_since_epoch();
	
	timestamp = dtn.count() * high_resolution_to_ms;

	this->mutex.acquireLock();
	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		BaseProbe *probe = this->probes[i];
		if(this->getValueCount(probe) != 0)
		{
			stats[probe->getName()] = probe->getStrData();
		}
	}
	this->mutex.releaseLock();

	if(stats.empty())
	{
		return;
	}

	collect_agent::send_stat(timestamp, stats, entity);
}

void OutputOpenbach::sendLog(const OutputLog *log,
                             log_level_t log_level, 
                             const string &message_text)
{
	if(!log)
	{
		goto outputs;
	}
	// This log should not be reported
	// Events are always reported to manager
	if(log_level > log->getDisplayLevel() &&
	   log_level <= LEVEL_DEBUG)
	{
		return;
	}

	if(this->collectorEnabled() &&
	   (this->logsEnabled() || log_level == LEVEL_EVENT))
	{
		// Send the debug message to the collector
		string message = string(message_text);
	
		collect_agent::send_log(OutputOpenbach::priority_map[log_level], message.c_str());
	}

outputs:
	// id there is no collector message are printed in syslog
	if ((!this->collectorEnabled() || this->syslogEnabled()) &&
		log_level < LEVEL_EVENT)
	{
		// Log the debug message with syslog
		syslog(log_level, "[%s] %s",
		       log ? this->getLogName(log).c_str(): "default",
		       message_text.c_str());
	}

	if (this->stdlogEnabled() && log_level < LEVEL_EVENT)
	{
		// Log the messages in console
		if (log_level > LEVEL_WARNING)
		{
			fprintf(stdout, "\x1B[%dm%s\x1B[0m - [%s] %s",
			        this->getColors()[log_level], this->getLevels()[log_level],
			        log ? this->getLogName(log).c_str(): "default",
			        message_text.c_str());
		}
		else
		{
			fprintf(stderr, "\x1B[%dm%s\x1B[0m - [%s] %s",
			        this->getColors()[log_level], this->getLevels()[log_level],
			        log ? this->getLogName(log).c_str(): "default",
			        message_text.c_str());
		}
	}
}

extern "C" OutputInternal* create(const char *entity)
{
	  return new OutputOpenbach(entity);
}

extern "C" void destroy(OutputInternal **object)
{
	  delete *object;
	  *object = NULL;
}

