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
 * @file AttenuationHandler.h
 * @brief Process the attenuation
 * @author Santiago PENA LUQUE <santiago.penaluque@cnes.fr>
 * @author Aurélien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef ATTENUATION_HANDLER_H
#define ATTENUATION_HANDLER_H


#include <string>

#include "DvbFrame.h"


template<typename> class Probe;
class OutputLog;
class MinimalConditionPlugin;
class ErrorInsertionPlugin;


/**
 * @class AttenuationHandler
 * @brief Process the attenuation
 */
class AttenuationHandler
{
private:
	/** Minimal Conditions (minimun C/N to have QEF communications)
	 *  of global link (i.e. considering the Modcod scheme)
	 */
	MinimalConditionPlugin *minimal_condition_model;

	/// Error Insertion object : defines who error will be introduced
	ErrorInsertionPlugin *error_insertion_model;

	/// Log
	std::shared_ptr<OutputLog> log_channel;

	/// Probes
	std::shared_ptr<Probe<float>> probe_minimal_condition;
	std::shared_ptr<Probe<int>> probe_drops;

public:
	/**
	 * @brief Constructor of the attenuation handler
	 *
	 * @param log_channel   the log output to use during attenuation processing
	 */
	AttenuationHandler(std::shared_ptr<OutputLog> log_channel);

	/**
	 * @brief Destroy the attenuation handler
	 */
	virtual ~AttenuationHandler();

	static void generateConfiguration();

	/**
	 * @brief Initialize the attenuation handler
	 *
	 * @param log_init      the log output to use during initialization
	 *
	 * @return true on success, false otherwise
	 */
	bool initialize(std::shared_ptr<OutputLog> log_init, const std::string &probe_prefix);

	/**
	 * @brief Process the attenuation on a DVB frame with a specific C/N
	 *
	 * @param dvb_frame  the DVB frame
	 * @param total_cn   the specific C/N
	 *
	 * @return true on success, false otherwise
	 */
	bool process(DvbFrame &dvb_frame, double cn_total);
};

#endif
