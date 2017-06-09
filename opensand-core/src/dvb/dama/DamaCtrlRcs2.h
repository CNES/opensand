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
 * @file DamaCtrlRcs2.h
 * @brief This library defines DAMA controller interfaces.
 *
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 */

#ifndef _DAMA_CONTROLLER_RCS2_H_
#define _DAMA_CONTROLLER_RCS2_H_

#include "DamaCtrlRcsCommon.h"

#include "FmtDefinitionTable.h"
#include "TerminalContextDamaRcs.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>

#include <stdio.h>
#include <math.h>
#include <map>
#include <vector>


/**
 * @class DamaCtrlRcs2
 * @brief Define methods to process DAMA request in the NCC
 */
class DamaCtrlRcs2: public DamaCtrlRcsCommon
{
 public:

	DamaCtrlRcs2(spot_id_t spot);
	virtual ~DamaCtrlRcs2();

	/// Reset all terminals allocations
	virtual bool resetTerminalsAllocations();

	// Update MODCOD for each terminal
	virtual void updateFmt();
};


#endif
