#ifndef ComParameters_e
#   define ComParameters_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ComParameters class implements the reading of 
               communication parameters configuration file 
    @HISTORY :
    03-02-26 : Creation
    03-10-17 : Add XML data (GM)
    04-04-06 : working both with AF_UNIX and AF_INET
*/
/*--------------------------------------------------------------------------*/
#   include <sys/socket.h>

/*********************/
/* PROJECT RESOURCES */
/*********************/
#   include "EnumCouple_e.h"
#   include "Error_e.h"
#   include "DominoConstants_e.h"
#   include "IPAddr_e.h"
#   include "FileReader_e.h"

#   define C_NB_DISPLAY_PORTS   3
										  /* Number of ports used for display */
#   define C_MAX_HOSTNAME_SIZE 50
										  /* Hostname maximum size */

/********************/
/* ENUM DEFINITIONS */
/********************/
typedef enum
{
	C_INET = AF_INET,				  // value 1
	C_UNIX = AF_UNIX,				  // value 2
	C_PORT_FAMILY_MAX = 3		  // value 3
} T_PORT_FAMILY_NUMBERS;


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/
typedef struct
{										  /* LEVEL 2 or LEVEL 3 */
	T_PORT_FAMILY_NUMBERS _Family;
	T_IP_ADDR _IpAddress;
} T_COM_STRUCT;


typedef struct
{										  /* LEVEL 1 */
	T_COM_STRUCT _ErrorController;
	T_COM_STRUCT _EventController;
	T_COM_STRUCT _ProbeController;
} T_CONTROLLERS_PORTS;


typedef struct
{
	T_COM_STRUCT _EventDisplay;
	T_COM_STRUCT _ErrorDisplay;
	T_COM_STRUCT _ProbeDisplay;
} T_DISPLAY_PORTS;

typedef struct
{										  /* LEVEL 0 */
	T_CONTROLLERS_PORTS _ControllersPorts;
	T_DISPLAY_PORTS _DisplayPorts;
	T_ENUM_COUPLE C_PORT_FAMILY_choices[C_PORT_FAMILY_MAX];
} T_COM_PARAMETERS;


T_ERROR COM_PARAMETERS_ReadConfigFile(
													 /* INOUT */ T_COM_PARAMETERS *
													 ptr_this);


T_ERROR COM_PARAMETERS_PrintConfigFile(
													  /* INOUT */ T_COM_PARAMETERS *
													  ptr_this);


#endif /* ComParameters_e */
