/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 CNES
 * Copyright © 2019 TAS
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
 * @file PhysicalLayerPlugin.h
 * @brief Plugins for ISL: IslDelay models
 */

#ifndef ISL_PLUGIN_H
#define ISL_PLUGIN_H


#include <memory>
#include <mutex>

#include <opensand_rt/Data.h>

#include "OpenSandCore.h"
#include "OpenSandPlugin.h"


class OutputLog;


/**
 * @class IslDelay
 * @brief IslDelay
 */
class IslDelayPlugin: public OpenSandPlugin
{
protected:
	/* Output log */
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_delay;

	/* The current delay */
	time_ms_t delay;

	/* satdelay refreshing period */
	time_ms_t refresh_period;

private:
	/* Mutex to prevent concurrent access to delay */
	mutable std::mutex delay_mutex;

public:
	/**
	* @brief IslDelayPlugin constructor
	*/
	IslDelayPlugin();

	/**
	* @brief IslDelayPlugin destructor
	*/
	virtual ~IslDelayPlugin();

	/**
	* @brief initialize the sat delay model
	*
	* @return true on success, false otherwise
	*/
	virtual bool init() = 0;

	/**
	* @brief Get the model current sat delay
	*/
	time_ms_t getSatDelay() const;

	/**
	* @brief Set the sat delay model current delay
	*
	* @param the current delay
	*/
	void setSatDelay(time_ms_t delay);

	/**
	* @brief get the refresh period
	*/
	time_ms_t getRefreshPeriod() const;

	/**
	* @brief update the sat delay model current delay
	*
	* @return true on success, false otherwise
	*/
	virtual bool updateIslDelay() = 0;

	/**
	* @brief Get the largest possible delay (needed to estimate timeouts)
	* @param the delay
	* @return true on succes
	*/
	virtual bool getMaxDelay(time_ms_t &delay) const = 0;
};


#endif
