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
 * Openbach is free software : you can redistribute it and/or modify it under the
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
 * @file OutputOpenbach.h
 * @brief Class used to hold openbach output library variables and methods.
 * @author Alban Fricot <africot@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
 */


#ifndef _OUTPUT_OPENBACH_H
#define _OUTPUT_OPENBACH_H

#include <opensand_output/OutputLog.h>
#include <opensand_output/OutputEvent.h>
#include <opensand_output/OutputMutex.h>
#include <opensand_output/OutputInternal.h>
#include <assert.h>
#include <sys/un.h>
#include <vector>
#include <map>

using std::vector;
using std::map;

extern "C" OutputInternal* create(const char *entity);

extern "C" void destroy(OutputInternal **object);

/**
 * @class hold openbach output library variables and methods
 */
class OutputOpenbach : public OutputInternal
{
	friend OutputInternal* create(const char *entity);

  public:
	~OutputOpenbach();

  protected:
	OutputOpenbach(const char *entity);

	/**
	 * @brief initialize the output element
	 *
	 * @param enable_collector  Whether the element is enabled
	 * @return true on success, false otherwise
	 */
	bool init(bool enable_collector);

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
	void sendLog(const OutputLog *log, log_level_t log_level,
	             const string &message_text);

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

  private:
	/// entity to forward instance name and id
	string entity;	
	
	/// mapping opensand log level to openbach priority
	static map<log_level_t, int> priority_map; 
};

#endif
