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

#include "FileReader_e.h"
#include "ProbesActivation_e.h"
#include "DominoConstants_e.h"
#include "FilePath_e.h"
#include "Trace_e.h"
#include "FileInfos_e.h"
#include "EnumParser_e.h"

#include <string.h>
#include <ctype.h>

/*********************/
/* MACRO DEFINITIONS */
/*********************/
#define C_MAX_SAMPLING_PERIOD   100
#define C_PROBE_MAX_OPERATOR_PARAMETER 20


T_ERROR PROBES_ACTIVATION_Init(
											/* INOUT */ T_PROBES_ACTIVATION * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;

	memset(ptr_this, 0, sizeof(T_PROBES_ACTIVATION));

	/* Initialise Enum structures associating Character strings with Integer values */
  /*------------------------------------------------------------------------------*/
	strcpy(ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[0]._StrValue,
			 "MIN");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[1]._StrValue,
			 "MAX");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[2]._StrValue,
			 "MEAN");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[3]._StrValue,
			 "LAST");
	ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[4]._StrValue[0] = '\0';

	ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[0]._IntValue = C_AGG_MIN;
	ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[1]._IntValue = C_AGG_MAX;
	ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[2]._IntValue =
		C_AGG_MEAN;
	ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[3]._IntValue =
		C_AGG_LAST;
	ptr_this->_ActivatedProbes.C_PROB_AGGREGATE_choices[4]._IntValue = 0;

	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[0]._StrValue,
			 "RAW");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[1]._StrValue,
			 "MIN");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[2]._StrValue,
			 "MAX");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[3]._StrValue,
			 "MEAN");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[4]._StrValue,
			 "STANDARD_DEVIATION");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[5]._StrValue,
			 "SLIDING_MIN");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[6]._StrValue,
			 "SLIDING_MAX");
	strcpy(ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[7]._StrValue,
			 "SLIDING_MEAN");
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[8]._StrValue[0] = '\0';

	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[0]._IntValue = C_ANA_RAW;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[1]._IntValue = C_ANA_MIN;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[2]._IntValue = C_ANA_MAX;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[3]._IntValue = C_ANA_MEAN;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[4]._IntValue =
		C_ANA_STANDARD_DEV;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[5]._IntValue =
		C_ANA_SLIDING_MIN;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[6]._IntValue =
		C_ANA_SLIDING_MAX;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[7]._IntValue =
		C_ANA_SLIDING_MEAN;
	ptr_this->_ActivatedProbes.C_PROB_ANALYSIS_choices[8]._IntValue = 0;

	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_GW]._StrValue, "GW");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_SAT]._StrValue, "SAT");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_ST]._StrValue, "ST");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_ST_AGG]._StrValue,
			 "AGGREGATE_ST");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_OBPC]._StrValue, "OBPC");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_TG]._StrValue,
			 "TRAFFIC");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_PROBE_CTRL]._StrValue,
			 "PROBE_CONTROLLER");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_EVENT_CTRL]._StrValue,
			 "EVENT_CONTROLLER");
	strcpy(ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_ERROR_CTRL]._StrValue,
			 "ERROR_CONTROLLER");
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_MAX]._StrValue[0] = '\0';

	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_GW]._IntValue = C_COMP_GW;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_SAT]._IntValue = C_COMP_SAT;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_ST]._IntValue = C_COMP_ST;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_ST_AGG]._IntValue = C_COMP_ST_AGG;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_OBPC]._IntValue = C_COMP_OBPC;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_TG]._IntValue = C_COMP_TG;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_PROBE_CTRL]._IntValue =
		C_COMP_PROBE_CTRL;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_EVENT_CTRL]._IntValue =
		C_COMP_EVENT_CTRL;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_ERROR_CTRL]._IntValue =
		C_COMP_ERROR_CTRL;
	ptr_this->C_PROBES_ACTIVATION_ComponentChoices[C_COMP_MAX]._IntValue = 0;

	return rid;
}


static T_ERROR ReadProbeActivation(
												 /* IN    */ T_FILE_READER * ptr_reader,
												 /*   OUT */
												 T_ACTIVATED_PROBE_TAB * ptr_probes,
												 /* IN    */ T_INT32 probeActivationIndex)
{
	T_ERROR rid = C_ERROR_OK;
	T_ACTIVATED_PROBE
		* ptrActivatedProbe = &(ptr_probes->_Probe[probeActivationIndex]);

	/* Initialise current line parsing */
  /*---------------------------------*/
	LINE_PARSER_Init(&(ptr_reader->_Parser));

	/* Read current line */
  /*-------------------*/
	JUMP_ERROR(FIN, rid,
				  FILE_READER_ReadLine(ptr_reader,
											  ptr_reader->_Parser._LineBuffer));

	/* Parse read line */
  /*-----------------*/
	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseString(&(ptr_reader->_Parser),
												  C_PROB_DEF_MAX_CAR_NAME,
												  ptrActivatedProbe->_Statistic._Name));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
												ptr_probes->C_PROB_AGGREGATE_choices,
												(T_INT32 *) ((unsigned char *) &ptrActivatedProbe->
																	_AggregationMode)));

	JUMP_ERROR(FIN, rid, LINE_PARSER_ParseInteger(&(ptr_reader->_Parser), 0, 1,
																 &(ptrActivatedProbe->
																	_DisplayFlag)));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseEnum(&(ptr_reader->_Parser),
												ptr_probes->C_PROB_ANALYSIS_choices,
												(T_INT32 *) ((unsigned char *) &ptrActivatedProbe->
																	_AnalysisOperator)));

	JUMP_ERROR(FIN, rid,
				  LINE_PARSER_ParseInteger(&(ptr_reader->_Parser), 1,
													C_PROBE_MAX_OPERATOR_PARAMETER,
													&(ptrActivatedProbe->
													  _OperatorParameter)));

 FIN:
	return rid;
}


T_ERROR PROBES_ACTIVATION_ReadConfigNamedFile(
																/* INOUT */ T_PROBES_ACTIVATION
																* ptr_this,
																/* IN    */ T_STRING name)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_READER config_reader;

	/* Initialise config_reader */
  /*--------------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_Init(&config_reader));

	/* Open config file */
  /*------------------*/
	JUMP_ERROR(FIN1, rid, FILE_READER_OpenFile(&config_reader, name));

	/* Read intial parameters : start frame, stop frame & sampling period */
  /*--------------------------------------------------------------------*/
	JUMP_ERROR(FIN2, rid,
				  FILE_READER_ReadNamedUInteger(&config_reader, "Start_frame", 0,
														  0xFFFFFFFF,
														  &(ptr_this->_StartFrame)));

	JUMP_ERROR(FIN2, rid,
				  FILE_READER_ReadNamedUInteger(&config_reader, "Stop_frame", 0,
														  0xFFFFFFFF,
														  &(ptr_this->_StopFrame)));

	JUMP_ERROR(FIN2, rid,
				  FILE_READER_ReadNamedUInteger(&config_reader, "Sampling_period",
														  1, C_MAX_SAMPLING_PERIOD,
														  &(ptr_this->_SamplingPeriod)));

	/* Read probed statistics loop */
  /*-----------------------------*/
	JUMP_ERROR(FIN2, rid, FILE_READER_ReadNamedLoop(&config_reader,
																	"Probed_statistics_number",
																	(T_READ_ITEM_FUNC)
																	ReadProbeActivation,
																	C_MAX_ACTIVATED_PROBE,
																	(T_ITEM_TAB *) & (ptr_this->
																							_ActivatedProbes)));

 FIN2:
	FILE_READER_CloseFile(&config_reader);

 FIN1:
	return rid;
}


/* Get Probes Activation complete file name */
/*------------------------------------------*/
T_ERROR PROBES_ACTIVATION_ReadConfigFile(
														 /* INOUT */ T_PROBES_ACTIVATION *
														 ptr_this,
														 /* IN    */
														 T_COMPONENT_TYPE ComponentLabel)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH file_name;
	T_FILE_PATH temp_file_name1, temp_file_name2, componentName, componentNameLow;
	T_UINT8 i;

	/* Initialize T_PROBES_ACTIVATION structure */
  /*------------------------------------------*/
	JUMP_ERROR(FIN, rid, PROBES_ACTIVATION_Init(ptr_this));

	/* Get the configuration path */
  /*------------------*/
	JUMP_ERROR(FIN, rid, FILE_PATH_GetConfPath(file_name));
	
	/* Get the complete file name */
  /*----------------------------*/
	JUMP_ERROR(FIN, rid, ENUM_PARSER_ParseString((T_INT32) ComponentLabel,
																ptr_this->
																C_PROBES_ACTIVATION_ComponentChoices,
																componentName));

	strcpy(temp_file_name1, FILE_INFOS_GetFileName(C_PROBE_ACT_FILE));

	i = 0;
	while(i < strlen(componentName))
	{
		componentNameLow[i] = tolower(componentName[i]);
		i++;
	}
	componentNameLow[i] = '\0';
	sprintf(temp_file_name2, temp_file_name1, componentNameLow);

	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(file_name, temp_file_name2));

	/* Call Read function */
  /*--------------------*/
	JUMP_ERROR(FIN, rid,
				  PROBES_ACTIVATION_ReadConfigNamedFile(ptr_this, file_name));

 FIN:
	return rid;
}



T_ERROR PROBES_ACTIVATION_UpdateDefinition(
															/* INOUT */ T_PROBES_ACTIVATION *
															ptr_this,
															/* IN    */
															T_PROBES_DEF * ptr_probesDef)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i, j;

	/* Update probe activation info */
  /*------------------------------*/
	for(i = 0; i < ptr_this->_ActivatedProbes._nbActivatedProbes; i++)
	{
		for(j = 0; j < ptr_probesDef->_nbStatistics; j++)
		{
			if(!strcmp(ptr_probesDef->_Statistic[j]._Name,
						  ptr_this->_ActivatedProbes._Probe[i]._Statistic._Name))
				break;
		}
		if(j != ptr_probesDef->_nbStatistics)
		{								  /* symbol found */
			memcpy((char *) &(ptr_this->_ActivatedProbes._Probe[i]._Statistic),
					 (char *) &(ptr_probesDef->_Statistic[j]), sizeof(T_PROBE_DEF));
		}
		else
		{
			TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_CONFIG,
							 C_TRACE_ERROR,
							 "PROBES_ACTIVATION_UpdateDefinition() cannot find symbol %s in the probes def file",
							 ptr_this->_ActivatedProbes._Probe[i]._Statistic._Name));
			rid = C_ERROR_CONF_INVAL;
			break;
		}
	}
	return rid;
}
