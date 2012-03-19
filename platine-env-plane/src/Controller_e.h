/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Paul LAFARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ExecContext class implements the execution context of template
    @HISTORY :
    03-03-21 : Creation
*/
/*--------------------------------------------------------------------------*/
#ifndef Controller_e
#   define Controller_e

#   include "EnumCouple_e.h"
#   include "DominoConstants_e.h"

/* define correspondance between component names and integer */
/* values used in methods implementation                     */
#   define COMPONENT_CHOICES(name) \
T_ENUM_COUPLE name[C_COMP_MAX+1] \
  = {{"GW", C_COMP_GW}, \
    {"SAT", C_COMP_SAT}, \
    {"ST", C_COMP_ST}, \
    {"AGGREGATE_ST", C_COMP_ST_AGG}, \
    {"OBPC", C_COMP_OBPC}, \
    {"TRAFFIC", C_COMP_TG}, \
    {"PROBE_CONTROLLER", C_COMP_PROBE_CTRL}, \
    {"EVENT_CONTROLLER", C_COMP_EVENT_CTRL}, \
    {"ERROR_CONTROLLER", C_COMP_ERROR_CTRL}, \
    C_ENUM_COUPLE_NULL};


#endif
