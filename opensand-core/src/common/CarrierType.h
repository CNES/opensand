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
 * @file CarrierType.h
 * @brief Types and utilities to deal with sat carrier ids
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 */


#ifndef CARRIER_TYPE_H
#define CARRIER_TYPE_H


#include "OpenSandCore.h"


enum class CarrierType : uint8_t
{
	LOGON_IN,
	LOGON_OUT,
	CTRL_IN_ST,
	CTRL_OUT_GW,
	CTRL_IN_GW,
	CTRL_OUT_ST,
	DATA_IN_ST,
	DATA_OUT_GW,
	DATA_IN_GW,
	DATA_OUT_ST
};


uint16_t operator +(uint16_t lhs, CarrierType rhs);
CarrierType extractCarrierType(uint16_t carrier_id);
bool isDataCarrier(CarrierType c);
bool isControlCarrier(CarrierType c);
bool isInputCarrier(CarrierType c);
bool isOutputCarrier(CarrierType c);
bool isTerminalCarrier(CarrierType c);
bool isGatewayCarrier(CarrierType c);


#endif /* CARRIER_TYPE_H */
