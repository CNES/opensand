#ifndef FilePath_e
#   define FilePath_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The FilePath class implements method to get exec path, 
               scenario path and run path for curent simulation. 
    @HISTORY :
    03-02-27 : Creation
*/
/*--------------------------------------------------------------------------*/

#   include "Error_e.h"

#   define C_FILE_PATH_MAX_CARS 256


typedef T_CHAR T_FILE_PATH[C_FILE_PATH_MAX_CARS];


/* Gets the simulation run path. 
    This directory will contain all files needed and produced by a simulation run.  */

T_ERROR FILE_PATH_InitClass();


T_ERROR FILE_PATH_GetConfPath(
										  /*   OUT */ T_STRING ptr_this);

T_ERROR FILE_PATH_GetScenarioPath(
												/*   OUT */ T_STRING ptr_this,
												/* IN    */ T_UINT16 reference);

T_ERROR FILE_PATH_GetRunPath(
										 /*   OUT */ T_STRING ptr_this,
										 /* IN    */ T_UINT16 reference,
										 /* IN    */ T_UINT16 run);

T_ERROR FILE_PATH_GetOutputPath(
											 /*   OUT */ T_STRING ptr_this,
											 /* IN    */ T_UINT16 reference,
											 /* IN    */ T_UINT16 run);

T_ERROR FILE_PATH_Concat(
									/*   OUT */ T_STRING ptr_this,
									/* IN    */ T_STRING file_name);

#endif
