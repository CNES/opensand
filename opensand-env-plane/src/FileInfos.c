/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file FileInfos.c
 * @author TAS
 * @brief The FileInfos class implements method to get filename for
 *        configuration files
 */

#include <string.h>
#include <stdio.h>
#include "FileInfos_e.h"
#include "Error_e.h"


/* This method may return a complete filename or a formatting string requiring further processing, 
    ex: "NCC_%d_param.conf"
    The caller shall then use sprintf(...) to obtain the complete filename.

  In optimised compilation mode, local variables are de-allocated as soon as the method is
  exited; that's why a local string is not used in this method and strings are returned
  directly using return() function.
*/

T_STRING FILE_INFOS_GetFileName(T_FILE_INFOS_INDEX file_index)
{
	switch (file_index)
	{
	case C_ARCHITECTURE_FILE:
		return ("architecture.conf");

	case C_PROBE_DEF_FILE:
		return ("stat_def_%s.conf");

	case C_PROBE_ACT_FILE:
		return ("stat_conf_%s.conf");

	case C_EVENT_DEF_FILE:
		return ("event_def.conf");

	case C_EVENT_ACT_FILE:
		return ("event_conf.conf");

	case C_ERROR_DEF_FILE:
		return ("error_def.conf");

	case C_MAIN_CONFIG_FILE:
		return ("global_conf.conf");

	case C_RADIO_RESOURCES_FILE:
		return ("radio_resources_conf.conf");

	case C_EVENT_PARAM_FILE:
		return ("event_param.conf");

	case C_ERROR_PARAM_FILE:
		return ("error_param.conf");

	case C_COM_PARAMETERS_FILE:
		return ("com_parameters.conf");

	case C_TRACE_DEF_FILE:
		return ("trace_param.conf");

	default:
		printf
			("ERROR!!! file_index ()%d  not handled : FileInfos.c must not be up to date!!!\n",
			 file_index);
		break;
	}

	return ("no_file");			  /* no config file corresponding */
}
