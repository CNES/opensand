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
 * @file Output.h
 * @brief Definition of the Output static class, used by the application to
 *        interact with the output library.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 * @author Fabrice Hobaya <fhobaya@toulouse.viveris.com>
 */


#ifndef _OUTPUT_H
#define _OUTPUT_H


#include "Probe.h"
#include "OutputEvent.h"
#include "OutputLog.h"
#include "OutputInternal.h"
#include "OutputOpensand.h"

#include <vector>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

#define PRINTFLIKE(fmt_pos, vararg_pos) __attribute__((format(printf,fmt_pos,vararg_pos)))

#define DFLTLOG(level, fmt, args...) \
	do \
	{ \
		Output::sendLog(level, \
		                " [%s:%s():%d] " fmt, \
		                __FILE__, __FUNCTION__, __LINE__, ##args); \
	} \
	while(0)

#define LOG(log, level, fmt, args...) \
	do \
	{ \
		if(level <= log->getDisplayLevel()) \
		{ \
			Output::sendLog(log, level, \
			                " [%s:%s():%d] " fmt, \
			                __FILE__, __FUNCTION__, __LINE__, ##args); \
		} \
	} \
	while(0)


#define DUMP(data, len) \
	do \
	{ \
		std::string str(""); char x[4]; int i; \
		for(i=0;i<(signed)len;i++) \
		{ \
			sprintf(x, "%02X ", ((unsigned char *)data)[i]); \
			str += x; \
		} \
		Output::sendLog(LEVEL_ERROR, \
		                "%s", str.c_str()); \
	} \
	while(0)

class Output
{
	friend class CommandThread;
	friend uint8_t receiveMessage(int, char*, size_t);

public:

	/**
	 * @brief Initialize the output library
	 *        Prepares the library for registering probes and logs
	 *
	 * @param enabled      Set to false to disable the output library
	 * @param sock_prefix  Custom socket path prefix (for testing purposes)
	 * @return true on success, false otherwise
	 */
	static bool init(bool enabled,
	                 const char *sock_prefix = NULL);

	/**
	 * @brief Register a probe in the output library
	 *
	 * @param name          The probe name
	 * @param enabled       Whether the probe is enabled by default
	 * @param sample_type_t   The sample type
	 *
	 * @return the probe object
	 **/
	template<typename T>
	static Probe<T> *registerProbe(const string &name,
	                               bool enabled,
	                               sample_type_t type);

	/**
	 * @brief Register a probe in the output library
	 *
	 * @param name     The probe name
	 * @param unit     The probe unit
	 * @param enabled  Whether the probe is enabled by default
	 * @param type     The sample type
	 *
	 * @return the probe object
	 **/
	template<typename T>
	static Probe<T> *registerProbe(const string &name,
	                               const string &unit,
	                               bool enabled, sample_type_t type);

	/**
	 * @brief Register a probe in the output library
	 *        with variable arguments in name
	 *
	 * @param enabled   Whether the probe is enabled by default
	 * @param type      The sample type
	 * @param name      The probe name with variable arguments
	 *
	 * @return the probe object
	 **/
	template<typename T>
	static Probe<T> *registerProbe(bool enabled,
	                               sample_type_t type,
	                               const char *msg_format, ...);

	/**
	 * @brief Register a probe in the output library
	 *        with variable arguments in name
	 *
	 * @param unit     The probe unit
	 * @param enabled  Whether the probe is enabled by default
	 * @param type     The sample type
	 * @param name     The probe name with variable arguments
	 *
	 * @return the probe object
	 **/
	template<typename T>
	static Probe<T> *registerProbe(const string &unit,
	                               bool enabled, sample_type_t type,
	                               const char *name, ...);
	/**
	 * @brief Register an event in the output library
	 *
	 * @param identifier   The event name
	 *
	 * @return the event object
	 **/
	static OutputEvent *registerEvent(const string &identifier);
	
	/**
	 * @brief Register a log with the level Warning in the output library
	 *
	 * @param display_level The minimum display level
	 * @param name          The log name
	 *
	 * @return the log object
	 **/
	static OutputLog *registerLog(log_level_t display_level, 
	                              const string &name);

	/**
	 * @brief Register an event in the output library
	 *        with variable arguments
	 *
	 * @param identifier   The event name with variable arguments
	 *
	 * @return the event object
	 **/
	static OutputEvent *registerEvent(const char *identifier, ...);
	
	/**
	 * @brief Register a log with the level Warning in the output library
	 *
	 * @param default_display_level  The default minimum display level for 
	 *                               this log
	 * @param name The log name
	 *
	 * @return the log object
	 **/
	static OutputLog *registerLog(log_level_t default_display_level,
	                              const char* name, ...);
	
	/**
	 * @brief Finish the output library initialization
	 *       Performs the library registration on the OpenSAND daemon.
	 *
	 * @warning Needs to be called after registering probes and before
	 *          starting using them.
	 **/
	static bool finishInit(void);

	/**
	 * @brief Send all probes which got new values sinces the last call.
	 **/
	static void sendProbes(void);

	/**
	 * @brief Send the specified event with the specified message format.
	 *
	 * @param event       The event
	 * @param msg_format  The message format
	 **/
	static void sendEvent(OutputEvent *event, const char *msg_format, ...)
		PRINTFLIKE(2, 3);

	/**
	 * @brief Sent the specified log (debug level) with the specified message
	 *        format
	 *
	 * @param log         The log
	 * @param log_level   The log level to send
	 * @param msg_format  The message format
	 **/
	static void sendLog(const OutputLog *log,
	                    log_level_t log_level, 
	                    const char *msg_format, ...)
		PRINTFLIKE(3, 4);
	
	/**
	 * @brief Sent a message (with no level specified) with the specified
	 *        message format
	 *
	 * @param log_level   The log level to send
	 * @param msg_format  The message format
	 **/
	static void sendLog(log_level_t log_level, 
	                    const char *msg_format, ...)
		PRINTFLIKE(2, 3);

	/**
	 * @brief Enable output on stdout/stderr
	 */
	static void enableStdlog(void);

	/**
	 * @brief Adjust the output log display level
	 *
	 * @param level  the new display level
	 */
	static void setDisplayLevel(log_level_t level);

	/**
	 * @brief Set the log levels as defined in the configuration
	 *
	 * @param levels    The log levels defines in configuration
	 * @param specific  User defined levels
	 */
	static void setLevels(const map<string, log_level_t> &levels,
	                      const map<string, log_level_t> &specific);

private:
	/**
	 * @brief  Get the daemon socket address
	 *
	 * @return the daemon socket address
	 */
	inline static const sockaddr_un *daemonSockAddr()
	{
		return &instance->daemon_sock_addr;
	};

	/**
	 * @brief  Get the output instance socket address
	 *
	 * @return the output instance socket address
	 */
	inline static const sockaddr_un *selfSockAddr()
	{
		return &instance->self_sock_addr;
	};

	Output();
	~Output();

	/**
	 * @brief Set the probe state
	 *
	 * @param probe_id  The probe ID
	 * @param enabled   Whether the probe is enabled or not
	 */
	static void setProbeState(uint8_t probe_id, bool enabled);

	/**
	 * @brief Set the log level
	 *
	 * @param log_id  The log id
	 * @param level   The log level
	 */
	static void setLogLevel(uint8_t log_id, log_level_t level);

	/**
	 * @brief disable all stats and logs
	 */
	static void disableCollector(void);

	/**
	 * @brief Enable output
	 */
	static void enableCollector(void);

	/**
	 * @brief disable logs output toward collector
	 */
	static void disableLogs(void);

	/**
	 * @brief Enable log output toward collector
	 */
	static void enableLogs(void);
	
	/**
	 * @brief disable syslog output
	 */
	static void disableSyslog(void);

	/**
	 * @brief Enable syslog output
	 */
	static void enableSyslog(void);

	/// The output instance
	static OutputOpensand opensand_instance;

	static OutputInternal *instance;
	
};

template<typename T>
Probe<T> *Output::registerProbe(const string &name, bool enabled,
                                sample_type_t type)
{
	return Output::registerProbe<T>(name, "", enabled, type);
}

template<typename T>
Probe<T> *Output::registerProbe(const string &name,
                                const string &unit,
                                bool enabled, sample_type_t type)
{
	return Output::instance->registerProbe<T>(name, unit, enabled, type);
}

template<typename T>
Probe<T> *Output::registerProbe(bool enabled,
                                sample_type_t type,
                                const char *name, ...)
{
	char buf[1024];
	va_list args;
	
	va_start(args, name);

	vsnprintf(buf, sizeof(buf), name, args);

	va_end(args);

	return Output::registerProbe<T>(buf, "", enabled, type);
}

#endif
