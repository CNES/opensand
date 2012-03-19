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
#include <netinet/in.h>
#include "ProbeController_e.h"
#include "ProbeControllerInterface_e.h"


int main(int argc, char *argv[])
{
	startProbeControllerInterface(argc, argv);
	return (0);
}
