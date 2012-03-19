#ifndef FileInfos_e
#   define FileInfos_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The FileInfos class implements method to get filename 
               for configuration files 
    @HISTORY :
    03-02-27 : Creation
*/
/*--------------------------------------------------------------------------*/

#   include "Types_e.h"
#   include "Error_e.h"

#   define C_FILE_NAME_MAX_CAR_NB 100

/* This method may return a complete filename or a formatting string requiring further processing, 
    ex: "NCC_%d_param.conf"
    The caller shall then use sprintf(...) to obtain the complete filename. */
T_STRING FILE_INFOS_GetFileName(T_FILE_INFOS_INDEX file_index);

#endif
