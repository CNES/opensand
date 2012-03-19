/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe RENAUDY - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    :  
    @HISTORY :
*/
/*--------------------------------------------------------------------------*/


#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include "ErrorController_e.h"
#include "ErrorControllerInterface_e.h"

int main(int argc, char *argv[])
{
	startErrorControllerInterface(argc, argv);
	return (0);
}
