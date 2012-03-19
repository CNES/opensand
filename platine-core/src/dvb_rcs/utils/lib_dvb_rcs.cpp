/**
 * @file lib_dvb_rcs.cpp
 * @brief This library defines DVB-RCS messages and provides helper functions.
 */

#include <string.h>
#include <math.h>

#include "lib_dvb_rcs.h"
#define DBG_PACKAGE PKG_DVB_RCS
#include "platine_conf/uti_debug.h"


/**
 * return the ith frame pointer associated with a buffer pointing to a
 * T_DVB_TBTP struct. Count is done from 0. There is no error check, the
 * structure must be correctly filled.
 * @param i the index
 * @param buff the pointer to the T_DVB_TBTP struct
 */
T_DVB_FRAME * ith_frame_ptr(int i, unsigned char *buff)
{
	int j;
	T_DVB_FRAME *ret;

	ret = first_frame_ptr(buff);
	for(j = 0; j < i; j++)
	{
		ret = next_frame_ptr(ret);
	};

	return ret;
}

/**
 *
 * Purpose : Compute the number of specified steps within the input value
 *
 *   @param ExactValue IN: exact value - double
 *   @param Step IN: step  value
 *   @return : integer nb of steps within the input value
 */

int set_multiple_value(int ExactValue, int Step)
{
	int TestMultipleQuot, TestMultipleRem;
	int ReturnedValue;

	/* compute quotient and reminder of integer division */
	TestMultipleQuot = ExactValue / Step;
	TestMultipleRem = ExactValue % Step;

	/* approximate to the nearest value : */
	/* previous value */
	if((TestMultipleRem < (Step / 2)) || (TestMultipleRem == 0))
	{
		ReturnedValue = TestMultipleQuot;
	}
	/* or next value */
	else
	{
		ReturnedValue = TestMultipleQuot + 1;
	}

	/* function return */
	return (ReturnedValue);
};


/**
 * Encode the capacity request xbdc and scaling factor in function of the request type and the requested capacity
 * The capacity request type must be set before the call
 * @param Cr is the pointer to the request structure
 * @param CapacityRequest is the requested capacity
 * @return 0 if success, -1 otherwhise
 */
int encode_request_value(T_DVB_SAC_CR_INFO * Cr, int CapacityRequest)
{
	if(Cr->type == DVB_CR_TYPE_VBDC)
	{
		if(CapacityRequest <= DVB_CR_VBDC_SCALING_FACTOR_OFFSET)
		{
			Cr->xbdc = (long) CapacityRequest;
			Cr->scaling_factor = 0;
		}
		else
		{
			Cr->xbdc =
				(long) set_multiple_value(CapacityRequest,
				                          DVB_CR_VBDC_SCALING_FACTOR);
			Cr->scaling_factor = 1;
		}
	}
	else if(Cr->type == DVB_CR_TYPE_RBDC)
	{
		if(CapacityRequest <= DVB_CR_RBDC_SCALING_FACTOR_OFFSET)
		{
			Cr->xbdc =
				(long) set_multiple_value(CapacityRequest,
				                          DVB_CR_RBDC_GRANULARITY);
			Cr->scaling_factor = 0;
		}
		else
		{
			Cr->xbdc =
				(long) set_multiple_value(CapacityRequest,
				                          DVB_CR_RBDC_SCALING_FACTOR);
			Cr->scaling_factor = 1;
		}
	}
	else
		return (-1);
	return (0);
}

/**
 * Decode the capacity request in function of the xbdc and scaling factor
 * @param Cr is the pointer to the request structure
 * @return Request Capacity if success, -1 otherwhise
 */
int decode_request_value(T_DVB_SAC_CR_INFO * Cr)
{
	int Request;

	if(Cr->type == DVB_CR_TYPE_VBDC)
	{
		if(Cr->scaling_factor == 0)
			Request = (int) Cr->xbdc;
		else
			Request = (int) Cr->xbdc * DVB_CR_VBDC_SCALING_FACTOR;
	}
	else if(Cr->type == DVB_CR_TYPE_RBDC)
	{
		if(Cr->scaling_factor == 0)
			Request = (int) Cr->xbdc * DVB_CR_RBDC_GRANULARITY;
		else
			Request = (int) Cr->xbdc * DVB_CR_RBDC_SCALING_FACTOR;
	}
	else
		return -1;

	return Request;
}

