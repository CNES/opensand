#ifndef ProbesDef_e
#   define ProbesDef_e

/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : Philippe LOPEZ - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ProbesDef class implements the reading of 
               statistics definition configuration file 
    @HISTORY :
    03-02-26 : Creation
    03-10-14 : Add XML data (GM)
*/
/*--------------------------------------------------------------------------*/

/********************/
/* SYSTEM RESOURCES */
/********************/
#   include "Error_e.h"
#   include "EnumCouple_e.h"

/*********************/
/* MACRO DEFINITIONS */
/*********************/

/* These limits shall be reconsidered at integration-time  */
/*---------------------------------------------------------*/
#   define C_PROB_DEF_MAX_CAR_NAME         48
#   define C_PROB_DEF_MAX_CAR_UNIT         32
#   define C_PROB_DEF_MAX_CAR_GRAPH_TYPE   16
#   define C_PROB_DEF_MAX_CAR_COMMENT      48
#   define C_PROB_DEF_MAX_CAR_LABEL        32
#   define C_MAX_PROBE_VALUE_NUMBER      1024/* Max number of probe value per socket */
#   define C_PROB_MAX_STAT_NUMBER          50
#   define C_PROB_MAX_LABEL_VALUE         864/* This value is computed by : 
															   3 UL beams * 8 channel types * 3 DL beams * 3 QoS * 4 types of throughput */

typedef T_CHAR T_STAT_LABEL[C_PROB_DEF_MAX_CAR_LABEL];


/*******************/
/* ENUM DEFINITION */
/*******************/

/* Define enum with all types available for statistics */
/*-----------------------------------------------------*/
enum
{
	C_PROBE_TYPE_INT = 0,
	C_PROBE_TYPE_FLOAT = 1,

	C_PROBE_TYPE_NB
};


/*************************/
/* STRUCTURE DEFINITIONS */
/*************************/

typedef struct
{										  /* LEVEL 2 */
	T_UINT32 _nbLabels;
	T_STAT_LABEL _StatLabelValue[C_PROB_MAX_LABEL_VALUE];
} T_STAT_LABEL_TAB;


typedef struct
{										  /* LEVEL 1 */
	T_INT32 _probeId;
	T_CHAR _Name[C_PROB_DEF_MAX_CAR_NAME];
	T_INT32 _Category;
	T_INT32 _Type;
	T_CHAR _Unit[C_PROB_DEF_MAX_CAR_UNIT];
	T_CHAR _Graph_Type[C_PROB_DEF_MAX_CAR_GRAPH_TYPE];
	T_CHAR _Comment[C_PROB_DEF_MAX_CAR_COMMENT];
	T_STAT_LABEL_TAB _StatLabels;

} T_PROBE_DEF;


typedef struct
{										  /* LEVEL 0 */
	T_UINT32 _nbStatistics;
	T_PROBE_DEF _Statistic[C_PROB_MAX_STAT_NUMBER];
	T_ENUM_COUPLE C_PROBES_DEFINITION_ComponentChoices[C_COMP_MAX + 1];
	T_ENUM_COUPLE C_PROBE_TYPE_choices[C_PROBE_TYPE_NB + 1];
} T_PROBES_DEF;


T_ERROR PROBES_DEF_ReadConfigFile(
												/* INOUT */ T_PROBES_DEF * ptr_this,
												/* IN    */
												T_COMPONENT_TYPE ComponentLabel);


#endif /* ProbesDef_e */
