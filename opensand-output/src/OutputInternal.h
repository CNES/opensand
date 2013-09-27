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
#include "Event.h"

#include <opensand_conf/uti_debug.h>

#include <assert.h>
#include <sys/un.h>
#include <vector>


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
	 * @param enabled      Whether the element is enabled
	 * @param min_level    The minimum event level
	 * @param sock_prefix  The socket prefix
	 */
	void init(bool enabled, event_level_t min_level, const char *sock_prefix);

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
	Probe<T> *registerProbe(const std::string &name,
	                        const std::string &unit,
	                        bool enabled, sample_type_t type);

	/**
	 * @brief Register an event for the element
	 *
	 * @param identifier   The event name
	 * @param event_level_t  The event severity
	 *
	 * @return the event object
	 **/
	Event *registerEvent(const std::string &identifier,
	                     event_level_t level);

	/**
	 * @brief Finish the element initialization
	 **/
	bool finishInit(void);

	/**
	 * @brief Send all probes which got new values sinces the last call.
	 **/
	void sendProbes(void);

	/**
	 * @brief Send the specified event with the specified message format.
	 *
	 * @param event       The event
	 * @param msg_format  The message format
	 **/
	void sendEvent(Event *event, const std::string &message);

	/**
	 * @brief Set the probe state
	 *
	 * @param probe_id  The probe ID
	 * @param enabled   Whether the probe is enabled or not
	 */
	void setProbeState(uint8_t probe_id, bool enabled);

	/**
	 * @brief disable all stats
	 */
	void disable();

	/**
	 * @brief Enable output
	 */
	void enable();

	/**
	 * @brief  Send registration for a probe outside initialization
	 *
	 * @param probe  The new probe to register
	 * @return true on success, false otherwise
	 */
	bool sendRegister(BaseProbe *probe);


	/// whether the element is enabled
	bool enabled;

	/// whether the element is in the initializing phase
	bool initializing;

	/// the minimum event level
	event_level_t min_level;

	/// the probes
	std::vector<BaseProbe*> probes;

	/// the events
	std::vector<Event*> events;

	/// the socket for communication with daemon
	int sock;

	/// the timestamp of the initialization
	uint32_t started_time;

	/// the dameon socket address
	sockaddr_un daemon_sock_addr;

	/// the element socket address
	sockaddr_un self_sock_addr;
};

template<typename T>
Probe<T>* OutputInternal::registerProbe(const std::string &name,
                                        const std::string &unit,
                                        bool enabled, sample_type_t type)
{
	if(!this->enabled)
	{
		return NULL;
	}

/*	if(!this->initializing)
	{
		UTI_ERROR("cannot register probe %s outside initialization, exit\n",
		          name.c_str());
		return NULL;
	}*/

	UTI_DEBUG("Registering probe %s with type %d\n", name.c_str(), type);

	uint8_t new_id = this->probes.size();
	Probe<T> *probe = new Probe<T>(new_id, name, unit, enabled, type);
	this->probes.push_back(probe);
	// single registration if process is already started
	if(!this->initializing && !this->sendRegister(probe))
	{
		{
			return NULL;
		}
	}

	return probe;
}

#endif
