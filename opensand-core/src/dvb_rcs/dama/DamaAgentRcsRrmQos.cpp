/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file    DamaAgentRcsRrmQos.cpp
 * @brief   Implementation of the DAMA agent for DVB-S2 emission standard.
 * @author  Audric Schiltknecht / Viveris Technologies
 * @author  Julien Bernard / Viveris Technologies
 *
 */


#include "DamaAgentRcsRrmQos.h"

#include <opensand_output/Output.h>

#include <algorithm>
#include <cmath>

/*** Request computing parameters  ***/
// TO DO: To read in conf

// Options for the alpha parameter
// 0 for alpha = 0
// 1 for alpha = 1
// 2 for alpha computed dynamically
#define ALPHA 2

// Options for the beta parameter
// 0 for beta = 0
// 1 for beta = 1
// 2 for beta computed dynamically
#define BETA 0

// Options to add the CRA to the last RBDC request (but not add it to the sent
// RBDC request in order to allow a better (a priori) alpha computing
// 0 to not add the CRA to the last RBDC request
// 1 to add the CRA to the last RBDC request
#define CRA 0

// Options to ignore the Modification 1 in the RBDC request computing
// 0 to use the  Modification 1
// 1 to not use the Modification 1
#define WITHOUT_MODIF_1 1

// Option to use the Legacy algorithm instead of the RRM-QoS Algorithm
// It is not a part of the R&T RRM-QoS but only used to compare it with
// the new algorithm
// 0 to use the RRM-QoS algorithm
// 1 to use the Legacy algorithm
#define LEGACY 0

// constants
const rate_kbps_t C_MAX_RBDC_IN_SAC = 8160.0; // 8160 kbits/s, limitation due
                                              // to CR value size in to SAC field
const vol_pkt_t C_MAX_VBDC_IN_SAC = 4080;     // 4080 packets/ceils, limitation
                                              // due to CR value size in to SAC field

using std::max;
using std::min;

DamaAgentRcsRrmQos::DamaAgentRcsRrmQos():
	DamaAgentRcs(),
	vbdc_credit_pkt(0)
{
}

DamaAgentRcsRrmQos::~DamaAgentRcsRrmQos()
{
	if(this->rin != NULL)
	{
		delete this->rin;
	}

	if(this->rin_coeff != NULL)
	{
		delete this->rin_coeff;
	}
	if(this->dyn_alloc != NULL)
	{
		delete this->dyn_alloc;
	
	}
}

bool DamaAgentRcsRrmQos::init()
{
	if(!DamaAgentRcs::init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Cannot call DamaAgentRcs::init()");
		return false;
	}

	if(this->rbdc_enabled)
	{
		double sum_rin_coeff = 0; // Sum of the coefficient applied to the incoming
		                          // rates of each frame (Rin)
		
		// Init the dyn alloc circular buffer	
		if ((this->sync_period_sf - this->msl_sf) >= 0)
			this->dyn_alloc = new CircularBuffer(this->sync_period_sf);
		else
		{
			this->dyn_alloc = new CircularBuffer(this->sync_period_sf);
			LOG(this->log_init, LEVEL_INFO,
			    "the time between two requests (syncPeriod) is "
			    "inferior to the Minimum Scheduling Latency (MSL), "
			    "this case should not be used in the context of "
			    "the RRM-QoS. However, the simulation is able to "
			    "continue with some simplifications of the request "
			    "computation algorithm (alpha =0)");
		}
		if (this->dyn_alloc == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create circular buffer to save "
			    "the last allocations\n");
			goto error;
		}

		// create circular buffer for saving the incoming rates during each frame
		// of the last SYNC period
		this->rin = new CircularBuffer(this->sync_period_sf);
		if(this->rin == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create circular buffer to save "
			    "the incoming rates\n");
			goto error;
		}

		// create and intialize circular buffer for saving the coefficient to
		// apply to the incoming rates
		this->rin_coeff = (double *) malloc(this->sync_period_sf * sizeof(double));
		if(this->rin_coeff == NULL)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot allocate memory to save "
			    "the incoming rate coefficients\n");
			goto error;
		}
		for (int i = 0; i < this->sync_period_sf; i++)
		{
			/*** TO DO: To read in conf in the next versions ***/
			this->rin_coeff[i] = 1.0/this->sync_period_sf;
			sum_rin_coeff += this->rin_coeff[i];
		}
		if (sum_rin_coeff != 1.0)
			LOG(this->log_init, LEVEL_NOTICE,
			    "the sum of the coefficient is not equal to 1.0. "
			    "It is not a problem for the simulation run but "
			    "the computation request algorithm has no sense\n");
	}

	return true;

 error:
	return false;
}

bool DamaAgentRcsRrmQos::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgentRcs::hereIsSOF(superframe_number_sf))
	 {
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: cannot call DamaAgentRcs::hereIsSOF()\n",
		    this->current_superframe_sf);
		return false;
	}

	this->rbdc_timer_sf++;
	// update dynamic allocation for next SF with allocation received
	// through TBTP during last SF
	this->dynamic_allocation_pkt = this->allocated_pkt;
	this->dyn_alloc->Update(this->allocated_pkt);
	this->allocated_pkt = 0;

	return true;
}

rate_kbps_t DamaAgentRcsRrmQos::computeRbdcRequest()
{
	rate_kbps_t rbdc_request_kbps;
	double rate_need_kbps;
	vol_b_t rbdc_length_b;
	//vol_b_t rbdc_pkt_arrival_b;
	//rate_kbps_t rbdc_req_in_previous_msl_kbps;
	rate_kbps_t last_rbdc_req_kbps; // Last rbdc request
	time_ms_t t_sync_ms;            // Time (in sec) between two requests
	time_ms_t t_loop_ms;            // Minimum Scheduling Latency (MSL) in sec
	double beta;  // Congestion factor 
	double alpha; // Congestion and anticipation factor
	rate_kbps_t rin_weighted_kbps;  // Weighted mean incoming trafic at layer 2
	                                // corresponding to RBDC associated queues
	                                // // in kbps
	rate_kbps_t alloc_since_last_request; // Allocation in kbps for 
	                                      // the last (t_sync - t_loop)
	

	/* get number of outstanding packets in RBDC related MAC FIFOs */
	rbdc_length_b =
		this->converter->pktToBits(this->getMacBufferLength(access_dama_rbdc));

	// Get number of packets arrived in RBDC related IP FIFOs since
	// last RBDC request sent
	// NB: arrivals in MAC FIFOs must NOT be taken into account because these
	// packets represent only packets buffered because there is no
	// more available allocation, but their arrival has been taken into account
	// in IP fifos
	//TODO: remove ?
	//rbdc_pkt_arrival_b =
		//this->converter->pktToBits(this->getMacBufferArrivals(access_dama_rbdc));

	// get the sum of RBDC request during last MSL
	// TODO: remove ?
	//rbdc_req_in_previous_msl_kbps = this->rbdc_request_buffer->GetSum();

	// get the last RBDC request value
	last_rbdc_req_kbps = this->rbdc_request_buffer->GetLastValue();

	// Get the MSL
	t_loop_ms = this->msl_sf * this->frame_duration_ms;

	// Get the time between two requests
	t_sync_ms = this->sync_period_sf * this->frame_duration_ms;

	if (LEGACY == 1) // Use the legacy algorithm instead of the RRM-QoS one
	{
		goto Legacy; 
	}

    // Compute beta
	/*** TO DO: Add a parameter in conf to choose between beta = 1, beta = 0 
	 *** and beta = dynamic ***/
    
	if (BETA == 2) // beta computed dynamically 
	{
		if (last_rbdc_req_kbps != 0)
		{
			beta = ((t_sync_ms * last_rbdc_req_kbps) - 
			        this->dyn_alloc->GetSum()) /
			       (t_sync_ms * last_rbdc_req_kbps);
		}
		else
		{
			beta = 1.0; // RRM-QoS: Modification 3
		}
		if (beta < 0)
		{
			beta = 0.0; // RRM-QoS: Modficiation 8
		}
	}
	else if (BETA == 1) // beta = 1
	{
		beta = 1.0;
	}
	else if (BETA == 0) // beta = 0
	{
		beta = 0.0; 
	}
	else // Unknown value for the beta parameter
	{
		LOG(this->log_request, LEVEL_WARNING,
		    "WARNING: Unknown value for the beta parameter\n");
		LOG(this->log_request, LEVEL_WARNING, 
		    "WARNING: beta is set to 0\n");
		beta = 0.0;
	}
	LOG(this->log_request, LEVEL_DEBUG,
	    "beta = %f\n", beta);

	// Compute alpha
	/*** TO DO: Add a parameter in conf to choose between alpha = 1, alpha = 0
	 *** and alpha = dynamic ***/
	if (ALPHA == 2) // alpha computed dynamically
	{
		if (t_sync_ms > t_loop_ms)
		{
			// Usual case (in the context of the R&T RRM-QoS)
			LOG(this->log_request, LEVEL_DEBUG,
			    "this->dyn_alloc_PartialSum = %d pkt\n",
			    this->dyn_alloc->GetPartialSumFromPrevious(
			    this->sync_period_sf - this->msl_sf));
			LOG(this->log_request, LEVEL_DEBUG,
			    "this->dyn_alloc_Sum = %d pkt\n",
			    this->dyn_alloc->GetSum());
			LOG(this->log_request, LEVEL_DEBUG,
			    "t_sync_ms - t_loop_ms = %u sec\n",
			    (t_sync_ms - t_loop_ms));
			if (last_rbdc_req_kbps > 0)
			{
				alpha =
					(this->dyn_alloc->GetPartialSumFromPrevious(
									this->sync_period_sf - this->msl_sf)) /
					((t_sync_ms - t_loop_ms) * last_rbdc_req_kbps);
			}
			else
			{
				alpha = 1.0; // RRM-QoS: Modification 2
			}
		}
		else if (t_sync_ms == t_loop_ms)
		{
			// Unusual case (in the context of the R&T RRM-QoS)
			if (last_rbdc_req_kbps > 0) // RRM-QoS: Modification 4
			{
				alpha = this->dyn_alloc->GetPreviousValue() /
				        (t_sync_ms * last_rbdc_req_kbps);
			}
			else
			{
				alpha = 1.0; // RRM-QoS: Modification 2
			}
		}
		else
		{
			// Nonused case in the R&T RRM-QoS
			LOG(this->log_request, LEVEL_NOTICE,
			    "the time between two requests (syncPeriod) is "
			    "inferior to the Minimum Scheduling Latency (MSL), "
			    "this case should not be used in the context of "
			    "the RRM-QoS. However, the simulation is able to "
			    "continue with alpha = 1\n");
			alpha = 1.0;
		}
		if (alpha > 1.0)
		{
	       alpha = 1.0;
	    }
	}
	else if (ALPHA == 1) // alpha = 1
	{
		alpha = 1.0;
	}
	else if (ALPHA == 0) // alpha = 0
	{
	    alpha = 0.0;
	}
	else // Unknown value for the alpha parameter
	{
		LOG(this->log_request, LEVEL_WARNING,
		    "WARNING: Unknown value for the alpha parameter\n");
		LOG(this->log_request, LEVEL_WARNING,
		    "WARNING: alpha is set to 1\n");
		alpha = 1.0;
	}
	LOG(this->log_request, LEVEL_DEBUG,
	    "alpha = %f\n", alpha);

	// Compute Rin in cell/sec
	rin_weighted_kbps = 0;
	for (int i = 0; i < this->sync_period_sf ; i++)
	{
		rin_weighted_kbps += (this->rin->GetValueIndex(i+1)) * 
			(this->rin_coeff[i]); // in cell/frame
		LOG(this->log_request, LEVEL_DEBUG,
		    "Rin to add = %d cell/frame\n", 
		    this->rin->GetValueIndex(i + 1));
		LOG(this->log_request, LEVEL_DEBUG,
		    "RinCoeff = %f\n", this->rin_coeff[i]);
		LOG(this->log_request, LEVEL_DEBUG,
		    "rin_weighted_kbps to add = %f cell/frame\n", 
		    (this->rin->GetValueIndex(i + 1)) * (this->rin_coeff[i]));
	}
	rin_weighted_kbps /= this->frame_duration_ms; // in cell/sec
	LOG(this->log_request, LEVEL_DEBUG,
	    "FrameDuration = %ums?\n", this->frame_duration_ms);
	LOG(this->log_request, LEVEL_DEBUG,
	    "rin_weigthed = %d kbps\n", rin_weighted_kbps);

	// Compute rate_need_kbps : estimation of the need of bandwith for traffic
	// in cell/sec (core of the algorithm)
	alloc_since_last_request = 
		this->dyn_alloc->GetPartialSumFromPrevious(this->sync_period_sf - this->msl_sf) / 
		                                           (t_sync_ms - t_loop_ms);
	if((last_rbdc_req_kbps < alloc_since_last_request) || 
	  (WITHOUT_MODIF_1 == 1)) // RRM-QoS: Modification 1 
	{
		if(rbdc_length_b + (alpha * t_loop_ms * rin_weighted_kbps) > 
		   t_loop_ms * last_rbdc_req_kbps * (1 - beta))
		{
			rate_need_kbps = (rbdc_length_b - (t_loop_ms * last_rbdc_req_kbps * (1 - beta)) + 
			                  (alpha * (t_sync_ms + t_loop_ms) * rin_weighted_kbps)) /
			                 t_sync_ms;
		} 
		else // RRM-QoS: Modification 6 - Option 2
		{
			rate_need_kbps = alpha * rin_weighted_kbps;
			LOG(this->log_request, LEVEL_DEBUG,
			    "alpha = %f\n", alpha);
			LOG(this->log_request, LEVEL_DEBUG,
			    "rin_weighted_kbps = %d\n", rin_weighted_kbps);
			LOG(this->log_request, LEVEL_DEBUG,
			    "rate_need_kbps = %f\n", rate_need_kbps);
		}
	} 
	else // last_rbdc_req_kbps >= alloc_since_last_request
	{
		if(rbdc_length_b + (alpha * t_loop_ms * rin_weighted_kbps) > 
		   t_loop_ms * alloc_since_last_request * (1 - beta))
		{
			rate_need_kbps = 
				(rbdc_length_b - (t_loop_ms * alloc_since_last_request * (1 - beta)) +
				 (alpha * (t_sync_ms + t_loop_ms) * rin_weighted_kbps)) /
				t_sync_ms;
		}
		else // RRM-QoS: Modification 6 - Option 2
		{
			rate_need_kbps = alpha * rin_weighted_kbps; 
			LOG(this->log_request, LEVEL_DEBUG,
			    "alpha = %f\n", alpha);
			LOG(this->log_request, LEVEL_DEBUG,
			    "rin_weighted_kbps = %d\n", rin_weighted_kbps);
			LOG(this->log_request, LEVEL_DEBUG,
			    "rate_need_kbps = %f\n", rate_need_kbps);
		}
	}
	if(rate_need_kbps < 0)
	{
		rate_need_kbps = 0;
	}

	LOG(this->log_request, LEVEL_DEBUG,
	    "frame = %d, rate_need_kbps = %3.f cell/s\n", 
	    this->current_superframe_sf, rate_need_kbps);

Legacy:
	if (LEGACY == 1)
	{
		rate_kbps_t rin_kbps = 0.0;
		rin_kbps = 0;
		for (int i = 0; i < this->sync_period_sf ; i++)
		{
			rin_kbps += (this->rin->GetValueIndex(i + 1) * 1.0 / 
			             this->sync_period_sf); // in cell/frame
		}
		if (rbdc_length_b > rin_kbps * t_sync_ms)
		{
			rate_need_kbps = rin_kbps + ( (rbdc_length_b - (rin_kbps * t_sync_ms)) / t_loop_ms );
		}
		else // rbdc_length_b < rin * t_sync_ms
		{
			rate_need_kbps = rin_kbps;
		}
	}
	
	// Compute actual RBDC request to be sent in Kbit/sec
	rbdc_request_kbps = (int) rate_need_kbps; 
	LOG(this->log_request, LEVEL_DEBUG,
	    "frame=%d,  theoretical rbdc_request_kbps = %d kbits/s",  
	    this->current_superframe_sf, rbdc_request_kbps);


	// Check if the RBDCmax is not reached
	if (rbdc_request_kbps > this->max_rbdc_kbps)
	{
		rbdc_request_kbps = this->max_rbdc_kbps;
	}
	// Deduct the CRA from the RBDC request
	if ((rbdc_request_kbps - this->cra_kbps) < 0) 
	{
		rbdc_request_kbps = 0;
	}
	else 
	{
		rbdc_request_kbps -= this->cra_kbps;
	}

	// Reduce the request value to the maximum theorical value 
	// and use the following units 2kbits/s or 16kbits/s 
	// in order to observe the DVB-RCS standard
	// RRM-QoS: Modification 7
	rbdc_request_kbps = min(rbdc_request_kbps, C_MAX_RBDC_IN_SAC);
	LOG(this->log_request, LEVEL_DEBUG,
	    "frame=%d,  updated rbdc_request_kbps = %d kbits/s in "
	    "SAC\n", this->current_superframe_sf, rbdc_request_kbps);

	LOG(this->log_request, LEVEL_DEBUG, "Sending request *** \n");
	    
	return rbdc_request_kbps;
}

vol_pkt_t DamaAgentRcsRrmQos::computeVbdcRequest()
{
	vol_pkt_t vbdc_need_pkt;
	vol_pkt_t vbdc_request_pkt;
	vol_pkt_t max_vbdc_pkt = this->converter->kbitsToPkt(this->max_vbdc_kb);
	// TODO there is a problem with vbdc_credit ! it is not decreased !
	//      At the moment, set 0
	//      we may decrease it from allocated packets number
	//      or from the number of packets removed in fifo with
	//      getRemoved accessor and resetRemoved in FIFO
	//      Whatever, the VBDC algorithm is very bad !
	this->vbdc_credit_pkt = 0;

	/* get number of outstanding packets in VBDC related MAC
	 * and IP FIFOs (in packets number) */
	vbdc_need_pkt = this->getMacBufferLength(access_dama_vbdc);
	LOG(this->log_request, LEVEL_DEBUG,
	    "SF#%u: MAC buffer length = %d, VBDC credit = "
	    "%u\n", this->current_superframe_sf,
	    vbdc_need_pkt, this->vbdc_credit_pkt);

	/* compute VBDC request: actual Vbdc request to be sent */
	vbdc_request_pkt = max(0, (vbdc_need_pkt - this->vbdc_credit_pkt));
	LOG(this->log_request, LEVEL_DEBUG,
	    "SF#%u: theoretical VBDC request = %u packets",
	    this->current_superframe_sf,
	    vbdc_request_pkt);

	/* adjust request in function of max_vbdc value */
	vbdc_request_pkt = min(vbdc_request_pkt, max_vbdc_pkt);

	// Ensure VBDC request value is not greater than SAC field
	vbdc_request_pkt = min(vbdc_request_pkt, C_MAX_VBDC_IN_SAC);
	LOG(this->log_request, LEVEL_DEBUG,
	    "updated VBDC request = %d packets in fonction of "
	    "max VBDC and max VBDC in SAC\n", vbdc_request_pkt);

	/* update VBDC Credit here */
	/* NB: the computed VBDC is always really sent if not null */
	this->vbdc_credit_pkt += vbdc_request_pkt;
	LOG(this->log_request, LEVEL_DEBUG,
	    "updated VBDC request = %d packets in SAC, VBDC credit = "
	    "%u\n", vbdc_request_pkt, this->vbdc_credit_pkt);

	return vbdc_request_pkt;
}


