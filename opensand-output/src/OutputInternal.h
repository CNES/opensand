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
 * @file OutputInternal.h
 * @brief Class used to hold internal output library variables and methods.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#ifndef _OUTPUT_INTERNAL_H
#define _OUTPUT_INTERNAL_H


#include "Probe.h"
#include "OutputLog.h"
#include "OutputEvent.h"
#include "OutputMutex.h"

#include <assert.h>
#include <sys/un.h>
#include <vector>

using std::vector;


/**
 * @class hold internal output library variables and methods
 */
class OutputInternal
{
	friend class Output;
	friend class BaseProbe;

private:
	OutputInternal();
	~OutputInternal();

	/**
	 * @brief initialize the output element
	 *
	 * @param enable_collector  Whether the element is enabled
	 * @param sock_prefix        The socket prefix
	 * @return true on success, false otherwise
	 */
	bool init(bool enable_collector, 
	          const char *sock_prefix);

	/**
	 * @brief Register a probe for the element
	 *
	 * @param name     The probe name
	 * @param unit     The probe unit
	 * @param enabled  Whether the probe is enabled by default
	 * @param type     The sample type
	 *
	 * @return the probe object
	 **/
	template<typename T>
	Probe<T> *registerProbe(const string &name,
	                        const string &unit,
	                        bool enabled, sample_type_t type);

	/**
	 * @brief Register an event for the element
	 *
	 * @param identifier   The event name
	 *
	 * @return the event object
	 **/
	OutputEvent *registerEvent(const string &identifier);

	/** 
	 * @brief Register a log with the desired maximum level
	 *
	 * @param display_level  The maximum level to display
	 * @param name           The log name
	 *
	 * @return the log object
	 **/
	OutputLog *registerLog(log_level_t log_level,
	                       const string &name);

	/**
	 * @brief Finish the element initialization
	 **/
	bool finishInit(void);

	/**
	 * @brief Send all probes which got new values sinces the last call.
	 **/
	void sendProbes(void);

	/**
	 * @brief Send the specified log with the specified message
	 *
	 * @param log		The log
	 * @param log_level	The log level
	 * @param message	The message
	 **/
	void sendLog(OutputLog *log, log_level_t log_level,
	             const string &message_text);

	/**
	 * @brief Send a log (for OutputInternal logging)
	 *
	 * @param log		The log
	 * @param log_level	The log level
	 * @param message	The message
	 **/
	void sendLog(OutputLog *log, log_level_t log_level,
	             const char *msg_format, ...);

	/**
	 * @brief Set the probe state
	 *
	 * @param probe_id  The probe ID
	 * @param enabled   Whether the probe is enabled or not
	 */
	void setProbeState(uint8_t probe_id, bool enabled);

	/**
	 * @brief Set the log level
	 *
	 * @param log_id  The log ID
	 * @param level   The log level
	 */
	void setLogLevel(uint8_t log_id, log_level_t level);

	/**
	 * @brief disable all stats
	 */
	void disableCollector(void);

	/**
	 * @brief Enable output
	 */
	void enableCollector(void);

	/**
	 * @brief disable syslog output
	 */
	void disableSyslog(void);

	/**
	 * @brief Enable syslog output
	 */
	void enableSyslog(void);

	/**
	 * @brief disable logs output toward collector
	 */
	void disableLogs(void);

	/**
	 * @brief Enable log output toward collector
	 */
	void enableLogs(void);
	
	/**
	 * @brief Enable output on stdout/stdin
	 */
	void enableStdlog(void);

	/**
	 * @brief  Send registration for a probe outside initialization
	 *
	 * @param probe  The new probe to register
	 * @return true on success, false otherwise
	 */
	bool sendRegister(BaseProbe *probe);

	/**
	 * @brief  Send registration for a log outside initialization
	 *
	 * @param probe  The new log to register
	 * @return true on success, false otherwise
	 */
	bool sendRegister(OutputLog *log);

	/**
	 * @brief  Send a message to the daemon
	 *
	 * @param message  The message
	 */
	bool sendMessage(const string &message) const;

	/**
	 * @brief receive a message from the daemon
	 *
	 * @return the command type on success, 0 on failure
	 */
	uint8_t rcvMessage(void) const;

	/**
	 * @brief whether the collector is enabled
	 */
	bool collectorEnabled(void) const;

	/**
	 * @brief Whether the logs are enabled
	 */
	bool logsEnabled(void) const;

	/**
	 * @brief Whether syslog is enabled
	 */
	bool syslogEnabled(void) const;

	/**
	 * @brief Whether the stdout/stderr are enabled
	 */
	bool stdlogEnabled(void) const;

	/**
	 * @brief Check if output is initializing
	 *
	 * @return initializing state
	 */
	bool isInitializing(void) const;

	/**
	 * @brief Set initializing state
	 *
	 * @param val the initializing state
	 */
	void setInitializing(bool val);

	/// whether the element is enabled
	bool enable_collector;

	/// whether the element is in the initializing phase
	bool initializing;

	/// whether the logs are sent to the collector
	bool enable_logs;

	/// wether the syslog is enabled;
	bool enable_syslog;

	/// whether the logs are printed on stdout/stderr
	bool enable_stdlog;

	/// the probes
	vector<BaseProbe *> probes;

	/// the logs
	vector<OutputLog *> logs;

	/// the socket for communication with daemon
	int sock;

	/// the timestamp of the initialization
	uint32_t started_time;

	/// the dameon socket address
	sockaddr_un daemon_sock_addr;

	/// the element socket address
	sockaddr_un self_sock_addr;

	/// a default log
	OutputLog *default_log;
	
	/// a output log
	OutputLog *log;

	/// The mutex on Output
	mutable OutputMutex mutex;
};


// TODO check if NULL in core, also for logs and events
template<typename T>
Probe<T> *OutputInternal::registerProbe(const string &name,
                                        const string &unit,
                                        bool enabled, sample_type_t type)
{
	this->mutex.acquireLock();
	uint8_t new_id = this->probes.size();
	Probe<T> *probe = new Probe<T>(new_id, name, unit, enabled, type);
	this->probes.push_back(probe);
	this->mutex.releaseLock();

	this->sendLog(this->log, LEVEL_INFO,
	              "Registering probe %s with type %d\n",
	              name.c_str(), type);

	// single registration if process is already started
	if(!this->isInitializing() && !this->sendRegister(probe))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Failed to register new probe %s\n", 
		              name.c_str());
	}

	return probe;
}

#endif
