/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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
	cra_in_cr(false),
	rbdc_timer_sf(0),
	vbdc_credit_pkt(0)
{
}

DamaAgentRcsRrmQos::~DamaAgentRcsRrmQos()
{
	delete this->ret_schedule;

	if(this->rbdc_request_buffer != NULL)
	{
		delete this->rbdc_request_buffer;
	}

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
	delete this->converter;
}

bool DamaAgentRcsRrmQos::init()
{
	this->ret_schedule = new ReturnSchedulingRcs(this->packet_handler,
	                                             this->dvb_fifos);

	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_none)
		{
			this->cra_in_cr = true;
		}
	}

	if(this->rbdc_enabled)
	{
		double sum_rin_coeff = 0; // Sum of the coefficient applied to the incoming
								  // rates of each frame (Rin)
		
		// create circular buffer for saving last RBDC requests during the past
		// MSL duration with size = integer part of MSL / OBR period
		// (in frame number)
		// NB: if size = 0, only last req is saved and sum is always 0
		this->rbdc_request_buffer =
			new CircularBuffer((size_t) this->msl_sf / this->obr_period_sf);
		if(this->rbdc_request_buffer == NULL)
		{
			UTI_ERROR("Cannot create circular buffer to save "
			          "the last RBDC requests\n");
			goto error;
		}

		// Init the dyn alloc circular buffer	
		if ((this->obr_period_sf - this->msl_sf) >= 0)
			this->dyn_alloc = new CircularBuffer(this->obr_period_sf);
		else
		{
			this->dyn_alloc = new CircularBuffer(this->obr_period_sf);
			UTI_DEBUG("the time between two requests (obrPeriod) is inferior "
					 "to the Minimum Scheduling Latency (MSL), this case should "
					 "not be used in the context of the RRM-QoS. However, the "
					 "simulation is able to continue with some simplifications "
					 "of the request computation algorithm (alpha =0)");
		}
		if (this->dyn_alloc == NULL)
		{
			UTI_ERROR("cannot create circular buffer to save "
					  "the last allocations\n");
			goto error;
		}

		// create circular buffer for saving the incoming rates during each frame
		// of the last OBR period
		this->rin = new CircularBuffer(this->obr_period_sf);
		if(this->rin == NULL)
		{
			UTI_ERROR("cannot create circular buffer to save "
					  "the incoming rates\n");
			goto error;
		}

		// create and intialize circular buffer for saving the coefficient to
		// apply to the incoming rates
		this->rin_coeff = (double *) malloc(this->obr_period_sf * sizeof(double));
		if(this->rin_coeff == NULL)
		{
			UTI_ERROR("cannot allocate memory to save "
					  "the incoming rate coefficients\n");
			goto error;
		}
		for (int i = 0; i < this->obr_period_sf; i++)
		{
			/*** TO DO: To read in conf in the next versions ***/
			this->rin_coeff[i] = 1.0/this->obr_period_sf;
			sum_rin_coeff += this->rin_coeff[i];
		}
		if (sum_rin_coeff != 1.0)
			UTI_INFO("the sum of the coefficient is not equal to 1.0. "
			         "It is not a problem for the simulation run but "
			         "the computation request algorithm has no sense\n");
	}

	// Initializes unit converter
	this->converter = new UnitConverter(this->packet_handler->getFixedLength(),
	                                    this->frame_duration_ms);

	return true;

 error:
	return false;
}

bool DamaAgentRcsRrmQos::hereIsSOF(time_sf_t superframe_number_sf)
{
	// Call parent method
	if(!DamaAgentRcs::hereIsSOF(superframe_number_sf))
	 {
		UTI_ERROR("SF#%u: cannot call DamaAgentRcs::hereIsSOF()\n",
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

// a TTP reading function that handles MODCOD but not priority
bool DamaAgentRcsRrmQos::hereIsTTP(Ttp &ttp)
{
	map<uint8_t, emu_tp_t> tp;

	if(this->group_id != ttp.getGroupId())
	{
		UTI_DEBUG_L3("SF#%u: TTP with different group_id (%d).\n",
		             this->current_superframe_sf, ttp.getGroupId());
		return true;
	}

	if(!ttp.getTp(this->tal_id, tp))
	{
		// Update stats and probes
		this->probe_st_total_allocation->put(0);
		return true;
	}

	for(map<uint8_t, emu_tp_t>::iterator it = tp.begin();
	    it != tp.end(); ++it)
	{
		time_pkt_t assign_pkt;

		assign_pkt = (*it).second.assignment_count;
		this->allocated_pkt += assign_pkt;
		UTI_DEBUG_L3("SF#%u: frame#%u: "
		             "offset:%u, assignment_count:%u, "
		             "fmt_id:%u priority:%u\n",
		             ttp.getSuperframeCount(),
		             (*it).first,
		             (*it).second.offset,
		             assign_pkt,
		             (*it).second.fmt_id,
		             (*it).second.priority);
	}

	// Update stats and probes
	this->probe_st_total_allocation->put(
		this->converter->pktpfToKbps(this->allocated_pkt));

	UTI_DEBUG("SF#%u: allocated TS=%u\n",
	          ttp.getSuperframeCount(), this->allocated_pkt);
	return true;
}


bool DamaAgentRcsRrmQos::processOnFrameTick()
{
	// Call parent method
	if(!DamaAgentRcs::processOnFrameTick())
	{
		UTI_ERROR("SF#%u: cannot call DamaAgentRcs::processOnFrameTick()\n",
		          this->current_superframe_sf);
		return false;
	}

	return true;
}


bool DamaAgentRcsRrmQos::buildSAC(cr_type_t UNUSED(cr_type),
                                  Sac &sac,
                                  bool &empty)
{
	bool send_rbdc_request = false;
	bool send_vbdc_request = false;
	rate_kbps_t rbdc_request_kbps = 0;
	vol_pkt_t vbdc_request_pkt = 0;
	empty = false;

	// Compute RBDC request if needed
	if(this->rbdc_enabled)
	{
		UTI_DEBUG("SF#%u: compute RBDC request\n",
		          this->current_superframe_sf);
		rbdc_request_kbps = this->computeRbdcRequest();

		// Send the request only if current RBDC timer > RBDC timeout / 2
		// or if CR is different from previous one
		if(rbdc_request_kbps > 0)
		{
// TODO do we keep that ? if not remove RBDC timeout from DAMA Agent ?
//      RBDC timeout is useful for inband request so
//      maybe we should keep it (but renaming it)
#ifdef OPTIMIZE
			if(rbdc_request_kbps != this->rbdc_request_buffer->GetPreviousValue()
			   || this->rbdc_timer_sf > (this->rbdc_timeout_sf / 2))

#endif
				send_rbdc_request = true;
#ifdef OPTIMIZE
			}
#endif
		}
		else
		{
			if(rbdc_request_kbps != this->rbdc_request_buffer->GetPreviousValue())
			{
				send_rbdc_request = true;
			}
		}
	}

	// Compute VBDC request if required
	if(this->vbdc_enabled)
	{
		UTI_DEBUG("SF#%u: Compute VBDC request\n", this->current_superframe_sf);
		vbdc_request_pkt = this->computeVbdcRequest();

		// Send the request only if it is not null
		if(vbdc_request_pkt > 0)
		{
			send_vbdc_request = true;
		}
	}

	// if no valid CR is built: skipping it
	if(!send_rbdc_request && !send_vbdc_request)
	{
		UTI_DEBUG_L3("SF#%u: RBDC CR = %d, VBDC CR = %d, no CR built.\n",
		             this->current_superframe_sf, rbdc_request_kbps,
		             vbdc_request_pkt);
		empty = true;
		goto end;
	}

	// set RBDC request (if any) in SAC
	if(send_rbdc_request)
	{
		sac.addRequest(0, cr_rbdc, rbdc_request_kbps);

		// update variables used for next RBDC CR computation
		this->rbdc_timer_sf = 0;
		this->rbdc_request_buffer->Update(rbdc_request_kbps);

		// reset counter of arrival packets in MAC FIFOs related to RBDC
		for(fifos_t::const_iterator it = this->dvb_fifos.begin();
		    it != this->dvb_fifos.end(); ++it)
		{
			(*it).second->resetNew(cr_rbdc);
		}

		// Update statistics
		this->probe_st_rbdc_req_size->put(rbdc_request_kbps);

	}

	// set VBDC request (if any) in SAC
	if(send_vbdc_request)
	{
		sac.addRequest(0, cr_vbdc, vbdc_request_pkt);

		// Update statistics
		this->probe_st_vbdc_req_size->put(
			this->converter->pktToKbits(vbdc_request_pkt));

	}

	UTI_DEBUG("SF#%u: build CR with %u kb/s in RBDC and %u packets in VBDC",
	          this->current_superframe_sf, rbdc_request_kbps, vbdc_request_pkt);

 end:
	return true;
}

bool DamaAgentRcsRrmQos::returnSchedule(list<DvbFrame *> *complete_dvb_frames)
{
	rate_kbps_t remaining_alloc_kbps;
	uint32_t remaining_alloc_pktpf = this->remaining_allocation_pktpf;

	UTI_DEBUG_L3("SF#%u: frame %u: allocation before scheduling %u\n",
	             this->current_superframe_sf, this->current_frame,
	             remaining_alloc_pktpf);
	if(!this->ret_schedule->schedule(this->current_superframe_sf,
	                                 this->current_frame,
	                                 0,
	                                 complete_dvb_frames,
	                                 remaining_alloc_pktpf))
	{
		UTI_ERROR("SF#%u: frame %u: Uplink Scheduling failed",
		          this->current_superframe_sf, this->current_frame);
		return false;
	}
	UTI_DEBUG_L3("SF#%u: frame %u: remaining allocation after scheduling %u\n",
	             this->current_superframe_sf, this->current_frame,
	             remaining_alloc_pktpf);
	this->remaining_allocation_pktpf = remaining_alloc_pktpf;

	remaining_alloc_kbps = this->converter->pktpfToKbps(this->remaining_allocation_pktpf);

	// Update stats and probes
	this->probe_st_remaining_allocation->put(remaining_alloc_kbps);

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
	time_ms_t t_sync_ms;			// Time (in sec) between two requests
	time_ms_t t_loop_ms;			// Minimum Scheduling Latency (MSL) in sec
	double beta;  // Congestion factor 
	double alpha; // Congestion and anticipation factor
	rate_kbps_t rin_weighted_kbps;	// Weighted mean incoming trafic at layer 2
									// corresponding to RBDC associated queues
									// in kbps
	rate_kbps_t alloc_since_last_request; // Allocation in kbps for 
								  		  // the last (t_sync - t_loop)
	

	/* get number of outstanding packets in RBDC related MAC FIFOs */
	rbdc_length_b =
		this->converter->pktToBits(this->getMacBufferLength(cr_rbdc));

	// Get number of packets arrived in RBDC related IP FIFOs since
	// last RBDC request sent
	// NB: arrivals in MAC FIFOs must NOT be taken into account because these
	// packets represent only packets buffered because there is no
	// more available allocation, but their arrival has been taken into account
	// in IP fifos
	//TODO: remove ?
	//rbdc_pkt_arrival_b =
		//this->converter->pktToBits(this->getMacBufferArrivals(cr_rbdc));

	// get the sum of RBDC request during last MSL
	// TODO: remove ?
	//rbdc_req_in_previous_msl_kbps = this->rbdc_request_buffer->GetSum();

	// get the last RBDC request value
	last_rbdc_req_kbps = this->rbdc_request_buffer->GetLastValue();

	// Get the MSL
	t_loop_ms = this->msl_sf * this->frame_duration_ms;

	// Get the time between two requests
	t_sync_ms = this->obr_period_sf * this->frame_duration_ms;

	if (LEGACY == 1) // Use the legacy algorithm instead of the RRM-QoS one
		goto Legacy; 

    // Compute beta
	/*** TO DO: Add a parameter in conf to choose between beta = 1, beta = 0 
	 *** and beta = dynamic ***/
    
	if (BETA == 2) // beta computed dynamically 
	{
		if (last_rbdc_req_kbps != 0)
			beta = ((t_sync_ms * last_rbdc_req_kbps) - 
				this->dyn_alloc->GetSum()) /
				(t_sync_ms * last_rbdc_req_kbps);
		else
			beta = 1.0; // RRM-QoS: Modification 3
		if (beta < 0)
			beta = 0.0; // RRM-QoS: Modficiation 8
	}
	else if (BETA == 1) // beta = 1
		beta = 1.0;
	else if (BETA == 0) // beta = 0
		beta = 0.0; 
	else // Unknown value for the beta parameter
	{
		UTI_INFO("WARNING: Unknown value for the beta parameter\n");
		UTI_INFO("WARNING; beta is set to 0\n");
		beta = 0.0;
	}
	UTI_DEBUG_L3("beta = %f\n", beta);

   // Compute alpha
   /*** TO DO: Add a parameter in conf to choose between alpha = 1, alpha = 0
    *** and alpha = dynamic ***/
    if (ALPHA == 2) // alpha computed dynamically
	{
		if (t_sync_ms > t_loop_ms)
		{
			// Usual case (in the context of the R&T RRM-QoS)
			UTI_DEBUG_L3("this->dyn_alloc_PartialSum = %d pkt\n",
				this->dyn_alloc->GetPartialSumFromPrevious(this->obr_period_sf -
				this->msl_sf));
			UTI_DEBUG_L3("this->dyn_alloc_Sum = %d pkt\n", this->dyn_alloc->GetSum());
			//UTI_DEBUG_L3("t_sync_ms - t_loop_ms = %f sec\n", (t_sync_ms - t_loop_ms));
			if (last_rbdc_req_kbps > 0)
			    alpha = (this->dyn_alloc->GetPartialSumFromPrevious(this->obr_period_sf -
				    this->msl_sf)) / ((t_sync_ms - t_loop_ms) * 
					last_rbdc_req_kbps);
			else
				alpha = 1.0; // RRM-QoS: Modification 2
		}
		else if (t_sync_ms == t_loop_ms)
		{
			// Unusual case (in the context of the R&T RRM-QoS)
            if (last_rbdc_req_kbps > 0) // RRM-QoS: Modification 4
				alpha = (this->dyn_alloc->GetPreviousValue()) /
					(t_sync_ms * last_rbdc_req_kbps);
			else
				alpha = 1.0; // RRM-QoS: Modification 2
		}
		else
		{
			// Nonused case in the R&T RRM-QoS
			UTI_INFO("the time between two requests (obrPeriod) is inferior"
				" to the Minimum Scheduling Latency (MSL), this case "
				"should not be used in the context of the RRM-QoS. However"
				", the simulation is able to continue with alpha = 1\n");
			alpha = 1.0;
		}
		if (alpha > 1.0)
	       alpha = 1.0;
	}
	else if (ALPHA == 1) // alpha = 1
	 	alpha = 1.0;
	else if (ALPHA == 0) // alpha = 0
	    alpha = 0.0;
    else // Unknown value for the alpha parameter
	{
		UTI_INFO("WARNING: Unknown value for the alpha parameter\n");
		UTI_INFO("WARNING: alpha is set to 1\n");
		alpha = 1.0;
	}
	UTI_DEBUG_L3("alpha = %f\n", alpha);

	// Compute Rin in cell/sec
	rin_weighted_kbps = 0;
	for (int i = 0; i < this->obr_period_sf ; i++)
	{
		rin_weighted_kbps += (this->rin->GetValueIndex(i+1)) * 
			(this->rin_coeff[i]); // in cell/frame
		/*UTI_DEBUG_L3("Rin to add = %f cell/frame\n", 
				this->rin->GetValueIndex(i+1));
		UTI_DEBUG_L3("RinCoeff = %f\n", this->rin_coeff[i]);
		UTI_DEBUG_L3("rin_weighted_kbps to add = %f cell/frame\n", 
			(this->rin->GetValueIndex(i+1)) * (this->rin_coeff[i]));*/
	}
	rin_weighted_kbps /= this->frame_duration_ms; // in cell/sec
	//UTI_DEBUG_L3("FrameDuration = %fms?\n", this->frame_duration_ms);
	UTI_DEBUG_L3("rin_weigthed = %d kbps\n", rin_weighted_kbps);

	// Compute rate_need_kbps : estimation of the need of bandwith for traffic
	// in cell/sec (core of the algorithm)
	alloc_since_last_request = 
		this->dyn_alloc->GetPartialSumFromPrevious(this->obr_period_sf - this->msl_sf) / 
		(t_sync_ms - t_loop_ms);
	if ((last_rbdc_req_kbps < alloc_since_last_request) || 
			(WITHOUT_MODIF_1 == 1)) // RRM-QoS: Modification 1 
	{
		if (rbdc_length_b + (alpha * t_loop_ms * rin_weighted_kbps) > 
			t_loop_ms * last_rbdc_req_kbps * (1 - beta))
		{
    		rate_need_kbps = (rbdc_length_b - (t_loop_ms * last_rbdc_req_kbps * (1 - beta)) + 
				(alpha * (t_sync_ms + t_loop_ms) * rin_weighted_kbps)) / t_sync_ms;
		} 
		else // RRM-QoS: Modification 6 - Option 2
		{
			rate_need_kbps = alpha * rin_weighted_kbps;
			/*UTI_INFO("alpha = %f\n", alpha);
			UTI_INFO("rin_weighted_kbps = %f\n", rin_weighted_kbps);
			UTI_INFO("rate_need_kbps = %f\n", rate_need_kbps);*/
		}	
	} 
	else // last_rbdc_req_kbps >= alloc_since_last_request
	{
		if (rbdc_length_b + (alpha * t_loop_ms * rin_weighted_kbps) > 
			t_loop_ms * alloc_since_last_request * (1 - beta))
		{
			rate_need_kbps = (rbdc_length_b - (t_loop_ms * alloc_since_last_request * (1 - beta)) +
				(alpha * (t_sync_ms + t_loop_ms) * rin_weighted_kbps)) / t_sync_ms;
		}
		else // RRM-QoS: Modification 6 - Option 2
		{
			rate_need_kbps = alpha * rin_weighted_kbps; 
			/*UTI_INFO("alpha = %f\n", alpha);
			UTI_INFO("rin_weighted_kbps = %f\n", rin_weighted_kbps);
			UTI_INFO("rate_need_kbps = %f\n", rate_need_kbps);*/
		}
	}
	if (rate_need_kbps < 0)
		rate_need_kbps = 0;

	UTI_DEBUG_L3("frame = %d, rate_need_kbps = %3.f cell/s\n", 
		this->current_superframe_sf, rate_need_kbps);

Legacy:
	if (LEGACY == 1)
	{
		rate_kbps_t rin_kbps = 0.0;
		rin_kbps = 0;
		for (int i = 0; i < this->obr_period_sf ; i++)
		{
			rin_kbps += (this->rin->GetValueIndex(i+1) * 1.0 / 
					this->obr_period_sf); // in cell/frame
		}
		if (rbdc_length_b > rin_kbps * t_sync_ms) 
			rate_need_kbps = rin_kbps + ( (rbdc_length_b - (rin_kbps * t_sync_ms)) / t_loop_ms );
		else // rbdc_length_b < rin * t_sync_ms
			rate_need_kbps = rin_kbps;
	}
	
	// Compute actual RBDC request to be sent in Kbit/sec
	rbdc_request_kbps = (int) rate_need_kbps; 
	UTI_DEBUG_L3("frame=%d,  theoretical rbdc_request_kbps = %d kbits/s",  
		this->current_superframe_sf, rbdc_request_kbps);


	// Check if the RBDCmax is not reached
	if (rbdc_request_kbps > this->max_rbdc_kbps)
		rbdc_request_kbps = this->max_rbdc_kbps;
	// Deduct the CRA from the RBDC request
	if ((rbdc_request_kbps - this->cra_kbps) < 0) 
		rbdc_request_kbps = 0;
	else 
		rbdc_request_kbps -= this->cra_kbps;

	// Reduce the request value to the maximum theorical value 
	// and use the following units 2kbits/s or 16kbits/s 
	// in order to observe the DVB-RCS standard
	// RRM-QoS: Modification 7
	rbdc_request_kbps = min(rbdc_request_kbps, C_MAX_RBDC_IN_SAC);
	UTI_DEBUG_L3("frame=%d,  updated rbdc_request_kbps = %d kbits/s in SAC\n",
		this->current_superframe_sf, rbdc_request_kbps);

	UTI_DEBUG_L3("Sending request *** \n");
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
	vbdc_need_pkt = this->getMacBufferLength(cr_vbdc);
	UTI_ERROR("SF#%u: frame %u: MAC buffer length = %d, VBDC credit = %u\n",
	             this->current_superframe_sf, this->current_frame,
	             vbdc_need_pkt, this->vbdc_credit_pkt);

	/* compute VBDC request: actual Vbdc request to be sent */
	vbdc_request_pkt = max(0, (vbdc_need_pkt - this->vbdc_credit_pkt));
	UTI_ERROR("SF#%u: frame %u: theoretical VBDC request = %u packets",
	             this->current_superframe_sf, this->current_frame,
	             vbdc_request_pkt);

	/* adjust request in function of max_vbdc value */
	vbdc_request_pkt = min(vbdc_request_pkt, max_vbdc_pkt);

	// Ensure VBDC request value is not greater than SAC field
	vbdc_request_pkt = min(vbdc_request_pkt, C_MAX_VBDC_IN_SAC);
	UTI_ERROR("updated VBDC request = %d packets in fonction of "
	             "max VBDC and max VBDC in SAC\n", vbdc_request_pkt);

	/* update VBDC Credit here */
	/* NB: the computed VBDC is always really sent if not null */
	this->vbdc_credit_pkt += vbdc_request_pkt;
	UTI_ERROR("updated VBDC request = %d packets in SAC, VBDC credit = %u\n",
	             vbdc_request_pkt, this->vbdc_credit_pkt);

	return vbdc_request_pkt;
}

vol_pkt_t DamaAgentRcsRrmQos::getMacBufferLength(cr_type_t cr_type)
{
	vol_pkt_t nb_pkt_in_fifo; // absolute number of packets in fifo

	nb_pkt_in_fifo = 0;
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_type)
		{
			nb_pkt_in_fifo += (*it).second->getCurrentSize();
		}
	}

	return nb_pkt_in_fifo;
}


vol_pkt_t DamaAgentRcsRrmQos::getMacBufferArrivals(cr_type_t cr_type)
{
	vol_pkt_t nb_pkt_input; // packets that filled the queue since last RBDC request

	nb_pkt_input = 0;
	for(fifos_t::const_iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getCrType() == cr_type)
		{
			nb_pkt_input += (*it).second->getNewSize();
		}
	}

	return nb_pkt_input;
}

void DamaAgentRcsRrmQos::updateStatistics()
{
}
