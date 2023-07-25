/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
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
 * @brief Definition of the Output singleton class, used by the application to
 *        interact with the output library.
 * @author Vincent Duvert     <vduvert@toulouse.viveris.com>
 * @author Fabrice Hobaya     <fhobaya@toulouse.viveris.com>
 * @author Alban FRICOT       <africot@toulouse.viveris.com>
 * @author Mathias Ettinger   <mathias.ettinger@viveris.fr>
 */


#ifndef _OUTPUT_H
#define _OUTPUT_H

#include <map>
#include <vector>
#include <memory>
#include <string>

#include "Probe.h"
#include "OutputLog.h"
#include "OutputMutex.h"


#define DFLTLOG(level, fmt, args...) \
	do \
	{ \
		Output::Get()->sendLog(level, \
		                       "[%s:%s():%d] " fmt, \
		                       __FILE__, __FUNCTION__, __LINE__, ##args); \
	} \
	while(0)

#define LOG(log, level, fmt, args...) \
	do \
	{ \
		log->sendLog(level, \
		             "[%s:%s():%d] " fmt, \
		             __FILE__, __FUNCTION__, __LINE__, ##args); \
	} \
	while(0)


/*
#define DUMP(data, len) \
	do \
	{ \
		std::string str(""); char x[4]; int i; \
		for(i=0;i<(signed)len;i++) \
		{ \
			sprintf(x, "%02X ", ((unsigned char *)data)[i]); \
			str += x; \
		} \
		Output::Get()->sendLog(LEVEL_ERROR, "%s", str.c_str()); \
	} \
	while(0)
*/


class OutputEvent;
class LogHandler;
class StatHandler;
class OutputDesiredLogLevel;


class Output
{
 public:
	/**
	 * @brief Implements the singleton pattern. Initialize the output library
	 *        on first use and retrieve the same instance afterwards.
	 * @return the unique Output instance of the application
	 */
	static std::shared_ptr<Output> Get();

	~Output();

	/**
	 * @brief Check if output is initialized.
	 *
	 * @return True is it is initialized, false otherwise.
	 */
	inline bool isInit() { return true; }

	/**
	* @brief Set this entity name to figure out who sent which logs and probes
	*/
	inline void setEntityName(const std::string& name) { entityName = name; }

	/**
	* @brief Get this entity name
	*/
	std::string getEntityName() const;

	/**
	 * @brief Register a probe in the output library
	 *
	 * @param name            The probe full name (section.subsection.name)
	 * @param enabled         Whether the probe is enabled by default
	 * @param sample_type_t   The sample type
	 *
	 * @return the probe object
	 **/
	template<typename T>
	std::shared_ptr<Probe<T>> registerProbe(const std::string& name, bool enabled, sample_type_t type);

	/**
	 * @brief Register a probe in the output library
	 *
	 * @param name            The probe full name (section.subsection.name)
	 * @param unit            The probe unit
	 * @param enabled         Whether the probe is enabled by default
	 * @param sample_type_t   The sample type
	 *
	 * @return the probe object
	 **/
	template<typename T>
	std::shared_ptr<Probe<T>> registerProbe(const std::string& name, const std::string& unit, bool enabled, sample_type_t type);

	/**
	 * @brief Register an event in the output library
	 *
	 * @param identifier   The event name
	 *
	 * @return the event object
	 **/
	std::shared_ptr<OutputEvent> registerEvent(const std::string& identifier);

	/**
	 * @brief Register a log with the level Warning in the output library
	 *
	 * @param display_level The minimum display level
	 * @param name          The log name
	 *
	 * @return the log object
	 **/
	std::shared_ptr<OutputLog> registerLog(log_level_t display_level, const std::string& name);

	/**
	 * @brief Set the probe state
	 *
	 * @param path      full name of a unit or a probe
	 * @param enabled   Whether the probe is enabled or not
	 */
	void setProbeState(const std::string& path, bool enabled);

	/**
	 * @brief Set the log level
	 *
	 * @param path    full name of a unit or a log
	 * @param level   The log level
	 */
	void setLogLevel(const std::string& path, log_level_t level);

	/**
	 * @brief Finalize the output library configuration.
	 *
	 * @warning Needs to be called after registering probes or they
	 *          wont send anything. Must also be called after each
	 *          reconfiguration.
	 **/
	void finalizeConfiguration(void);

	/**
	 * @brief Configure the output library to use file-based logs and probes
	 *
	 * @param folder      The path to store produced files in
	 * @param entityName  The name of the entity that will be part of the final files names
	 * @return            Whether or not the configuration was successful
	 **/
	bool configureLocalOutput(const std::string& folder);

	/**
	 * @brief Configure the output library to use UDP socket-based logs and probes
	 *
	 * @param address   Address of the remote host listening for messages
	 * @param statsPort Port used by the remote host to listen for probes
	 * @param logsPort  Port used by the remote host to listen for logs
	 * @return          Whether or not the configuration was successful
	 **/
	bool configureRemoteOutput(const std::string& address,
	                           unsigned short statsPort,
	                           unsigned short logsPort);

	/**
	 * @brief Configure the output library to use the stderr stream for logs
	 *
	 * @return          Whether or not the configuration was successful
	 */
	bool configureTerminalOutput();

	/**
	 * @brief Send all probes which got new values sinces the last call.
	 **/
	void sendProbes(void);

	/**
	 * @brief Sent a message (with no level specified) with the specified
	 *        message format
	 *
	 * @param log_level   The log level to send
	 * @param msg_format  The message format
	 **/
	template<typename ... Args>
	void sendLog(log_level_t log_level, char const * const msg_format, Args const & ... args);

	/**
	 * @brief Adjust the output log display level
	 *
	 * @param level  the new display level
	 */
	void setDisplayLevel(log_level_t level);

	/**
	 * @brief Set the log levels as defined in the configuration
	 *
	 * @param levels    The log levels defined in configuration
	 */
	void setLevels(const std::map<std::string, log_level_t> &levels);

 private:
	Output();
	void registerProbe(const std::string& name, std::shared_ptr<BaseProbe> probe);

	std::string entityName;

	class OutputSection;
	std::shared_ptr<OutputSection> getOrCreateSection(const std::vector<std::string>& names);

	OutputMutex lock;
	std::shared_ptr<OutputSection> root;
	std::shared_ptr<OutputLog> privateLog;
	std::shared_ptr<OutputLog> defaultLog;
	std::vector<std::shared_ptr<BaseProbe>> enabledProbes;
	std::vector<std::shared_ptr<LogHandler>> logHandlers;
	std::vector<std::shared_ptr<StatHandler>> probeHandlers;

	std::shared_ptr<OutputDesiredLogLevel> desiredLogLevels;
};


template<typename T>
std::shared_ptr<Probe<T>> Output::registerProbe(const std::string& name, bool enabled, sample_type_t type)
{
	return registerProbe<T>(name, "", enabled, type);
}


template<>
std::shared_ptr<Probe<int32_t>> Output::registerProbe(const std::string& name,
                                                      const std::string& unit,
                                                      bool enabled,
                                                      sample_type_t type);


template<>
std::shared_ptr<Probe<float>> Output::registerProbe(const std::string& name,
                                                    const std::string& unit,
                                                    bool enabled,
                                                    sample_type_t type);


template<>
std::shared_ptr<Probe<double>> Output::registerProbe(const std::string& name,
                                                     const std::string& unit,
                                                     bool enabled,
                                                     sample_type_t type);


template<typename... Args>
void Output::sendLog(log_level_t log_level, char const * const msg_format, Args const & ... args)
{
	defaultLog->sendLog(log_level, msg_format, args...);
}


#endif
