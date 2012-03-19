#ifndef ProbesActivation_e
#   define ProbesActivation_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ProbesActivation class implements the reading of 
               statistics activation configuration file 
    @HISTORY :
    03-02-26 : Creation
    03-10-17 : Add XML data (GM)
*/
/*--------------------------------------------------------------------------*/

#   include "EnumCouple_e.h"
#   include "Error_e.h"
#   include "DominoConstants_e.h"
#   include "ProbesDef_e.h"


/********************/
/*     CONSTANTS    */
/********************/
#   define C_MAX_ACTIVATED_PROBE     32/* Max number of active statistics probed */

/********************/
/* ENUM DEFINITIONS */
/********************/


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/
typedef struct
{										  /* LEVEL 2 */
	T_PROBE_DEF _Statistic;
	T_PROB_AGG _AggregationMode;
	T_INT32 _DisplayFlag;
	T_PROB_ANA _AnalysisOperator;
	T_INT32 _OperatorParameter;
} T_ACTIVATED_PROBE;


typedef struct
{										  /* LEVEL 1 */
	T_UINT32 _nbActivatedProbes;
	T_ACTIVATED_PROBE _Probe[C_MAX_ACTIVATED_PROBE];

	T_ENUM_COUPLE C_PROB_AGGREGATE_choices[C_AGG_NB + 1];
	T_ENUM_COUPLE C_PROB_ANALYSIS_choices[C_ANA_NB + 1];
} T_ACTIVATED_PROBE_TAB;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _StartFrame;
	T_UINT32 _StopFrame;
	T_UINT32 _SamplingPeriod;
	T_ACTIVATED_PROBE_TAB _ActivatedProbes;

	T_ENUM_COUPLE C_PROBES_ACTIVATION_ComponentChoices[C_COMP_MAX + 1];
} T_PROBES_ACTIVATION;


T_ERROR PROBES_ACTIVATION_ReadConfigFile(
														 /* INOUT */ T_PROBES_ACTIVATION *
														 ptr_this,
														 /* IN    */
														 T_COMPONENT_TYPE ComponentLabel);


T_ERROR PROBES_ACTIVATION_UpdateDefinition(
															/* INOUT */ T_PROBES_ACTIVATION *
															ptr_this,
															/* IN    */
															T_PROBES_DEF * ptr_probesDef);

#endif /* ProbesActivation_e */
