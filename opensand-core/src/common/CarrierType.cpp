/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @file CarrierType.cpp
 * @brief Types and utilities to deal with sat carrier ids
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 */


#include "CarrierType.h"


uint16_t operator +(uint16_t lhs, CarrierType rhs)
{
	return lhs + to_underlying(rhs);
}


CarrierType extractCarrierType(uint16_t carrier_id)
{
	uint8_t id = carrier_id % 10;
	return to_enum<CarrierType>(id);
}


bool isControlCarrier(CarrierType c)
{
	switch (c)
	{
		case CarrierType::LOGON_IN:
		case CarrierType::LOGON_OUT:
		case CarrierType::CTRL_IN_ST:
		case CarrierType::CTRL_OUT_GW:
		case CarrierType::CTRL_IN_GW:
		case CarrierType::CTRL_OUT_ST:
			return true;
		default:
			return false;
	}
}


bool isDataCarrier(CarrierType c)
{
	switch (c)
	{
		case CarrierType::DATA_IN_ST:
		case CarrierType::DATA_OUT_GW:
		case CarrierType::DATA_IN_GW:
		case CarrierType::DATA_OUT_ST:
			return true;
		default:
			return false;
	}
}


bool isInputCarrier(CarrierType c)
{
	switch (c)
	{
		case CarrierType::LOGON_IN:
		case CarrierType::CTRL_IN_ST:
		case CarrierType::CTRL_IN_GW:
		case CarrierType::DATA_IN_ST:
		case CarrierType::DATA_IN_GW:
			return true;
		default:
			return false;
	}
}


bool isOutputCarrier(CarrierType c)
{
	switch (c)
	{
		case CarrierType::LOGON_OUT:
		case CarrierType::CTRL_OUT_GW:
		case CarrierType::CTRL_OUT_ST:
		case CarrierType::DATA_OUT_GW:
		case CarrierType::DATA_OUT_ST:
			return true;
		default:
			return false;
	}
}


bool isTerminalCarrier(CarrierType c)
{
	switch (c)
	{
		case CarrierType::LOGON_IN:
		case CarrierType::CTRL_IN_ST:
		case CarrierType::CTRL_OUT_ST:
		case CarrierType::DATA_IN_ST:
		case CarrierType::DATA_OUT_ST:
			return true;
		default:
			return false;
	}
}


bool isGatewayCarrier(CarrierType c)
{
	switch (c)
	{
		case CarrierType::LOGON_OUT:
		case CarrierType::CTRL_OUT_GW:
		case CarrierType::CTRL_IN_GW:
		case CarrierType::DATA_OUT_GW:
		case CarrierType::DATA_IN_GW:
			return true;
		default:
			return false;
	}
}
