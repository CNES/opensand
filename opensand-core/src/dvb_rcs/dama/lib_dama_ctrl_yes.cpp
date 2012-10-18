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
 * @file lib_dama_ctrl_yes.cpp
 * @brief This library defines a DAMA controller that allocates every request.
 * @author Viveris Technologies
 */

#include "lib_dvb_rcs.h"
#include "lib_dama_ctrl_yes.h"

// environment plane
#include "opensand_env_plane/EnvironmentAgent_e.h"
extern T_ENV_AGENT EnvAgent;

#define DBG_PACKAGE PKG_DAMA_DC
#include "opensand_conf/uti_debug.h"
#define DC_DBG_PREFIX "[yes]"


/**
 * Constructor
 */
DvbRcsDamaCtrlYes::DvbRcsDamaCtrlYes()
{
}

/**
 * Destructor
 */
DvbRcsDamaCtrlYes::~DvbRcsDamaCtrlYes()
{
}

/**
 * This function run the Dama, it allocates exactly what have been asked
 * We use internal SACT, TBTP and context for doing that.
 * After DAMA computation, TBTP is completed and context is reinitialized
 * @return 0 (always succeed)
 */
int DvbRcsDamaCtrlYes::runDama()
{
	// Iterators
	DC_Context::iterator st;

	// Parameters used for ease of reading
	DC_stId st_id;  // Id of the ST being under examination
	DC_St *ThisSt;  // points to the internal context map associated with the Id
	int Request;
	int Alloc;
	int rbdc_request_number; // the number of RBDC requests
	int vbdc_request_number; // the number of VBDC requests
	int rbdc_request_sum;    // RBDC requests sum
	int vbdc_request_sum;    // VBDC requests sum

	rbdc_request_number = 0;
	vbdc_request_number = 0;
	rbdc_request_sum = 0;
	vbdc_request_sum = 0;

	for(st = m_context.begin(); st != m_context.end(); st++)
	{
		st_id = st->first;
		ThisSt = st->second;

		// retrieve the RBDC request
		Request = ThisSt->GetRbdc();

		if(Request != 0)
		{
			rbdc_request_number++;
			rbdc_request_sum += Request;

			Alloc = ThisSt->SetAllocation(Request, cr_rbdc);

			UTI_DEBUG("ST#%d has been fully served for RBDC "
			          "(%d timeslots)", st_id, Alloc);
		}
		// retrieve the VBDC request
		Request = ThisSt->GetVbdc();

		if(Request != 0)
		{
			vbdc_request_number++;
			vbdc_request_sum += Request;

			Alloc = ThisSt->SetAllocation(Request, cr_vbdc);

			UTI_DEBUG("ST#%d has been fully served for VBDC "
			          "(%d timeslots)", st_id, Alloc);
		}
	}
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_RBDC_REQUEST_NUMBER,
	                       0, rbdc_request_number);
	DC_RECORD_STAT("RBDC REQUEST NB %d", rbdc_request_number);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_RBDC_REQUESTED_CAPACITY,
	                       0,
	                       (int) Converter->
	                       ConvertFromCellsPerFrameToKbits((double)
	                       rbdc_request_sum));
	DC_RECORD_STAT("RBDC REQUEST SUM %d kbits/s",
	               (int) Converter->
	               		ConvertFromCellsPerFrameToKbits((double) rbdc_request_sum));
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_VBDC_REQUEST_NUMBER,
	                       0, vbdc_request_number);
	DC_RECORD_STAT("VBDC REQUEST NB %d", vbdc_request_number);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_VBDC_REQUESTED_CAPACITY,
	                       0, vbdc_request_sum);
	DC_RECORD_STAT("VBDC REQUEST SUM %d slot(s)", vbdc_request_sum);
	ENV_AGENT_Probe_PutInt(&EnvAgent,
	                       C_PROBE_GW_RBDC_ALLOCATION,
	                       0,
	                       (int) Converter->
	                       ConvertFromCellsPerFrameToKbits((double)
	                       rbdc_request_sum));
	DC_RECORD_STAT("ALLOC RBDC %d kbits/s",
	               (int) Converter->
	               		ConvertFromCellsPerFrameToKbits((double) rbdc_request_sum));
	ENV_AGENT_Probe_PutInt(&EnvAgent, C_PROBE_GW_VBDC_ALLOCATION, 0,
	                       (int) Converter->
	                       ConvertFromCellsPerFrameToKbits((double)
	                       vbdc_request_sum));
	DC_RECORD_STAT("ALLOC VBDC %d kbits/s",
	               (int) Converter->
	               		ConvertFromCellsPerFrameToKbits((double) vbdc_request_sum));

	return 0;
}
