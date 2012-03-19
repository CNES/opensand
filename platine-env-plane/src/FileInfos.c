/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The FileInfos class implements method to get filename 
               for configuration files 
    @HISTORY :
    03-02-27 : Creation
    03-03-25 : Update to match coding rules (Philippe LOPEZ)
    03-09-25 : DB Add C_TERRESTRIAL_VPVC_FILE and C_MULTICAST_FILE cases for NAT
    03-10-10 : Add PEA component files cases (GM)
*/
/*--------------------------------------------------------------------------*/

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
