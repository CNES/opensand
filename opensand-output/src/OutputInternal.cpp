/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
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
 */


#include "OutputInternal.h"

#include "Output.h"
#include "CommandThread.h"
#include "Messages.h"

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

static uint32_t getMilis()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}



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
	sock(-1),
	default_log(NULL),
	log(NULL),
	levels(),
	specific(),
	blocked(0),
	mutex("Output")
{
	memset(&this->daemon_sock_addr, 0, sizeof(this->daemon_sock_addr));
	memset(&this->self_sock_addr, 0, sizeof(this->self_sock_addr));

	this->sock = 0;
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

	if(this->sock != 0)
	{
		// Close the command socket
		// (will exit the command thread)
		shutdown(this->sock, SHUT_RDWR);
		close(this->sock);
		this->enable_collector = false;

		// Remove the socket file
		const char *path = this->self_sock_addr.sun_path;
		if(unlink(path) < 0)
		{
			this->default_log = NULL;
			this->log = NULL;
			this->sendLog(this->log, LEVEL_ERROR,
			              "Unable to delete the socket \"%s\": %s\n",
			               path, strerror(errno));
		}
	}
	// close syslog
	closelog();
}

bool OutputInternal::init(bool enable_collector,
                          const char *sock_prefix)
{
	char *path;
	string message;
	sockaddr_un address;

	if(enable_collector)
	{
		this->enableCollector();
	}

	if(sock_prefix == NULL)
	{
		sock_prefix = "/var/run/sand-daemon";
	}

	if(enable_collector)
	{
		this->daemon_sock_addr.sun_family = AF_UNIX;
		path = this->daemon_sock_addr.sun_path;
		snprintf(path, sizeof(this->daemon_sock_addr.sun_path),
		         "%s/" DAEMON_SOCK_NAME, sock_prefix);

		this->self_sock_addr.sun_family = AF_UNIX;
		path = this->self_sock_addr.sun_path;
		snprintf(path, sizeof(this->self_sock_addr.sun_path),
		         "%s/" SELF_SOCK_NAME, sock_prefix, getpid());
	
		// Initialization of the UNIX socket
		this->sock = socket(AF_UNIX, SOCK_DGRAM, 0);

		if(this->sock == -1)
		{
			this->sendLog(this->log, LEVEL_ERROR,
			              "Socket allocation failed: %s\n", strerror(errno));
			return false;
		}

		path = this->self_sock_addr.sun_path;
		unlink(path);

		memset(&address, 0, sizeof(address));
		address.sun_family = AF_UNIX;
		strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
		if(bind(this->sock, (const sockaddr*)&address, sizeof(address)) < 0)
		{
			this->sendLog(this->log, LEVEL_ERROR,
			              "Socket binding failed: %s\n", strerror(errno));
			return false;
		}
	}
	
	this->log = this->registerLog(LEVEL_WARNING, "output");
	this->default_log = this->registerLog(LEVEL_WARNING, "default");

	this->sendLog(this->log, LEVEL_INFO, "Output initialization done (%s)\n",
	              enable_collector ? "enabled" : "disabled");


	this->sendLog(this->log, LEVEL_INFO,
	              "Daemon socket address is \"%s\", "
	              "own socket address is \"%s\"\n",
	              this->daemon_sock_addr.sun_path,
	              this->self_sock_addr.sun_path);


	this->setInitializing(true);
	return true;
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

bool OutputInternal::finishInit(void)
{
	this->started_time = getMilis();
	
	if(!this->collectorEnabled())
	{
		this->setInitializing(false);
		return true;
	}

	if(!this->isInitializing())
	{
		this->sendLog(this->log, LEVEL_ERROR, "initialization already done\n");
		return true;
	}

	this->sendLog(this->log, LEVEL_INFO, "Opening output communication socket\n");

	uint8_t command_id;
	string message;
	// TODO this is never deleted ?!
	CommandThread *command_thread;
	
	int ret;
	struct timespec tv;
	fd_set readfds;
	sigset_t sigmask;

	// Send the initial probe list
	// Build header
	msgHeaderRegisterEnd(message, getpid(), this->probes.size(),
	                     0);

	// Add the probes
	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		const string name = this->probes[i]->getName();
		const string unit = this->probes[i]->getUnit();

		message.append(1, probes[i]->id);
		message.append(1, (((int)this->probes[i]->isEnabled()) << 7) |
		                   this->probes[i]->storageTypeId());
		message.append(1, name.size());
		message.append(1, unit.size());
		message.append(name);
		message.append(unit);
	}
	

	// Add the logs
/*	for(size_t i = 0 ; i < this->logs.size() ; i++)
	{
		uint8_t level = this->logs[i]->getDisplayLevel();
		const string name = this->logs[i]->getName();

		message.append(1, level);
		message.append(1, name.size());
		message.append(name);
	}*/
	// lock to be sure to ge the correct message when trying to receive
	if(!this->sendMessage(message))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Sending initial probe and log list failed: %s\n",
		              strerror(errno));
		this->disableCollector();
		this->setInitializing(false);
		return false;
	}

	// Wait for the ACK response

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);

	FD_ZERO(&readfds);
	FD_SET(this->sock, &readfds);

	tv.tv_sec = TIMEOUT;
	tv.tv_nsec = 0;
	ret = pselect(this->sock + 1, &readfds, NULL, NULL, &tv, &sigmask);
	if(ret <= 0)
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "cannot contact daemon or no answer in the "
		              "last %d seconds\n", TIMEOUT);
		this->disableCollector();
		this->setInitializing(false);
		return false;
	}
	command_id = this->rcvMessage();
	if(command_id != MSG_CMD_ACK)
	{
		if(command_id == MSG_CMD_NACK)
		{
			this->sendLog(this->log, LEVEL_WARNING,
			              "receive NACK for initial probe list, disable output\n");
			this->disableCollector();
			this->setInitializing(false);
		}
		else
		{
			this->sendLog(this->log, LEVEL_ERROR,
			              "Incorrect ACK response for initial probe list\n");
		}
		return false;
	}

	this->setInitializing(false);
	// Start the command thread
	command_thread = new CommandThread(this->sock);
	if(!command_thread->start())
	{
		this->sendLog(this->log, LEVEL_ERROR, "Cannot start command thread\n");
		return false;
	}

	this->sendLog(this->log, LEVEL_INFO, "output initialized\n");

	return true;
}

void OutputInternal::sendProbes(void)
{
	if(!this->collectorEnabled())
	{
		return;
	}

	bool needs_sending = false;

	uint32_t timestamp = getMilis() - this->started_time;
	string message;
	msgHeaderSendProbes(message, timestamp);

	this->mutex.acquireLock();
	for(size_t i = 0 ; i < this->probes.size() ; i++)
	{
		BaseProbe *probe = this->probes[i];
		if(probe->isEnabled() && probe->values_count != 0)
		{
			needs_sending = true;
			message.append(1, (uint8_t)i);
			probe->appendValueAndReset(message);
		}
	}
	this->mutex.releaseLock();

	if(!needs_sending)
	{
		return;
	}

	if(!this->sendMessage(message))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Sending probe values failed: %s\n",
		              strerror(errno));
	}
}


// TODO about the log level:
// if there is no collector, we won't be able to set a custom log
// level, so it would be great, in this case to load a level per
// block + a default one in the configuration file as it was
// done before.
// TODO create a circular buffer running in a thread, to avoid sending
//      too much logs and also to avoid sending sames logs many times
//      and also to avoid waiting on output when logging...
void OutputInternal::sendLog(const OutputLog *log,
                             log_level_t log_level, 
                             const string &message_text)
{
	if(!log)
	{
		// for internal output
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
		string message;	
		msgHeaderSendLog(message, log->id, log_level);
		message.append(message_text);

		if(!this->sendMessage(message, false))
		{
			// do not call sendLog again, we may loop...
			syslog(LEVEL_ERROR,
			       "Sending log failed: %s\n", strerror(errno));
		}
	}

outputs:
	// id there is no collector message are printed in syslog
	if ((!this->collectorEnabled() || this->syslogEnabled()) &&
		log_level < LEVEL_EVENT)
	{
		// Log the debug message with syslog
		syslog(log_level, "[%s] %s",
		       log ? log->getName().c_str(): "default",
		       message_text.c_str());
	}

	if (this->stdlogEnabled() && log_level < LEVEL_EVENT)
	{
		// Log the messages in console
		if (log_level > LEVEL_WARNING)
		{
			fprintf(stdout, "\x1B[%dm%s\x1B[0m - [%s] %s",
			        OutputLog::colors[log_level], OutputLog::levels[log_level],
			        log ? log->getName().c_str(): "default",
			        message_text.c_str());
		}
		else
		{
			fprintf(stderr, "\x1B[%dm%s\x1B[0m - [%s] %s",
			        OutputLog::colors[log_level], OutputLog::levels[log_level],
			        log ? log->getName().c_str(): "default",
			        message_text.c_str());
		}
	}
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

// TODO factorize with finish init
// TODO we could create a new function that would add new probes without sending
//      and a function to send them
// TODO do the same with events ?
bool OutputInternal::sendRegister(BaseProbe *probe)
{
	string message;

	const string name = probe->getName();
	const string unit = probe->getUnit();

	if(this->isInitializing())
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Cannot live register a probe in initialization\n");
		return false;
	}

	msgHeaderRegisterLive(message, getpid(), 1, 0);

	message.append(1, probe->id);
	message.append(1, (((int)probe->isEnabled()) << 7) |
	                   probe->storageTypeId());
	message.append(1, name.size());
	message.append(1, unit.size());
	message.append(name);
	message.append(unit);

	if(!this->sendMessage(message))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Sending new probe failed: %s\n",
		              strerror(errno));
		return false;
	}

	this->sendLog(this->log, LEVEL_INFO,
	              "New probe %s registration sent.\n",
	              name.c_str());

	return true;
}

bool OutputInternal::sendRegister(OutputLog *log)
{
	string message;
	bool receive = false;

	const string name = log->getName();
	const uint8_t level = (uint8_t)log->getDisplayLevel();

	// Send the new log
	if(this->isInitializing())
	{
		msgHeaderRegister(message, getpid(), 0, 1);
		receive = true;
	}
	else
	{
		// after initialization, command thread is running, we should 
		// not try to interecept received message
		msgHeaderRegisterLive(message, getpid(), 0, 1);
	}

	message.append(1, log->id);
	message.append(1, level);
	message.append(1, name.size());
	message.append(name);

	if(!this->sendMessage(message))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Sending new log failed: %s\n", strerror(errno));
		return false;
	}

	if(receive)
	{
		uint8_t command_id;
		command_id = this->rcvMessage();
		if(command_id != MSG_CMD_ACK)
		{
			if(command_id == MSG_CMD_NACK)
			{
				this->sendLog(this->log, LEVEL_WARNING,
				              "receive NACK for log %s registration\n",
				              name.c_str());
				return false;
			}
			else
			{
				this->sendLog(this->log, LEVEL_ERROR,
				              "Incorrect ACK response (%u) for log %s registration\n",
				              command_id, name.c_str());
			}
			return false;
		}
	}

	this->sendLog(this->log, LEVEL_DEBUG,
	              "New log %s registration sent\n", name.c_str());

	return true;
}

bool OutputInternal::sendMessage(const string &message, bool block) const
{
	OutputLock lock(this->mutex);
	if(sendto(this->sock, message.data(), message.size(),
	          ((block == true) ? 0 : MSG_DONTWAIT),
	          (const sockaddr*)&this->daemon_sock_addr,
	          sizeof(this->daemon_sock_addr)) < (signed)message.size())
	{
		if(!block && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			this->blocked++;
			return true;
		}
		return false;
	}
	if(this->blocked > 0)
	{
		syslog(LEVEL_WARNING,
		       "%d messages were not sent due to non-blocking socket operations\n",
		       this->blocked);
		this->blocked = 0;
	}
	return true;
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

uint8_t OutputInternal::rcvMessage(void) const
{
	OutputLock lock(this->mutex);
	char buffer[32];
	return receiveMessage(this->sock, buffer, sizeof(buffer));
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
		std::transform(log->getName().begin(), log->getName().end(),
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
		std::transform(log->getName().begin(), log->getName().end(),
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
