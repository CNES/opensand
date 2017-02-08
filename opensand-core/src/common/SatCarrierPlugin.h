/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 CNES
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
 * @file SatCarrierPlugin.h
 * @brief Plugins for SatCarrier Layer (where satellite delay is emulated)
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

#ifndef SAT_CARRIER_PLUGIN_H
#define SAT_CARRIER_PLUGIN_H

#include "OpenSandPlugin.h"
#include "OpenSandCore.h"

#include <opensand_rt/RtMutex.h>
#include <opensand_output/Output.h>
#include <opensand_conf/conf.h>

#include <string>
#include <stdint.h>


using std::string;


/**
 * @class SatDelay
 * @brief SatDelay
 */
class SatDelayPlugin: public OpenSandPlugin
{
 protected:
	/* Output log */
	OutputLog *log_init;
	OutputLog *log_delay;

	/* The current delay */
	time_ms_t delay;

	/* satdelay refreshing period */
	time_ms_t refresh_period_ms;

 private:
	/* Mutex to prevent concurrent access to delay */
	mutable RtMutex delay_mutex;

 public:

	/**
	 * @brief SatDelayPlugin constructor
	 */
	SatDelayPlugin(): 
	    OpenSandPlugin(),
			delay(0),
			refresh_period_ms(1000),
	    delay_mutex("delay")
	{
		this->log_init = Output::registerLog(LEVEL_WARNING, "SatDelay.init");
		this->log_delay = Output::registerLog(LEVEL_WARNING, "SatDelay.Delay");
	};

	/**
	 * @brief SatDelayPlugin destructor
	 */
	virtual ~SatDelayPlugin() {};

	/**
	 * @brief initialize the sat delay model
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool init(ConfigurationList conf) = 0;

	/**
	 * @brief Get the model current sat delay
	 */
	time_ms_t getSatDelay() {
		RtLock lock(this->delay_mutex);
		return this->delay;
	};

	/**
	 * @brief Set the sat delay model current delay
	 *
	 * @param the current delay
	 */
	void setSatDelay(time_ms_t delay)
	{
		RtLock lock(this->delay_mutex);
		this->delay = delay;
	};

	/**
	 * @brief get the refresh period
	 */
	time_ms_t getRefreshPeriod()
	{
		return this->refresh_period_ms;
	};

	/**
	 * @brief update the sat delay model current delay
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool updateSatDelay() = 0;

	/**
	 * @brief Get the largest possible delay (needed to estimate timeouts)
	 * @param the delay
	 * @return true on succes
	 */
	virtual bool getMaxDelay(time_ms_t &delay) = 0;
};

#endif
