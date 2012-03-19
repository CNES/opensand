/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file ProbeHolder.c
 * @author TAS
 * @brief The ProbeHolder class provides the Probe consolidation buffer
 */

/* SYSTEM RESOURCES */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
/* PROJECT RESOURCES */
#include "Types_e.h"
#include "Error_e.h"
#include "Trace_e.h"
#include "ProbesDef_e.h"
#include "ProbesActivation_e.h"
#include "ProtoConstants_e.h"
#include "CircularBuffer_e.h"
#include "ErrorAgent_e.h"
#include "ProbeHolder_e.h"
#include "unused.h"


/*  @ROLE    : This function initialises the probe holder
    @RETURN  : Error code */
T_ERROR PROBE_HOLDER_Init(
									 /* INOUT */ T_PROBE_HOLDER * ptr_this,
									 /* IN    */ T_PROBES_DEF * ptr_probesDef,
									 /* IN    */ T_COMPONENT_TYPE componentType,
									 /* IN    */ T_UINT16 UNUSED(simReference),
									 /* IN    */ T_UINT16 UNUSED(simRun),
									 /* IN    */ T_BOOL controlerConf,
									 /* IN    */ T_ERROR_AGENT * ptr_errorAgent)
{
	T_PROBES_ACTIVATION probesAct;
	T_ERROR rid = C_ERROR_OK;
	T_UINT32 i, j;
	T_UINT8 probeId;

	memset(ptr_this, 0, sizeof(T_PROBE_HOLDER));
	ptr_this->_controlerConf = controlerConf;

	/* Read probe activation file */
  /*----------------------------*/
	rid =
		PROBES_ACTIVATION_ReadConfigFile(&probesAct, componentType);

	/* Init internal data */
  /*--------------------*/
	memset(&(ptr_this->_probeInfo[0]), 0,
			 sizeof(T_PROBE_INFO) * (C_PROB_MAX_STAT_NUMBER + 1));
	memset(&(ptr_this->_ptr_probeValue[0]), 0,
			 sizeof(T_PROBE_VALUE *) * (C_PROB_MAX_STAT_NUMBER + 1));

	if(rid == C_ERROR_OK)
	{
		/* Update probe activation info */
	 /*------------------------------*/
		SEND_AG_ERRNO_JUMP(FIN, rid,
								 PROBES_ACTIVATION_UpdateDefinition(&probesAct,
																				ptr_probesDef),
								 ptr_errorAgent, C_ERROR_CRITICAL, C_PROBE_ACT_FILE,
								 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
								  C_TRACE_ERROR,
								  "PROBES_ACTIVATION_UpdateDefinition() failed"));

		/* Init internal data */
	 /*--------------------*/
		ptr_this->_statIsActivated = TRUE;
		ptr_this->_startFrame = (T_UINT32) probesAct._StartFrame;
		ptr_this->_stopFrame = (T_UINT32) probesAct._StopFrame;
		ptr_this->_samplingPeriod = (T_UINT32) probesAct._SamplingPeriod;
		ptr_this->_displayFrame = (T_UINT32) 0;
		ptr_this->_lastFrame = (T_UINT32) 0;
		ptr_this->_nbStat = (T_UINT8) ptr_probesDef->_nbStatistics;
		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
					  "PROBE_HOLDER_Init() cmpt[%d] startFrame[%d] stopFrame[%d] samplingPeriod[%d]",
					  componentType, ptr_this->_startFrame, ptr_this->_stopFrame,
					  ptr_this->_samplingPeriod));

		/* browse the probe activation */
		for(i = 0; i < probesAct._ActivatedProbes._nbActivatedProbes; i++)
		{
			probeId =
				(T_UINT8) probesAct._ActivatedProbes._Probe[i]._Statistic._probeId;
			/* ativate the probe id */
			ptr_this->_probeInfo[probeId]._activate = TRUE;

			/* category id */
			ptr_this->_probeInfo[probeId]._categoryId
				=
				(T_UINT8) probesAct._ActivatedProbes._Probe[i]._Statistic._Category;

			/* label number */
			ptr_this->_probeInfo[probeId]._nbLabels
				=
				(T_UINT16) probesAct._ActivatedProbes._Probe[i]._Statistic.
				_StatLabels._nbLabels;

			/* display flag */
			ptr_this->_probeInfo[probeId]._displayFlag
				= (T_BOOL) probesAct._ActivatedProbes._Probe[i]._DisplayFlag;

			/* probe type */
			ptr_this->_probeInfo[probeId]._type
				= (T_UINT8) probesAct._ActivatedProbes._Probe[i]._Statistic._Type;

			/* operator */
			ptr_this->_probeInfo[probeId]._aggregationMode
				= probesAct._ActivatedProbes._Probe[i]._AggregationMode;
			ptr_this->_probeInfo[probeId]._analysisOperator
				= probesAct._ActivatedProbes._Probe[i]._AnalysisOperator;
			ptr_this->_probeInfo[probeId]._operatorParameter
				= probesAct._ActivatedProbes._Probe[i]._OperatorParameter;

			ptr_this->_probeInfo[probeId]._frameNb = 0;

			/* allocate the  T_PROBE_VALUE structure */
			ptr_this->_ptr_probeValue[probeId]
				= (T_PROBE_VALUE *) malloc(sizeof(T_PROBE_VALUE)
													*
													(ptr_this->_probeInfo[probeId].
													 _nbLabels + 1));
			if(ptr_this->_ptr_probeValue[probeId] == NULL)
			{
				SEND_AG_ERRNO_JUMP(FIN, rid, C_ERROR_ALLOC, ptr_errorAgent,
										 C_ERROR_CRITICAL, 0, (C_TRACE_THREAD_UNKNOWN,
																	  C_TRACE_COMP_PROBE,
																	  C_TRACE_ERROR,
																	  "T_PROBE_VALUE malloc() failed"));
			}
			memset(ptr_this->_ptr_probeValue[probeId], 0,
					 sizeof(T_PROBE_VALUE) *
					 (ptr_this->_probeInfo[probeId]._nbLabels + 1));

			/* init the  T_PROBE_VALUE structure */
			for(j = 0; j <= ptr_this->_probeInfo[probeId]._nbLabels; j++)
			{
				ptr_this->_ptr_probeValue[probeId][j]._intValue = (T_UINT32) 0;
				ptr_this->_ptr_probeValue[probeId][j]._floatValue = (T_FLOAT) 0.0;
				ptr_this->_ptr_probeValue[probeId][j]._probeId = (T_UINT8) probeId;
				ptr_this->_ptr_probeValue[probeId][j]._index = (T_UINT16) j;
				ptr_this->_ptr_probeValue[probeId][j]._valueNumber = (T_UINT32) 0;
				ptr_this->_ptr_probeValue[probeId][j]._valueChange =
					C_PROBE_VALUE_EMPTY;
				/* check sliding windows */
				if((ptr_this->_controlerConf)
					&& (probesAct._ActivatedProbes._Probe[i]._AnalysisOperator >=
						 C_ANA_STANDARD_DEV))
				{
					SEND_AG_ERRNO_JUMP(FIN, rid,
											 CIRCULAR_BUFFER_Init(&
																		 (ptr_this->
																		  _ptr_probeValue[probeId]
																		  [j]._buffer),
																		 sizeof(T_UINT32),
																		 probesAct.
																		 _ActivatedProbes.
																		 _Probe[i].
																		 _OperatorParameter),
											 ptr_errorAgent, C_ERROR_CRITICAL, 0,
											 (C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE,
											  C_TRACE_ERROR,
											  "CIRCULAR_BUFFER_Init() failed"));
				}
				else
					CIRCULAR_BUFFER_Init(&
												(ptr_this->_ptr_probeValue[probeId][j].
												 _buffer), 0, 0);
			}

			TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_DEBUG,
						  "PROBE_HOLDER_Init() cmpt[%d] add stat id[%d] cat[%d] type[%d] agg[%d] with max index %d",
						  componentType, probeId,
						  ptr_this->_probeInfo[probeId]._categoryId,
						  ptr_this->_probeInfo[probeId]._aggregationMode,
						  ptr_this->_probeInfo[probeId]._type,
						  ptr_this->_probeInfo[probeId]._nbLabels));
		}
	}
	else if(rid == C_ERROR_FILE_OPEN)
	{									  /* no statistic conf file */

		/* Init internal data */
	 /*--------------------*/
		ptr_this->_statIsActivated = FALSE;
		ptr_this->_startFrame = (T_UINT32) 0;
		ptr_this->_stopFrame = (T_UINT32) 0;
		ptr_this->_samplingPeriod = (T_UINT32) 0;
		ptr_this->_displayFrame = (T_UINT32) 0;
		ptr_this->_lastFrame = (T_UINT32) 0;
		ptr_this->_nbStat = (T_UINT8) 0;

		ERROR_AGENT_SetLastErrorErrno(ptr_errorAgent, C_ERROR_MINOR,
												C_PROBE_ACT_FILE, C_ERROR_FILE_OPEN);

		TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
					  "PROBE_HOLDER_Init() cannot find statistic activation file for cmpt %d",
					  componentType));
		rid = C_ERROR_OK;
	}
	else
	{
		ERROR_AGENT_SetLastErrorErrno(ptr_errorAgent, C_ERROR_CRITICAL,
												C_PROBE_ACT_FILE, rid);
		TRACE_ERROR((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_ERROR,
						 "PROBES_ACTIVATION_ReadConfigFile() failed for cmpt %d",
						 componentType));
		goto FIN;
	}


	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PROBE_HOLDER_Init() sucessful"));

 FIN:
	return rid;
}


/*  @ROLE    : This function terminates the probe holder
    @RETURN  : Error code */
T_ERROR PROBE_HOLDER_Terminate(
											/* INOUT */ T_PROBE_HOLDER * ptr_this)
{
	T_INT32 i, j;

	for(i = 0; i < (C_PROB_MAX_STAT_NUMBER + 1); i++)
	{
		/* check the file descriptor */
	 /*---------------------------*/
		if(ptr_this->_probeInfo[i]._file != NULL)
			fclose(ptr_this->_probeInfo[i]._file);

		/* disallocate the probe value allocation */
	 /*----------------------------------------*/
		if(ptr_this->_ptr_probeValue[i] != NULL)
		{
			if(ptr_this->_ptr_probeValue[i][0]._buffer._circularBuf._buffer !=
				NULL)
			{
				for(j = 0; j < (ptr_this->_probeInfo[i]._nbLabels + 1); j++)
					CIRCULAR_BUFFER_Terminate(&
													  (ptr_this->_ptr_probeValue[i][j].
														_buffer));
			}
			free(ptr_this->_ptr_probeValue[i]);
		}
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_PROBE, C_TRACE_VALID,
				  "PROBE_HOLDER_Terminate() sucessful"));

	return C_ERROR_OK;
}
