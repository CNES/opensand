/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ErrorController class implements the error controller 
    @HISTORY :
    03-02-24 : Creation
*/
/*--------------------------------------------------------------------------*/

#ifndef _ERROR_CONTROLLER_INTERFACE_H
#   define _ERROR_CONTROLLER_INTERFACE_H

#   include "Types_e.h"
#   include <stdio.h>
#   include "GenericPacket_e.h"
#   include "GenericPort_e.h"
#   include "UDPSocket_e.h"
#   include "ErrorOutputFormatter_e.h"
#   include "ComParameters_e.h"
#   include "ErrorDef_e.h"
#   include "CircularBuffer_e.h"

typedef struct
{

	T_ERROR _Error;
	T_BOOL _DisplayFlag;
	T_UINT32 _Pid;
	FILE *_TraceFile;
	T_GENERIC_PORT _ServerErrPort;
	T_GENERIC_PKT *_ReceivedPacket;
	T_UDP_SOCKET _DisplayPort;

	T_BOOL _simuIsRunning;		  /* the simulation is running */

	T_ERROR_OUTPUT_FORMATTER _OutputFormat;

	T_COM_PARAMETERS _ComParams;

	T_ERRORS_DEF _ErrorsDefinition;

} T_ERR_CTRL;


/*  @ROLE    : This function starts controller's interface
    @RETURN  : Error code */
T_ERROR startErrorControllerInterface(T_INT32 argc, T_CHAR * argv[]);


/*  @ROLE    : This function intialises Error Controller process
    @RETURN  : Error code */
T_ERROR ERR_CTRL_Init(
								/*  INOUT  */ T_ERR_CTRL * ptr_this,
/*  IN     */ T_BOOL display);


/*  @ROLE    : This function intialises Error Controller for current simulation
    @RETURN  : Error code */
T_ERROR ERR_CTRL_InitSimulation(
/*  IN     */ T_ERR_CTRL * ptr_this);


/*  @ROLE    : This function sets Error Controller in a proper state 
               at the end of current simulation
    @RETURN  : Error code */
T_ERROR ERR_CTRL_EndSimulation(
											/* INOUT */ T_ERR_CTRL * ptr_this,
											/* IN    */ T_BOOL storeError);


/*  @ROLE    : This function stops Error controller properly.
    @RETURN  : Error code */
T_ERROR ERR_CTRL_Terminate(
/*  IN     */ T_ERR_CTRL * ptr_this);


/*  @ROLE    : This function writes the error message into log file
               and, if expected, sends it to display.
    @RETURN  : Error code */
T_ERROR ERR_CTRL_SendTrace(
/*  IN     */ T_ERR_CTRL * ptr_this,
/*  IN     */ T_ELT_GEN_PKT * eltGenPkt);

/*  @ROLE    : This function creates error messages and writes them to log file
    @RETURN  : Error code */
T_ERROR ERR_CTRL_DoPacket(
/*  IN     */ T_ERR_CTRL * ptr_this);


#endif /* _ERROR_CONTROLLER_INTERFACE_H */
