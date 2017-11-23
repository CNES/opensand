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
 * @file OutputOpensand.cpp
 * @brief Class used to hold opensand output variables and methods.
 * @author Alban Fricot <africot@toulouse.viveris.com>
 */


#include "OutputOpensand.h"

#include "Messages.h"
#include "Output.h"
#include "OutputInternal.h"
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

static uint32_t getMilis()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

OutputOpensand::OutputOpensand():
	sock(-1)
{
	memset(&this->daemon_sock_addr, 0, sizeof(this->daemon_sock_addr));
	memset(&this->self_sock_addr, 0, sizeof(this->self_sock_addr));

	this->sock = 0;
}

OutputOpensand::~OutputOpensand()
{
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
			this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
										  "Unable to delete the socket \"%s\": %s\n",
			                              path, strerror(errno));
		}
	}
	// close syslog
	closelog();
}

bool OutputOpensand::init(bool enable_collector,
                          const char *sock_prefix)
{
	char *path;
	string message;
	sockaddr_un address;
    
    FILE * fp;
    
    fp = fopen ("/tmp/toto.txt", "w+");
    



	fprintf(fp, "1");

	if(enable_collector)
	{
		this->enableCollector();
	}

	fprintf(fp, "2");
	if(sock_prefix == NULL)
	{
		sock_prefix = "/var/run/sand-daemon";
	}

	fprintf(fp, "3");
	if(enable_collector)
	{
		
	fprintf(fp, "4");
		this->daemon_sock_addr.sun_family = AF_UNIX;
		path = this->daemon_sock_addr.sun_path;
		snprintf(path, sizeof(this->daemon_sock_addr.sun_path),
		         "%s/" DAEMON_SOCK_NAME, sock_prefix);

	fprintf(fp, "5");
		this->self_sock_addr.sun_family = AF_UNIX;
		path = this->self_sock_addr.sun_path;
		snprintf(path, sizeof(this->self_sock_addr.sun_path),
		         "%s/" SELF_SOCK_NAME, sock_prefix, getpid());
	
	fprintf(fp, "6");
		// Initialization of the UNIX socket
		this->sock = socket(AF_UNIX, SOCK_DGRAM, 0);

	fprintf(fp, "7");
	
		if(this->sock == -1)
		{
			this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
			                              "Socket allocation failed: %s\n", strerror(errno));
			return false;
		}

	fprintf(fp,"8");
		path = this->self_sock_addr.sun_path;
		unlink(path);

	fprintf(fp, "9");
		memset(&address, 0, sizeof(address));
		address.sun_family = AF_UNIX;
		strncpy(address.sun_path, path, sizeof(address.sun_path) - 1);
		if(bind(this->sock, (const sockaddr*)&address, sizeof(address)) < 0)
		{
			this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
										  "Socket binding failed: %s\n", strerror(errno));
			return false;
		}
	}
	
	fprintf(fp, "10");
	this->log = this->registerLog(LEVEL_WARNING, "output");
	
	fprintf(fp, "11");
	this->default_log = this->registerLog(LEVEL_WARNING, "default");

	fprintf(fp, "12");
	this->OutputInternal::sendLog(this->log, LEVEL_INFO, "Output initialization done (%s)\n",
								  enable_collector ? "enabled" : "disabled");


	fprintf(fp, "13");
	this->OutputInternal::sendLog(this->log, LEVEL_INFO,
				                  "Daemon socket address is \"%s\", "
						          "own socket address is \"%s\"\n",
								  this->daemon_sock_addr.sun_path,
								  this->self_sock_addr.sun_path);


	fprintf(fp, "14");
	this->setInitializing(true);
	
	fprintf(fp, "15");
	return true;
}

uint8_t OutputOpensand::rcvMessage(void) const
{
	OutputLock lock(this->mutex);
	char buffer[32];
	return receiveMessage(this->sock, buffer, sizeof(buffer), this->daemon_sock_addr.sun_path);
}


bool OutputOpensand::finishInit(void)
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

		message.append(1, this->getBaseProbeId(probes[i]));
		message.append(1, (((int)this->probes[i]->isEnabled()) << 7) |
		                   this->getStorageTypeId(this->probes[i]));
		message.append(1, name.size());
		message.append(1, unit.size());
		message.append(name);
		message.append(unit);
	}
	
	// lock to be sure to ge the correct message when trying to receive
	if(!this->sendMessage(message))
	{
		this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
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
		this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
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
	command_thread = new CommandThread(this->sock, this->daemon_sock_addr);
	if(!command_thread->start())
	{
		this->sendLog(this->log, LEVEL_ERROR, "Cannot start command thread\n");
		return false;
	}

	this->sendLog(this->log, LEVEL_INFO, "output initialized\n");

	return true;
}


bool OutputOpensand::sendRegister(BaseProbe *probe)
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
	  	    	                        
	message.append(1, this->getBaseProbeId(probe));
	message.append(1, (((int)probe->isEnabled()) << 7) |
	                   this->getStorageTypeId(probe));
	message.append(1, name.size());
	message.append(1, unit.size());
	message.append(name);
	message.append(unit);

	if(!this->sendMessage(message))
	{
		this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
									  "Sending new probe failed: %s\n",
									  strerror(errno));
		return false;
	}

	this->OutputInternal::sendLog(this->log, LEVEL_INFO,
								  "New probe %s registration sent.\n",
								  name.c_str());

	return true;
}

bool OutputOpensand::sendRegister(OutputLog *log)
{
	string message;
	bool receive = false;

	const string name = this->getLogName(log);
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

	message.append(1, this->getLogId(log));
	message.append(1, level);
	message.append(1, name.size());
	message.append(name);

	if(!this->sendMessage(message))
	{
		this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
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
				this->OutputInternal::sendLog(this->log, LEVEL_WARNING,
											  "receive NACK for log %s registration\n",
											  name.c_str());
				return false;
			}
			else
			{
				this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
											  "Incorrect ACK response (%u) for log %s registration\n",
											  command_id, name.c_str());
			}
			return false;
		}
	}

	this->OutputInternal::sendLog(this->log, LEVEL_DEBUG,
								  "New log %s registration sent\n", name.c_str());

	return true;
}


void OutputOpensand::sendProbes(void)
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
		if(probe->isEnabled() && this->getValueCount(probe) != 0)
		{
			needs_sending = true;
			message.append(1, (uint8_t)i);
			this->appendValueAndReset(probe, message);
		}
	}
	this->mutex.releaseLock();

	if(!needs_sending)
	{
		return;
	}

	if(!this->sendMessage(message))
	{
		this->OutputInternal::sendLog(this->log, LEVEL_ERROR,
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
void OutputOpensand::sendLog(const OutputLog *log,
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
		msgHeaderSendLog(message, this->getLogId(log), log_level);
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


bool OutputOpensand::sendMessage(const string &message, bool block) const
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

