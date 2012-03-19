/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ProbeHolder class provides the Probe consolidation buffer
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/
#ifndef ProbeHolder_e
#   define ProbeHolder_e


/* SYSTEM RESOURCES */
#   include <stdio.h>
#   include <math.h>

/* PROJECT RESOURCES */
#   include "Types_e.h"
#   include "Error_e.h"
#   include "CircularBuffer_e.h"
#   include "DominoConstants_e.h"
#   include "ProbesDef_e.h"
#   include "ErrorAgent_e.h"

typedef enum
{
	C_PROBE_VALUE_EMPTY = 0,
	C_PROBE_VALUE_CHANGE,
	C_PROBE_VALUE_UPDATED
} T_PROBE_VALUE_CHANGE;

typedef struct
{
	T_UINT32 _intValue;			  /* int element value */
	T_FLOAT _floatValue;			  /* float element value */
	T_UINT8 _probeId;				  /* probe id */
	T_UINT16 _index;				  /* index number */
	T_UINT32 _valueNumber;		  /* the number of value */
	T_PROBE_VALUE_CHANGE _valueChange;	/* the value is modifed */
	T_CIRCULAR_BUFFER _buffer;	  /* the sliding buffer */
} T_PROBE_VALUE;

typedef struct
{
	T_BOOL _activate;				  /* the activation flag */
	T_BOOL _displayFlag;
	T_UINT8 _categoryId;			  /* category  Id of element */
	T_UINT8 _type;					  /* O->INT / 1->FLOAT */
	T_UINT16 _nbLabels;			  /* the label number */
	T_UINT32 _frameNb;			  /* the last frame number */
	T_PROB_AGG _aggregationMode;
	T_PROB_ANA _analysisOperator;
	T_INT32 _operatorParameter;
	FILE *_file;					  /* is only used by the Probe controller to store statistic */
} T_PROBE_INFO;

typedef struct
{
	T_BOOL _statIsActivated;	  /* indicates if the stat activation file exists */
	T_BOOL _controlerConf;		  /* indicates if the ProbeHolder */
	/* is used by the probe controller */
	T_UINT32 _startFrame;
	T_UINT32 _stopFrame;
	T_UINT32 _samplingPeriod;
	T_UINT32 _displayFrame;		  /* the displayed frame  */
	T_UINT32 _lastFrame;			  /* the last frame where the log has been performed */
	T_PROBE_INFO _probeInfo[C_PROB_MAX_STAT_NUMBER + 1];	/* index 0 never used */
	T_UINT8 _nbStat;				  /* the statistic number */
	T_PROBE_VALUE *_ptr_probeValue[C_PROB_MAX_STAT_NUMBER + 1];
} T_PROBE_HOLDER;


/*  @ROLE    : This function initialises the probe holder
    @RETURN  : Error code */
extern T_ERROR PROBE_HOLDER_Init(
											  /* INOUT */ T_PROBE_HOLDER * ptr_this,
											  /* IN    */ T_PROBES_DEF * ptr_probesDef,
											  /* IN    */ T_COMPONENT_TYPE componentType,
											  /* IN    */ T_UINT16 simReference,
											  /* IN    */ T_UINT16 simRun,
											  /* IN    */ T_BOOL controlerConf,
											  /* IN    */ T_ERROR_AGENT * ptr_errorAgent);


/*  @ROLE    : This function terminates the probe holder
    @RETURN  : Error code */
extern T_ERROR PROBE_HOLDER_Terminate(
													 /* INOUT */ T_PROBE_HOLDER * ptr_this);


/*  @ROLE    : implements the MIN operator */
#   define OPERATOR_MIN(value,newValue,cmpt) \
  (((cmpt == 0) || (newValue < value)) ?  newValue: value)

/*  @ROLE    : implements the MAX operator */
#   define OPERATOR_MAX(value,newValue,cmpt) \
  (((cmpt == 0) || (newValue > value)) ?  newValue: value)

/*  @ROLE    : implements the MEAN operator */
#   define ANALYSIS_MEAN(value,newValue,oldValue,cmpt) \
{ \
  oldValue = value; \
  value += newValue; \
  /* check overflow */ \
  if(oldValue > value) { \
    value = newValue; \
    cmpt = 0; \
  } \
}
#   define OPERATOR_MEAN(value,newValue) \
  value += newValue;
#   define OPERATOR_END_MEAN_INT(value,cmpt) \
  value = (T_UINT32) llrint((T_DOUBLE)value / (T_DOUBLE)cmpt);
#   define OPERATOR_END_MEAN_FLOAT(value,cmpt) \
  value = (T_FLOAT)((T_FLOAT)value / (T_FLOAT)cmpt);

/*  @ROLE    : implements the LAST operator */
#   define OPERATOR_LAST(value,newValue) \
  value = newValue;

/*  @ROLE    : implements the MIN analysis */
#   define ANALYSIS_SLIDING_COMPARE(ptr_buffer,ptr_value,value,forIndex,cast,operator) \
{ \
  CIRCULAR_BUFFER_GetFirstReadBuffer(ptr_buffer,(T_BUFFER*)&ptr_value); \
  value = *(cast*)ptr_value; \
  for(forIndex=0;forIndex<(CIRCULAR_BUFFER_GetEltNumber(ptr_buffer)-1);forIndex++) { \
    CIRCULAR_BUFFER_GetPrevReadBuffer(ptr_buffer,forIndex,(T_BUFFER*)&ptr_value); \
    if(*(cast*)ptr_value operator value) \
      value = *(cast*)ptr_value; \
  } \
}

#   define ANALYSIS_SLIDING_MIN(ptr_buffer,ptr_value,value,forIndex,cast) \
  ANALYSIS_SLIDING_COMPARE(ptr_buffer,ptr_value,value,forIndex,cast,<);

#   define ANALYSIS_SLIDING_MAX(ptr_buffer,ptr_value,value,forIndex,cast) \
  ANALYSIS_SLIDING_COMPARE(ptr_buffer,ptr_value,value,forIndex,cast,>);

#endif /* ProbeHolder_e */
