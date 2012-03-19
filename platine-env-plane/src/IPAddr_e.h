/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The IpAddr class generates "system-formatted" addresses
    @HISTORY :
    03-02-20 : Creation
    04-04-06 : working both with AF_UNIX and AF_INET
*/
/*--------------------------------------------------------------------------*/
#ifndef IPAddr_e
#   define IPAddr_e

/* SYSTEM RESOURCES */
#   include <netinet/in.h>
#   include <sys/un.h>
/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"

#   define C_IP_MAX_LEN              20/* IP address or IP MASK maximum length */

typedef struct
{
	T_CHAR _addr[C_IP_MAX_LEN];
	T_UINT16 _padding;
	T_UINT16 _port;
	T_UINT8 _family;
	struct sockaddr_in _addrInet;
	struct sockaddr_un _addrUnix;
} T_IP_ADDR;


/*  @ROLE    : This function creates "system-formatted" addresses
    @RETURN  : Error code */
extern T_ERROR IP_ADDR_Init(
										/* INOUT */ T_IP_ADDR * ptr_this,
										/* IN    */ T_STRING addr,
										/* IN    */ T_UINT16 port,
										/* IN    */ T_UINT8 family);

/*  @ROLE    : This function terminates a IpAddr class
    @RETURN  : Error code */
extern T_ERROR IP_ADDR_Terminate(
											  /* INOUT */ T_IP_ADDR * ptr_this);

/*  @ROLE    : This function gives the IP "system-formatted" address
    @RETURN  : Error code */
extern T_ERROR IP_ADDR_GetIpAddr(
											  /* INOUT */ T_IP_ADDR * ptr_this,
											  /*   OUT */ T_UINT32 * ptr_addr);

#endif /* IPAddr_e */
