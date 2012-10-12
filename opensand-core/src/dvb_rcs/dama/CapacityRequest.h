/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file    CapacityRequest.h
 * @brief   Represent a CR (Capacity Request)
 * @author  Audric Schiltknecht / Viveris Technologies
 */

#ifndef _CAPACITY_REQUEST_H_
#define _CAPACITY_REQUEST_H_

// TODO: add other members of a T_DVB_SAC_CR_INFO


/**
 * @class CapacityRequest
 * @brief Represent a CR
 */
class CapacityRequest
{
 public:

	CapacityRequest(unsigned int tal_id,
	                int priority,
					int request_type,
					double request_value_kb) :
		tal_id(tal_id),
		priority(priority),
		request_type(request_type),
		request_value_kb(request_value_kb)
	{};

	/**
	 * @brief   Get the terminal Id.
	 *
	 * @return  terminal Id.
	 */
	unsigned int getTerminalId() const
	{
		return this->tal_id;
	}
	
	/**
	 * @brief   Get priority of capacity request.
	 *
	 * @return  priority of the CR.
	 */
	int getPriority() const
	{
		return this->priority;
	}

	/**
	 * @brief   Get request type.
	 *
	 * @return  request type.
	 */
	int getType() const
	{
		return this->request_type;
	}


	/**
	 * @brief   Get CR value (in kb).
	 *
	 * @return  CR value (in kb).
	 */
	double getValue() const
	{
		return this->request_value_kb;
	}

 private:

	unsigned int tal_id;
	int priority;
	int request_type;
	double request_value_kb;
};

#endif

