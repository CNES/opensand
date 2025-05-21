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
 * @brief Plugins for Physical Layer Minimal conditions,
 *        Error insertion, Attenuation and SatDelay models
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <joaquin.muguerza@toulouse.viveris.com>
 */

#ifndef PHYSICAL_LAYER_PLUGIN_H
#define PHYSICAL_LAYER_PLUGIN_H


#include <memory>
#include <mutex>

#include <opensand_rt/Data.h>

#include "OpenSandCore.h"
#include "OpenSandPlugin.h"


class OutputLog;
enum class EmulatedMessageType: uint8_t;


/**
 * @class AttenuationModel
 * @brief AttenuationModel
 */
class AttenuationModelPlugin: public OpenSandPlugin
{
protected:
	/* Output log */
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_attenuation;

	/* The model current attenuation */
	double attenuation;

	/* channel refreshing period */
	time_ms_t refresh_period;

public:
	/**
	 * @brief AttenuationModelPlugin constructor
	 */
	AttenuationModelPlugin();

	/**
	 * @brief AttenuationModelPlugin destructor
	 */
	virtual ~AttenuationModelPlugin();

	/**
	 * @brief initialize the attenuation model
	 *
	 * @param refresh_period_ms the attenuation refreshing period
	 * @param link        the link
	 * @return true on success, false otherwise
	 */
	virtual bool init(time_ms_t refresh_period, std::string link) = 0;

	/**
	 * @brief Get the model current attenuation
	 */
	double getAttenuation() const;

	/**
	 * @brief Set the attenuation model current attenuation
	 *
	 * @param attenuation the model attenuation
	 */
	void setAttenuation(double attenuation);

	/**
	 * @brief update the attenuation model current attenuation
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool updateAttenuationModel() = 0;
};


/**
 * @class Minimal Condition
 * @brief Minimal Condition only used on downlink
 */
class MinimalConditionPlugin: public OpenSandPlugin
{
protected:
	/* Output log */
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_minimal;

	/// MinimalCondition C/N in clear sky conditions
	double minimal_cn;

public:
	/**
	 * @brief MinimalConditionPlugin constructor
	 */
	MinimalConditionPlugin();

	/**
	 * @brief MinimalConditionPlugin destructor
	 */
	virtual ~MinimalConditionPlugin();

	/**
	 * @brief initialize the minimal condition
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool init(void) = 0;

	/**
	 * @brief Set the minimalCondition current Carrier to Noise ratio
	 *        according to time
	 *
	 * @param time the current time
	 */
	virtual double getMinimalCN() const;

	/**
	 * @brief Updates Thresold when a msg arrives to Channel
	 *
	 * @param modcod_id  The MODCOD id carried by the frame
	 * @param message_type  The frame type
	 * @return true on success, false otherwise
	 */
	virtual bool updateThreshold(uint8_t modcod_id, EmulatedMessageType message_type) = 0;
};

/**
* @class ErrorInsertion
* @brief ErrorInsertion
*/
class ErrorInsertionPlugin: public OpenSandPlugin 
{
public:
	/**
	 * @brief ErrorInsertionPlugin constructor
	 */
	ErrorInsertionPlugin();

	/**
	 * @brief ErrorInsertionPlugin destructor
	 */
	virtual ~ErrorInsertionPlugin();

	/**
	 * @brief initialize the error insertion
	 *
	 * @return true on success, false otherwise
	 */
	virtual bool init() = 0;

	/**
	 * @brief Determine if a Packet shall be corrupted or not depending on
	 *        the attenuationModel conditions 
	 *
	 * @param cn_total       The total C/N of the link
	 * @param threshold_qef  The minimal C/N of the link
	 *
	 * @return true if it must be corrupted, false otherwise 
	 */
	virtual bool isToBeModifiedPacket(double cn_total,
	                                  double threshold_qef) = 0;

	/**
	 * @brief Corrupt a packet with error bits 
	 *
	 * @param payload the payload to the frame that should be modified 
	 * @return true if DVB header should be tagged as corrupted,
	 *         false otherwise
	 *         If packet is modified by the function but should be forwarded
	 *         to other layers return false else it will be discarded
	 */
	virtual bool modifyPacket(const Rt::Data &payload) = 0;

protected:
	/* Output log */
	std::shared_ptr<OutputLog> log_init;
	std::shared_ptr<OutputLog> log_error;
};


/**
 * @class SatDelay
 * @brief SatDelay
 */
class SatDelayPlugin: public OpenSandPlugin
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
	* @brief SatDelayPlugin constructor
	*/
	SatDelayPlugin();

	/**
	* @brief SatDelayPlugin destructor
	*/
	virtual ~SatDelayPlugin();

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
	virtual bool updateSatDelay() = 0;

	/**
	* @brief Get the largest possible delay (needed to estimate timeouts)
	* @param the delay
	* @return true on succes
	*/
	virtual bool getMaxDelay(time_ms_t &delay) const = 0;
};


#endif
