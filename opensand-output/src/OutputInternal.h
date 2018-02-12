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
 * @file OutputInternal.h
 * @brief Class used to hold internal output library variables and methods.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 * @author Alban FRICOT <africot@toulouse.viveris.com>
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
#include <map>

using std::vector;
using std::map;


/**
 * @class hold internal output library variables and methods
 */
class OutputInternal
{
	friend class Output;
	friend class BaseProbe;
	
  protected:
	OutputInternal();
	
	/**
	 * @brief Get Base Probe Id
	 *
	 * @param probe object
	 * @return base probe id
	 */
    uint8_t getBaseProbeId(BaseProbe *probe) const;
	
	/**
	 * @brief Get storage type id
	 *
	 * @param probe object
	 * @return storage type id 
	 */
    uint8_t getStorageTypeId(BaseProbe *probe) const;
	
	/**
	 * @brief Get log name
	 *
	 * @param log object
	 * @return the log name
	 */
	string getLogName(const OutputLog *log) const;
	
	/**
	 * @brief Get log id
	 *
	 * @param log object
	 * @return the log Id
	 */
	uint8_t getLogId(const OutputLog *log) const;
	
	/**
	 * @brief Get count values
	 *
	 * @param probe object
	 * @return count values
	 */
	uint16_t getValueCount(BaseProbe *probe) const;
	
	/**
	 * @brief Get color for logs levels 
	 *
	 * @return array of colors for a log level 
	 */
    const int *getColors() const;	
	
	/**
	 * @brief Get elvels of logs
	 *
	 * @return array of levels for logs
	 */
	const char **getLevels() const;

	/**
	 * @brief Set initializing state
	 *
	 * @param val the initializing state
	 */
	void setInitializing(bool val);

  public:
	virtual ~OutputInternal();

	/**
	 * @brief initialize the output element
	 *
	 * @param enable_collector  Whether the element is enabled
	 * @return true on success, false otherwise
	 */
	virtual bool init(bool enable_collector) = 0;

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
	virtual bool finishInit(void) = 0;

	/**
	 * @brief Send all probes which got new values sinces the last call.
	 **/
	virtual void sendProbes(void) = 0;

	/**
	 * @brief Send the specified log with the specified message
	 *
	 * @param log		The log
	 * @param log_level	The log level
	 * @param message	The message
	 **/
	virtual void sendLog(const OutputLog *log, log_level_t log_level,
	             const string &message_text) = 0;

	/**
	 * @brief Send default log with the specified message
	 *
	 * @param log_level	The log level
	 * @param message	The message
	 **/
	void sendLog(log_level_t log_level,
	             const string &message_text);

	/**
	 * @brief Send a log (for OutputInternal logging)
	 *
	 * @param log		The log
	 * @param log_level	The log level
	 * @param message	The message
	 **/
	void sendLog(const OutputLog *log, log_level_t log_level,
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
	virtual bool sendRegister(BaseProbe *probe) = 0;

	/**
	 * @brief  Send registration for a log outside initialization
	 *
	 * @param probe  The new log to register
	 * @return true on success, false otherwise
	 */
	virtual bool sendRegister(OutputLog *log) = 0;

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
	 * @brief Set the log levels as defined in the configuration
	 *
	 * @param levels    The log levels defines in configuration
	 * @param specific  Used defined levels
	 */
	void setLevels(const map<string, log_level_t> &levels,
	               const map<string, log_level_t> &specific);

	/**
	 * @brief Check a log level in order to update it according to
	 *        levels loaded in configuration
	 *
	 * @param log  The log to check
	 */
	void checkLogLevel(OutputLog *log);

  protected:
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

	/// the timestamp of the initialization
	uint32_t started_time;

	/// a default log
	OutputLog *default_log;
	
	/// an output log
	OutputLog *log;

	/// the levels loaded in configuration
	map<string, log_level_t> levels;

	/// the user defined levels loaded in configuration
	map<string, log_level_t> specific;

	/// The number of blocked operations
	mutable int blocked;

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
	if(this->collectorEnabled() && !this->isInitializing() &&
	   !this->sendRegister(probe))
	{
		this->sendLog(this->log, LEVEL_ERROR,
		              "Failed to register new probe %s\n", 
		              name.c_str());
	}

	return probe;
}

#endif
