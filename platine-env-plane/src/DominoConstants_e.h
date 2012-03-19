#ifndef DominoConstants_e
#   define DominoConstants_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The DominoConstants defines Domino system constants.
    @HISTORY :
    03-01-06 : Creation
    03-09-17 : DB - change C_MAX_CONNECTIONS_NB to C_MAX_ST_CONNECTIONS_NB.
               Add C_MAX_NAT_CONNECTIONS_NB.
    03-09-30 : DB - add C_UBR_P in T_QOS enumeration for NAT purposes.
    03-10-06 : PLo : Specific Domino Constants moved to DominoSystemConstants_e.h
    03-12-04 : DB - add IS_IP_TYPE_D macro for NAT multicast purposes.
    04-05-10 : NC A0095 size of UL and DL buffers 
*/
/*--------------------------------------------------------------------------*/


/* Category types */
typedef enum
{
	C_CAT_INIT = 0,
	C_CAT_END = 1,
	C_CAT_NB
} T_CATEGORY_TYPE;

/* Probe Aggregation modes */
typedef enum
{
	C_AGG_MIN = 0,
	C_AGG_MAX,
	C_AGG_MEAN,
	C_AGG_LAST,
	C_AGG_NB
} T_PROB_AGG;

/* Probe Analysis operator */
typedef enum
{
	C_ANA_RAW = 0,
	C_ANA_MIN,
	C_ANA_MAX,
	C_ANA_MEAN,
	C_ANA_STANDARD_DEV,
	C_ANA_SLIDING_MIN,
	C_ANA_SLIDING_MAX,
	C_ANA_SLIDING_MEAN,
	C_ANA_NB
} T_PROB_ANA;


/*
 * Macros for fast min/max.
 */
#   ifndef MIN
#      define MIN(a,b) (((a)<(b))?(a):(b))
#   endif
		 /* MIN */

#   ifndef MAX
#      define MAX(a,b) (((a)>(b))?(a):(b))
#   endif
		 /* MAX */


/* Component types */
typedef enum
{
	C_COMP_GW = 0,
	C_COMP_SAT = 1,
	C_COMP_ST = 2,
	C_COMP_ST_AGG = 3,
	C_COMP_OBPC = 4,
	C_COMP_TG = 5,
	C_COMP_PROBE_CTRL = 6,
	C_COMP_EVENT_CTRL = 7,
	C_COMP_ERROR_CTRL = 8,
	C_COMP_MAX = 9						  /* number of values in enum type */
} T_COMPONENT_TYPE;


#endif /* DominoConstants_e */
