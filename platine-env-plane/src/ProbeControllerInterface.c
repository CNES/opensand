/*--------------------------------------------------------------------------*/
/*  @PROJECT : DOMINO2 ASP
    @COMPANY : ALCATEL SPACE
    @AUTHOR  : David DEBARGE - TRANSICIEL
    @ID      : $Name: v2_0_0 $ $Revision: 1.1.1.1 $ $Date: 2006/08/02 11:50:28 $
	
    @ROLE    : The ProbeController implements the probe controller mechanism
    @HISTORY :
    03-02-20 : Creation
*/
/*--------------------------------------------------------------------------*/

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>

/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Time_e.h"
#include "Trace_e.h"
#include "FilePath_e.h"
#include "ProbesDef_e.h"
#include "ProbesActivation_e.h"
#include "ComParameters_e.h"
#include "EnumParser_e.h"
#include "GenericPacket_e.h"
#include "ProtoConstants_e.h"
#include "GenericPort_e.h"
#include "IPAddr_e.h"
#include "ProbeControllerInterface_e.h"
#include "Controller_e.h"

T_PRB_CTRL *ptr_ctrl;


COMPONENT_CHOICES(PROBE_CONTROLLER_ComponentChoices)	/* ENUM_COUPLE array is defined in ExecContext_e.h */
	  static void GenerateHeader(
											 /* INOUT */ T_PRB_CTRL * ptr_this,
											 /* IN    */ T_UINT8 componentType,
											 /* IN    */ T_UINT8 instance,
											 /* IN    */ T_UINT8 probeId)
{
	T_UINT32 i;
	T_PROBE_INFO * probeInfo;
	T_PROBE_DEF * probeDef;
	char mode[50];

	probeInfo
		=
		&(ptr_this->_ptr_probeData[componentType][instance]._probeInfo[probeId]);
	probeDef = &(ptr_this->_probesDef[componentType]._Statistic[probeId - 1]);
	fprintf(probeInfo->_file, "# <name>\t%s\n", probeDef->_Name);
	fprintf(probeInfo->_file, "# <category>\t%ld\n", probeDef->_Category);
	fprintf(probeInfo->_file, "# <type>\t%s\n",
	       ((probeDef->_Type == 0) ? "INT" : "FLOAT"));
	fprintf(probeInfo->_file, "# <unit>\t%s\n", probeDef->_Unit);
    fprintf(probeInfo->_file, "# <instance>\t%i\n", instance);
	fprintf(probeInfo->_file, "# <graph_type>\t%s\n", probeDef->_Graph_Type);
	fprintf(probeInfo->_file, "# <comment>\t%s\n", probeDef->_Comment);
	ENUM_PARSER_ParseString(probeInfo->_aggregationMode,
									ptr_this->C_PROB_AGGREGATE_choices, mode);
	fprintf(probeInfo->_file, "# <aggregate>\t%s\n", mode);
	ENUM_PARSER_ParseString(probeInfo->_analysisOperator,
									ptr_this->C_PROB_ANALYSIS_choices, mode);
	fprintf(probeInfo->_file, "# <analysis>\t%s\n", mode);
	fprintf(probeInfo->_file, "# <analysis op>\t%ld\n",
			  probeInfo->_operatorParameter);

	fprintf(probeInfo->_file, "time");
	if(probeInfo->_nbLabels != 0)
	{
		for(i = 0; i < probeInfo->_nbLabels; i++)
			fprintf(probeInfo->_file, ";%s_%s",
					  probeDef->_Name, probeDef->_StatLabels._StatLabelValue[i]);
	}
	else
		fprintf(probeInfo->_file, ";%s", probeDef->_Name);
	fprintf(probeInfo->_file, "\n");
	fflush(probeInfo->_file);
}

// !CB used for the float conversion
//Inverse l'ordre des octets
float ByteReverse(const float in)
{
	float out;
	const char *pin = (const char *) &in;
	char *pout = (char *) (&out + 1) - 1;

	int i;
	for(i = sizeof(float); i > 0; --i)
	{
		*pout-- = *pin++;
	}
	return out;
}


static void StoreComponentData(
											/* INOUT */ T_PRB_CTRL * ptr_this,
											/* IN    */ T_UINT8 cmptId,
											/* IN    */ T_UINT8 instance)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i;
	T_INT32 j;
	T_UINT32 intValue = 0, k;
	T_FLOAT floatValue = 0.0, thisTime;
	T_DOUBLE doubleMean, doubleValue, doubleSum;
	T_PROBE_HOLDER * probeHolder;
	T_DISPLAY_DATA displayData;
	T_BUFFER ptr_value;

	probeHolder =
		(T_PROBE_HOLDER *) (&ptr_this->_ptr_probeData[cmptId][instance]);

	/* Init display data */
  /*-------------------*/
	displayData._componentId = MAKE_COMPONENT_ID(cmptId, instance);
	//ptr_this->_FRSDuration = 0.05;
	thisTime =
		(T_FLOAT) probeHolder->_displayFrame *  ptr_this->_FRSDuration;
	/* fprintf (stdout, "frame %lu time %f\n", probeHolder->_displayFrame,ptr_this->_FRSDuration); 
	  displayData._time=htonl(displayData._time); */
	displayData._time = ByteReverse(thisTime);

	/* Store value in file */
  /*---------------------*/
	for(i = 1; i < ptr_this->_probesDef[cmptId]._nbStatistics + 1; i++)
	{
		if(probeHolder->_probeInfo[i]._activate == TRUE)
		{
			/* store all values */
		/*------------------*/
			fprintf(probeHolder->_probeInfo[i]._file, "%.03f", thisTime);
			if(probeHolder->_probeInfo[i]._nbLabels != 0)
				j = 1;
			else
				j = 0;
			for(; j < probeHolder->_probeInfo[i]._nbLabels + 1; j++)
			{
				if(probeHolder->_ptr_probeValue[i][j]._valueChange ==
					C_PROBE_VALUE_UPDATED)
				{
					probeHolder->_ptr_probeValue[i][j]._valueChange =
						C_PROBE_VALUE_CHANGE;
					/* get stat value */
			 /*----------------*/
					switch (probeHolder->_probeInfo[i]._analysisOperator)
					{
					case C_ANA_MEAN:
						if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)
						{				  /* INT type */
							intValue =
								(T_UINT32) lrint((T_DOUBLE) probeHolder->
													  _ptr_probeValue[i][j]._intValue /
													  (T_DOUBLE) probeHolder->
													  _ptr_probeValue[i][j]._valueNumber);
						}
						else
						{
							floatValue = probeHolder->_ptr_probeValue[i][j]._floatValue
								/
								(T_FLOAT) probeHolder->_ptr_probeValue[i][j].
								_valueNumber;
						}
						break;
					case C_ANA_STANDARD_DEV:
						if(CIRCULAR_BUFFER_GetEltNumber
							(&(probeHolder->_ptr_probeValue[i][j]._buffer)) > 1)
						{
							/* calculate the mean double value */
							doubleMean = 0.0;
							for(k = 0;
								 k <
								 CIRCULAR_BUFFER_GetEltNumber(&
																		(probeHolder->
																		 _ptr_probeValue[i][j].
																		 _buffer)); k++)
							{
								CIRCULAR_BUFFER_GetPrevReadBuffer(&
																			 (probeHolder->
																			  _ptr_probeValue[i]
																			  [j]._buffer), k,
																			 (T_BUFFER *) &
																			 ptr_value);
								if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)	/* INT type */
									doubleMean += (T_DOUBLE) (*(T_UINT32 *) ptr_value);
								else
									doubleMean += (T_DOUBLE) (*(T_FLOAT *) ptr_value);
							}
							doubleMean /=
								CIRCULAR_BUFFER_GetEltNumber(&
																	  (probeHolder->
																		_ptr_probeValue[i][j].
																		_buffer));
							/* calculate the deviation value */
							doubleSum = 0.0;
							for(k = 0;
								 k <
								 CIRCULAR_BUFFER_GetEltNumber(&
																		(probeHolder->
																		 _ptr_probeValue[i][j].
																		 _buffer)); k++)
							{
								CIRCULAR_BUFFER_GetPrevReadBuffer(&
																			 (probeHolder->
																			  _ptr_probeValue[i]
																			  [j]._buffer), k,
																			 (T_BUFFER *) &
																			 ptr_value);
								if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)	/* INT type */
									doubleValue =
										(T_DOUBLE) ((T_DOUBLE) (*(T_UINT32 *) ptr_value) -
														doubleMean);
								else
									doubleValue =
										(T_DOUBLE) ((T_DOUBLE) (*(T_FLOAT *) ptr_value) -
														doubleMean);
								doubleValue = pow(doubleValue, 2);
								doubleSum += doubleValue;
							}
							doubleSum /=
								(CIRCULAR_BUFFER_GetEltNumber
								 (&(probeHolder->_ptr_probeValue[i][j]._buffer)) - 1);
							doubleSum = sqrt(doubleSum);
							floatValue = (T_FLOAT) doubleSum;
						}
						else
							floatValue = 0.0;
						break;
					case C_ANA_SLIDING_MIN:
						if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)
						{				  /* INT type */
							ANALYSIS_SLIDING_MIN(&
														(probeHolder->_ptr_probeValue[i][j].
														 _buffer), ptr_value, intValue, k,
														T_UINT32);
						}
						else
						{
							ANALYSIS_SLIDING_MIN(&
														(probeHolder->_ptr_probeValue[i][j].
														 _buffer), ptr_value, floatValue, k,
														T_FLOAT);
						}
						break;
					case C_ANA_SLIDING_MAX:
						if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)
						{				  /* INT type */
							ANALYSIS_SLIDING_MAX(&
														(probeHolder->_ptr_probeValue[i][j].
														 _buffer), ptr_value, intValue, k,
														T_UINT32);
						}
						else
						{
							ANALYSIS_SLIDING_MAX(&
														(probeHolder->_ptr_probeValue[i][j].
														 _buffer), ptr_value, floatValue, k,
														T_FLOAT);
						}
						break;
					case C_ANA_SLIDING_MEAN:
						if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)
						{				  /* INT type */
							intValue = 0;
							for(k = 0;
								 k <
								 CIRCULAR_BUFFER_GetEltNumber(&
																		(probeHolder->
																		 _ptr_probeValue[i][j].
																		 _buffer)); k++)
							{
								CIRCULAR_BUFFER_GetPrevReadBuffer(&
																			 (probeHolder->
																			  _ptr_probeValue[i]
																			  [j]._buffer), k,
																			 (T_BUFFER *) &
																			 ptr_value);
								intValue += *(T_UINT32 *) ptr_value;
							}
							intValue =
								(T_UINT32) lrint((T_DOUBLE) intValue /
													  (T_DOUBLE)
													  CIRCULAR_BUFFER_GetEltNumber(&
																							 (probeHolder->
																							  _ptr_probeValue
																							  [i][j].
																							  _buffer)));
						}
						else
						{
							floatValue = 0.0;
							for(k = 0;
								 k <
								 CIRCULAR_BUFFER_GetEltNumber(&
																		(probeHolder->
																		 _ptr_probeValue[i][j].
																		 _buffer)); k++)
							{
								CIRCULAR_BUFFER_GetPrevReadBuffer(&
																			 (probeHolder->
																			  _ptr_probeValue[i]
																			  [j]._buffer), k,
																			 (T_BUFFER *) &
																			 ptr_value);
								floatValue += *(T_FLOAT *) ptr_value;
							}
							floatValue /= (T_FLOAT)
								CIRCULAR_BUFFER_GetEltNumber(&
																	  (probeHolder->
																		_ptr_probeValue[i][j].
																		_buffer));
						}
						break;
					default:
						if(probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT)	/* INT type */
							intValue = probeHolder->_ptr_probeValue[i][j]._intValue;
						else
							floatValue =
								probeHolder->_ptr_probeValue[i][j]._floatValue;
						break;
					}
					/* store value in log file */
			 /*-------------------------*/
					if((probeHolder->_probeInfo[i]._type == C_PROBE_TYPE_INT) && (probeHolder->_probeInfo[i]._analysisOperator != C_ANA_STANDARD_DEV))	/* INT type */
						fprintf(probeHolder->_probeInfo[i]._file, ";%lu", intValue);
					else
						fprintf(probeHolder->_probeInfo[i]._file, ";%.03f",
								  floatValue);
					/* send value to display */
			 /*-----------------------*/
					if((probeHolder->_probeInfo[i]._displayFlag)
						&& (ptr_this->_displayPortReady))
					{
						displayData._probeId =
							probeHolder->_ptr_probeValue[i][j]._probeId;
						if(probeHolder->_probeInfo[i]._analysisOperator !=
							C_ANA_STANDARD_DEV)
							displayData._type = probeHolder->_probeInfo[i]._type;
						else
							displayData._type = C_PROBE_TYPE_FLOAT;
						displayData._index =
							probeHolder->_ptr_probeValue[i][j]._index;
						if(displayData._type == C_PROBE_TYPE_INT)
						{
							/* INT type */
							displayData._value = intValue;
							/*    fprintf (stdout, "%f component %d sending to display probe %d value %lu index %lu\n",
							   displayData._time, displayData._componentId, displayData._probeId , displayData._value, displayData._index); */
						}
						else
						{
							displayData._value = (T_UINT32) floatValue;
							/*    fprintf (stdout, "%f component %d sending to display probe %d value %f index %lu\n",
							   displayData._time, displayData._componentId, displayData._probeId , (float)displayData._value, displayData._index); */
						}
						/* !CB byte order is MSB in the display */
						displayData._type = htons(displayData._type);
						displayData._index = htonl(displayData._index);
						displayData._value = htonl(displayData._value);

						SEND_AG_ERRNO(rid,
										  UDP_SOCKET_SendBytes(&(ptr_this->_displayPort),
																	  (T_BUFFER) & displayData,
																	  sizeof(T_DISPLAY_DATA)),
										  &(ptr_this->_errorAgent), C_ERROR_MINOR,
										  C_EI_PD_SOCKET, (C_TRACE_THREAD_UNKNOWN,
																 C_TRACE_COMP_PROBE,
																 C_TRACE_ERROR,
																 "UDP_SOCKET_SendBytes() failed for display"));
						if(rid != C_ERROR_OK)
							ptr_this->_displayPortReady = FALSE;
					}
				}
				else
				{
					fprintf(probeHolder->_probeInfo[i]._file, ";");
				}
			}
			fprintf(probeHolder->_probeInfo[i]._file, "\n");
			fflush(probeHolder->_probeInfo[i]._file);
		}
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
				  "StoreComponentData() cmptId:%d instance:%d", cmptId, instance));
}


static T_ERROR AllocateProbeData(
											  /* INOUT */ T_PRB_CTRL * ptr_this,
											  /* IN    */ T_COMPONENT_TYPE componentType,
											  /* IN    */ T_UINT16 simRef,
											  /* IN    */ T_UINT16 simRun,
											  /* IN    */ T_UINT8 instanceNumber,
											  /* IN    */ T_ERROR_AGENT * ptr_errorAgent)
{
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i;

	/* store the instance number */
	ptr_this->_instanceNumber[componentType] = instanceNumber;

	/* allocate the probe data */
	if(instanceNumber != 0)
	{
		ptr_this->_ptr_probeData[componentType] =
			malloc(sizeof(T_PROBE_HOLDER) * instanceNumber);
		if(ptr_this->_ptr_probeData[componentType] == NULL)
		{
			SEND_AG_ERRNO_JUMP(FIN, rid, C_ERROR_ALLOC, &(ptr_this->_errorAgent),
									 C_ERROR_CRITICAL, 0,
									 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
									  C_TRACE_ERROR, "malloc() failed"));
		}
		for(i = 0; i < instanceNumber; i++)
		{
			rid = PROBE_HOLDER_Init(&(ptr_this->_ptr_probeData[componentType][i]),
											&(ptr_this->_probesDef[componentType]),
											componentType, simRef, simRun, TRUE,
											ptr_errorAgent);
			if(rid != C_ERROR_OK)
			{
				ERROR_AGENT_SendError(&(ptr_this->_errorAgent));
				TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
								 C_TRACE_ERROR, "PROBE_HOLDER_Init() failed"));
				goto FIN;
			}
		}

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
					  "AllocateProbeData() cmptId:%d instanceNb:%d",
					  componentType, instanceNumber));
	}
	else
		ptr_this->_ptr_probeData[componentType] = NULL;

 FIN:
	return (rid);
}


static T_ERROR DisallocateProbeData(
												  /* INOUT */ T_PRB_CTRL * ptr_this,
												  /* IN    */
												  T_COMPONENT_TYPE componentType)
{
	T_UINT32 i;

	if(ptr_this->_ptr_probeData[componentType] != NULL)
	{
		for(i = 0; i < ptr_this->_instanceNumber[componentType]; i++)
			PROBE_HOLDER_Terminate(&(ptr_this->_ptr_probeData[componentType][i]));
		free(ptr_this->_ptr_probeData[componentType]);
		ptr_this->_ptr_probeData[componentType] = NULL;

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
					  "DisallocateProbeData() cmptId:%d instanceNb:%d",
					  componentType, ptr_this->_instanceNumber[componentType]));
	}

	return C_ERROR_OK;
}


T_ERROR PRB_CTRL_Init(
								/* INOUT */ T_PRB_CTRL * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_COM_PARAMETERS comParams;

	memset(ptr_this, 0, sizeof(T_PRB_CTRL));


	/* Initialise Enum structures associating Character strings with Integer values */
  /*------------------------------------------------------------------------------*/
	strcpy(ptr_this->C_PROB_AGGREGATE_choices[0]._StrValue, "MIN");
	strcpy(ptr_this->C_PROB_AGGREGATE_choices[1]._StrValue, "MAX");
	strcpy(ptr_this->C_PROB_AGGREGATE_choices[2]._StrValue, "MEAN");
	strcpy(ptr_this->C_PROB_AGGREGATE_choices[3]._StrValue, "LAST");
	strcpy(ptr_this->C_PROB_AGGREGATE_choices[4]._StrValue, "");

	ptr_this->C_PROB_AGGREGATE_choices[0]._IntValue = C_AGG_MIN;
	ptr_this->C_PROB_AGGREGATE_choices[1]._IntValue = C_AGG_MAX;
	ptr_this->C_PROB_AGGREGATE_choices[2]._IntValue = C_AGG_MEAN;
	ptr_this->C_PROB_AGGREGATE_choices[3]._IntValue = C_AGG_LAST;
	ptr_this->C_PROB_AGGREGATE_choices[4]._IntValue = 0;

	strcpy(ptr_this->C_PROB_ANALYSIS_choices[0]._StrValue, "RAW");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[1]._StrValue, "MIN");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[2]._StrValue, "MAX");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[3]._StrValue, "MEAN");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[4]._StrValue, "STANDARD_DEVIATION");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[5]._StrValue, "SLIDING_MIN");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[6]._StrValue, "SLIDING_MAX");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[7]._StrValue, "SLIDING_MEAN");
	strcpy(ptr_this->C_PROB_ANALYSIS_choices[8]._StrValue, "");

	ptr_this->C_PROB_ANALYSIS_choices[0]._IntValue = C_ANA_RAW;
	ptr_this->C_PROB_ANALYSIS_choices[1]._IntValue = C_ANA_MIN;
	ptr_this->C_PROB_ANALYSIS_choices[2]._IntValue = C_ANA_MAX;
	ptr_this->C_PROB_ANALYSIS_choices[3]._IntValue = C_ANA_MEAN;
	ptr_this->C_PROB_ANALYSIS_choices[4]._IntValue = C_ANA_STANDARD_DEV;
	ptr_this->C_PROB_ANALYSIS_choices[5]._IntValue = C_ANA_SLIDING_MIN;
	ptr_this->C_PROB_ANALYSIS_choices[6]._IntValue = C_ANA_SLIDING_MAX;
	ptr_this->C_PROB_ANALYSIS_choices[7]._IntValue = C_ANA_SLIDING_MEAN;
	ptr_this->C_PROB_ANALYSIS_choices[8]._IntValue = 0;

	/* read communication definition file */
  /*------------------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, COM_PARAMETERS_ReadConfigFile(&comParams),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
							"COM_PARAMETERS_ReadConfigFile() failed"));

	/* create the error agent */
  /*------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, ERROR_AGENT_Init(&(ptr_this->_errorAgent),
															  &(comParams._ControllersPorts.
																 _ErrorController._IpAddress),
															  C_COMP_PROBE_CTRL, 0, NULL,
															  NULL), (C_TRACE_THREAD_UNKNOWN,
																		 C_TRACE_COMP_PROBE,
																		 C_TRACE_ERROR,
																		 "ERROR_AGENT_Init() failed"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
				  "PRB_CTRL_Init() open error agent %s port %d",
				  comParams._ControllersPorts._ErrorController._IpAddress._addr,
				  comParams._ControllersPorts._ErrorController._IpAddress._port));

	/* Allocate the generic packet */
  /*-----------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PACKET_Create(&(ptr_this->_ptr_genPacket),
														  C_MAX_PROBE_VALUE_NUMBER),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, 0,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_ERROR, "GENERIC_PACKET_Create() failed"));

	/* open the generic port to receive probe generic packet */
  /*-------------------------------------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PORT_InitReceiver(&(ptr_this->_probePort),
																&(comParams._ControllersPorts.
																  _ProbeController._IpAddress),
																C_MAX_PROBE_VALUE_NUMBER),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, C_II_P_SOCKET,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_ERROR, "GENERIC_PORT_InitReceiver() failed"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
				  "PRB_CTRL_Init() open probe receiver %s port %d",
				  comParams._ControllersPorts._ProbeController._IpAddress._addr,
				  comParams._ControllersPorts._ProbeController._IpAddress._port));

	/* open the UDP port to send data to display */
  /*-------------------------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 UDP_SOCKET_InitSender(&(ptr_this->_displayPort),
														  &(comParams._DisplayPorts.
															 _ProbeDisplay._IpAddress),
														  (sizeof(T_DISPLAY_DATA) *
															C_UDP_SEND_MAX_PKG) +
														  (C_SOCKET_HEADER_SIZE *
															C_UDP_SEND_MAX_PKG)),
							 &(ptr_this->_errorAgent), C_ERROR_MINOR, C_EI_PD_SOCKET,
							 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
							  C_TRACE_ERROR,
							  "UDP_SOCKET_InitSender() failed for display"));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
				  "PRB_CTRL_Init() open udp display socket %s port %d",
				  comParams._DisplayPorts._ProbeDisplay._IpAddress._addr,
				  comParams._DisplayPorts._ProbeDisplay._IpAddress._port));

	/* Read probe defintion file */
  /*---------------------------*/

	SEND_AG_ERRNO_JUMP(FIN, rid,
							 PROBES_DEF_ReadConfigFile(&
																(ptr_this->
																 _probesDef[C_COMP_GW]),
																C_COMP_GW),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_PROBE_DEF_FILE, (C_TRACE_THREAD_UNKNOWN,
													  C_TRACE_COMP_PROBE, C_TRACE_ERROR,
													  "PROBES_DEF_ReadConfigFile() failed for cmpt GW"));
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 PROBES_DEF_ReadConfigFile(&
																(ptr_this->
																 _probesDef[C_COMP_SAT]),
																C_COMP_SAT),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_PROBE_DEF_FILE, (C_TRACE_THREAD_UNKNOWN,
													  C_TRACE_COMP_PROBE, C_TRACE_ERROR,
													  "PROBES_DEF_ReadConfigFile() failed for cmpt SAT"));
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 PROBES_DEF_ReadConfigFile(&
																(ptr_this->
																 _probesDef[C_COMP_ST]),
																C_COMP_ST),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_PROBE_DEF_FILE, (C_TRACE_THREAD_UNKNOWN,
													  C_TRACE_COMP_PROBE, C_TRACE_ERROR,
													  "PROBES_DEF_ReadConfigFile() failed for cmpt ST"));

	/* Init internal data */
  /*--------------------*/
	ptr_this->_simuIsRunning = FALSE;
	memset(&(ptr_this->_ptr_probeData[0]), 0,
			 sizeof(T_PROBE_HOLDER *) * C_CMPT_MAX);
	memset(&(ptr_this->_instanceNumber), 0, sizeof(T_UINT8) * C_CMPT_MAX);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PRB_CTRL_Init() sucessful"));

 FIN:
	return (rid);
}


T_ERROR PRB_CTRL_Terminate(
									  /* INOUT */ T_PRB_CTRL * ptr_this)
{

	/* terminate the error agent */
	ERROR_AGENT_Terminate(&(ptr_this->_errorAgent));

	/* free the generic packet */
	GENERIC_PACKET_Delete(&(ptr_this->_ptr_genPacket));

	/* close the generic port */
	GENERIC_PORT_Terminate(&(ptr_this->_probePort));

	/* close the udp port */
	UDP_SOCKET_Terminate(&(ptr_this->_displayPort));

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PRB_CTRL_Terminate() sucessful"));

	return C_ERROR_OK;
}


T_ERROR PRB_CTRL_EndSimulation(
											/* INOUT */ T_PRB_CTRL * ptr_this)
{
	T_UINT32 i, j;

	/* store the last data */
  /*---------------------*/
	for(i = 0; i < C_CMPT_MAX; i++)
		for(j = 0; j < ptr_this->_instanceNumber[i]; j++)
			if(ptr_this->_ptr_probeData[i] != NULL)
				if(ptr_this->_ptr_probeData[i][j]._lastFrame !=
					ptr_this->_ptr_genPacket->_frameNumber)
					StoreComponentData(ptr_this, (T_UINT8) i, (T_UINT8) j);

	/* Terminate probe data */
  /*----------------------*/
	DisallocateProbeData(ptr_this, C_COMP_GW);
	DisallocateProbeData(ptr_this, C_COMP_SAT);
	DisallocateProbeData(ptr_this, C_COMP_ST);

	/* Initialise internal data */
  /*--------------------------*/
	ptr_this->_simuIsRunning = FALSE;

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PRB_CTRL_EndSimulation() sucessful"));

	return C_ERROR_OK;
}


T_ERROR PRB_CTRL_InitSimulation(
											 /* INOUT */ T_PRB_CTRL * ptr_this)
{
	T_ERROR rid = C_ERROR_OK;
	T_ELT_GEN_PKT * eltGeneric;
	T_FILE_PATH cmptName, name, logFileName, currentDir;
	T_UINT32 i, j, k;
	T_UINT16 simRef, simRun;

	/* check the simulation running status */
  /*-------------------------------------*/
	if(ptr_this->_simuIsRunning == TRUE)
		PRB_CTRL_EndSimulation(ptr_this);

	ptr_this->_simuIsRunning = TRUE;
	ptr_this->_displayPortReady = TRUE;

	/* extract the first element of the initpacket */
  /*---------------------------------------------*/
	SEND_AG_ERRNO_JUMP(FIN, rid,
							 GENERIC_PACKET_GetEltPkt(ptr_this->_ptr_genPacket, 0,
															  &eltGeneric),
							 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
							 C_PROBE_COMMAND, (C_TRACE_THREAD_UNKNOWN,
													 C_TRACE_COMP_PROBE, C_TRACE_ERROR,
													 "GENERIC_PACKET_GetEltPkt() cannot get elt generic packet n°0"));

	/* extract run path from initpacket */
  /*----------------------------------*/
	simRun = (T_UINT16) (eltGeneric->_value & 0x0000FFFF);
	simRef = (T_UINT16) ((eltGeneric->_value >> 16) & 0x0000FFFF);

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
				  "PRB_CTRL_InitSimulation() for simRef %d simRun %d", simRef,
				  simRun));


	/* CB the frame duration is now set at the init of the controller
	   thanks to command line option -f */

	/* !CB the FSM is useless here
	   ptr_this->_FSMNb = arch_params._FSMNb;
	 */
	ptr_this->_FSMNb = 1;

	/* Init probe data */
  /*-----------------*/
	JUMP_ERROR(FIN, rid,
				  AllocateProbeData(ptr_this, C_COMP_GW, simRef, simRun, 1,
										  &(ptr_this->_errorAgent)));
	JUMP_ERROR(FIN, rid,
				  AllocateProbeData(ptr_this, C_COMP_SAT, simRef, simRun, 1,
										  &(ptr_this->_errorAgent)));
	/*  !CB the number of observed ST is defined statically
	   JUMP_ERROR(FIN,rid,
	   AllocateProbeData(ptr_this,C_COMP_ST,simRef,simRun,
	   (T_UINT8)radioResources._ObservedSTNumber,&(ptr_this->_errorAgent))); */
	JUMP_ERROR(FIN, rid,
				  AllocateProbeData(ptr_this, C_COMP_ST, simRef, simRun,
										  C_ST_MAX, &(ptr_this->_errorAgent)));

	/* Get the run path */
  /*------------------*/
	FILE_PATH_GetOutputPath(currentDir, simRef, simRun);

	/* Get the complete file name */
  /*----------------------------*/
	for(i = 0; i < C_CMPT_MAX; i++)
	{
		/* get the component name */
	 /*------------------------*/
		SEND_AG_ERRNO_JUMP(FIN, rid,
								 ENUM_PARSER_ParseString((T_INT32) i,
																 PROBE_CONTROLLER_ComponentChoices,
																 cmptName),
								 &(ptr_this->_errorAgent), C_ERROR_CRITICAL, 0,
								 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
								  C_TRACE_ERROR, "ENUM_PARSER_ParseString() failed"));
		for(j = 0; j < ptr_this->_instanceNumber[i]; j++)
		{
			/* check if the statistic is activated */
		/*-------------------------------------*/
			for(k = 1; k < ptr_this->_probesDef[i]._nbStatistics + 1; k++)
			{
				if(ptr_this->_ptr_probeData[i][j]._probeInfo[k]._activate == TRUE)
				{
					/* create the log file name */
			 /*--------------------------*/
					if(ptr_this->_instanceNumber[i] > 1)
						sprintf(name, "stat_%s_%s_%02lu.pb", cmptName,
								  ptr_this->_probesDef[i]._Statistic[k - 1]._Name, j);
					else
						sprintf(name, "stat_%s_%s.pb", cmptName,
								  ptr_this->_probesDef[i]._Statistic[k - 1]._Name);
					strcpy(logFileName, currentDir);
					FILE_PATH_Concat(logFileName, name);
					/* open the log file name */
			 /*------------------------*/
					ptr_this->_ptr_probeData[i][j]._probeInfo[k]._file =
						fopen(logFileName, "w");
					if(ptr_this->_ptr_probeData[i][j]._probeInfo[k]._file == NULL)
					{
						SEND_AG_ERRNO_JUMP(FIN, rid, C_ERROR_FILE_OPEN,
												 &(ptr_this->_errorAgent), C_ERROR_CRITICAL,
												 C_PROBE_LOG_FILE, (C_TRACE_THREAD_UNKNOWN,
																		  C_TRACE_COMP_PROBE,
																		  C_TRACE_ERROR,
																		  "fopen() failed"));
					}
					/* init the header in the log file */
			 /*---------------------------------*/
					GenerateHeader(ptr_this, (T_UINT8) i, (T_UINT8) j, (T_UINT8) k);
				}
			}
		}
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PRB_CTRL_InitSimulation() sucessful"));

 FIN:
	return (rid);
}


T_ERROR PRB_CTRL_DoPacket(
									 /* INOUT */ T_PRB_CTRL * ptr_this)
{
	T_BUFFER ptr_value;
	T_UINT32 intValue, i;
	T_ELT_GEN_PKT * eltGenPkt;
	T_UINT8 cmptId, instance;
	T_PROBE_HOLDER * probeHolder;
	T_PROBE_VALUE * probeValue;
	T_FLOAT floatValue;

	/* Extract value from the header */
  /*-------------------------------*/
	EXTRACT_COMPONENT_ID(ptr_this->_ptr_genPacket->_componentId, cmptId,
								instance);
	probeHolder =
		(T_PROBE_HOLDER *) (&ptr_this->_ptr_probeData[cmptId][instance]);

	/* Store value in file and send to display */
  /*-----------------------------------------*/
	if(ptr_this->_ptr_genPacket->_frameNumber >=
		(probeHolder->_lastFrame + probeHolder->_samplingPeriod))
	{
		StoreComponentData(ptr_this, (T_UINT8) cmptId, (T_UINT8) instance);
		probeHolder->_lastFrame = ptr_this->_ptr_genPacket->_frameNumber;
	}
	probeHolder->_displayFrame = ptr_this->_ptr_genPacket->_frameNumber;

	/* Extract value from the generic packet */
  /*---------------------------------------*/
	eltGenPkt =
		(T_ELT_GEN_PKT *) ((T_BYTE *) ptr_this->_ptr_genPacket + HD_GEN_PKT_SIZE);
	for(i = 0; i < ptr_this->_ptr_genPacket->_elementNumber; i++)
	{
		probeValue =
			(T_PROBE_VALUE *) (&probeHolder->
									 _ptr_probeValue[eltGenPkt->_id][eltGenPkt->_index]);
		probeHolder->_probeInfo[eltGenPkt->_id]._frameNb =
			ptr_this->_ptr_genPacket->_frameNumber;
		/* store the new value */
	 /*---------------------*/
		switch (probeHolder->_probeInfo[eltGenPkt->_id]._analysisOperator)
		{
		case C_ANA_RAW:
			if(probeHolder->_probeInfo[eltGenPkt->_id]._type == C_PROBE_TYPE_INT)	/* INT type */
				probeValue->_intValue = eltGenPkt->_value;
			else
				probeValue->_floatValue = (T_FLOAT) eltGenPkt->_value;
			break;
		case C_ANA_MIN:
			if(probeHolder->_probeInfo[eltGenPkt->_id]._type == C_PROBE_TYPE_INT)
			{							  /* INT type */
				probeValue->_intValue
					=
					OPERATOR_MIN(probeValue->_intValue, eltGenPkt->_value,
									 probeValue->_valueChange);
			}
			else
			{
				floatValue = (T_FLOAT) eltGenPkt->_value;
				probeValue->_floatValue
					=
					OPERATOR_MIN(probeValue->_floatValue, floatValue,
									 probeValue->_valueChange);
			}
			break;
		case C_ANA_MAX:
			if(probeHolder->_probeInfo[eltGenPkt->_id]._type == C_PROBE_TYPE_INT)
			{							  /* INT type */
				probeValue->_intValue
					=
					OPERATOR_MAX(probeValue->_intValue, eltGenPkt->_value,
									 probeValue->_valueChange);
			}
			else
			{
				floatValue = (T_FLOAT) eltGenPkt->_value;
				probeValue->_floatValue
					=
					OPERATOR_MAX(probeValue->_floatValue, floatValue,
									 probeValue->_valueChange);
			}
			break;
		case C_ANA_MEAN:
			if(probeHolder->_probeInfo[eltGenPkt->_id]._type == C_PROBE_TYPE_INT)
			{							  /* INT type */
				ANALYSIS_MEAN(probeValue->_intValue, eltGenPkt->_value,
								  intValue, probeValue->_valueNumber);
			}
			else
			{
				ANALYSIS_MEAN(probeValue->_floatValue,
								  (T_FLOAT) eltGenPkt->_value, floatValue,
								  probeValue->_valueNumber);
			}
			probeValue->_valueNumber++;
			break;
		case C_ANA_STANDARD_DEV:
		case C_ANA_SLIDING_MIN:
		case C_ANA_SLIDING_MAX:
		case C_ANA_SLIDING_MEAN:
			CIRCULAR_BUFFER_GetWriteBuffer(&(probeValue->_buffer),
													 (T_BUFFER *) & ptr_value);
			if(probeHolder->_probeInfo[eltGenPkt->_id]._type == C_PROBE_TYPE_INT)
				*(T_UINT32 *) ptr_value = eltGenPkt->_value;
			else
				*(T_FLOAT *) ptr_value = (T_FLOAT) eltGenPkt->_value;
			break;
		case C_ANA_NB:
			/* not a real value, max of the enum, nothing to do */
			break;
		}
		probeValue->_valueChange = C_PROBE_VALUE_UPDATED;

		/* next generic element */
	 /*----------------------*/
		eltGenPkt = (T_ELT_GEN_PKT *) ((T_BYTE *) eltGenPkt + ELT_GEN_PKT_SIZE);
	}

	return C_ERROR_OK;
}

int startProbeControllerInterface(int argc, char *argv[])
{
	T_ERROR rid = C_ERROR_OK;
	T_ELT_GEN_PKT * eltGenPkt;

	T_PRB_CTRL * _ctrl;

	char command[50], *str;
	extern char *optarg;
	int cmptId, level, opt;
	int duration = 0;
	T_UINT64 flag;


	/* Init time */
  /*-----------*/
	_ctrl = malloc(sizeof(T_PRB_CTRL));
	if(_ctrl == NULL)
	{
		fprintf(stderr, "malloc failed\n");
		exit(-1);
	}
	ptr_ctrl = _ctrl;

	TIME_Init();

	/* activate TRACE option */
  /*-----------------------*/
	while((opt = getopt(argc, argv, "-T:ht:f:d")) != EOF)
	{
		switch (opt)
		{
		case 'T':
			/* get component id */
			strcpy(command, optarg);
			str = strtok(command, ":");
			if(str == NULL)
			{
				fprintf(stderr, "bad parameter: -T%s\n", optarg);
				exit(-1);
			}
			cmptId = atoi(str);
			flag = 0x1L;
			flag <<= C_TRACE_THREAD_MAX + cmptId;

			/* get level id */
			str = strtok(NULL, ":");
			if(str == NULL)
			{
				level = 0xff;
				fprintf(stdout, "activate all traces for component id %d\n",
						  cmptId);
			}
			else
			{
				level = atoi(str);
				fprintf(stdout, "activate trace level %d for component id %d\n",
						  level, cmptId);
			}

			/* activate trace */
			TRACE_ACTIVATE(flag, level);
			break;

		case 't':
			level = atoi(optarg);
			if(level == 0)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_0 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			if(level == 1)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_1 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			if(level == 2)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_2 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			if(level == 3)
			{
				TRACE_ACTIVATE_ALL(C_TRACE_VALID_3 | C_TRACE_ERROR | C_TRACE_FUNC);
			}
			/* !CB      TRACE_ACTIVATE_ALL(level); */
			fprintf(stdout, "activate trace level %d for all components\n", level);
			break;
		case 'd':
			/* not used */
			break;
		case 'f':
			/* frame duration */
			duration = atoi(optarg);
			break;
		case 'h':
		case '?':
			fprintf(stderr,
					  "usage: %s [-f<duration>] [-h] [-t<level> -t<level> ...] [-T<cmptId> -T<cmptId> ...]\n",
					  argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
			fprintf(stderr,
					  "\t-f<frame duration>   set the frame duration in ms (to display output)\n");
			fprintf(stderr,
					  "\t-d                   activate the display (not used yet)\n");
			fprintf(stderr,
					  "\t-t<level>            activate <level> trace for all components\n");
			fprintf(stderr,
					  "\t                     -t0     activate debug trace for all components\n");
			fprintf(stderr,
					  "\t-T<cmptId:level>     activate trace for <cmptId>\n");
			fprintf(stderr,
					  "\t                     -T5     activate all traces for component id 5\n");
			fprintf(stderr,
					  "\t                     -T5:1   activate debug trace for component id 5\n");
			exit(-1);
			break;
		}
	}

	/* Initialise config path and output path */
  /*----------------------------------------*/
	JUMP_ERROR_TRACE(FIN, rid, FILE_PATH_InitClass(),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
							"FILE_PATH_InitClass() failed"));

	/* init Session  */
  /*---------------*/
	JUMP_ERROR_TRACE(FIN, rid, PRB_CTRL_Init(_ctrl),
						  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
							"PRB_CTRL_Init() failed"));

	/* set the frame duration */
	/* -----------------------*/
	_ctrl->_FRSDuration = (T_FLOAT)duration * 1e-3;
	fprintf(stdout, "Frame duration = %f ms\n", ptr_ctrl->_FRSDuration * 1e3);
	
	/* infinite main loop of generic packet reception */
  /*------------------------------------------------*/
	while(1)
	{
		SEND_AG_ERRNO_JUMP(CLEAN, rid,
								 GENERIC_PORT_RecvGenPacket(&(_ctrl->_probePort),
																	 _ctrl->_ptr_genPacket),
								 &(_ctrl->_errorAgent), C_ERROR_CRITICAL, C_II_P_SOCKET,
								 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
								  C_TRACE_ERROR,
								  "GENERIC_PORT_RecvGenPacket() failed"));

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
					  "Receive data"));

		/* check category id */
	 /*-------------------*/
		GENERIC_PACKET_GetEltPkt(_ctrl->_ptr_genPacket, 0, &eltGenPkt);

		switch (eltGenPkt->_categoryId)
		{
		case C_CAT_INIT:			  /* Init packet */
			if(PRB_CTRL_InitSimulation(_ctrl) != C_ERROR_OK)
			{
				TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
								 C_TRACE_ERROR, "PRB_CTRL_InitSimulation() failed"));
				PRB_CTRL_EndSimulation(_ctrl);
			}
			break;
		case C_CAT_END:			  /* Terminate packet */
			PRB_CTRL_EndSimulation(_ctrl);
			break;
		default:
			if(_ctrl->_simuIsRunning)
			{
				if(PRB_CTRL_DoPacket(_ctrl) != C_ERROR_OK)
				{
					TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
									 C_TRACE_ERROR, "PRB_CTRL_DoPacket() failed"));
					PRB_CTRL_EndSimulation(_ctrl);
				}
			}
			else
			{
				SEND_AG_ERRNO(rid, C_ERROR_INIT_REF,
								  &(_ctrl->_errorAgent), C_ERROR_CRITICAL, 0,
								  (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
									C_TRACE_ERROR,
									"GENERIC_PORT_RecvGenPacket() receive data without start packet"));
			}
			break;
		}
	}

 CLEAN:
	/* close Session  */
  /*----------------*/
	PRB_CTRL_Terminate(_ctrl);

 FIN:
	return rid;
}
