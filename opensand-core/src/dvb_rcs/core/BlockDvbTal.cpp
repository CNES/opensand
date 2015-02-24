/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file BlockDvbTal.cpp
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal, compatible
 *        with Legacy and RrmQosDama agent
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */


#include <opensand_output/Output.h>

#include "BlockDvbTal.h"

#include "DamaAgentRcsLegacy.h"
#include "DamaAgentRcsRrmQos.h"
#include "TerminalCategoryDama.h"
#include "ScpcScheduling.h"
#include "SlottedAlohaPacketData.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Ttp.h"
#include "Sof.h"

#include <opensand_rt/Rt.h>


#include <sstream>
#include <assert.h>
#include <unistd.h>

int BlockDvbTal::Downward::Downward::qos_server_sock = -1;


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/


BlockDvbTal::BlockDvbTal(const string &name, tal_id_t UNUSED(mac_id)):
	BlockDvb(name)
{
}

BlockDvbTal::~BlockDvbTal()
{
}

bool BlockDvbTal::onInit(void)
{
	return true;
}


bool BlockDvbTal::onDownwardEvent(const RtEvent *const event)
{
	return ((Downward *)this->downward)->onEvent(event);
}


bool BlockDvbTal::onUpwardEvent(const RtEvent *const event)
{
	return ((Upward *)this->upward)->onEvent(event);
}


/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/

BlockDvbTal::Downward::Downward(Block *const bl, tal_id_t mac_id):
	DvbDownward(bl),
	mac_id(mac_id),
	state(state_initializing),
	group_id(),
	tal_id(),
	cra_kbps(0),
	max_rbdc_kbps(0),
	max_vbdc_kb(0),
	dama_agent(NULL),
	saloha(NULL),
	scpc_carr_duration_ms(0),
	scpc_timer(-1),
	ret_fmt_groups(),
	scpc_fmt_simu(),
	scpc_sched(NULL),
	scpc_frame_counter(0),
	carrier_id_ctrl(),
	carrier_id_logon(),
	carrier_id_data(),
	dvb_fifos(),
	default_fifo_id(0),
	sync_period_frame(-1),
	obr_slot_frame(-1),
	complete_dvb_frames(),
	logon_timer(-1),
	cni(100),
	qos_server_host(),
	event_login(NULL),
	log_frame_tick(NULL),
	log_qos_server(NULL),
	log_saloha(NULL),
	probe_st_queue_size(),
	probe_st_queue_size_kb(),
	probe_st_l2_to_sat_before_sched(),
	probe_st_l2_to_sat_after_sched(),
	l2_to_sat_total_bytes(0),
	probe_st_l2_to_sat_total(NULL),
	probe_st_l2_from_sat(NULL)
{
}

BlockDvbTal::Downward::~Downward()
{
	if(this->dama_agent != NULL)
	{
		delete this->dama_agent;
	}

	if(this->saloha)
	{
		delete this->saloha;
	}
	
	if(this->scpc_sched)
	{
		delete this->scpc_sched;
	}
	
	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for(fmt_groups_t::iterator it = this->ret_fmt_groups.begin();
	    it != this->ret_fmt_groups.end(); ++it)
	{
		delete (*it).second;
	}

	// delete fifos
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();

	// close QoS Server socket if it was opened
	if(BlockDvbTal::Downward::Downward::qos_server_sock != -1)
	{
		close(BlockDvbTal::Downward::Downward::qos_server_sock);
	}

	this->complete_dvb_frames.clear();
}


bool BlockDvbTal::Downward::onInit(void)
{
	this->log_qos_server = Output::registerLog(LEVEL_WARNING, 
	                                           "Dvb.QoSServer");	
	this->log_frame_tick = Output::registerLog(LEVEL_WARNING, 
	                                           "Dvb.DamaAgent.FrameTick");	

	// get the common parameters
	if(!this->initCommon(RETURN_UP_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the initialisation\n");
		goto error;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common initialisation\n");
		goto error;
	}

	if(!this->initCarrierId())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the initialisation\n");
		goto error;
	}

	if(!this->initMacFifo())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the MAC FIFO part of the initialisation\n");
		goto error;
	}

	if(!this->initDama())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the DAMA part of the initialisation\n");
		goto error;
	}

	if(!this->initSlottedAloha())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of Slotted Aloha\n");
		goto error;
	}

	if(!this->initScpc())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the SCPC part of the initialisation\n");
		goto error;
	}

	if(!this->dama_agent && !this->saloha && !this->scpc_sched)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to instanciate DAMA or Slotted Aloha or SCPC, "
		    "check your configuration\n");
		return false;
	}


	if(!this->initQoSServer())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the QoS Server part of the initialisation\n");
		goto error;
	}

	this->initStatsTimer(this->ret_up_frame_duration_ms);

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output\n");
		goto error;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialization of timers\n");
		goto error;
	}

	// now everyhing is initialized so we can do some processing

	// after all of things have been initialized successfully,
	// send a logon request
	LOG(this->log_init, LEVEL_DEBUG,
	    "send a logon request with MAC ID %d to NCC\n",
	    this->mac_id);
	this->state = state_wait_logon_resp;
	if(!this->sendLogonReq())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to send the logon request to the NCC\n");
		goto error;
	}

	return true;
error:
// TODO something to release ?
	return false;
}


bool BlockDvbTal::Downward::initCarrierId(void)
{
	// Get the ID for control carrier
	if(!Conf::getValue(SATCAR_SECTION,
	                   DVB_CAR_ID_CTRL,
	                   this->carrier_id_ctrl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_CTRL, SATCAR_SECTION);
		goto error;
	}

	// Get the ID for logon carrier
	if(!Conf::getValue(SATCAR_SECTION,
	                   DVB_CAR_ID_LOGON,
	                   this->carrier_id_logon))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_LOGON, SATCAR_SECTION);
		goto error;
	}

	// Get the ID for data carrier
	if(!Conf::getValue(SATCAR_SECTION,
	                   DVB_CAR_ID_DATA,
	                          this->carrier_id_data))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u %s missing from section %s\n",
		    this->super_frame_counter,
		    DVB_CAR_ID_DATA, SATCAR_SECTION);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: carrier IDs for Ctrl = %u, Logon = %u, "
	    "Data = %u\n", this->super_frame_counter,
	    this->carrier_id_ctrl,
	    this->carrier_id_logon, this->carrier_id_data);

	return true;
error:
	return false;
}

bool BlockDvbTal::Downward::initMacFifo(void)
{
	ConfigurationList fifo_list;
	ConfigurationList::iterator iter;

	/*
	* Read the MAC queues configuration in the configuration file.
	* Create and initialize MAC FIFOs
	*/
	if(!Conf::getListItems(DVB_TAL_SECTION, FIFO_LIST, fifo_list))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s, %s': missing fifo list", DVB_TAL_SECTION,
		    FIFO_LIST);
		goto err_fifo_release;
	}

	for(iter = fifo_list.begin(); iter != fifo_list.end(); iter++)
	{
		qos_t fifo_priority = 0;
		vol_pkt_t fifo_size = 0;
		string fifo_name;
		string fifo_access_type;
		DvbFifo *fifo;

		// get fifo_id --> fifo_priority
		if(!Conf::getAttributeValue(iter, FIFO_PRIO, fifo_priority))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_PRIO, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_name
		if(!Conf::getAttributeValue(iter, FIFO_NAME, fifo_name))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_NAME, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get fifo_size
		if(!Conf::getAttributeValue(iter, FIFO_SIZE, fifo_size))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_SIZE, DVB_TAL_SECTION, FIFO_LIST);
			goto err_fifo_release;
		}
		// get the fifo CR type
		if(!Conf::getAttributeValue(iter, FIFO_ACCESS_TYPE, fifo_access_type))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot get %s from section '%s, %s'\n",
			    FIFO_ACCESS_TYPE, DVB_TAL_SECTION,
			    FIFO_LIST);
			goto err_fifo_release;
		}

		fifo = new DvbFifo(fifo_priority, fifo_name,
		                   fifo_access_type, fifo_size);

		LOG(this->log_init, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "CR type %d\n",
		    fifo->getPriority(),
		    fifo->getName().c_str(),
		    fifo->getMaxSize(),
		    fifo->getAccessType());

		// the default FIFO is the last one = the one with the smallest priority
		// actually, the IP plugin should add packets in the default FIFO if
		// the DSCP field is not recognize, default_fifo_id should not be used
		// this is only used if traffic categories configuration and fifo configuration
		// are not coherent.
		this->default_fifo_id = std::max(this->default_fifo_id, fifo->getPriority());

		this->dvb_fifos.insert(pair<unsigned int, DvbFifo *>(fifo->getPriority(), fifo));
	} // end for(queues are now instanciated and initialized)


	this->l2_to_sat_total_bytes = 0;
	
	return true;

err_fifo_release:
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		delete (*it).second;
	}
	this->dvb_fifos.clear();
	return false;
}


bool BlockDvbTal::Downward::initDama(void)
{
	time_ms_t sync_period_ms = 0;
	time_sf_t rbdc_timeout_sf = 0;
	time_sf_t msl_sf = 0;
	string dama_algo;
	bool cr_output_only;
	bool is_dama_fifo = false;

	TerminalCategories<TerminalCategoryDama> dama_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	TerminalCategoryDama *tal_category = NULL;
	TerminalMapping<TerminalCategoryDama>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getAccessType() == access_dama_rbdc ||
		   (*it).second->getAccessType() == access_dama_vbdc ||
		   (*it).second->getAccessType() == access_dama_cra)
		{
			is_dama_fifo = true;
		}
	}

	// init fmt_simu
	if(!this->initModcodFiles(RETURN_UP_MODCOD_DEF_RCS,
	                          RETURN_UP_MODCOD_TIME_SERIES))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD files\n");
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(RETURN_UP_BAND,
	                                         DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->satellite_type,
	                                         this->fmt_simu.getModcodDefinitions(),
	                                         dama_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}

	if(dama_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No DAMA carriers\n");
		return true;
	}

	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by DAMA
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a DAMA category\n");
			goto release_cat;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = (*tal_map_it).second;
	}

	// check if there is DAMA carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No DAMA carrier\n");
		if(is_dama_fifo)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Remove DAMA FIFOs because there is no "
			    "DAMA carrier\n");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				if((*it).second->getAccessType() == access_dama_rbdc ||
				   (*it).second->getAccessType() == access_dama_vbdc ||
				   (*it).second->getAccessType() == access_dama_cra)
				{
					delete (*it).second;
					this->dvb_fifos.erase(it);
				}
			}
		}
		goto release_cat;
	}

	if(!is_dama_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The DAMA carrier won't be used as there is no DAMA FIFO\n");
		goto release_cat;
	}

	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	if(!Conf::getValue(DVB_TAL_SECTION, CRA,
	                   this->cra_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", CRA);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "cra_kbps = %d kbits/s\n", this->cra_kbps);

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	if(!Conf::getValue(DA_TAL_SECTION, DA_MAX_RBDC_DATA,
	                   this->max_rbdc_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n",
		    DA_MAX_RBDC_DATA);
		goto error;
	}

	// Max VBDC
	if(!Conf::getValue(DA_TAL_SECTION, DA_MAX_VBDC_DATA,
	                   this->max_vbdc_kb))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_MAX_VBDC_DATA);
		goto error;
	}

	// MSL duration -- in frames number
	if(!Conf::getValue(DA_TAL_SECTION, DA_MSL_DURATION, msl_sf))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_MSL_DURATION);
		goto error;
	}

	// CR computation rule
	if(!Conf::getValue(DA_TAL_SECTION, DA_CR_RULE, cr_output_only))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", DA_CR_RULE);
		goto error;
	}

	// get the OBR period
	if(!Conf::getValue(GLOBAL_SECTION, SYNC_PERIOD,
	                   sync_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s", SYNC_PERIOD);
		goto error;
	}
	this->sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                              (double)this->ret_up_frame_duration_ms);

	// deduce the Obr slot position within the multi-frame, from the mac
	// address and the OBR period
	// ObrSlotFrame= MacAddress 'modulo' Obr Period
	// NB : ObrSlotFrame is within [0, Obr Period -1]
	this->obr_slot_frame = this->mac_id % this->sync_period_frame;
	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: MAC adress = %d, SYNC period = %d, "
	    "OBR slot frame = %d\n", this->super_frame_counter,
	    this->mac_id, this->sync_period_frame, this->obr_slot_frame);

	rbdc_timeout_sf = this->sync_period_frame + 1;

	LOG(this->log_init, LEVEL_NOTICE,
	    "ULCarrierBw %d kbits/s, "
	    "RBDC max %d kbits/s, RBDC Timeout %d frame, "
	    "VBDC max %d kbits, mslDuration %d frames, "
	    "getIpOutputFifoSizeOnly %d\n",
	    this->cra_kbps, this->max_rbdc_kbps,
	    rbdc_timeout_sf, this->max_vbdc_kb, msl_sf,
	    cr_output_only);

	// dama algorithm
	if(!Conf::getValue(DVB_TAL_SECTION, DAMA_ALGO,
	                   dama_algo))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section '%s': missing parameter '%s'\n",
		    DVB_TAL_SECTION, DAMA_ALGO);
		goto error;
	}

	if(dama_algo == "Legacy")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create Legacy DAMA agent\n",
		    this->super_frame_counter);

		this->dama_agent = new DamaAgentRcsLegacy();
	}
	else if(dama_algo == "RrmQos")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create RrmQos DAMA agent\n",
		    this->super_frame_counter);

		this->dama_agent = new DamaAgentRcsRrmQos();
	}
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot create DAMA agent: algo named '%s' is not "
		    "managed by current MAC layer\n", dama_algo.c_str());
		goto error;
	}

	if(this->dama_agent == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create DAMA agent\n");
		goto error;
	}

	// Initialize the DamaAgent parent class
	if(!this->dama_agent->initParent(this->ret_up_frame_duration_ms,
	                                 this->cra_kbps,
	                                 this->max_rbdc_kbps,
	                                 rbdc_timeout_sf,
	                                 this->max_vbdc_kb,
	                                 msl_sf,
	                                 this->sync_period_frame,
	                                 cr_output_only,
	                                 this->pkt_hdl,
	                                 this->dvb_fifos))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u Dama Agent Initialization failed.\n",
		    this->super_frame_counter);
		goto err_agent_release;
	}

	// Initialize the DamaAgentRcsXXX class
	if(!this->dama_agent->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Agent initialization failed.\n");
		goto err_agent_release;
	}

release_cat:
	for(cat_it = dama_categories.begin();
	    cat_it != dama_categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	return true;

err_agent_release:
	delete this->dama_agent;
error:
	for(cat_it = dama_categories.begin();
	    cat_it != dama_categories.end(); ++cat_it)
	{
		delete (*cat_it).second;
	}
	return false;
}

bool BlockDvbTal::Downward::initSlottedAloha(void)
{
	bool is_sa_fifo = false;

	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> terminal_affectation;
	TerminalCategorySaloha *default_category;
	TerminalCategorySaloha *tal_category = NULL;
	TerminalMapping<TerminalCategorySaloha>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategorySaloha>::iterator cat_it;

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getAccessType() == access_saloha)
		{
			is_sa_fifo = true;
		}
	}

	// TODO use the up return frame duration for Slotted Aloha
	// fmt_simu was initialized in initDama
	if(!this->initBand<TerminalCategorySaloha>(RETURN_UP_BAND,
	                                           ALOHA,
	                                           this->ret_up_frame_duration_ms,
	                                           this->satellite_type,
	                                           this->fmt_simu.getModcodDefinitions(),
	                                           sa_categories,
	                                           terminal_affectation,
	                                           &default_category,
	                                           this->ret_fmt_groups))
	{
		return false;
	}

	if(sa_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No Slotted Aloha carriers\n");
		return true;
	}

	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by Slotted Aloha
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a Slotted Aloha category\n");
			return true;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = (*tal_map_it).second;
	}

	// check if there is Slotted Aloha carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No Slotted Aloha carrier\n");
		if(is_sa_fifo)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "Remove Slotted Aloha FIFOs because there is no "
			    "Slotted Aloha carrier\n");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				if((*it).second->getAccessType() == access_saloha)
				{
					delete (*it).second;
					this->dvb_fifos.erase(it);
				}
			}
		}
		return true;
	}

	if(!is_sa_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The Slotted Aloha carrier won't be used as there is no "
		    "Slotted Aloha FIFO\n");
		for(cat_it = sa_categories.begin();
		    cat_it != sa_categories.end(); ++cat_it)
		{
			delete (*cat_it).second;
		}
		return true;
	}

	for(cat_it = sa_categories.begin();
	    cat_it != sa_categories.end(); ++cat_it)
	{
		if((*cat_it).second->getLabel() != tal_category->getLabel())
		{
			delete (*cat_it).second;
		}
	}

	// cannot use Slotted Aloha with regenerative satellite
	if(this->satellite_type == REGENERATIVE)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Carrier configured with Slotted Aloha while satellite "
		    "is regenerative\n");
		return false;
	}

	// Create the Slotted ALoha part
	this->saloha = new SlottedAlohaTal();
	if(!this->saloha)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create Slotted Aloha\n");
		return false;
	}

	// Initialize the Slotted Aloha parent class
	// Unlike (future) scheduling, Slotted Aloha get all categories because
	// it also handles received frames and in order to know to which
	// category a frame is affected we need to get source terminal ID
	if(!this->saloha->initParent(this->ret_up_frame_duration_ms,
	                             this->pkt_hdl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_saloha;
	}

	if(!this->saloha->init(this->mac_id,
	                       tal_category,
	                       this->dvb_fifos))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_saloha;
	}

	return true;

release_saloha:
	delete this->saloha;
	return false;
}


bool BlockDvbTal::Downward::initScpc(void)
{
	bool is_scpc_fifo = false;

	TerminalCategories<TerminalCategoryDama> scpc_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	TerminalCategoryDama *tal_category = NULL;
	TerminalCategoryDama *cat;
	TerminalMapping<TerminalCategoryDama>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;
	
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getAccessType() == access_scpc)
		{
			is_scpc_fifo = true;
		}
	}
	
	// init fmt_simu
	if(!this->initModcodFiles(FORWARD_DOWN_MODCOD_DEF_S2, 
		                      FORWARD_DOWN_MODCOD_TIME_SERIES,
		                      this->scpc_fmt_simu))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the down/forward MODCOD files\n");
		goto error;
	}
	
	//  Duration of the carrier -- in ms
	if(!Conf::getValue(SCPC_SECTION, SCPC_C_DURATION,
	                   this->scpc_carr_duration_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing %s\n", SCPC_C_DURATION);
		goto error;
	}

	LOG(this->log_init, LEVEL_NOTICE,
	    "scpc_carr_duration_ms = %d ms\n", this->scpc_carr_duration_ms);

	if(!this->initBand<TerminalCategoryDama>(RETURN_UP_BAND,
	                                         SCPC,
	                                         this->scpc_carr_duration_ms,
	                                         this->satellite_type,
	                                         this->scpc_fmt_simu.getModcodDefinitions(),
	                                         scpc_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         this->ret_fmt_groups))
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "InitBand not correctly initialized \n");
		
		return false;
	}
	
	if(scpc_categories.size() == 0)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No SCPC carriers\n");
		goto release_scpc;
	}
	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by SCPC
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a SCPC category\n");
			return true;
		}
		tal_category = default_category;
	}   
	else
	{
		tal_category = (*tal_map_it).second;
	}
	// check if there are SCPC carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No SCPC carrier\n");
		if(is_scpc_fifo)
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "Remove SCPC FIFOs because there is no "
			    "SCPC carrier in the return_up_band configuration\n");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				if((*it).second->getAccessType() == access_scpc)
				{
					delete (*it).second;
					this->dvb_fifos.erase(it);
				}
			}
			goto error;
		}
		goto release_scpc;
	}
	if(!is_scpc_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The SCPC carrier won't be used as there is no "
		    "SCPC FIFO in Terminal\n");
		for(cat_it = scpc_categories.begin();
		    cat_it != scpc_categories.end(); ++cat_it)
		{
			delete (*cat_it).second;
		}
		goto release_scpc;
	}
	
	//Check if there are DAMA or SALOHA FIFOs in the terminal
    if(this->dama_agent || this->saloha)	
    {
    	LOG(this->log_init, LEVEL_ERROR,
    	    "Conflict: SCPC FIFOs and DAMA or SALOHA FIFOs "
    	    "in the same Terminal\n");
    	goto error;
    }
	
	//TODO: veritfy that 2ST are not using the same carrier and category

	// TODO cannot use SCPC with regenerative satellite
	if(this->satellite_type == REGENERATIVE)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Carrier configured with SCPC while satellite "
		    "is regenerative\n");
		goto error;
	}
	   
	//Initialise Encapsulation scheme
	
	if(!this->initPktHdl("GSE",
	                     &this->pkt_hdl, true))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed get packet handler\n");
		goto error;
	}
	
	// Create the SCPC scheduler
	cat = scpc_categories.begin()->second;
	this->scpc_sched = new ScpcScheduling(this->scpc_carr_duration_ms,
										  this->pkt_hdl,
										  this->dvb_fifos,
										  &this->scpc_fmt_simu,
										  cat);    
	if(!this->scpc_sched)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize SCPC\n");
		goto error;
	}

	return true;

release_scpc: //Something TODO
	return true;

error:
	return false;
}



bool BlockDvbTal::Downward::initQoSServer(void)
{
	// QoS Server: read hostname and port from configuration
	if(!Conf::getValue(SECTION_QOS_AGENT, QOS_SERVER_HOST,
	                   this->qos_server_host))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section %s, %s missing",
		    SECTION_QOS_AGENT, QOS_SERVER_HOST);
		goto error;
	}

	if(!Conf::getValue(SECTION_QOS_AGENT, QOS_SERVER_PORT,
	                   this->qos_server_port))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section %s, %s missing\n",
		    SECTION_QOS_AGENT, QOS_SERVER_PORT);
		goto error;
	}
	else if(this->qos_server_port <= 1024 || this->qos_server_port > 0xffff)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "QoS Server port (%d) not valid\n",
		    this->qos_server_port);
		goto error;
	}

	// QoS Server: catch the SIGFIFO signal that is sent to the process
	// when QoS Server kills the TCP connection
	if(signal(SIGPIPE, BlockDvbTal::Downward::closeQosSocket) == SIG_ERR)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "cannot catch signal SIGPIPE\n");
		goto error;
	}

	// QoS Server: try to connect to remote host
	this->connectToQoSServer();

	return true;
error:
	return false;
}

bool BlockDvbTal::Downward::initOutput(void)
{
	this->event_login = Output::registerEvent("DVB.login");

	if(this->saloha)
	{
		this->log_saloha = Output::registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
	}

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		const char *fifo_name = ((*it).second)->getName().data();
		unsigned int id = (*it).first;

		this->probe_st_queue_size[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_LAST,
			                           "Queue size.packets.%s", fifo_name);
		this->probe_st_queue_size_kb[id] =
			Output::registerProbe<int>("kbits", true, SAMPLE_LAST,
			                           "Queue size.%s", fifo_name);

		this->probe_st_l2_to_sat_before_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
			                           "Throughputs.L2_to_SAT_before_sched.%s",
			                           fifo_name);
		this->probe_st_l2_to_sat_after_sched[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_AVG,
			                           "Throughputs.L2_to_SAT_after_sched.%s",
			                           fifo_name);
		this->probe_st_queue_loss[id] =
			Output::registerProbe<int>("Packets", true, SAMPLE_SUM,
		                               "Queue loss.packets.%s",
		                               fifo_name);
		this->probe_st_queue_loss_kb[id] =
			Output::registerProbe<int>("Kbits/s", true, SAMPLE_SUM,
		                               "Queue loss.%s",
		                               fifo_name);
	}
	this->probe_st_l2_to_sat_total =
		Output::registerProbe<int>("Throughputs.L2_to_SAT_after_sched.total",
		                           "Kbits/s", true, SAMPLE_AVG);
	return true;
}


bool BlockDvbTal::Downward::initTimers(void)
{
	this->logon_timer = this->addTimerEvent("logon", 5000,
	                                        false, // do not rearm
	                                        false // do not start
	                                        );
	// QoS Server: check connection status in 5 seconds
	this->qos_server_timer = this->addTimerEvent("qos_server", 5000);
	if(this->scpc_sched)
	{	
		this->scpc_timer = this->addTimerEvent("scpc_timer",
		                                       this->scpc_carr_duration_ms);
	}
	return true;
}


bool BlockDvbTal::Downward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			// first handle specific messages
			if(((MessageEvent *)event)->getMessageType() == msg_sig)
			{
				DvbFrame *frame = (DvbFrame *)((MessageEvent *)event)->getData();
				if(!this->handleDvbFrame(frame))
				{
					return false;
				}
				break;
			}
			// messages from upper layer: burst of encapsulation packets
			NetBurst *burst;
			NetBurst::iterator pkt_it;
			std::string message;
			std::ostringstream oss;
			int ret;
			// TODO move saloha handling in a specific function ?
			// Slotted Aloha variables
			unsigned int sa_burst_size = 0; // burst size
			unsigned int sa_offset = 0; // packet position (offset) in the burst
			SlottedAlohaPacketData *sa_packet = NULL; // Slotted Aloha packet

			burst = (NetBurst *)((MessageEvent *)event)->getData();

			sa_burst_size = burst->length();
			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: encapsulation burst received (%d "
			    "packets)\n", this->super_frame_counter,
			    sa_burst_size);


			// set each packet of the burst in MAC FIFO
			for(pkt_it = burst->begin(); pkt_it != burst->end(); pkt_it++)
			{
				qos_t fifo_priority = (*pkt_it)->getQos();

				LOG(this->log_receive, LEVEL_DEBUG,
				    "SF#%u: encapsulation packet has QoS value %u\n",
				    this->super_frame_counter, fifo_priority);

				// find the FIFO associated to the IP QoS (= MAC FIFO id)
				// else use the default id
				if(this->dvb_fifos.find(fifo_priority) == this->dvb_fifos.end())
				{
					fifo_priority = this->default_fifo_id;
				}

				// Slotted Aloha
				sa_packet = NULL;
				if(this->saloha &&
				   this->dvb_fifos[fifo_priority]->getAccessType() == access_saloha)
				{
					sa_packet = this->saloha->addSalohaHeader(*pkt_it,
					                                          sa_offset++,
					                                          sa_burst_size);
					if(!sa_packet)
					{
						LOG(this->log_saloha, LEVEL_ERROR,
						    "SF#%u: unable to "
						    "store received Slotted Aloha encapsulation "
						    "packet (see previous errors)\n",
						    this->super_frame_counter);
						return false;
					}
				}

				LOG(this->log_receive, LEVEL_INFO,
				    "SF#%u: store one encapsulation packet "
				    "(QoS = %d)\n", this->super_frame_counter,
				    fifo_priority);

				// store the encapsulation packet in the FIFO
				if(!this->onRcvEncapPacket(sa_packet ? sa_packet : *pkt_it,
				                           this->dvb_fifos[fifo_priority],
				                           0))
				{
					// a problem occured, we got memory allocation error
					// or fifo full and we won't empty fifo until next
					// call to onDownwardEvent => return
					LOG(this->log_receive, LEVEL_ERROR,
					    "SF#%u: unable to "
					    "store received encapsulation "
					    "packet (see previous errors)\n",
					    this->super_frame_counter);
					burst->clear();
					delete burst;
					return false;
				}

			}
			burst->clear(); // avoid deteleting packets when deleting burst
			delete burst;

			// Cross layer information: if connected to QoS Server, build XML
			// message and send it
			// TODO move in a dedicated class
			if(BlockDvbTal::Downward::Downward::qos_server_sock == -1)
			{
				break;
			}

 			message = "";
			message.append("<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n");
			message.append("<XMLQoSMessage>\n");
			message.append(" <Sender>");
			message.append("CrossLayer");
			message.append("</Sender>\n");
			message.append(" <Type type=\"CrossLayer\" >\n");
			message.append(" <Infos ");
			for(fifos_t::iterator it = this->dvb_fifos.begin();
			    it != this->dvb_fifos.end(); ++it)
			{
				int nbFreeFrames = (*it).second->getMaxSize() -
				                   (*it).second->getCurrentSize();
				// bits
				int nbFreeBits = nbFreeFrames * this->pkt_hdl->getFixedLength() * 8;
				// bits/ms or kbits/s
				float macRate = nbFreeBits / this->ret_up_frame_duration_ms ;
				oss << "File=\"" << (int) macRate << "\" ";
				message.append(oss.str());
				oss.str("");
			}
			message.append("/>");
			message.append(" </Type>\n");
			message.append("</XMLQoSMessage>\n");

			ret = write(BlockDvbTal::Downward::Downward::qos_server_sock,
			            message.c_str(),
			            message.length());
			if(ret == -1)
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "failed to send message to QoS Server: %s "
				    "(%d)\n", strerror(errno), errno);
			}
		}
		break;

		case evt_timer:
		{
			if(*event == this->logon_timer)
			{
				if(this->state == state_wait_logon_resp)
				{
					// send another logon_req and raise timer
					// only if we are in the good state
					LOG(this->log_receive, LEVEL_NOTICE,
					    "still no answer from NCC to the "
					    "logon request we sent for MAC ID %d, "
					    "send a new logon request\n",
					    this->mac_id);
					this->sendLogonReq();
				}
			}
			else if(*event == this->qos_server_timer)
			{
				// try to re-connect to QoS Server if not already connected
				if(BlockDvbTal::Downward::Downward::qos_server_sock == -1)
				{
					if(!this->connectToQoSServer())
					{
						LOG(this->log_receive, LEVEL_INFO,
						    "failed to connect with QoS Server, "
						    "cannot send cross layer information");
					}
				}
			}
			else if(*event == this->scpc_timer)
			{
				
				uint32_t remaining_alloc_sym = 0;
				
				this->updateStats();
				this->scpc_frame_counter++;
				//Schedule Creation
				if(!this->scpc_sched->schedule(this->scpc_frame_counter,
				                               this->getCurrentTime(),
				                               &this->complete_dvb_frames,
				                               remaining_alloc_sym))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "failed to schedule SCPC encapsulation "
					    "packets stored in DVB FIFO\n");
					return false;

				}
				
				LOG(this->log_receive, LEVEL_INFO,
			        "SF#%u: %u symbol remaining after "
			        "scheduling\n", this->super_frame_counter,
			        remaining_alloc_sym);
			   	
			   	// send on the emulated DVB network the DVB frames that contain
				// the encapsulation packets scheduled by the SCPC agent algorithm
				if(!this->sendBursts(&this->complete_dvb_frames,
					this->carrier_id_data))
					{
						LOG(this->log_frame_tick, LEVEL_ERROR,
						    "failed to send bursts in DVB frames\n");
						return false;
					}
			}		
			else
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "SF#%u: unknown timer event received %s",
				    this->super_frame_counter, event->getName().c_str());
				return false;
			}
			break;
		}
		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown event received %s",
			    this->super_frame_counter,
			    event->getName().c_str());
			return false;
	}

	return true;
}

bool BlockDvbTal::Downward::sendLogonReq(void)
{
	LogonRequest *logon_req = new LogonRequest(this->mac_id,
	                                           this->cra_kbps,
	                                           this->max_rbdc_kbps,
	                                           this->max_vbdc_kb);

	// send the message to the lower layer
	if(!this->sendDvbFrame((DvbFrame *)logon_req,
		                   this->carrier_id_logon))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send Logon Request\n");
		goto error;
	}
	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u Logon Req. sent to lower layer\n",
	    this->super_frame_counter);

	if(!this->startTimer(this->logon_timer))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "cannot start logon timer");
		goto error;
	}

	// send the corresponding event
	Output::sendEvent(this->event_login, "Login sent to GW");
	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_CORRUPTED:
			if(!this->with_phy_layer)
			{
				delete dvb_frame;
				goto error;
			}
			// get ACM parameters that will be transmited to GW in SAC
			this->cni = dvb_frame->getCn();
			delete dvb_frame;
			break;

		case MSG_TYPE_SALOHA_CTRL:
			if(this->saloha && !this->saloha->onRcvFrame(dvb_frame))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to handle Slotted Aloha Signal Controls frame");
				goto error;
			}
			break;

		case MSG_TYPE_SOF:
			if(!this->handleStartOfFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle SoF\n");
				delete dvb_frame;
				goto error;
			}
			delete dvb_frame;
			break;

		case MSG_TYPE_TTP:
		{
			// TODO Ttp *ttp = dynamic_cast<Ttp *>(dvb_frame);
			Ttp *ttp = (Ttp *)dvb_frame;
			if(this->dama_agent && !this->dama_agent->hereIsTTP(ttp))
			{
				delete dvb_frame;
				goto error_on_TTP;
			}
			delete dvb_frame;
		}
		break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->handleLogonResp(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle logon response\n");
				delete dvb_frame;
				goto error;
			}
			delete dvb_frame;
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}

	return true;

error_on_TTP:
	LOG(this->log_receive, LEVEL_ERROR,
	    "TTP Treatments failed at SF#%u",
	    this->super_frame_counter);
	return false;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u",
	    this->super_frame_counter);
	return false;
}


bool BlockDvbTal::Downward::sendSAC(void)
{
	bool empty;
	Sac *sac;

	if(!this->dama_agent)
	{
		return true;
	}
	sac = new Sac(this->tal_id, this->group_id);
	// Set CR body
	// NB: access_type parameter is not used here as CR is built for both
	// RBDC and VBDC
	if(!this->dama_agent->buildSAC(access_dama_cra,
	                               sac,
	                               empty))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u: DAMA cannot build CR\n",
		    this->super_frame_counter);
		goto error;
	}
	// Set the ACM parameters
	if(this->with_phy_layer)
	{
		sac->setAcm(this->cni);
	}

	if(empty)
	{
		LOG(this->log_send, LEVEL_DEBUG,
		    "SF#%u: Empty CR\n",
		    this->super_frame_counter);
		// keep going as we can send ACM parameters
	}

	// Send message
	if(!this->sendDvbFrame((DvbFrame *)sac,
	                       this->carrier_id_ctrl))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "SF#%u: failed to send SAC\n",
		    this->super_frame_counter);
		delete sac;
		goto error;
	}

	LOG(this->log_send, LEVEL_INFO,
	    "SF#%u: SAC sent\n", this->super_frame_counter);

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleStartOfFrame(DvbFrame *dvb_frame)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	LOG(this->log_frame_tick, LEVEL_DEBUG,
	    "SOF reception SFN #%u super frame nb counter %u\n",
	    sfn, this->super_frame_counter);
	LOG(this->log_frame_tick, LEVEL_DEBUG,
	    "superframe number: %u", sfn);

	// if the NCC crashed, we must reinitiate a logon
	if(sfn < this->super_frame_counter &&
	   (sfn != 0 || (this->super_frame_counter + 1) % 65536 != 0))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: it seems NCC rebooted => flush buffer & "
		    "resend a logon request\n",
		    this->super_frame_counter);

		this->deletePackets();
		if(!this->sendLogonReq())
		{
			goto error;
		}

		this->state = state_wait_logon_resp;
		this->super_frame_counter = sfn;
		goto error;
	}

	// update the frame numerotation
	this->super_frame_counter = sfn;

	// Inform dama agent
	if(this->dama_agent && !this->dama_agent->hereIsSOF(sfn))
	{
		goto error;
	}

	// There is a risk of unprecise timing so the following hack

	LOG(this->log_frame_tick, LEVEL_INFO,
	    "SF#%u: all frames from previous SF are "
	    "consumed or it is the first frame\n",
	    this->super_frame_counter);


	// we have consumed all of our frames, we start a new one immediately
	// this is the first frame of the new superframe
	if(this->processOnFrameTick() < 0)
	{
		// exit because the bloc is unable to continue
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "SF#%u: treatments failed\n",
		    this->super_frame_counter);
		goto error;
	}

	if(this->saloha)
	{
		// Slotted Aloha
		if(!this->saloha->schedule(this->complete_dvb_frames,
		                           this->super_frame_counter))
		{
			LOG(this->log_saloha, LEVEL_ERROR,
			    "SF#%u: failed to process Slotted Aloha frame tick\n",
			     this->super_frame_counter);
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::processOnFrameTick(void)
{
	this->updateStats();

	LOG(this->log_frame_tick, LEVEL_INFO,
	    "SF#%u: start processOnFrameTick\n",
	    this->super_frame_counter);

	if(this->dama_agent)
	{
		// ---------- tell the DAMA agent that a new frame begins ----------
		// Inform dama agent, and update total Available Allocation
		// for current frame
		if(!this->dama_agent->processOnFrameTick())
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "SF#%u: failed to process frame tick\n",
			    this->super_frame_counter);
			goto error;
		}

		// ---------- schedule and send data frames ---------
		// schedule packets extracted from DVB FIFOs according to
		// the algorithm defined in DAMA agent
		if(!this->dama_agent->returnSchedule(&this->complete_dvb_frames))
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "SF#%u: failed to schedule packets from DVB "
			    "FIFOs\n", this->super_frame_counter);
			goto error;
		}
	}

	// send on the emulated DVB network the DVB frames that contain
	// the encapsulation packets scheduled by the DAMA agent algorithm
	if(!this->sendBursts(&this->complete_dvb_frames,
	                     this->carrier_id_data))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "failed to send bursts in DVB frames\n");
		goto error;
	}

	// ---------- SAC ----------
	// compute Capacity Request and send SAC...
	// only if the OBR period has been reached
	if((this->super_frame_counter % this->sync_period_frame) == this->obr_slot_frame)
	{
		if(!this->sendSAC())
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "failed to send SAC\n");
			goto error;
		}
	}

	return true;

error:
	return false;
}


bool BlockDvbTal::Downward::handleLogonResp(DvbFrame *frame)
{
	// TODO static or dynamic_cast
	LogonResponse *logon_resp = (LogonResponse *)frame;
	// Remember the id
	this->group_id = logon_resp->getGroupId();
	this->tal_id = logon_resp->getLogonId();
	
	// Inform Dama agent
	if(this->dama_agent && !this->dama_agent->hereIsLogonResp(logon_resp))
	{
		return false;
	}

	// Set the state to "running"
	this->state = state_running;

	// send the corresponding event
	Output::sendEvent(event_login, "Login complete with MAC %d",
	                  this->mac_id);

	return true;
}


void BlockDvbTal::Downward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}

	if(this->dama_agent)
	{
		this->dama_agent->updateStatistics(this->stats_period_ms);
	}

	mac_fifo_stat_context_t fifo_stat;
	// MAC fifos stats
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->getStatsCxt(fifo_stat);

		this->l2_to_sat_total_bytes += fifo_stat.out_length_bytes;

		// write in statitics file
		this->probe_st_l2_to_sat_before_sched[(*it).first]->put(
			fifo_stat.in_length_bytes * 8 /
			this->stats_period_ms);
		this->probe_st_l2_to_sat_after_sched[(*it).first]->put(
			fifo_stat.out_length_bytes* 8 /
			this->stats_period_ms);

		this->probe_st_queue_size[(*it).first]->put(fifo_stat.current_pkt_nbr);
		this->probe_st_queue_size_kb[(*it).first]->put(
				fifo_stat.current_length_bytes * 8 / 1000);
		this->probe_st_queue_loss[(*it).first]->put(fifo_stat.drop_pkt_nbr);
		this->probe_st_queue_loss_kb[(*it).first]->put(fifo_stat.drop_bytes * 8);
	}
	this->probe_st_l2_to_sat_total->put(this->l2_to_sat_total_bytes * 8 /
	                                    this->stats_period_ms);

	// reset stat 
	this->l2_to_sat_total_bytes = 0;
}


// TODO: move to a dedicated class
/**
 * Signal callback called upon SIGFIFO reception.
 *
 * This function is declared as static.
 *
 * @param sig  The signal that called the function
 */
void BlockDvbTal::Downward::closeQosSocket(int UNUSED(sig))
{
	// TODO static function, no this->
	DFLTLOG(LEVEL_NOTICE,
	        "TCP connection broken, close socket\n");
	close(BlockDvbTal::Downward::Downward::qos_server_sock);
	BlockDvbTal::Downward::qos_server_sock = -1;
}


// TODO: move to a dedicated class
/**
 * Try to connect to the QoS Server
 *
 * The qos_server_host and qos_server_port class variables must be correctly
 * initialized. The qos_server_sock variable should be -1 when calling this
 * function.
 *
 * @return   true if connection is successful, false otherwise
 */
bool BlockDvbTal::Downward::connectToQoSServer()
{
	struct addrinfo hints;
	struct protoent *tcp_proto;
	struct servent *serv;
	struct addrinfo *addresses;
	struct addrinfo *address;
	char straddr[INET6_ADDRSTRLEN];
	int ret;

	if(BlockDvbTal::Downward::qos_server_sock != -1)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "already connected to QoS Server, do not call this "
		    "function when already connected\n");
		goto skip;
	}

	// set criterias to resolve hostname
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// get TCP protocol number
	tcp_proto = getprotobyname("TCP");
	if(tcp_proto == NULL)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "TCP is not available on the system\n");
		goto error;
	}
	hints.ai_protocol = tcp_proto->p_proto;

	// get service name
	serv = getservbyport(htons(this->qos_server_port), "tcp");
	if(serv == NULL)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "service on TCP/%d is not available\n",
		    this->qos_server_port);
		goto error;
	}

	// resolve hostname
	ret = getaddrinfo(this->qos_server_host.c_str(), serv->s_name, &hints, &addresses);
	if(ret != 0)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "cannot resolve hostname '%s': %s (%d)\n",
		    this->qos_server_host.c_str(),
		    gai_strerror(ret), ret);
		goto error;
	}

	// try to create socket with available addresses
	address = addresses;
	while(address != NULL && BlockDvbTal::Downward::qos_server_sock == -1)
	{
		bool is_ipv4;
		void *sin_addr;
		const char *retptr;

		is_ipv4 = (address->ai_family == AF_INET);
		if(is_ipv4)
			sin_addr = &((struct sockaddr_in *) address->ai_addr)->sin_addr;
		else // ipv6
			sin_addr = &((struct sockaddr_in6 *) address->ai_addr)->sin6_addr;

		retptr = inet_ntop(address->ai_family,
		                   sin_addr,
		                   straddr,
		                   sizeof(straddr));
		if(retptr != NULL)
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "try IPv%d address %s\n",
			    is_ipv4 ? 4 : 6, straddr);
		}
		else
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "try an IPv%d address\n",
			    is_ipv4 ? 4 : 6);
		}

		BlockDvbTal::Downward::qos_server_sock = socket(address->ai_family,
		                                      address->ai_socktype,
		                                      address->ai_protocol);
		if(BlockDvbTal::Downward::qos_server_sock == -1)
		{
			LOG(this->log_qos_server, LEVEL_INFO,
			    "cannot create socket (%s) with address %s\n",
			    strerror(errno), straddr);
			address = address->ai_next;
			continue;
		}

		LOG(this->log_qos_server, LEVEL_INFO,
		    "socket created for address %s\n",
		    straddr);
	}

	if(BlockDvbTal::Downward::qos_server_sock == -1)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "no valid address found for hostname %s\n",
		    this->qos_server_host.c_str());
		goto free_dns;
	}

	LOG(this->log_qos_server, LEVEL_INFO,
	    "try to connect with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// try to connect with the socket
	ret = connect(BlockDvbTal::Downward::qos_server_sock,
	              address->ai_addr, address->ai_addrlen);
	if(ret == -1)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "connect() failed: %s (%d)\n",
		    strerror(errno), errno);
		LOG(this->log_qos_server, LEVEL_INFO,
		    "will retry to connect later\n");
		goto close_socket;
	}

	LOG(this->log_qos_server, LEVEL_NOTICE,
	    "connected with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// clean allocated addresses
	freeaddrinfo(addresses);

skip:
	return true;

close_socket:
	close(BlockDvbTal::Downward::qos_server_sock);
	BlockDvbTal::Downward::qos_server_sock = -1;
free_dns:
	freeaddrinfo(addresses);
error:
	return false;
}

void BlockDvbTal::Downward::deletePackets()
{
	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		(*it).second->flush();
	}
}

/*****************************************************************************/
/*                               Upward                                      */
/*****************************************************************************/

BlockDvbTal::Upward::Upward(Block *const bl, tal_id_t mac_id):
	DvbUpward(bl),
	mac_id(mac_id),
	group_id(),
	tal_id(),
	state(state_initializing),
	probe_st_l2_from_sat(NULL),
	probe_st_real_modcod(NULL),
	probe_st_received_modcod(NULL),
	probe_st_rejected_modcod(NULL),
	probe_sof_interval(NULL)
	
{
}


bool BlockDvbTal::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
		case evt_message:
		{
			DvbFrame *dvb_frame = (DvbFrame *)((MessageEvent *)event)->getData();

			if(this->probe_sof_interval->isEnabled() &&
			   dvb_frame->getMessageType() == MSG_TYPE_SOF)
			{
				struct timeval time = event->getTimeFromCustom();
				float val = time.tv_sec * 1000000L + time.tv_usec;
				event->setCustomTime();
				this->probe_sof_interval->put(val/1000);
			}

			// message from lower layer: DL dvb frame
			LOG(this->log_receive, LEVEL_DEBUG,
			    "SF#%u DVB frame received (len %u)\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageLength());

			if(!this->onRcvDvbFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "SF#%u: failed to handle received DVB frame\n",
				    this->super_frame_counter);
				// a problem occured, trace is made in onRcvDVBFrame()
				// carry on simulation
				return false;
			}
		}
		break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown event received %s",
			    this->super_frame_counter,
			    event->getName().c_str());
			return false;
	}

	return true;
}


bool BlockDvbTal::Upward::onInit(void)
{
	// get the common parameters
	if(!this->initCommon(FORWARD_DOWN_ENCAP_SCHEME_LIST))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation");
		return false;
	}

	if(!this->initMode())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output");
		return false;
	}

	// we synchornize with SoF reception so use the return frame duration here
	this->initStatsTimer(this->ret_up_frame_duration_ms);

	return true;
}

// TODO remove receptionStd as functions are merged but contains part
//      dedicated to each host ?
bool BlockDvbTal::Upward::initMode(void)
{
	this->receptionStd = new DvbS2Std(this->pkt_hdl);
	if(this->receptionStd == NULL)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize reception standard\n");
		goto error;
	}

	return true;

error:
	return false;
}

bool BlockDvbTal::Upward::initOutput(void)
{
	if(!this->with_phy_layer)
	{
		// maximum modcod if physical layer is enabled => not useful
		this->probe_st_real_modcod = Output::registerProbe<int>("ACM.Required_modcod",
		                                                        "modcod index",
		                                                        true, SAMPLE_LAST);


	}
	this->probe_st_received_modcod = Output::registerProbe<int>("ACM.Received_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_st_rejected_modcod = Output::registerProbe<int>("ACM.Rejected_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_sof_interval = Output::registerProbe<float>("Perf.SOF_interval",
	                                                        "ms", true,
	                                                        SAMPLE_LAST);

	this->probe_st_l2_from_sat =
		Output::registerProbe<int>("Throughputs.L2_from_SAT",
		                           "Kbits/s", true, SAMPLE_AVG);
	return true;
}


bool BlockDvbTal::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		case MSG_TYPE_CORRUPTED:
		{
			NetBurst *burst = NULL;
			DvbS2Std *std = (DvbS2Std *)this->receptionStd;

			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getMessageLength();
			this->l2_from_sat_bytes -= sizeof(T_DVB_HDR);

			if(this->with_phy_layer)
			{
				DvbFrame *frame_copy = new DvbFrame(dvb_frame);
				if(!this->shareFrame(frame_copy))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit Frame to opposite channel\n");
				}
			}

			if(!this->receptionStd->onRcvFrame(dvb_frame,
			                                   this->tal_id,
			                                   &burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle the reception of "
				    "BB frame (len = %u)\n",
				    dvb_frame->getMessageLength());
				goto error;
			}
			if(msg_type != MSG_TYPE_CORRUPTED)
			{
				// update MODCOD probes
				if(!this->with_phy_layer)
				{
					this->probe_st_real_modcod->put(std->getRealModcod());
				}
				this->probe_st_received_modcod->put(std->getReceivedModcod());
			}
			else
			{
				this->probe_st_rejected_modcod->put(std->getReceivedModcod());
			}

			// send the message to the upper layer
			if(burst && !this->enqueueMessage((void **)&burst))
			{
				LOG(this->log_send, LEVEL_ERROR, 
				    "failed to send burst of packets to upper layer\n");
				delete burst;
				goto error;
			}
			LOG(this->log_send, LEVEL_INFO, 
			    "burst sent to the upper layer\n");
			break;
		}

		// Start of frame (SOF):
		// treat only if state is running --> otherwise just ignore (other
		// STs can be logged)
		case MSG_TYPE_SOF:
			this->updateStats();
			// get superframe number
			if(!this->onStartOfFrame(dvb_frame))
			{
				delete dvb_frame;
				goto error;
			}
			// continue here
		case MSG_TYPE_TTP:
			const char *state_descr;

			if(this->state == state_running)
				state_descr = "state_running";
			else if(this->state == state_initializing)
				state_descr = "state_initializing";
			else
				state_descr = "other";

			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: received SOF or TTP in state %s\n",
			    this->super_frame_counter, state_descr);

			if(this->state == state_running)
			{
				if(!this->shareFrame(dvb_frame))
				{
					LOG(this->log_receive, LEVEL_ERROR,
					    "Unable to transmit TTP to opposite channel\n");
					goto error;
				}
			}
			else
			{
				delete dvb_frame;
			}
			break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->onRcvLogonResp(dvb_frame))
			{
				goto error;
			}
			break;

		// messages sent by current or another ST for the NCC --> ignore
		case MSG_TYPE_SAC:
		case MSG_TYPE_SESSION_LOGON_REQ:
			delete dvb_frame;
			break;

		case MSG_TYPE_SALOHA_CTRL:
			if(!this->shareFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Unable to transmit Slotted Aloha Control frame "
				    "to opposite channel\n");
				goto error;
			}
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			delete dvb_frame;
			goto error;
	}
	return true;

error:
	LOG(this->log_receive, LEVEL_ERROR,
	    "Treatments failed at SF#%u",
	    this->super_frame_counter);
	return false;
}


bool BlockDvbTal::Upward::shareFrame(DvbFrame *frame)
{
	if(!this->shareMessage((void **)&frame, sizeof(frame), msg_sig))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit frame to opposite channel\n");
		delete frame;
		return false;
	}
	return true;
}


bool BlockDvbTal::Upward::onStartOfFrame(DvbFrame *dvb_frame)
{
	uint16_t sfn; // the superframe number piggybacked by SOF packet
	// TODO Sof *sof = dynamic_cast<Sof *>(dvb_frame);
	Sof *sof = (Sof *)dvb_frame;

	sfn = sof->getSuperFrameNumber();

	// update the frame numerotation
	this->super_frame_counter = sfn;

	return true;
}


bool BlockDvbTal::Upward::onRcvLogonResp(DvbFrame *dvb_frame)
{
	T_LINK_UP *link_is_up;
	// TODO LogonResponse *logon_resp = dynamic_cast<LogonResponse *>(dvb_frame);
	LogonResponse *logon_resp = (LogonResponse *)(dvb_frame);
	// Retrieve the Logon Response frame
	if(logon_resp->getMac() != this->mac_id)
	{
		LOG(this->log_receive, LEVEL_INFO,
		    "SF#%u Loggon_resp for mac=%d, not %d\n",
		    this->super_frame_counter, logon_resp->getMac(),
		    this->mac_id);
		delete dvb_frame;
		goto ok;
	}

	// Remember the id
	this->group_id = logon_resp->getGroupId();
	this->tal_id = logon_resp->getLogonId();

	if(!this->shareFrame(dvb_frame))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "Unable to transmit LogonResponse to opposite channel\n");
	}

	// Send a link is up message to upper layer
	// link_is_up
	link_is_up = new T_LINK_UP;
	if(link_is_up == 0)
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u Memory allocation error on link_is_up\n",
		    this->super_frame_counter);
		goto error;
	}
	link_is_up->group_id = this->group_id;
	link_is_up->tal_id = this->tal_id;

	if(!this->enqueueMessage((void **)(&link_is_up),
	                         sizeof(T_LINK_UP),
	                         msg_link_up))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u: failed to send link up message to upper layer",
		    this->super_frame_counter);
		delete link_is_up;
		goto error;
	}
	LOG(this->log_receive, LEVEL_DEBUG,
	    "SF#%u Link is up msg sent to upper layer\n",
	    this->super_frame_counter);

	// Set the state to "running"
	this->state = state_running;
	LOG(this->log_receive, LEVEL_NOTICE,
	    "SF#%u: logon succeeded, running as group %u and logon"
	    " %u\n", this->super_frame_counter,
	    this->group_id, this->tal_id);

 ok:
	return true;
 error:
	 // do not delete here, this will be done by opposite channel
	return false;
}



void BlockDvbTal::Upward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}

	this->probe_st_l2_from_sat->put(
		this->l2_from_sat_bytes * 8 / this->stats_period_ms);

	// send all probes
	// in upward because this block has less events to handle => more time
	Output::sendProbes();

	// reset stat context for next frame
	this->resetStatsCxt();
}

void BlockDvbTal::Upward::resetStatsCxt(void)
{
	this->l2_from_sat_bytes = 0;
}




