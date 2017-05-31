/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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

/*
 * @file DamaCtrlRcs.h
 * @brief This library defines DAMA controller interfaces.
 * @author satip6 (Eddy Fromentin)
 */

#ifndef _DAMA_CONTROLLER_RCS_H_
#define _DAMA_CONTROLLER_RCS_H_

#include "DamaCtrlRcsCommon.h"

#include "UnitConverter.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <stdio.h>
#include <math.h>
#include <map>
#include <vector>


/**
 * @class DamaCtrlRcs
 * @brief Define methods to process DAMA request in the NCC
 */
class DamaCtrlRcs: public DamaCtrlRcsCommon
{
 public:

	DamaCtrlRcs(spot_id_t spot, vol_bytes_t packet_length_bytes);
	virtual ~DamaCtrlRcs();

	virtual bool init();

	// Update MODCOD for each terminal
	virtual void updateFmt();

 protected:
	vol_bytes_t packet_length_bytes;
	UnitConverter *converter;  ///< Used to convert from/to KB to encap packets

	/**
	 * @brief  Get the unit converter of a carriers' category.
	 *
	 * @param   category_label  The label of the carriers' category
	 * @return  unit converter if success, null otherwise
	 */
	virtual UnitConverter *getUnitConverter(string category_label);

	// Reset dama
	virtual bool resetDama() = 0;
};

#endif
