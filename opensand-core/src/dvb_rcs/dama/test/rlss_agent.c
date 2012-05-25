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

/*************************** IDENTIFICATION *****************************
**
** Project          : GAC
**
** Module           : Simulators/RLSS_agent
**
** File name        : rlss_agent.c
**
** Description      : MIB group implementation
**
** Reference(s)     : 
**
** Contact          : Poucet B.
**
** Creation date    : 03/06/2002
**
************************************************************************/ 
	
/*#######################################################################
 #                                                                      #
 #  INCLUDES                                                            #
 #                                                                      #
 ######################################################################*/ 
/* include important headers */ 
#include <config.h>
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
	
/* needed by util_funcs.h */ 
#if TIME_WITH_SYS_TIME
#   ifdef WIN32
#      include <sys/timeb.h>
#   else
#      include <sys/time.h>
#   endif
#   include <time.h>
#else
#   if HAVE_SYS_TIME_H
#      include <sys/time.h>
#   else
#      include <time.h>
#   endif
#endif
	
#if HAVE_WINSOCK_H
#   include <winsock.h>
#endif
#if HAVE_NETINET_IN_H
#   include <netinet/in.h>
#endif
	
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
	
/* mibincl.h contains all the snmp specific headers to define the return types and various defines and structures. */ 
#include "mibincl.h"
	
/* header_generic() comes from here */ 
#include "util_funcs.h"
	
#include "agent_read_config.h"
#include <agent_callbacks.h>
	
/* global variables */ 
unsigned long currentRequestId;



#define damaServPort 5555		  // OpenSAND interface
#define servIp "127.0.0.1"		  // OpenSAND interface
	
/* include our .h files */ 
#include "rlss_agent.h"
#include "mibdb.h"
#include "mibdb.c"
#include "rlss_trap.h"
#include "rlss_trap.c"
	
/*#######################################################################
 #                                                                      #
 #  INTERFACE CONSTANT AND INTERFACE VARIABLE DEFINITIONS               #
 #                                                                      #
 ######################################################################*/ 
/*   This array structure defines a representation of the MIB being implemented.
     The type of the array is 'struct variableN', where N is large enough to contain the longest OID sub-component being loaded.  
     This will normally be the maximum value of the fifth field in each line.  In this case, the second and third entries are both of size 2, 
     so we're using 'struct variable2'
     The supported values for N are listed in <agent/var_struct.h>
     If the value you need is not listed there, simply use the next largest that is.
     The format of each line is as follows (using the first entry as an example):
     	    1: EXAMPLESTRING:
     		The magic number defined in the example header file.
     		This is passed to the callback routine and is used to determine which object is being queried.
     	    2: ASN_OCTET_STR:
     		The type of the object. Valid types are listed in <snmp_impl.h>
     	    3: RONLY (or RWRITE):
     		Whether this object can be SET or not.
     	    4: var_example:
     		The callback routine, used when the object is queried.
     		This will usually be the same for all objects in a module	and is typically defined later in this file.
 	    5: 1:
     		The length of the OID sub-component (the next field)
     	    6: {1}:
     		The OID sub-components of this entry.
     		In other words, the bits of the full OID that differ between the various entries of this array.
     		This value is appended to the common prefix (defined later) to obtain the full OID of each entry.
     */ 
	
{
1, 3, 6, 1, 4, 1, 3937, 1, 2, 6, 1, 2, 1, 1};

		{MN_sitDescr, ASN_OCTET_STR, RWRITE, var_mibdb_simplevar, 1, {1}}, 
	{MN_tmSitId, ASN_OCTET_STR, RWRITE, var_mibdb_simplevar, 1, {2}}, 
	{MN_tmMsgLatency, ASN_INTEGER, RWRITE, var_mibdb_simplevar, 1, {3}}, 
	{MN_terminalId, ASN_INTEGER, RONLY, var_mibdb_tablevar, 3, {4, 1, 1}}, 
	{MN_terminalMacAddr, ASN_OCTET_STR, RONLY, var_mibdb_tablevar, 3, {4, 1, 2}},
	
	
	
	
	
	
	  {4, 1, 8}}, 
						 3, {4, 1, 9}}, 
												var_mibdb_tablevar, 3, {4, 1, 10}},
	
	  {4, 1, 11}}, 
						  var_mibdb_tablevar, 3, {4, 1, 12}}, 
																			 ASN_OCTET_STR, RONLY,
																			 var_mibdb_tablevar, 3,
																			 {4, 1, 13}},
	
	  {4, 1, 14}}, 
						  var_mibdb_simplevar, 1, {7}}, 
};


{
1, 3, 6, 1, 4, 1, 3937, 1, 2, 6, 4, 2, 1, 1};

		{MN_rmRlssState, ASN_INTEGER, RONLY, var_mibdb_tablevar, 1, {3}}, 
	{MN_rmRlssPrimeShipFlag, ASN_INTEGER, RONLY, var_mibdb_tablevar, 1, {5}}, 
	{MN_rmRlssPrimeIpAddress, ASN_IPADDRESS, RONLY, var_mibdb_tablevar, 1, {6}},
	
	  {7}}, 
};


/*#######################################################################
 #                                                                      #
 #  INTERFACE FUNCTION DEFINITIONS                                      #
 #                                                                      #
 ######################################################################*/ 
/************************************************************************
** Function Name     : init_rlss_agent
** Input Parameters  : none
** Output Parameters : none
** Return Value      : none
** Preconditions     : 
** Description       : This function is called at the time the agent starts up
**                           to do any initializations that might be required.
** Postconditions    :
** Exceptions        : 
** Notes             : 
** Examples          : 
** See Also          : 
*/ 
void init_rlss_agent(void) 
{
	
/* Register ourselves with the agent to handle our mib tree.
   The arguments are:
         descr:   A short description of the mib group being loaded.
         var:     The variable structure to load. (the name of the variable structure defined above)
         vartype: The type of this variable structure
         theoid:  The OID pointer this MIB is being registered underneath.
*/ 
	struct sockaddr_in echoServAddr;	/* DAMA server address */
	
						satelliteTerminal_variables_oid);
	
						admin_variables_oid);
	
									 
									 0);
	
	
		/* opensand interface */ 
		/* open a socket to the DAMA */ 
		if((damaSocket = socket(AF_INET, SOCK_STREAM, 0)))
		
	{
		
			/* Construct the server address structure */ 
			memset(&damaServAddr, 0, sizeof(damaServAddr));	/* Zero out structure */
		
		
		
		
			/* Establish the connection to the echo server */ 
			if(connect
				(damaSocket, (struct sockaddr *) &damaServAddr,
				 sizeof(damaServAddr)) < 0)
			
		{
			
		
	
	
	else
		
	{
		
	



/*! 
 * \brief incoming PDU request client-function. This function
 * is intended to be used as a callback, supplied to the agent
 * framework.
*/ 
int requestCallback(int majorID, int minorID, void *serverarg,
						  void *clientarg) 
{
	
	
	
	


{
	
	
	
	
					  
	



/************************************************************************
** Function Name     : var_mibdb_simplevar
** Input Parameters  : entry in the variableN array, OID from the request, length of the OID, flag to indicate an GET/SET request or GETNEXT request
**                              pointer to the SET fumction
** Output Parameters : OID being returned, length of the OID, length of the answer being returned
** Return Value      : pointer to the appropriate value
** Preconditions     : 
** Description       : Callback routine used when the object is queried for simple instance
** Postconditions    :
** Exceptions        : 
** Notes             : 
** Examples          : 
** See Also          : 
*/ 
	u_char * var_mibdb_simplevar(struct variable * vp, 
										  
										  
{
	
	
	
	
	
	
		/* Before returning an answer, we need to check that the request refers to a valid instance of this object.  
		   The utility routine 'header_generic' can be used to do this for scalar objects. */ 
		if(header_generic(vp, name, length, exact, var_len, write_method) ==
			MATCH_FAILED)
		
	
		/* fetch the value from the database */ 
		value = mibdb_get_simplevar(name, *length);
	
	{
		
			/* value could not be retrieved */ 
			return NULL;
	
	
		
	{									  /* write_method assumes that the object can be set */
		
	
	
	
		free(value);
	



/************************************************************************
** Function Name     : var_mibdb_tablevar2
** Input Parameters  : entry in the variableN array, OID from the request, length of the OID, flag to indicate an GET/SET request or GETNEXT request
**                              pointer to the SET fumction
** Output Parameters : OID being returned, length of the OID, length of the answer being returned
** Return Value      : pointer to the appropriate value
** Preconditions     : 
** Description       : Callback routine used when the object is queried for table column instance
** Postconditions    :
** Exceptions        : 
** Notes             : 
** Examples          : 
** See Also          : 
*/ 
	u_char * var_mibdb_tablevar(struct variable * vp, 
										 
										 
{
	
		/* the snmp vars handled here are supposed to be table column instances */ 
	
	
	
	
	
	
	
	
	
	
		
	{
		
			 == MATCH_FAILED)
		{
			
				var_len = 0;
			
		
	
	
		/* fetch the value from the database */ 
		value = mibdb_get_tablevar(vp, name, length, exact);
	
	{
		
			var_len = 0;
		
	
	
	{
		
	
	
	
					 "mibdb_dbvalue2value returned value : %d (%s)\n", *ret, value));
	
		/* send CRA information to the DAMA if needed */ 
		if(vp->magic == MN_terminalCraTrfc)
	{
		
		{
			
			
			
				/* Send the string to the server */ 
				if(send(damaSocket, send_buff, stringLen, 0) != stringLen)
			{
				
			
		
	
	
	



/************************************************************************
** Function Name     : write_mibdb_simplevar
** Input Parameters  : action variable, new desired value, value type, value length, value returned from a GET (ignore)
**                             OID to be set, length of the OID
** Output Parameters : none
** Return Value      : status of the SET (successful or not)
** Preconditions     : 
** Description       : Writeable object SET handling routine for simple instance
** Postconditions    :
** Exceptions        : 
** Notes             : 
** Examples          : 
** See Also          : 
*/ 
int write_mibdb_simplevar(int action, 
								  
								  
{
	
	
					  "write_mibdb_simplevar entered, pass %d\n", action));
	
	{
	
		
			/* Check that the value being set is acceptable */ 
			switch (var_val_type)
		{
		
			
		
		
		
			
			{
				
								 "write_mibdb_simplevar--wrong length %x for type %d",
								 var_val_len, var_val_type));
				
			
			
		
			
			{
				
								 "write_mibdb_simplevar--wrong length %x for ASN_IPADDRESS",
								 var_val_len));
				
			
			
		
		
	
		
			/* This is conventially where any necesary resources are allocated (e.g. calls to malloc)
			   Here, we are using static variables so don't need to worry about this. */ 
			beginTransaction();
		
	
		
			/* This is where any of the above resources are freed again (because one of the other
			   values being SET failed for some reason). Again, since we are using static variables
			   we don't need to worry about this either. */ 
			break;
	
		
		
		
	
		
			/* Something failed, so re-set the variable to its previous value (and free any allocated resources). */ 
			abortTransaction();
		
	
		
			/* Everything worked, so we can discard any saved information, and make the change permanent.
			   We also free any allocated resources. In this case, there's nothing to do. */ 
			/* Set the variable as requested */ 
			endTransaction();
		
	
	



/************************************************************************
** Function Name     : write_mibdb_tablevar
** Input Parameters  : action variable, new desired value, value type, value length, value returned from a GET (ignore)
**                             OID to be set, length of the OID
** Output Parameters : none
** Return Value      : status of the SET (successful or not)
** Preconditions     : 
** Description       : Writeable object SET handling routine for table column instance
** Postconditions    :
** Exceptions        : 
** Notes             : 
** Examples          : 
** See Also          : 
*/ 
int write_mibdb_tablevar(int action, 
								 
								 
{
	
	
					  "write_mibdb_tablelevar entered, pass %d\n", action));
	
	{
	
		
			/* Check that the value being set is acceptable */ 
			switch (var_val_type)
		{
		
			
		
		
		
			
			{
				
								 "write_mibdb_tablelevar--wrong length %x for type %d",
								 var_val_len, var_val_type));
				
			
			
		
			
			{
				
								 "write_mibdb_tablelevar--wrong length %x for ASN_IPADDRESS",
								 var_val_len));
				
			
			
		
		
	
		
			/* This is conventially where any necesary resources are allocated (e.g. calls to malloc)
			   Here, we are using static variables so don't need to worry about this. */ 
			beginTransaction();
		
	
		
			/* This is where any of the above resources are freed again (because one of the other
			   values being SET failed for some reason). Again, since we are using static variables
			   we don't need to worry about this either. */ 
			break;
	
		
		
		
	
		
			/* Something failed, so re-set the variable to its previous value (and free any allocated resources). */ 
			abortTransaction();
		
	
		
			/* Everything worked, so we can discard any saved information, and make the change permanent.
			   We also free any allocated resources. In this case, there's nothing to do. */ 
			/* Set the variable as requested */ 
			endTransaction();
		
	
	


