/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
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
 * @file FilePath.c
 * @author TAS
 * @brief The FilePath class implements method to get exec path,
 *        scenario path and run path for curent simulation
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "FilePath_e.h"

#define C_CONF_DIRECTORY_PATH "/etc/opensand/env_plane/"
#define C_PROBE_DIRECTORY_PATH "%s/.opensand/"

#define C_RUN_DIRECTORY_NAME "/run_%d/" /* Generic run directory formatted name */
#define C_REF_DIRECTORY_NAME "/config/scenario_%d/" /* Generic reference directory formatted name */
#define C_SCENARIO_NAME "/scenario_%d/"   /* Generic output directory base name */
#define C_OUTPUT_DIRECTORY_NAME "/scenario_%d/run_%d/" /* Generic output directory formatted name */

static T_FILE_PATH BaseOutputPath; /* Defined as static because will only be set by 1 thread : main thread */
                                   /*   Other threads will only read this value */

/* TODO a quoi sert C_REF_DIRECTORY_NAME, ca ne devrait pas plutot etre /scenario_%d/ ???? */
T_ERROR FILE_PATH_InitClass()
{
	T_ERROR rid = C_ERROR_OK;
	T_STRING home;

	home = getenv("HOME");
	if(home == NULL)
	{
		fprintf(stderr, "Cannot find $HOME environment variable, "
		                 "use /tmp/ instead\n");
		home = "/tmp/";
	}
	if(access(C_CONF_DIRECTORY_PATH, R_OK) < 0)
	{
		fprintf(stderr, "Cannot access configuration path %s (%s)\n",
		        C_CONF_DIRECTORY_PATH, strerror(errno));
		JUMP(FIN, rid, C_ERROR_FILE_OPEN);
	}

	sprintf(BaseOutputPath, C_PROBE_DIRECTORY_PATH, home);
	/* create the path if it does no exists */
	if(access(BaseOutputPath, R_OK | W_OK) < 0)
	{
		if(mkdir(BaseOutputPath, 0755) < 0)
		{
			fprintf(stderr, "Cannot create configuration path %s (%s)\n",
			        BaseOutputPath, strerror(errno));
			JUMP(FIN, rid, C_ERROR_FILE_OPEN);
		}
	}

	TRACE_LOG((C_TRACE_THREAD_UNKNOWN, C_TRACE_COMP_ERROR, C_TRACE_VALID,
	           "Working with configPath=%s and outputPath=%s",
	           C_CONF_DIRECTORY_PATH, BaseOutputPath));

FIN:
	return rid;
}


T_ERROR FILE_PATH_GetConfPath(
										  /*   OUT */ T_STRING ptr_this)
{
	strcpy(ptr_this, C_CONF_DIRECTORY_PATH);

	return C_ERROR_OK;
}

T_ERROR FILE_PATH_GetScenarioPath(
												/*   OUT */ T_STRING ptr_this,
												/* IN    */ T_UINT16 reference)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH intermediateFileName;

	strcpy(ptr_this, C_CONF_DIRECTORY_PATH);
	if(sprintf(intermediateFileName, C_REF_DIRECTORY_NAME, reference) < 0)
	{
		rid = C_ERROR_FILE_OPEN;
		return rid;
	}

	JUMP_ERROR(FIN, rid, FILE_PATH_Concat(ptr_this, intermediateFileName));

	/* create the path if it does no exists */
	if(access(ptr_this, R_OK | W_OK) < 0)
	{
		if(mkdir(ptr_this, 0755) < 0)
		{
			fprintf(stderr, "Cannot create scenario path %s (%s)\n",
			        ptr_this, strerror(errno));
			JUMP(FIN, rid, C_ERROR_FILE_OPEN);
		}
	}

 FIN:
	return rid;
}


T_ERROR FILE_PATH_GetRunPath(
										 /*   OUT */ T_STRING ptr_this,
										 /* IN    */ T_UINT16 reference,
										 /* IN    */ T_UINT16 run)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH runPathName;

	/* Get scenario path */
	JUMP_ERROR(FIN, rid, FILE_PATH_GetScenarioPath(ptr_this, reference));

	/* Concatenate to get Run path */
	if(sprintf(runPathName, C_RUN_DIRECTORY_NAME, run) < 0)
		JUMP(FIN, rid, C_ERROR_FILE_OPEN);
   
	strcat(ptr_this, runPathName);

	/* create the path if it does no exists */
	if(access(ptr_this, R_OK | W_OK) < 0)
	{
		if(mkdir(ptr_this, 0755) < 0)
		{
			fprintf(stderr, "Cannot create run path %s (%s)\n",
			        ptr_this, strerror(errno));
			JUMP(FIN, rid, C_ERROR_FILE_OPEN);
		}
	}


 FIN:
	return rid;
}

T_ERROR FILE_PATH_GetOutputPath(
											 /*   OUT */ T_STRING ptr_this,
											 /* IN    */ T_UINT16 reference,
											 /* IN    */ T_UINT16 run)
{
	T_ERROR rid = C_ERROR_OK;
	T_FILE_PATH outputPathName;
	T_FILE_PATH outputScenario;
	T_FILE_PATH temp;

	/* Create the output base path if it does not exist */
	strcpy(outputScenario, BaseOutputPath);
	if(sprintf(temp, C_SCENARIO_NAME, reference) < 0)
		JUMP(FIN, rid, C_ERROR_FILE_OPEN);

	strcat(outputScenario, temp);

	if(access(outputScenario, R_OK | W_OK) < 0)
	{
		if(mkdir(outputScenario, 0755) < 0)
		{
			fprintf(stderr, "Cannot create output scenario path %s (%s)\n",
			        outputScenario, strerror(errno));
			JUMP(FIN, rid, C_ERROR_FILE_OPEN);
		}
	}

	strcpy(ptr_this, BaseOutputPath);
	/* Concatenate to get Run path */
	if(sprintf(outputPathName, C_OUTPUT_DIRECTORY_NAME, reference, run) < 0)
		JUMP(FIN, rid, C_ERROR_FILE_OPEN);

	strcat(ptr_this, outputPathName);

	/* create the path if it does no exists */
	if(access(ptr_this, R_OK | W_OK) < 0)
	{
		if(mkdir(ptr_this, 0755) < 0)
		{
			fprintf(stderr, "Cannot create output path %s (%s)\n",
			        ptr_this, strerror(errno));
			JUMP(FIN, rid, C_ERROR_FILE_OPEN);
		}
	}

FIN:
	return rid;
}



T_ERROR FILE_PATH_Concat(
									/*   OUT */ T_STRING ptr_this,
									/* IN    */ T_STRING file_name)
{
	T_ERROR rid = C_ERROR_OK;

	strcat(ptr_this, file_name);

	return rid;
}
