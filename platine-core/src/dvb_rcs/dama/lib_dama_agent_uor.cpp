/**
 * @file lib_dama_agent_uor.cpp
 * @brief This library defines UoR DAMA agent
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 *
 * UoR DAMA agent is still now the same of ESA DAMA agent.
 */

#include <string>
#include <math.h>
#include <stdlib.h>

#include "platine_margouilla/mgl_time.h"
#include "lib_dvb_rcs.h"
#include "lib_dama_agent_uor.h"
#include "lib_dama_utils.h"

#define DBG_PACKAGE PKG_DAMA_DA
#define DA_DBG_PREFIX "[UOR]"
#include "platine_conf/uti_debug.h"


/**
 * Constructor
 */
DvbRcsDamaAgentUoR::DvbRcsDamaAgentUoR():
	DvbRcsDamaAgentEsa()
{
	// DAMA ESA initialises everything, nothing more to do
}

