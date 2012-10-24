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
 * @file lib_dama_ctrl.cpp
 * @brief This library defines a generic DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

#include <math.h>
#include <string>
#include <cstdlib>

// PEP interface
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <iostream>
#include <sstream>

using namespace std;

#include "lib_dama_ctrl.h"

// environment plane
#include "opensand_env_plane/EnvPlane.h"

#define DC_DBG_PREFIX "[Generic]"
#define DBG_PACKAGE PKG_DAMA_DC
#include "opensand_conf/uti_debug.h"

// Note on the whole algorithm
// ---------------------------
//
// Invariant 1:
//    By construction, the following property is true
//      for all st_id,
//         (no CR has been received from st_id during current superframe)
//        <=>
//         (
//            m_context[st_id]->own_cr == NULL pointer
//          AND
//            m_context[st_id]->btp_entry == NULL pointer
//         )
//
// We must maintain these invariants in all method, so:
//    for all st_id that have been examinated (CR received)
//    particularly, the following property (Invariant 1) must hold _after_ runDama()
//       m_context[st_id]->own_cr               reseted to  NULL pointer
//       m_context[st_id]->timeslots_allocated  reseted to  NULL pointer
//    please insure it when implementing runDama() (it can be a loop as in the method)

// Note on the building of TBTP and on the exploitation of SACT data
// -----------------------------------------------------------------
//
// Before running DAMA, we should have scanned the SACT table in order to:
//  - cleanup it from loggued off stations,
//  - update the context to compute allocation
//
// We do that work upon receiption of CR but it was mainly implemented to catch
// duplicate CR.
// In the case of SACT, we do the work in asingle loop upon reception.
//
// However there is still an unavoidable race condition in the case of SACT.
// Logoff can be emitted while we allocate a bandwidth...
//

// Final Note on Implementation
// ----------------------------
//
// The method runDama() is missing.
// It must be implemented in inherited class.
// Those inherited class have sufficient material to do the computation:
//    - a complete SACT
//    - a prefilled TBTP
//    - a context updated with information from SACT and built TBTP
// So normally there is only to loop on the context to do the computation
// See Dama_crtl_yes.cpp for an example.
//


// Static environment plane events and probes
Event* DvbRcsDamaCtrl::error_alloc = NULL;
Event* DvbRcsDamaCtrl::error_ncc_req = NULL;

Probe<int>* DvbRcsDamaCtrl::probe_gw_rdbc_req_num = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_rdbc_req_capacity = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_vdbc_req_num = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_vdbc_req_capacity = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_cra_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_cra_st_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_rbdc_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_rbdc_st_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_rbdc_max_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_rbdc_max_st_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_vbdc_alloc = NULL;
Probe<int>* DvbRcsDamaCtrl::probe_gw_logger_st_num = NULL;


/**
 * Constructor
 */
DvbRcsDamaCtrl::DvbRcsDamaCtrl()
{
	m_currentSuperFrame = 0;
	m_tbtp = NULL;
	m_tbtp_size = 0;
	m_total_capacity = 0;
	m_allocation_cycle = 0;
	m_carrier_capacity = 0;
	m_total_cra = -1;
	m_fca = 0;
	m_nb_st = 0;
	m_rbdc_start_ptr = -1;
	m_vbdc_start_ptr = -1;
	m_fca_start_ptr = -1;
	m_rbdc_timeout = 0;
	m_min_vbdc = 0;
	m_max_rbdc = 0;
	m_cra_decrease = 0;
	this->Converter = NULL;
	this->event_file = NULL;
	this->stat_file = NULL;

	if (error_alloc == NULL) {
		// Initialize the environment plane events and probes

		error_alloc = EnvPlane::register_event("lib_dama_ctrl:alloc", LEVEL_ERROR);
		error_ncc_req = EnvPlane::register_event("lib_dama_ctrl:ncc_req", LEVEL_ERROR);

		probe_gw_rdbc_req_num = EnvPlane::register_probe<int>("RBDC_requests_number", "requests", true, SAMPLE_LAST);
		probe_gw_rdbc_req_capacity = EnvPlane::register_probe<int>("RBDC_requested_capacity", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_vdbc_req_num = EnvPlane::register_probe<int>("VBDC_requests_number", "requests", true, SAMPLE_LAST);
		probe_gw_vdbc_req_capacity = EnvPlane::register_probe<int>("VBDC_requested_capacity", "time slots", true, SAMPLE_LAST);
		probe_gw_cra_alloc = EnvPlane::register_probe<int>("CRA_allocation", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_cra_st_alloc = EnvPlane::register_probe<int>("CRA_st_allocation", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_rbdc_alloc = EnvPlane::register_probe<int>("RBDC_allocation", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_rbdc_st_alloc = EnvPlane::register_probe<int>("RBDC_st_allocation", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_rbdc_max_alloc = EnvPlane::register_probe<int>("RBDC_MAX_allocation", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_rbdc_max_st_alloc = EnvPlane::register_probe<int>("RBDC_MAX_st_allocation", "Kbits/s", true, SAMPLE_LAST);
		probe_gw_vbdc_alloc = EnvPlane::register_probe<int>("VBDC_allocation", "Kbits/s", true, SAMPLE_LAST);
		// FIXME: Unit?
		probe_gw_logger_st_num = EnvPlane::register_probe<int>("Logged_ST_number", true, SAMPLE_LAST);
	}
}


/**
 * Destructor
 */
DvbRcsDamaCtrl::~DvbRcsDamaCtrl()
{
	DC_Context::iterator st;

	for(st = m_context.begin(); st != m_context.end(); st++)
		delete st->second;

	if(NULL != m_tbtp)
		free(m_tbtp);

	if(this->Converter != NULL)
		delete this->Converter;
}


/**
 * Initializes internal data structure according to configuration file
 *
 * @param carrier_id        the id of the control carrier
 * @param frame_duration    the frame duration in ms
 * @param allocation_cycle  the DAMA allocation periodicity in frame number
 * @param packet_length     the encapsulation packet length in bytes
 * @param dra_def_table     the table of dra scheme definitions
 * @return                  0 if ok -1 otherwise
 */
int DvbRcsDamaCtrl::init(long carrier_id, int frame_duration,
                         int allocation_cycle, int packet_length,
                         DraSchemeDefinitionTable *dra_def_table)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[init]";
	long transmission_rate;
	int carrier_number;

	// store the table of DRA scheme definitions
	this->dra_scheme_def_table = dra_def_table;

	// Storing carrier id for control messages
	m_carrierId = carrier_id;

	// Storing the allocation cycle (number of frame per superframe)
	m_allocation_cycle = allocation_cycle;

	// Retrieving the cra decrease parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_CRA_DECREASE, m_cra_decrease))
	{
		UTI_ERROR("%s missing %s parameter", FUNCNAME, DC_CRA_DECREASE);
		goto error;
	}
	UTI_INFO("%s cra_decrease = %s\n", FUNCNAME,
	         m_cra_decrease == true ? "true" : "false");

	// Retrieving the free capacity assignement parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_FREE_CAP, m_fca))
	{
		UTI_ERROR("%s missing %s parameter", FUNCNAME, DC_FREE_CAP);
		goto error;
	}
	UTI_INFO("%s fca = %d\n", FUNCNAME, m_fca);

	// Retrieving the rbdc timeout parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_RBDC_TIMEOUT, m_rbdc_timeout))
	{
		UTI_ERROR("%s missing %s parameter", FUNCNAME, DC_RBDC_TIMEOUT);
		goto error;
	}
	UTI_INFO("%s rbdc_timeout = %d\n", FUNCNAME, m_rbdc_timeout);

	// Retrieving the min VBDC parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_MIN_VBDC, m_min_vbdc))
	{
		UTI_ERROR("%s missing %s parameter", FUNCNAME, DC_MIN_VBDC);
		goto error;
	}
	UTI_INFO("%s min_vbdc = %d\n", FUNCNAME, m_min_vbdc);

	// Retrieving carrier rate
	if(!globalConfig.getValue(DC_SECTION_MAC_LAYER,
	                          DC_CARRIER_TRANS_RATE, transmission_rate))
	{
		UTI_ERROR("%s missing %s parameter", FUNCNAME, DC_CARRIER_TRANS_RATE);
		goto error;
	}
	UTI_INFO("%s carrier_transmission_rate = %ld\n", FUNCNAME, transmission_rate);

	// Retrieving the carrier number
	if(!globalConfig.getValue(DC_SECTION_MAC_LAYER,
	                          DC_CARRIER_NUMBER, carrier_number))
	{
		UTI_ERROR("%s missing %s parameter", FUNCNAME, DC_CARRIER_NUMBER);
		goto error;
	}
	UTI_INFO("%s carrier_number = %d\n", FUNCNAME, carrier_number);

	this->frame_duration = frame_duration; //< add from UoR

	// converting capacity into packets per frames
	this->Converter = new DU_Converter(frame_duration, packet_length);
	if(this->Converter == NULL)
	{
		UTI_ERROR("%s cannot create the DU converter\n", FUNCNAME);
		goto error;
	}
	m_carrier_capacity =
		(int) Converter->ConvertFromKbitsToCellsPerFrame(transmission_rate);
	m_total_capacity = carrier_number * m_carrier_capacity;
	UTI_INFO("%s total_capacity = %ld\n", FUNCNAME, m_total_capacity);
	UTI_INFO("%s carrier_capacity = %d\n", FUNCNAME, m_carrier_capacity);
	m_fca = (int) ceil(Converter->ConvertFromKbitsToCellsPerFrame(m_fca));

	// Retrieving the max RBDC parameter
	if(!globalConfig.getValue(DC_SECTION_NCC, DC_MAX_RBDC, m_max_rbdc))
	{
		UTI_INFO("%s missing %s parameter, default value set (%d).\n",
		         FUNCNAME, DC_MAX_RBDC, m_carrier_capacity);
		m_max_rbdc = m_carrier_capacity;
	}
	m_max_rbdc = (int) Converter->ConvertFromKbitsToCellsPerFrame(m_max_rbdc);
	UTI_INFO("%s max_rbdc = %d kbits/s corresponding to %d cells/frame\n",
	               FUNCNAME, (int) Converter->ConvertFromCellsPerFrameToKbits(m_max_rbdc),
	               m_max_rbdc);

	// Set the total cra allocated capacity for RT to zero (no one have loggued)
	m_total_cra = 0;

	// Allocating internal tbtp buffer
	m_tbtp_size = sizeof(unsigned char) * m_buff_alloc;
	m_tbtp = (unsigned char *) malloc(m_tbtp_size);
	if(m_tbtp == NULL)
	{
		UTI_ERROR("%s error during memory allocation.\n", FUNCNAME);
		EnvPlane::send_event(error_alloc, "%s Error allocating memory: %s (%d)",
			FUNCNAME, strerror(errno), errno);
		goto destroy_converter;
	}
	bzero(m_tbtp, m_tbtp_size);
	cleanTBTP();

	return 0;

destroy_converter:
	delete this->Converter;
error:
	return -1;
}


/**
 * @brief Process a logon Request Frame as an information from Dvb layer
 *        and fill an internal context
 *
 * @param buf     point to the buffer containing logon
 * @param len     the length of the buffer
 * @param dra_id  TODO
 * @return        0 on success, -1 on failure
 */
int DvbRcsDamaCtrl::hereIsLogonReq(unsigned char *buf, long len, int dra_id)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[hereIsLogonReq]";
	T_DVB_LOGON_REQ *dvb_logon_req;
	DC_Context::iterator st;
	DC_St *NewSt;
	T_DVB_BTP *Btp;
	int Cra;
	dvb_logon_req = (T_DVB_LOGON_REQ *) buf;

	// WARNING: Currently (date of the CVS update) we take into account the fact
	// that the identifier is considered the same as the mac address in the dvb
	// ncc bloc layer. We should use a conversion map.
	//
	// Check for an already registered station, if not register it
	//
	st = m_context.find(dvb_logon_req->mac);
	Cra =
		(int) ceil(Converter->
		           ConvertFromKbitsToCellsPerFrame(dvb_logon_req->rt_bandwidth));
	UTI_INFO("%s CRA = %ld kbits/s corresponding to %d cells/frame, for ST mac %d.\n",
		 FUNCNAME, dvb_logon_req->rt_bandwidth, Cra, dvb_logon_req->mac);
	if(Cra > m_carrier_capacity)
	{
		UTI_INFO("%s CRA value exceed carrier capacity %ld kbits/s "
		         "corresponding to %d cells/frame -> set to carrier capacity %d.\n",
		          FUNCNAME, dvb_logon_req->rt_bandwidth, Cra, m_carrier_capacity);
		Cra = m_carrier_capacity;
	}

	if(st == m_context.end())
	{
		// create a BTP for this terminal
		if((Btp = appendBtpEntry(dvb_logon_req->mac)) == (T_DVB_BTP *) NULL)
			return -1;

		// create the terminal instance
		NewSt =
			new DC_St(m_carrier_capacity, Cra, m_fca, m_max_rbdc, m_min_vbdc,
			          m_rbdc_timeout, m_allocation_cycle, Btp, dra_id);

		// RT bandwidth of the new st
		m_total_cra += Cra;

		// object mapping
		m_context[dvb_logon_req->mac] = NewSt;

		// increase the terminal number
		m_nb_st++;
	}
	else
	{
		UTI_INFO("%s duplicate Logon received for logon id #%d.\n",
		         FUNCNAME, dvb_logon_req->mac);
		// retrieve the ST context
		NewSt = st->second;

		// update the ST CRA and total CRA values
		m_total_cra += NewSt->SetCra(Cra);
	}

	DC_RECORD_EVENT("LOGON st%d rt = %ld", dvb_logon_req->mac,
	                dvb_logon_req->rt_bandwidth);

	return 0;
}


/**
 * Process a logoff Frame, must update the context
 * @param buf points to the buffer containing logoff
 * @param len is the length of the buffer
 * @return 0 on success , -1 otherwise
 */
int DvbRcsDamaCtrl::hereIsLogoff(unsigned char *buf, long len)
{
	T_DVB_LOGOFF *dvb_logoff;
	DC_Context::iterator st;

	dvb_logoff = (T_DVB_LOGOFF *) buf;

	// WARNING: Currently (date of the CVS update) we take into account the fact
	// that the identifier is considered the same as the mac address in the dvb
	// ncc bloc layer. We should use a conversion map.
	//
	st = m_context.find(dvb_logoff->mac);
	if(st != m_context.end())
	{
		// remove the BTP associated to this ST
		removeBtpEntry(dvb_logoff->mac);

		// update the total RT bandwidth
		m_total_cra -= st->second->GetCra();

		// remove the ST context
		delete st->second;

		// unmap the context
		m_context.erase(dvb_logoff->mac);

		// decrease the number of ST
		m_nb_st -= 1;
	}

	DC_RECORD_EVENT("LOGOFF st%d", dvb_logoff->mac);
	return 0;
}


/**
 * When receiving a CR, fill the internal SACT table and internal TBTP table
 * Must maintain Invariant 1 true
 *
 * @param buf     the pointer to CR buff to be copied
 * @param len     the length of the buffer
 * @param dra_id  TODO
 * @return        0 on success, -1 otherwise
 */
int DvbRcsDamaCtrl::hereIsCR(unsigned char *buf, long len, int dra_id)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[hereIsCR]";
	T_DVB_SAC_CR *sac_cr;
	T_DVB_SAC_CR_INFO *cr;
	DC_Context::iterator st;
	DC_St *ThisSt;
	double Request;
	int xbdc;
	int i;

	sac_cr = (T_DVB_SAC_CR *) buf;

	// Type sanity check
	if(sac_cr->hdr.msg_type != MSG_TYPE_CR)
	{
		UTI_ERROR("unattended type (%ld) of DVB frame, drop frame\n",
		          sac_cr->hdr.msg_type);
		EnvPlane::send_event(error_ncc_req, "Unattended type (%ld) of "
			"DVB frame, drop frame\n", sac_cr->hdr.msg_type);

		goto error;
	}

	for(i = 0; i < sac_cr->cr_number; i++)
	{
		// check the request
		cr = &sac_cr->cr[i];

		// Checking if the station is registered
		st = m_context.find(cr->logon_id);
		if(st == m_context.end())
		{
			UTI_ERROR("%s CR for a unknown st (logon_id=%d). Discarded.\n",
			          FUNCNAME, cr->logon_id);
			EnvPlane::send_event(error_ncc_req, "%s CR for a unknown st "
				"(logon_id=%d). Discarded.\n", FUNCNAME, cr->logon_id);
			goto error;
		}
		ThisSt = st->second; // Now st_context points to a valid context

		UTI_DEBUG("%s ST %d requests %ld %s\n",
		          FUNCNAME, cr->logon_id, cr->xbdc,
		          (cr->type == DVB_CR_TYPE_VBDC) ?
		          " slots in VBDC" : " kbits/s in RBDC");

		// retrieve the requested capacity
		xbdc = decode_request_value(cr);
		if(xbdc == -1)
		{
			UTI_ERROR("%s Capacity request decoding error. Discarded.\n",
			          FUNCNAME);
			EnvPlane::send_event(error_ncc_req, "%s Capacity request "
				"decoding error. Discarded.\n", FUNCNAME);
			goto error;
		}

		// take into account the new request
		if(cr->type == DVB_CR_TYPE_VBDC)
		{
			if(ThisSt->SetVbdc(xbdc) == 0)
			{
				// Now we are sure to have a valid cr for a valid context
				DC_RECORD_EVENT("CR st%d cr=%d type=%d", cr->logon_id, xbdc,
				                DVB_CR_TYPE_VBDC);
			}
		}
		else if(cr->type == DVB_CR_TYPE_RBDC)
		{
			if(m_cra_decrease == 1)
			{
				// remove the CRA of the RBDC request
				Request =
					MAX(Converter->ConvertFromKbitsToCellsPerFrame(xbdc) -
					    (double) ThisSt->GetCra(), 0.0);
			}
			else
			{
				Request = Converter->ConvertFromKbitsToCellsPerFrame(xbdc);
			}

			if(ThisSt->SetRbdc(Request) == 0)
			{
				// Now we are sure to have a valid cr for a valid context
				DC_RECORD_EVENT("CR st%d cr=%d type=%d", cr->logon_id, xbdc,
				                DVB_CR_TYPE_RBDC);
			}
		}
	}

	return 0;

error:
	return -1;
}

/**
 * When receiving a SACT, memcpy into the internal SACT table and build TBTP
 * @param buf the pointer to SACT buff to be copied
 * @param len the lenght of the buffer
 * @return 0 on succes, -1 otherwise
 */

int DvbRcsDamaCtrl::hereIsSACT(unsigned char *buf, long len)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[hereIsSACT]";

	// Iterators
	DC_Context::iterator st;

	// Variables used for casting and loop evolution
	T_DVB_SACT *sact;             // Local pointer to the internal sact table
	T_DVB_SAC_CR_INFO *cr;        // points to a cr inside the internal sact table
	T_DVB_SAC_CR_INFO *beyond_cr; // its a sentinel, points after the internal sact table

	// Parameters used for ease of reading
	DC_stId st_id; // Id of the ST being under examination
	DC_St *ThisSt; // points to the internal context map associated with the Id

	sact = (T_DVB_SACT *) buf;

	// Type sanity check
	if(sact->hdr.msg_type != MSG_TYPE_SACT)
	{
		UTI_ERROR("wrong dvb pkt type type (%ld). Discarded.\n",
		          sact->hdr.msg_type);
		goto error;
	}

	// Size sanity check
	if(len < 0 || len_sac_pkt(sact) > ((unsigned long) len))
	{
		UTI_ERROR("SACT, buffer len %ld lower than announced size %lu. Discarding.\n",
		          len, len_sac_pkt(sact));
		goto error;
	}

	// Ok, we can now check the requests

	// Loop on SACT, build TBTP (and internal SACT too, it doesn't harm)
	cr = first_sac_ptr(sact);
	beyond_cr = ith_sac_ptr(sact->qty_element + 1, sact); // sentinel

	while(cr != beyond_cr)
	{ // for each cr in SACT
		st_id = cr->logon_id;
		st = m_context.find(st_id);
		// Capacity request of an unregistered st, we must discard
		if(st == m_context.end())
		{
			UTI_DEBUG_L3
				("%s found a SAC_CR without context (id=%d). Discarded.\n",
				 FUNCNAME, st_id);
			cr = next_sac_ptr(cr);
			continue;
		}
		// Now we have a valid context associated with st_id
		ThisSt = st->second;

		// take into account the new request
		if(cr->type == DVB_CR_TYPE_VBDC)
		{
			if(ThisSt->SetVbdc(cr->xbdc) == 0)
			{
				// Now we are sure to have a valid cr for a valid context
				DC_RECORD_EVENT("CR st%d cr=%ld type=%d", cr->logon_id,
				                cr->xbdc, DVB_CR_TYPE_VBDC);
			}
		}
		else if(cr->type == DVB_CR_TYPE_RBDC)
		{
			if(ThisSt->SetRbdc(Converter->ConvertFromKbitsToCellsPerFrame(cr->xbdc)) == 0)
			{
				// Now we are sure to have a valid cr for a valid context
				DC_RECORD_EVENT("CR st%d cr=%ld type=%d", cr->logon_id,
				                cr->xbdc, DVB_CR_TYPE_RBDC);
			}
		}

		cr = next_sac_ptr(cr);
	}

error:
	return (-1);
}


/**
 * Copy the internal TBTP structure into the zone pointed by buf. Then clean TBTP.
 *
 * @param buf  the buffer where the TBTP must be copied
 * @param len  the lenght of the buffer
 * @return     0 on succes, -1 otherwise
 */
int DvbRcsDamaCtrl::buildTBTP(unsigned char *buf, long len)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[buildTBTP]";
	T_DVB_TBTP *tbtp;
	T_DVB_FRAME *frame;
	T_DVB_BTP *btp; // points to a btp within a frame
	int i;

	tbtp = (T_DVB_TBTP *) m_tbtp;

	if(tbtp->hdr.msg_length > len)
	{
		UTI_ERROR("%s buffer len too small (%ld<%ld)", FUNCNAME, len,
		           tbtp->hdr.msg_length);
		return (-1);
	}

	// Here is supposed that it exists a single global allocation for the
	// whole Superframe. Hence only one frame is considered in the TBTP.
	frame = first_frame_ptr(tbtp);

	// We do not memcpy a TBTP frame if there is no allocation demands
	if(frame->btp_loop_count == 0)
	{
		UTI_DEBUG_L3("%s no requests received, skipping TBTP memcpy.\n",
		             FUNCNAME);
		return -1;
	}

	UTI_DEBUG_L3("%s memcpy(%p,%p,%ld)", FUNCNAME, buf, tbtp,
	             tbtp->hdr.msg_length);
	memcpy(buf, tbtp, tbtp->hdr.msg_length);

	UTI_DEBUG_L3("%s btp nb=%d.\n", FUNCNAME, frame->btp_loop_count);
	for(i = 0; i < frame->btp_loop_count; i++)
	{
		btp = ith_btp_ptr(i, frame);
		UTI_DEBUG_L3("   -> %ld,%d,%d,<%d>,%d,%d.\n",
		             btp->assignment_count, btp->assignment_type,
		             btp->channel_id, btp->logon_id,
		             btp->multiple_channel_flag, btp->start_slot);
	}

	return 0;
}

/**
 * Things to do when a SOF is detected process dama() and reset SACT, at issue Invariant 1
 *
 * @param frame_nb  the super frame number
 * @return          0 on success, -1 on failure
 */
int DvbRcsDamaCtrl::runOnSuperFrameChange(long frame_nb)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runOnSuperFrameChange]";
	int resul;
	int total_rbdc_max;
	unsigned int St_id;
	DC_St *ThisSt; // points to the internal context map associated with the Id
	DC_Context::iterator st;

	UTI_DEBUG("%s ********** frame %ld ***********", FUNCNAME,
				 m_currentSuperFrame);

	// statistics
	probe_gw_cra_alloc->put((int)Converter->ConvertFromCellsPerFrameToKbits((double) m_total_cra));
	DC_RECORD_STAT("ALLOC CRA %f kbits/s",
	               Converter->
	               		ConvertFromCellsPerFrameToKbits((double) m_total_cra));
	probe_gw_logger_st_num->put(m_nb_st);
	DC_RECORD_STAT("ALLOC NB ST %d", m_nb_st);


	// init the TBTP (reset the allocation)
	cleanTBTP();

	total_rbdc_max = 0;
	// update the ST context and probe the cra and rbdc_max value (and set the allocation to CRA)
	for(st = m_context.begin(); st != m_context.end(); st++)
	{
		St_id = st->first;
		ThisSt = st->second;
		total_rbdc_max += ThisSt->GetRbdcMax();
		ThisSt->Update();

		// ignore simulated ST in stats, there ID is > 100
		// TODO limitation caused by environment plane,
		//      remove if environment plane is rewritten
		if(St_id > 100)
		{
			continue;
		}


		probe_gw_cra_st_alloc->put((int)Converter->ConvertFromCellsPerFrameToKbits((double)ThisSt->GetCra()));
		probe_gw_rbdc_st_alloc->put((int)Converter->ConvertFromCellsPerFrameToKbits((double)ThisSt->GetRbdc()));
		probe_gw_rbdc_max_st_alloc->put((int)Converter->ConvertFromCellsPerFrameToKbits((double)ThisSt->GetRbdcMax()));
	}
	probe_gw_rbdc_max_alloc->put((int)Converter->ConvertFromCellsPerFrameToKbits((double)total_rbdc_max));

	// dama processing
	resul = runDama();

	// we cannot clean TBTP before a caller requested it! See buildTBTP()

	// as long as the frame is changing, send all probes
	EnvPlane::send_probes();

	// update the frame numerotation
	m_currentSuperFrame = frame_nb;

	if(resul == -1)
	{ // dama_error
		UTI_ERROR("%s error during Dama Computation.\n", FUNCNAME);
		return (-1);
	}

	return 0;
}


// UTILITIES
// ========
//

/**
 * @brief Update the ST resources allocations according to given PEP request
 *
 * @param request  The request that updates the ST resources allocations
 */
bool DvbRcsDamaCtrl::applyPepCommand(PepRequest *request)
{
	DC_Context::iterator it;
	DC_St *st;
	bool success = true;

	// check that the ST is logged on
	it = this->m_context.find(request->getStId());
	if(it == this->m_context.end())
	{
		UTI_ERROR("ST%d is not logged on, ignore %s request\n",
		          request->getStId(),
		          request->getType() == PEP_REQUEST_ALLOCATION ?
		          "allocation" : "release");
		goto abort;
	}
	st = it->second;

	// update CRA allocation ?
	if(request->getCra() != 0)
	{
		int craCells;

		craCells = (int) ceil(Converter->ConvertFromKbitsToCellsPerFrame(request->getCra()));
		this->m_total_cra += st->SetCra(craCells);
		UTI_INFO("ST%u: update the CRA value to %u kbits/s\n",
		         request->getStId(), request->getCra());
	}

	// update RDBCmax threshold ?
	if(request->getRbdcMax() != 0)
	{
		double rbdcMaxCells;

		rbdcMaxCells = Converter->ConvertFromKbitsToCellsPerFrame(request->getRbdcMax());

		if(st->SetMaxRbdc(rbdcMaxCells) == 0)
		{
			UTI_INFO("ST%u: update RBDC MAX to %u kbits/s\n",
			         request->getStId(), request->getRbdcMax());
		}
		else
		{
			UTI_ERROR("ST%u: failed to update RBDC MAX to %u kbits/s\n",
			           request->getStId(), request->getRbdcMax());
			success = false;
		}
	}

	// inject one RDBC allocation ?
	if(request->getRbdc() != 0)
	{
		double rbdcCells;

		rbdcCells = Converter->ConvertFromKbitsToCellsPerFrame(request->getRbdc());

		// increase the RDBC timeout in order to be sure that RDBC
		// will not expire before the session is established
		st->SetTimeout(100);

		if(st->SetRbdc(rbdcCells) == 0)
		{
			UTI_INFO("ST%u: inject RDBC request of %u kbits/s\n",
			         request->getStId(), request->getRbdc());
		}
		else
		{
			UTI_ERROR("ST%u: failed to inject RDBC request "
			          "of %u kbits/s\n", request->getStId(),
			          request->getRbdc());
			success = false;
		}

		// change back RDBC timeout
		st->SetTimeout(m_rbdc_timeout);
	}

	return success;

abort:
	return false;
}


/**
 * The purpose of this function is to clean the TBTP internal table upon each superframe
 * @return 0 (always succed)
 * Here is supposed that it exists a unique global allocation for the whole Superframe
 * Hence only one frame is considered in the TBTP.
 *
 */
int DvbRcsDamaCtrl::cleanTBTP()
{
	T_DVB_TBTP *tbtp;   // Local pointer to the internal tbtp table
	T_DVB_FRAME *frame; // points to a frame inside the internal tbtp table
	T_DVB_BTP *btp;     // points to a btp within a frame
	int i;

	tbtp = (T_DVB_TBTP *) m_tbtp;
	frame = first_frame_ptr(tbtp);

	// DVB header
	// real size of the packet
	tbtp->hdr.msg_length = sizeof(T_DVB_TBTP) + sizeof(T_DVB_FRAME) +
	                       sizeof(T_DVB_BTP) * (frame->btp_loop_count + 1);
	tbtp->hdr.msg_type = MSG_TYPE_TBTP; // TBTP type

	UTI_DEBUG_L3("tbtp->hdr.msg_length = %ld.\n", tbtp->hdr.msg_length);

	// TBTP header
	tbtp->group_id = 0; // Unused. To be filled by the DVB Bloc
	tbtp->superframe_count = m_currentSuperFrame + 2; // #sf when it will be received
	tbtp->frame_loop_count = 1; // As said above, only one frame

	// FRAME header
	frame->frame_number = 0;

	// cleaning...
	// TBC : this is done in the ST update method
	for(i = 0; i < frame->btp_loop_count; i++)
	{
		btp = ith_btp_ptr(i, frame);
		// reset the allocation
		btp->assignment_count = 0;
	}

	return 0;
}

/**
 * Add an entry at the end of the internal TBTP
 * @param st_id         st logon id
 * @return BTP pointer on success, NULL pointer on failure
 * Call Hypothesis:
 *     cr is valid (> 0) and correspond to a registered st whose context is st_context
 *     Invariant 1 must be satisfied
 */
T_DVB_BTP *DvbRcsDamaCtrl::appendBtpEntry(DC_stId st_id)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[appendBtpEntry]";
	T_DVB_TBTP *tbtp;   // Local pointer to the internal tbtp table
	T_DVB_FRAME *frame; // points to a frame inside the internal tbtp table
	T_DVB_BTP *btp;     // points to a btp within a frame
	long needed_size;   // needed size for the TBTP structure to fill
	int resul;


	UTI_DEBUG("%s st_id=%d\n", FUNCNAME, st_id);

	// Here is supposed that it exists a unique global allocation for the whole Superframe
	// Hence only one frame is considered in the TBTP.
	//
	tbtp = (T_DVB_TBTP *) m_tbtp;
	frame = first_frame_ptr(tbtp);

	needed_size =
		sizeof(T_DVB_TBTP)
		+ sizeof(T_DVB_FRAME) + sizeof(T_DVB_BTP) * (frame->btp_loop_count + 1);

	resul = bufferCheck(&m_tbtp, &m_tbtp_size, needed_size);
	if(resul == -1)
	{
		UTI_ERROR("%s error while reallocating m_tbtp.\n", FUNCNAME);
		goto error;
	}
	// We reaffect our pointers if realloc() occured and then update our
	// TBTP header
	tbtp = (T_DVB_TBTP *) m_tbtp;
	frame = first_frame_ptr(tbtp);

	// Now we update the TBTP headers
	//
	frame->btp_loop_count += 1;
	tbtp->hdr.msg_length += sizeof(T_DVB_BTP);
	btp = ith_btp_ptr(frame->btp_loop_count - 1, frame);

	UTI_DEBUG_L3("tbtp->hdr.msg_length = %ld\n", tbtp->hdr.msg_length);

	// btp field is filled
	btp->assignment_count = 0; // for the moment, must be completed by dama computation
	btp->assignment_type = DVB_BTP_ONE_TIME_ASSIGNMENT;
	btp->channel_id = 0;
	btp->logon_id = st_id;
	btp->multiple_channel_flag = 0;
	btp->start_slot = 0;

	// Context is updated
	return (btp);

 error:
	return ((T_DVB_BTP *) NULL);
}

/**
 * Remove an entry of the internal TBTP
 * @param st_id refers to the st logon id
 * @return 0 on succes -1 on failure
 */
int DvbRcsDamaCtrl::removeBtpEntry(DC_stId st_id)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[removeBtpEntry]";
	T_DVB_TBTP *tbtp;   // Local pointer to the internal tbtp table
	T_DVB_FRAME *frame; // points to a frame inside the internal tbtp table
	T_DVB_BTP *btp;     // points to a btp within a frame
	T_DVB_BTP *end_btp; // points to the last btp within a frame
	int i;
	DC_Context::iterator st;

	UTI_DEBUG("%s st_id=%d\n", FUNCNAME, st_id);

	// Here is supposed that it exists a unique global allocation for the whole Superframe
	// Hence only one frame is considered in the TBTP.
	//
	tbtp = (T_DVB_TBTP *) m_tbtp;
	frame = first_frame_ptr(tbtp);

	if(frame->btp_loop_count > 0)
	{
		// find the index of the BTP for this terminal
		for(i = 0; i < frame->btp_loop_count; i++)
		{
			btp = ith_btp_ptr(i, frame);
			if(btp->logon_id == st_id)
			{
				if(i != frame->btp_loop_count - 1)
				{
					end_btp = ith_btp_ptr(frame->btp_loop_count - 1, frame);
					// move the last BTP of the frame to the BTP of the logged off TS
					memcpy(btp, end_btp, sizeof(T_DVB_BTP));

					// change the pointer in the TS context
					st = m_context.find(btp->logon_id);
					st->second->SetBtp(btp);
				}
			}
		}
	}
	// decrease the BTP number
	frame->btp_loop_count--;

	return 0;
}





/**
 * Check if there is enough room in the buffer, if not realloc it
 *
 * @param buffer       the pointer to the pointer to the buffer (input/ouput)
 * @param buffer_size  the pointer to the current buffer size variable (input/output)
 * @param wanted_size  the wanted size (input)
 * @return 0           on success, -1 otherwise
 */
int DvbRcsDamaCtrl::bufferCheck(unsigned char **buffer,
                                long *buffer_size,
                                long wanted_size)
{
	unsigned char *new_buff;
	long new_size;

	if(*buffer_size < wanted_size)
	{
		// new_size is
		// *buffer_size plus
		//      the number of m_buff_alloc size buffers
		//    times
		//      the size (m_buff_alloc) of such a buffer
		//
		new_size = *buffer_size +
			((wanted_size - *buffer_size) / m_buff_alloc + 1) * m_buff_alloc;

		new_buff = (unsigned char *) realloc(*buffer, new_size);
		// Sanity check
		//
		if(new_buff == NULL)
			return (-1);

		*buffer = new_buff;
		*buffer_size = new_size;
	}

	return 0;
}


/**
 * This function run the Dama, it allocates exactly what have been asked
 * We use internal SACT, TBTP and context for doing that.
 * After DAMA computation, TBTP is completed and context is reinitialized
 * @return 0 (always succeed)
 */
int DvbRcsDamaCtrl::runDama()
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDama]";
	DC_Context::iterator st;
	DC_stId st_id; // Id of the ST being under examination
	DC_St *ThisSt;

	for(st = m_context.begin(); st != m_context.end(); st++)
	{
		st_id = st->first;
		ThisSt = st->second;

		/*    st_context->btp_entry->assignment_count = 0;
		   st_context->btp_entry                   = NULL; */
	}

	UTI_ERROR("%s ---------------------------------", FUNCNAME);
	UTI_ERROR("%s void dama algorithm don't use it ", FUNCNAME);
	UTI_ERROR("%s ---------------------------------", FUNCNAME);

	return -1;
}
/**
 * Get the carrier Id for DAMA controller.
 * @return the carrier id
 */
long DvbRcsDamaCtrl::getCarrierId()
{
	return this->m_carrierId;
}

/**
 * Set the file descriptor for storing events
 * @param event_stream is the file descriptor of an append only opened stream used for event record
 * @param stat_stream is the file descriptor of an append only opened stream used for stat record
 *
 */
void DvbRcsDamaCtrl::setRecordFile(FILE * event_stream, FILE * stat_stream)
{
	this->event_file = event_stream;
	DC_RECORD_EVENT("%s", "# --------------------------------------\n");
	this->stat_file = stat_stream;
	DC_RECORD_STAT("%s", "# --------------------------------------\n");
}

