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
 * @file BlockDvb.cpp
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include "BlockDvb.h"
#include "BBFrame.h"
#include "Sac.h"
#include "Ttp.h"

#include "Plugin.h"
#include "DvbS2Std.h"
#include "SimpleEncapPlugin.h"
#include "OpenSandModelConf.h"

#include <opensand_output/Output.h>


BlockDvb::BlockDvb()
{
	auto output = Output::Get();
	// register static logs
	BBFrame::bbframe_log = output->registerLog(LEVEL_WARNING, "Dvb.Net.BBFrame");
	Sac::sac_log = output->registerLog(LEVEL_WARNING, "Dvb.SAC");
	Ttp::ttp_log = output->registerLog(LEVEL_WARNING, "Dvb.TTP");
}


void BlockDvb::generateConfiguration()
{
	Plugin::generatePluginsConfiguration(nullptr,
	                                     PluginType::Encapsulation,
	                                     "encapsulation_scheme",
	                                     "Encapsulation Scheme");
}

