/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 */


#include <limits>

#include "BlockDvbTal.h"

#include "DamaAgentRcs2Legacy.h"
#include "TerminalCategoryDama.h"
#include "ScpcScheduling.h"
#include "SlottedAlohaPacketData.h"

#include "DvbRcsStd.h"
#include "DvbS2Std.h"
#include "Ttp.h"
#include "Sof.h"

#include "UnitConverterFixedSymbolLength.h"
#include "OpenSandModelConf.h"

#include <opensand_rt/Rt.h>
#include <opensand_rt/MessageEvent.h>
#include <opensand_output/Output.h>
#include <opensand_output/OutputEvent.h>

#include <sstream>
#include <assert.h>
#include <unistd.h>
#include <signal.h>


int BlockDvbTal::Downward::qos_server_sock = -1;


template<typename T>
bool releaseMap(T& container, bool isError)
{
	for(auto&& item : container)
	{
		delete item.second;
	}
	container.clear();

	return !isError;
}


const char* stateDescription(TalState state)
{
	switch(state)
	{
		case TalState::running:
			return "state_running";
		case TalState::initializing:
			return "state_initializing";
		default:
			return "other";
	}
}


/*****************************************************************************/
/*                                Block                                      */
/*****************************************************************************/


BlockDvbTal::BlockDvbTal(const std::string &name, struct dvb_specific specific):
	BlockDvb{name},
	disable_control_plane{specific.disable_control_plane},
	input_sts{nullptr},
	output_sts{nullptr}
{
}

BlockDvbTal::~BlockDvbTal()
{
	delete this->input_sts;
	delete this->output_sts;
}

void BlockDvbTal::generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane)
{
	auto Conf = OpenSandModelConf::Get();
	auto types = Conf->getModelTypesDefinition();

	{ // Access section when control plane is enabled
		auto access = Conf->getOrCreateComponent("access", "Access", "MAC layer configuration");
		Conf->setProfileReference(access, disable_ctrl_plane, false);

		types->addEnumType("st_fifo_access_type", "Access Type", {"DAMA_RBDC", "DAMA_VBDC", "DAMA_CRA", "SALOHA"});
		// TODO: Keep in sync with topology
		types->addEnumType("carrier_group", "Carrier Group", {"Standard", "Premium", "Professional", "SVNO1", "SVNO2", "SVNO3", "SNO"});
		types->addEnumType("dama_algorithm", "DAMA Agent Algorithm", {"Legacy"});

		auto settings = access->addComponent("settings", "Settings");
		settings->addParameter("category", "Category", types->getType("carrier_group"));

		auto dama_enabled = settings->addParameter("dama_enabled", "Enable DAMA", types->getType("bool"));
		auto dama = access->addComponent("dama", "DAMA");
		Conf->setProfileReference(dama, dama_enabled, true);
		dama->addParameter("cra", "CRA", types->getType("int"))->setUnit("kb/s");
		auto enabled = dama->addParameter("rbdc_enabled", "Enable RBDC", types->getType("bool"));
		auto rbdc = dama->addParameter("rbdc_max", "Max RBDC", types->getType("int"));
		rbdc->setUnit("kb/s");
		Conf->setProfileReference(rbdc, enabled, true);
		enabled = dama->addParameter("vbdc_enabled", "Enable VBDC", types->getType("bool"));
		Conf->setProfileReference(enabled, dama_enabled, true);
		auto vbdc = dama->addParameter("vbdc_max", "Max VBDC", types->getType("int"));
		vbdc->setUnit("kb/sync period");
		Conf->setProfileReference(vbdc, enabled, true);
		dama->addParameter("algorithm", "DAMA Agent Algorithm", types->getType("dama_algorithm"));
		dama->addParameter("duration", "MSL Duration", types->getType("int"))->setUnit("frames");

		SlottedAlohaTal::generateConfiguration();

		auto scpc_enabled = settings->addParameter("scpc_enabled", "Enabled SCPC", types->getType("bool"));
		auto scpc = access->addComponent("scpc", "SCPC");
		Conf->setProfileReference(scpc, scpc_enabled, true);
		scpc->addParameter("carrier_duration", "SCPC Carrier Duration", types->getType("int"))->setUnit("ms");
	}
	auto network = Conf->getOrCreateComponent("network", "Network", "The DVB layer configuration");
	auto fifos = network->getOrCreateList("st_fifos", "FIFOs to send messages to Gateway", "st_fifo");
	auto pattern = fifos->getPattern();
	pattern->getOrCreateParameter("priority", "Priority", types->getType("int"));
	pattern->getOrCreateParameter("name", "Name", types->getType("string"));
	pattern->getOrCreateParameter("capacity", "Capacity", types->getType("int"))->setUnit("packets");
	pattern->getOrCreateParameter("access_type", "Access Type", types->getType("st_fifo_access_type"));

	{ // Access section when control plane is disabled
		auto access = Conf->getOrCreateComponent("access2", "Access", "MAC layer configuration");
		Conf->setProfileReference(access, disable_ctrl_plane, true);
		auto scpc = access->addComponent("scpc", "SCPC");
		scpc->addParameter("carrier_duration", "SCPC Carrier Duration", types->getType("int"))->setUnit("ms");
	}
}

bool BlockDvbTal::onInit(void)
{
	if(!this->initListsSts())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize the lists of Sts\n");
		return false;
	}

	return true;
}

bool BlockDvbTal::initListsSts()
{
	this->input_sts = new StFmtSimuList("in");
	if(this->input_sts == nullptr)
	{
		return false;
	}

	// no output except for SCPC because it is directly handled
	// in Dama Agent (this->modcod_id)
	static_cast<Upward*>(this->upward)->setInputSts(this->input_sts);
	static_cast<Downward*>(this->downward)->setInputSts(this->input_sts);

	bool is_scpc = this->disable_control_plane;;
	if (!is_scpc)
	{
		auto access = OpenSandModelConf::Get()->getProfileData()->getComponent("access");
		auto scpc_enabled = access->getComponent("settings")->getParameter("scpc_enabled");
		OpenSandModelConf::extractParameterData(scpc_enabled, is_scpc);
	}

	if(is_scpc)
	{
		this->output_sts = new StFmtSimuList("out");
		if(this->output_sts == nullptr)
		{
			return false;
		}

		static_cast<Upward*>(this->upward)->setOutputSts(this->output_sts);
		static_cast<Downward*>(this->downward)->setOutputSts(this->output_sts);
	}

	return true;
}



/*****************************************************************************/
/*                              Downward                                     */
/*****************************************************************************/

BlockDvbTal::Downward::Downward(const std::string &name, struct dvb_specific specific):
    DvbDownward{name},
    mac_id{specific.mac_id},
    state{TalState::initializing},
    group_id{},
    tal_id{},
    gw_id{specific.spot_id},
    is_scpc{specific.disable_control_plane},
    disable_control_plane{specific.disable_control_plane},
    cra_kbps{0},
    max_rbdc_kbps{0},
    max_vbdc_kb{0},
    dama_agent{nullptr},
    saloha{nullptr},
    scpc_carr_duration_ms{0},
    scpc_timer{-1},
    ret_fmt_groups{},
    scpc_sched{nullptr},
    scpc_frame_counter{0},
    carrier_id_ctrl{},
    carrier_id_logon{},
    carrier_id_data{},
    dvb_fifos{},
    default_fifo_id{0},
    sync_period_frame{std::numeric_limits<decltype(sync_period_frame)>::max()},
    obr_slot_frame{std::numeric_limits<decltype(obr_slot_frame)>::max()},
    complete_dvb_frames{},
    logon_timer{-1},
    qos_server_host{},
    event_login{nullptr},
    log_frame_tick{nullptr},
    log_qos_server{nullptr},
    log_saloha{nullptr},
    probe_st_queue_size{},
    probe_st_queue_size_kb{},
    probe_st_l2_to_sat_before_sched{},
    probe_st_l2_to_sat_after_sched{},
    l2_to_sat_total_bytes{0},
    probe_st_l2_to_sat_total{nullptr},
    probe_st_phy_to_sat{nullptr},
    probe_st_required_modcod{nullptr}
{
}

BlockDvbTal::Downward::~Downward()
{
	delete this->dama_agent;
	delete this->saloha;
	delete this->scpc_sched;

	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for (auto&& group : this->ret_fmt_groups)
	{
		delete group.second;
	}

	// delete fifos
	releaseMap(this->dvb_fifos, false);

	// close QoS Server socket if it was opened
	if(BlockDvbTal::Downward::qos_server_sock != -1)
	{
		close(BlockDvbTal::Downward::qos_server_sock);
	}

	this->complete_dvb_frames.clear();
}


bool BlockDvbTal::Downward::onInit(void)
{
	this->log_qos_server = Output::Get()->registerLog(LEVEL_WARNING,
						   "Dvb.QoSServer");
	this->log_frame_tick = Output::Get()->registerLog(LEVEL_WARNING,
						   "Dvb.DamaAgent.FrameTick");
	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// get the common parameters
	if(!this->initCommon(EncapSchemeList::RETURN_UP))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the initialisation\n");
		return false;
	}
	if(!this->initDown())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the downward common initialisation\n");
		return false;
	}

	if(!this->initCarrierId())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the initialisation\n");
		return false;
	}

	if(!this->initMacFifo())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the MAC FIFO part of the initialisation\n");
		return false;
	}

	// Initialization od fow_modcod_def (useful to send SAC)
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD definition file\n");
		return false;
	}

	if (!this->disable_control_plane)
	{
		auto access = OpenSandModelConf::Get()->getProfileData()->getComponent("access");
		auto scpc_enabled = access->getComponent("settings")->getParameter("scpc_enabled");
		OpenSandModelConf::extractParameterData(scpc_enabled, this->is_scpc);
	}

	if(!this->is_scpc)
	{
		if(!this->initDama())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the DAMA part of the initialisation\n");
			return false;
		}

		if(!this->initSlottedAloha())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the initialisation of Slotted Aloha\n");
			return false;
		}
	}
	else
	{
		if(!this->initScpc())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to complete the SCPC part of the initialisation\n");
			return false;
		}
	}

	if(!this->dama_agent && !this->saloha && !this->scpc_sched)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "unable to instanciate DAMA or Slotted Aloha or SCPC, "
		    "check your configuration\n");
		return false;
	}

	if(!this->disable_control_plane && !this->initQoSServer())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the QoS Server part of the initialisation\n");
		return false;
	}

	this->initStatsTimer(this->dama_agent || this->saloha ? this->ret_up_frame_duration_ms : this->scpc_carr_duration_ms);

	// Init the output here since we now know the FIFOs
	if(!this->initOutput())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of output\n");
		return false;
	}

	if(!this->initTimers())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialization of timers\n");
		return false;
	}

	// now everyhing is initialized so we can do some processing
	if (!this->disable_control_plane)
	{
		// after all of things have been initialized successfully,
		// send a logon request
		LOG(this->log_init, LEVEL_DEBUG,
		    "send a logon request with MAC ID %d to NCC\n",
		    this->mac_id);
		this->state = TalState::wait_logon_resp;
		if(!this->sendLogonReq())
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to send the logon request to the NCC\n");
			return false;
		}
	}
	else
	{
		this->tal_id = this->mac_id;
		this->group_id = this->gw_id;
		this->state = TalState::running;
	}

	return true;
}


bool BlockDvbTal::Downward::initCarrierId(void)
{
	auto Conf = OpenSandModelConf::Get();

	// this->gw_id = 0;
	// if(!Conf->getGwWithTalId(this->mac_id, this->gw_id))
	// {
	// 	LOG(this->log_init_channel, LEVEL_ERROR,
	// 	    "couldn't find gw for tal %d",
	// 	    this->mac_id);
	// 	return false;
	// }

	OpenSandModelConf::spot_infrastructure carriers;
	if (!Conf->getSpotInfrastructure(this->gw_id, carriers))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "couldn't create spot infrastructure for gw %d",
		    this->gw_id);
		return false;
	}

	this->carrier_id_ctrl = carriers.ctrl_in_st.id;
	this->carrier_id_data = carriers.data_in_st.id;
	this->carrier_id_logon = carriers.logon_in.id;

	LOG(this->log_init, LEVEL_NOTICE,
	    "SF#%u: carrier IDs for Ctrl = %u, Logon = %u, "
	    "Data = %u\n", this->super_frame_counter,
	    this->carrier_id_ctrl,
	    this->carrier_id_logon, this->carrier_id_data);

	return true;
}

bool BlockDvbTal::Downward::initMacFifo(void)
{
	auto Conf = OpenSandModelConf::Get();
	auto network = Conf->getProfileData()->getComponent("network");

	for (auto& item : network->getList("st_fifos")->getItems())
	{
		auto fifo_item = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		int fifo_prio;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("priority"), fifo_prio))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo priority from section 'network, fifos'\n");
			return releaseMap(this->dvb_fifos, true);
		}
		qos_t fifo_priority = fifo_prio;

		std::string fifo_name;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("name"), fifo_name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo name from section 'network, fifos'\n");
			return releaseMap(this->dvb_fifos, true);
		}

		int fifo_capa;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("capacity"), fifo_capa))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo capacity from section 'network, fifos'\n");
			return releaseMap(this->dvb_fifos, true);
		}
		vol_pkt_t fifo_size = fifo_capa;

		std::string fifo_access_type;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("access_type"), fifo_access_type))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo access type from section 'network, fifos'\n");
			return releaseMap(this->dvb_fifos, true);
		}

		DvbFifo *fifo = new DvbFifo(fifo_priority, fifo_name,
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

		this->dvb_fifos.insert({fifo->getPriority(), fifo});
	}

	this->l2_to_sat_total_bytes = 0;

	return true;
}


bool BlockDvbTal::Downward::initDama(void)
{
	time_ms_t sync_period_ms = 0;
	time_sf_t rbdc_timeout_sf = 0;
	time_sf_t msl_sf = 0;
  std::string dama_algo;
	bool is_dama_fifo = false;

	TerminalCategories<TerminalCategoryDama> dama_categories;
	TerminalMapping<TerminalCategoryDama> terminal_affectation;
	TerminalCategoryDama *default_category;
	TerminalCategoryDama *tal_category{nullptr};
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

	// init
	if(!this->initModcodDefFile(MODCOD_DEF_RCS2,
	                            &this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the up/return MODCOD definition file\n");
		return false;
	}

	// get current spot into return up band section
	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotReturnCarriers(this->gw_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into return up frequency plan\n",
		    this->gw_id);
		return false;
	}

	// init band
	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "return up frequency plan",
	                                         AccessType::DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->rcs_modcod_def,
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

	auto Conf = OpenSandModelConf::Get();
	auto dama = Conf->getProfileData()->getComponent("access")->getComponent("dama");

	// Find the category for this terminal
	tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by DAMA
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a DAMA category\n");
			return releaseMap(dama_categories, false);
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
		return releaseMap(dama_categories, false);
	}

	if(!is_dama_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The DAMA carrier won't be used as there is no DAMA FIFO\n");
		return releaseMap(dama_categories, false);
	}

	OpenSandModelConf::extractParameterData(dama->getParameter("dama_enabled"), is_dama_fifo);
	if(!is_dama_fifo)
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "The DAMA carrier won't be used as requested by the configuration file\n");
		return releaseMap(dama_categories, false);
	}

	//  allocated bandwidth in CRA mode traffic -- in kbits/s
	int cra_kbps;
	if(!OpenSandModelConf::extractParameterData(dama->getParameter("cra"), cra_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section 'access', Missing 'CRA'\n");
		return releaseMap(dama_categories, true);
	}
	this->cra_kbps = cra_kbps;

	LOG(this->log_init, LEVEL_NOTICE,
	    "cra_kbps = %d kbits/s\n", this->cra_kbps);

	// Max RBDC (in kbits/s) and RBDC timeout (in frame number)
	bool rbdc_enabled = false;
	OpenSandModelConf::extractParameterData(dama->getParameter("rbdc_enabled"), rbdc_enabled);

	int max_rbdc_kbps = 0;
	if(rbdc_enabled && !OpenSandModelConf::extractParameterData(
				dama->getParameter("rbdc_max"),
				max_rbdc_kbps))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section 'access', Missing 'max RBDC'\n");
		return releaseMap(dama_categories, true);
	}
	this->max_rbdc_kbps = max_rbdc_kbps;

	// Max VBDC
	bool vbdc_enabled = false;
	OpenSandModelConf::extractParameterData(dama->getParameter("vbdc_enabled"), vbdc_enabled);

	int max_vbdc_kb = 0;
	if(vbdc_enabled && !OpenSandModelConf::extractParameterData(
				dama->getParameter("vbdc_max"),
				max_vbdc_kb))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section 'access', Missing 'max VBDC'\n");
		return releaseMap(dama_categories, true);
	}
	this->max_vbdc_kb = max_vbdc_kb;

	// MSL duration -- in frames number
	int duration;
	if(!OpenSandModelConf::extractParameterData(dama->getParameter("duration"), duration))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section 'access', Missing 'MSL duration'\n");
		return releaseMap(dama_categories, true);
	}
	msl_sf = duration;

	// get the OBR period
	if(!Conf->getSynchroPeriod(sync_period_ms))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Missing 'sync period'\n");
		return releaseMap(dama_categories, true);
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
	    "VBDC max %d kbits, mslDuration %d frame\n",
	    this->cra_kbps, this->max_rbdc_kbps,
	    rbdc_timeout_sf, this->max_vbdc_kb, msl_sf);

	// dama algorithm
	if(!OpenSandModelConf::extractParameterData(dama->getParameter("algorithm"), dama_algo))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "section 'access': missing parameter 'dama algorithm'\n");
		return releaseMap(dama_categories, true);
	}

	if(dama_algo == "Legacy")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create Legacy DAMA agent\n",
		    this->super_frame_counter);

		this->dama_agent = new DamaAgentRcs2Legacy(this->rcs_modcod_def);
	}
	/*else if(dama_algo == "RrmQos")
	{
		LOG(this->log_init, LEVEL_NOTICE,
		    "SF#%u: create RrmQos DAMA agent\n",
		    this->super_frame_counter);

		if(this->return_link_std == DVB_RCS)
		{
			this->dama_agent = new DamaAgentRcsRrmQos(this->rcs_modcod_def);
		}
		else
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "cannot create DAMA agent: algo named '%s' is not "
			    "managed by current MAC layer\n", dama_algo.c_str());
			return releaseMap(dama_categories, true);
		}
	}*/
	else
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot create DAMA agent: algo named '%s' is not "
		    "managed by current MAC layer\n", dama_algo.c_str());
		return releaseMap(dama_categories, true);
	}

	if(this->dama_agent == nullptr)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to create DAMA agent\n");
		return releaseMap(dama_categories, true);
	}

	// Initialize the DamaAgent parent class
	if(!this->dama_agent->initParent(this->ret_up_frame_duration_ms,
					 this->cra_kbps,
					 this->max_rbdc_kbps,
					 rbdc_timeout_sf,
					 this->max_vbdc_kb,
					 msl_sf,
					 this->sync_period_frame,
					 this->pkt_hdl,
					 this->dvb_fifos))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "SF#%u Dama Agent Initialization failed.\n",
		    this->super_frame_counter);
		delete this->dama_agent;
		return releaseMap(dama_categories, true);
	}

	// Initialize the DamaAgentRcsXXX class
	if(!this->dama_agent->init())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Dama Agent initialization failed.\n");
		delete this->dama_agent;
		return releaseMap(dama_categories, true);
	}

	return releaseMap(dama_categories, false);
}

bool BlockDvbTal::Downward::initSlottedAloha(void)
{
	bool is_sa_fifo = false;
	auto Conf = OpenSandModelConf::Get();

	TerminalCategories<TerminalCategorySaloha> sa_categories;
	TerminalMapping<TerminalCategorySaloha> terminal_affectation;
	TerminalCategorySaloha *default_category;
	TerminalCategorySaloha *tal_category{nullptr};
	TerminalMapping<TerminalCategorySaloha>::const_iterator tal_map_it;
	TerminalCategories<TerminalCategorySaloha>::iterator cat_it;
	UnitConverter *converter{nullptr};
	vol_sym_t length_sym = 0;

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		if((*it).second->getAccessType() == access_saloha)
		{
			is_sa_fifo = true;
		}
	}

	// get current spot into return up band section
	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotReturnCarriers(this->gw_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into return up frequency plan\n",
		    this->gw_id);
		return false;
	}

	if(!this->initBand<TerminalCategorySaloha>(current_spot,
	                                           "return up frequency plan",
	                                           AccessType::ALOHA,
	                                           this->ret_up_frame_duration_ms,
	                                           // initialized in DAMA
	                                           this->rcs_modcod_def,
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

	// TODO should manage several Saloha carrier
	for(cat_it = sa_categories.begin();
	    cat_it != sa_categories.end(); ++cat_it)
	{
		if((*cat_it).second->getCarriersGroups().size() > 1)
		{
			LOG(this->log_init, LEVEL_WARNING,
			    "If you use more than one Slotted Aloha carrier group "
			    "with different parameters, the behaviour won't be correct "
			    "for time division and MODCOD support.\n");
			break;
		}
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

	auto saloha_section = Conf->getProfileData()->getComponent("access")->getComponent("random_access");
	bool is_sa_enabled = false;
	OpenSandModelConf::extractParameterData(saloha_section->getParameter("ra_enabled"), is_sa_enabled);

	if(!(is_sa_fifo && is_sa_enabled))
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
		    "Slotted Aloha Tal Initialization failed.\n");
		delete this->saloha;
		return false;
	}

	if(!OpenSandModelConf::Get()->getRcs2BurstLength(length_sym))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "cannot get 'burst length' value");
		delete this->saloha;
		return false;
	}
	converter = new UnitConverterFixedSymbolLength(this->ret_up_frame_duration_ms,
	                                               0,
	                                               length_sym);

	if(!this->saloha->init(this->mac_id,
	                       tal_category,
	                       this->dvb_fifos,
	                       converter))
	{
		delete converter;
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the Slotted Aloha Tal\n");
		delete this->saloha;
		return false;
	}

	delete converter;
	return true;
}


bool BlockDvbTal::Downward::initScpc(void)
{
	bool success = false;

	//  Duration of the carrier -- in ms
	auto access = OpenSandModelConf::Get()->getProfileData()->getComponent(disable_control_plane ? "access2" : "access");
	auto duration = access->getComponent("scpc")->getParameter("carrier_duration");
	int scpc_carrier_duration;
	if(!OpenSandModelConf::extractParameterData(duration, scpc_carrier_duration))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Section 'access', Missing 'SCPC carrier duration'\n");
		return false;
	}
	this->scpc_carr_duration_ms = scpc_carrier_duration;

	LOG(this->log_init, LEVEL_NOTICE,
	    "scpc_carr_duration_ms = %d ms\n", this->scpc_carr_duration_ms);

	// get current spot into return up band section
	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotReturnCarriers(this->gw_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
		    "%d into return up frequency plan\n",
		    this->gw_id);
		return false;
	}

	TerminalCategories<TerminalCategoryDama> scpc_categories{};
	TerminalMapping<TerminalCategoryDama> terminal_affectation{};
	TerminalCategoryDama *default_category{nullptr};
	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "return up frequency plan",
	                                         AccessType::SCPC,
	                                         this->scpc_carr_duration_ms,
	                                         // input modcod for S2
	                                         this->s2_modcod_def,
	                                         scpc_categories,
	                                         terminal_affectation,
	                                         &default_category,
	                                         this->ret_fmt_groups))
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "InitBand not correctly initialized \n");

		return false;
	}

	if(!scpc_categories.size())
	{
		LOG(this->log_init, LEVEL_WARNING,
		    "No SCPC carriers\n");
		// no SCPC: return
		return false;
	}

	// Find the category for this terminal
	TerminalCategoryDama *tal_category{nullptr};
	auto tal_map_it = terminal_affectation.find(this->mac_id);
	if(tal_map_it == terminal_affectation.end())
	{
		// check if the default category is concerned by SCPC
		if(!default_category)
		{
			LOG(this->log_init, LEVEL_INFO,
			    "ST not affected to a SCPC category\n");
			goto error;
		}
		tal_category = default_category;
	}
	else
	{
		tal_category = tal_map_it->second;
	}

	// check if there are SCPC carriers
	if(!tal_category)
	{
		LOG(this->log_init, LEVEL_INFO,
		    "No SCPC carrier\n");
		LOG(this->log_init, LEVEL_ERROR,
		    "Remove SCPC FIFOs because there is no "
		    "SCPC carrier in the return_up_band configuration\n");
		goto error;
	}

	// Check if there are DAMA or SALOHA FIFOs in the terminal
	if(this->dama_agent || this->saloha)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Conflict: SCPC FIFOs and DAMA or SALOHA FIFOs "
		    "in the same Terminal\n");
		goto error;
	}

	//TODO: veritfy that 2ST are not using the same carrier and category

	// Initialise Encapsulation scheme
	if(!this->initScpcPktHdl(&this->pkt_hdl))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed get packet handler\n");
		goto error;
	}

	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the return MODCOD definition file for SCPC\n");
		goto error;
	}

	// register GW
	if(!this->addOutputTerminal(this->gw_id, this->s2_modcod_def))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "failed to register simulated ST with MAC "
		    "ID %u\n", this->tal_id);
		goto error;
	}

	// Create the SCPC scheduler
	this->scpc_sched = new ScpcScheduling(this->scpc_carr_duration_ms,
	                                      this->pkt_hdl,
	                                      this->dvb_fifos,
	                                      this->output_sts,
	                                      this->s2_modcod_def,
	                                      scpc_categories.begin()->second,
	                                      this->gw_id);
	if(!this->scpc_sched)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize SCPC\n");
		goto error;
	}
	scpc_categories.begin()->second = nullptr;
	success = true;

error:
	terminal_affectation.clear();
	for (auto&& category : scpc_categories)
	{
		delete category.second;
	}
	return success;
}


bool BlockDvbTal::Downward::initQoSServer(void)
{
	if(!OpenSandModelConf::Get()->getQosServerHost(this->qos_server_host, this->qos_server_port))
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "section entity, is missing QoS server informations\n");
		return false;
	}
	if(this->qos_server_port <= 1024 || this->qos_server_port > 0xffff)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "QoS Server port (%d) not valid\n",
		    this->qos_server_port);
		return false;
	}

	// QoS Server: catch the SIGFIFO signal that is sent to the process
	// when QoS Server kills the TCP connection
	if(signal(SIGPIPE, BlockDvbTal::Downward::closeQosSocket) == SIG_ERR)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "cannot catch signal SIGPIPE\n");
		return false;
	}

	// QoS Server: try to connect to remote host
	this->connectToQoSServer();

	return true;
}

bool BlockDvbTal::Downward::initOutput(void)
{
	auto output = Output::Get();

	this->event_login = output->registerEvent("DVB.login");

	if(this->saloha)
	{
		this->log_saloha = output->registerLog(LEVEL_WARNING, "Dvb.SlottedAloha");
	}

	for(fifos_t::iterator it = this->dvb_fifos.begin();
	    it != this->dvb_fifos.end(); ++it)
	{
		unsigned int id = (*it).first;

		this->probe_st_queue_size[id] =
			output->registerProbe<int>("Queue size.packets." + ((*it).second)->getName(),
						   "Packets", true, SAMPLE_LAST);
		this->probe_st_queue_size_kb[id] =
			output->registerProbe<int>("Queue size.capacity." + ((*it).second)->getName(),
						   "kbits", true, SAMPLE_LAST);

		this->probe_st_l2_to_sat_before_sched[id] =
			output->registerProbe<int>("Throughputs.L2_to_SAT_before_sched." +
						   ((*it).second)->getName(), "Kbits/s", true,
																 SAMPLE_AVG);
		this->probe_st_l2_to_sat_after_sched[id] =
			output->registerProbe<int>("Throughputs.L2_to_SAT_after_sched." +
						   ((*it).second)->getName(), "Kbits/s", true,
																 SAMPLE_AVG);
		this->probe_st_queue_loss[id] =
			output->registerProbe<int>("Queue loss.packets." + ((*it).second)->getName(),
						   "Packets", true, SAMPLE_LAST);
		this->probe_st_queue_loss_kb[id] =
			output->registerProbe<int>("Queue loss.capacity." + ((*it).second)->getName(),
						   "kbits", true, SAMPLE_LAST);
	}
	this->probe_st_l2_to_sat_total =
		output->registerProbe<int>("Throughputs.L2_to_SAT_after_sched.total",
					   "Kbits/s", true, SAMPLE_AVG);

	this->probe_st_required_modcod = output->registerProbe<int>("Down_Forward_modcod.Required_modcod",
								    "modcod index",
								    true, SAMPLE_LAST);
	return true;
}


bool BlockDvbTal::Downward::initTimers(void)
{
	if (!this->disable_control_plane)
	{
		this->logon_timer = this->addTimerEvent("logon", 5000,
		                                        false, // do not rearm
		                                        false // do not start
		                                        );
		// QoS Server: check connection status in 5 seconds
		this->qos_server_timer = this->addTimerEvent("qos_server", 5000);
	}

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
    case EventType::Message:
		{
	    auto msg_event = static_cast<const MessageEvent*>(event);
      InternalMessageType msg_type = to_enum<InternalMessageType>(msg_event->getMessageType());

			// first handle specific messages
			if(msg_type == InternalMessageType::msg_sig)
			{
				return this->handleDvbFrame(static_cast<DvbFrame*>(msg_event->getData()));
			}

			// TODO move saloha handling in a specific function ?
			// Slotted Aloha variables
	    auto burst = static_cast<NetBurst*>(msg_event->getData());
			unsigned int sa_burst_size = burst->length(); // burst size
			unsigned int sa_offset = 0; // packet position (offset) in the burst

			sa_burst_size = burst->length();
			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: encapsulation burst received (%d "
			    "packets)\n", this->super_frame_counter,
			    sa_burst_size);


			// set each packet of the burst in MAC FIFO
			for (auto&& packet : *burst)
			{
				qos_t fifo_priority = packet->getQos();

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
				if (this->saloha && this->dvb_fifos[fifo_priority]->getAccessType() == access_saloha)
				{
					packet = this->saloha->addSalohaHeader(std::move(packet),
					                                       sa_offset++,
					                                       sa_burst_size);
					if(!packet)
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
				if(!this->onRcvEncapPacket(std::move(packet),
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
					delete burst;
					return false;
				}

			}
			delete burst;

			// Cross layer information: if connected to QoS Server, build XML
			// message and send it
			// TODO move in a dedicated class
			if(BlockDvbTal::Downward::qos_server_sock == -1)
			{
				break;
			}

			// messages from upper layer: burst of encapsulation packets
			std::ostringstream oss{};
			oss << "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n"
			       "<XMLQoSMessage>\n"
			       " <Sender>CrossLayer</Sender>\n"
			       " <Type type=\"CrossLayer\" >\n"
			       "  <Infos ";
			for (auto const& it : this->dvb_fifos)
			{
				int nbFreeFrames = it.second->getMaxSize() - it.second->getCurrentSize();
				// bits
				int nbFreeBits = nbFreeFrames * this->pkt_hdl->getFixedLength() * 8;
				// bits/ms or kbits/s
				float macRate = nbFreeBits / this->ret_up_frame_duration_ms ;
				oss << "File=\"" << int(macRate) << "\" ";
			}
			oss << "/>\n"
			       " </Type>\n"
			       "</XMLQoSMessage>\n";

			std::string message{oss.str()};
			if(0 > write(BlockDvbTal::Downward::qos_server_sock,
			             message.c_str(),
			             message.length()))
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "failed to send message to QoS Server: %s "
				    "(%d)\n", strerror(errno), errno);
			}
		}
		break;

    case EventType::Timer:
		{
			if(*event == this->logon_timer)
			{
				if(this->state == TalState::wait_logon_resp)
				{
					// send another logon_req and raise timer
					// only if we are in the good state
					LOG(this->log_receive, LEVEL_NOTICE,
					    "still no answer from NCC to the "
					    "logon request we sent for MAC ID %d, "
					    "send a new logon request\n",
					    this->mac_id);
					return this->sendLogonReq();
				}
				return true;
			}
			if(this->state != TalState::running)
			{
				LOG(this->log_receive, LEVEL_DEBUG,
				    "Ignore timer event %s while not logged\n",
				    event->getName().c_str());
				return true;
			}

			if(*event == this->qos_server_timer)
			{
				// try to re-connect to QoS Server if not already connected
				if(BlockDvbTal::Downward::qos_server_sock == -1)
				{
					if(!this->connectToQoSServer())
					{
						LOG(this->log_receive, LEVEL_INFO,
						    "failed to connect with QoS Server, "
						    "cannot send cross layer informationi\n");
					}
				}
			}
			else if(*event == this->scpc_timer)
			{
				// TODO fct ++ add extension dans GSE
				uint32_t remaining_alloc_sym = 0;

				this->updateStats();
				this->scpc_frame_counter++;

				if(!this->addCniExt())
				{
					LOG(this->log_send_channel, LEVEL_ERROR,
					    "fail to add CNI extension");
					return false;
				}

				//Schedule Creation
				// TODO we should send packets containing CNI extension with
				//      the most robust MODCOD
				if(!this->scpc_sched->schedule(this->scpc_frame_counter,
				                               getCurrentTime(),
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
				    "SF#%u: unknown timer event received %s\n",
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

bool BlockDvbTal::Downward::addCniExt(void)
{
	bool in_fifo = false;

	// Create list of first packet from FIFOs
	for(fifos_t::iterator fifos_it = this->dvb_fifos.begin();
	    fifos_it != this->dvb_fifos.end(); ++fifos_it)
	{
		DvbFifo *fifo = (*fifos_it).second;
    std::vector<MacFifoElement *> queue = fifo->getQueue();
    std::vector<MacFifoElement *>::iterator queue_it;

		for(queue_it = queue.begin() ;
		    queue_it != queue.end()  ;
		    ++queue_it)
		{
			MacFifoElement* elem = (*queue_it);
			NetPacket *packet = (NetPacket*)elem->getElem();
			tal_id_t gw = packet->getDstTalId();

			if(gw == this->gw_id &&
			   this->is_scpc && this->getCniInputHasChanged(this->tal_id))
			{
				if(!this->setPacketExtension(this->pkt_hdl,
				                             elem, fifo,
				                             packet,
				                             this->tal_id, gw,
				                             "encodeCniExt",
				                             this->super_frame_counter,
				                             false))
				{
					return false;
				}

				LOG(this->log_send_channel, LEVEL_DEBUG,
				    "SF #%d: packet belongs to FIFO #%d\n",
				    this->super_frame_counter, (*fifos_it).first);
				// Delete old packet
				delete packet;
				in_fifo = true;
			}
		}
	}

	if(this->is_scpc && this->getCniInputHasChanged(this->tal_id)
	   && !in_fifo)
	{

		// set packet extension to this new empty packet
		if(!this->setPacketExtension(this->pkt_hdl,
		                             nullptr,
		                             this->dvb_fifos[0],
		                             nullptr,
		                             this->tal_id ,this->gw_id,
		                             "encodeCniExt",
		                             this->super_frame_counter,
		                             false))
		{
			return false;
		}

		LOG(this->log_send_channel, LEVEL_DEBUG,
		    "SF #%d: adding empty packet into FIFO NM\n",
		    this->super_frame_counter);
	}

	return true;
}

bool BlockDvbTal::Downward::sendLogonReq(void)
{
	LogonRequest *logon_req = new LogonRequest(this->mac_id,
	                                           this->cra_kbps,
	                                           this->max_rbdc_kbps,
	                                           this->max_vbdc_kb,
	                                           this->is_scpc);

	// send the message to the lower layer
	if(!this->sendDvbFrame((DvbFrame *)logon_req,
	                       this->carrier_id_logon))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "Failed to send Logon Request\n");
		return false;
	}
	LOG(this->log_send, LEVEL_DEBUG,
	    "SF#%u Logon Req. sent to lower layer\n",
	    this->super_frame_counter);

	if(!this->startTimer(this->logon_timer))
	{
		LOG(this->log_send, LEVEL_ERROR,
		    "cannot start logon timer");
		return false;
	}

	// send the corresponding event
	event_login->sendEvent("Login sent to GW");
	return true;
}


bool BlockDvbTal::Downward::handleDvbFrame(DvbFrame *dvb_frame)
{
	// frames transmitted from Upward
	if (this->disable_control_plane)
	{
		delete dvb_frame;
	  return true;
	}

	switch(dvb_frame->getMessageType())
	{
		case MSG_TYPE_SALOHA_CTRL:
			if(this->saloha && !this->saloha->onRcvFrame(dvb_frame))
			{
				LOG(this->log_saloha, LEVEL_ERROR,
				    "failed to handle Slotted Aloha Signal Controls frame\n");
				LOG(this->log_receive, LEVEL_ERROR,
				    "Treatments failed at SF#%u\n",
				    this->super_frame_counter);
				return false;
			}
			break;

		case MSG_TYPE_SOF:
			if(!this->handleStartOfFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle SoF\n");
				LOG(this->log_receive, LEVEL_ERROR,
				    "Treatments failed at SF#%u\n",
				    this->super_frame_counter);
				delete dvb_frame;
				return false;
			}
			delete dvb_frame;
			break;

		case MSG_TYPE_TTP:
		{
			Ttp *ttp = reinterpret_cast<Ttp *>(dvb_frame);
			if(this->dama_agent && !this->dama_agent->hereIsTTP(ttp))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "TTP Treatments failed at SF#%u\n",
				    this->super_frame_counter);
				delete dvb_frame;
				return false;
			}
			delete dvb_frame;
		}
		break;

		case MSG_TYPE_SESSION_LOGON_RESP:
			if(!this->handleLogonResp(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Cannot handle logon response\n");
				LOG(this->log_receive, LEVEL_ERROR,
				    "Treatments failed at SF#%u\n",
				    this->super_frame_counter);
				delete dvb_frame;
				return false;
			}
			delete dvb_frame;
			break;

		default:
			LOG(this->log_receive, LEVEL_ERROR,
			    "SF#%u: unknown type of DVB frame (%u), ignore\n",
			    this->super_frame_counter,
			    dvb_frame->getMessageType());
			LOG(this->log_receive, LEVEL_ERROR,
			    "Treatments failed at SF#%u\n",
			    this->super_frame_counter);
			delete dvb_frame;
			return false;
	}

	return true;
}


bool BlockDvbTal::Downward::sendSAC(void)
{
	bool empty;
	Sac *sac;
	double cni;

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
	cni = this->getRequiredCniInput(this->tal_id);
	sac->setAcm(cni);

	this->probe_st_required_modcod->put(this->getCurrentModcodIdInput(this->tal_id));

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
	    "superframe number: %u\n", sfn);

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

		this->state = TalState::wait_logon_resp;
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
	if(!this->processOnFrameTick())
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
			return false;
		}

		// ---------- schedule and send data frames ---------
		// schedule packets extracted from DVB FIFOs according to
		// the algorithm defined in DAMA agent
		if(!this->dama_agent->returnSchedule(&this->complete_dvb_frames))
		{
			LOG(this->log_frame_tick, LEVEL_ERROR,
			    "SF#%u: failed to schedule packets from DVB "
			    "FIFOs\n", this->super_frame_counter);
			return false;
		}
	}

	// send on the emulated DVB network the DVB frames that contain
	// the encapsulation packets scheduled by the DAMA agent algorithm
	if(!this->sendBursts(&this->complete_dvb_frames,
			     this->carrier_id_data))
	{
		LOG(this->log_frame_tick, LEVEL_ERROR,
		    "failed to send bursts in DVB frames\n");
		return false;
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
			return false;
		}
	}

	return true;
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
	this->state = TalState::running;

	// send the corresponding event
	event_login->sendEvent("Login complete with MAC %d", this->mac_id);

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
			fifo_stat.out_length_bytes * 8 /
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
	close(BlockDvbTal::Downward::qos_server_sock);
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
		return true;
	}

	// set criterias to resolve hostname
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// get TCP protocol number
	tcp_proto = getprotobyname("TCP");
	if(tcp_proto == nullptr)
	{
		LOG(this->log_qos_server, LEVEL_ERROR,
		    "TCP is not available on the system\n");
		return false;
	}
	hints.ai_protocol = tcp_proto->p_proto;

	// get service name
	serv = getservbyport(htons(this->qos_server_port), "tcp");
	if(serv == nullptr)
	{
		LOG(this->log_qos_server, LEVEL_INFO,
		    "service on TCP/%d is not available\n",
		    this->qos_server_port);
		return false;
	}

	// resolve hostname
	ret = getaddrinfo(this->qos_server_host.c_str(), serv->s_name, &hints, &addresses);
	if(ret != 0)
	{
		LOG(this->log_qos_server, LEVEL_NOTICE,
		    "cannot resolve hostname '%s': %s (%d)\n",
		    this->qos_server_host.c_str(),
		    gai_strerror(ret), ret);
		return false;
	}

	// try to create socket with available addresses
	address = addresses;
	while(address != nullptr && BlockDvbTal::Downward::qos_server_sock == -1)
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
		if(retptr != nullptr)
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
		freeaddrinfo(addresses);
		return false;
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
		close(BlockDvbTal::Downward::qos_server_sock);
		BlockDvbTal::Downward::qos_server_sock = -1;
		freeaddrinfo(addresses);
		return false;
	}

	LOG(this->log_qos_server, LEVEL_NOTICE,
	    "connected with QoS Server at %s[%s]:%d\n",
	    this->qos_server_host.c_str(), straddr,
	    this->qos_server_port);

	// clean allocated addresses
	freeaddrinfo(addresses);
	return true;
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

BlockDvbTal::Upward::Upward(const std::string &name, struct dvb_specific specific):
    DvbUpward{name, specific.disable_control_plane},
    reception_std{nullptr},
    mac_id{specific.mac_id},
    group_id{},
    tal_id{},
    gw_id{specific.spot_id},
    is_scpc{specific.disable_control_plane},
    state{TalState::initializing},
    probe_st_l2_from_sat{nullptr},
    probe_st_received_modcod{nullptr},
    probe_st_rejected_modcod{nullptr},
    probe_sof_interval{nullptr}
{
}

BlockDvbTal::Upward::~Upward()
{
	// release the reception DVB standards
	delete this->reception_std;
}


bool BlockDvbTal::Upward::onEvent(const RtEvent *const event)
{
	switch(event->getType())
	{
    case EventType::Message:
		{
	    DvbFrame *dvb_frame = static_cast<DvbFrame*>(static_cast<const MessageEvent*>(event)->getData());

			if(this->probe_sof_interval->isEnabled() &&
			   dvb_frame->getMessageType() == MSG_TYPE_SOF)
			{
				time_val_t time = event->getAndSetCustomTime();
				this->probe_sof_interval->put(time/1000.f);
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
	// Initialization of gw_id
	auto Conf = OpenSandModelConf::Get();
	// if(!Conf->getGwWithTalId(this->mac_id, this->gw_id))
	// {
	// 	LOG(this->log_init_channel, LEVEL_ERROR,
	// 	    "couldn't find gw for tal %d",
	// 	    this->mac_id);
	// 	return false;
	// }

	if (!this->disable_control_plane)
	{
		auto access = Conf->getProfileData()->getComponent("access");
		auto scpc_enabled = access->getComponent("settings")->getParameter("scpc_enabled");
		OpenSandModelConf::extractParameterData(scpc_enabled, this->is_scpc);
	}

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// get the common parameters
	if(!this->initCommon(EncapSchemeList::FORWARD_DOWN))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the common part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initModcodSimu())
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to complete the initialisation of the Modcod Simu\n");
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
		    "failed to complete the initialisation of output\n");
		return false;
	}

	// we synchornize with SoF reception so use the return frame duration here
	this->initStatsTimer(this->ret_up_frame_duration_ms);

	if (this->disable_control_plane)
	{
		this->tal_id = this->mac_id;
		this->group_id = this->gw_id;
		this->state = TalState::running;
	}

	return true;
}

// TODO remove reception_std as functions are merged but contains part
//      dedicated to each host ?
bool BlockDvbTal::Upward::initMode(void)
{
	auto reception_std{new DvbS2Std(this->pkt_hdl)};
	if (reception_std == nullptr)
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "Failed to initialize reception standard\n");
		return false;
	}

	reception_std->setModcodDef(this->s2_modcod_def);
	this->reception_std = reception_std;
	return true;
}


bool BlockDvbTal::Upward::initModcodSimu(void)
{
	// tal_id_t gw_id = 0;
	// if(!OpenSandModelConf::Get()->getGwWithTalId(this->mac_id, gw_id))
	// {
	// 	LOG(this->log_init_channel, LEVEL_ERROR,
	// 	    "couldn't find gw for tal %d",
	// 	    this->mac_id);
	// 	return false;
	// }

	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init, LEVEL_ERROR,
		    "failed to initialize the down/forward MODCOD definition file\n");
		return false;
	}

	if(this->is_scpc)
	{
		if(!this->initModcodDefFile(MODCOD_DEF_S2,
		                            &this->s2_modcod_def))
		{
			LOG(this->log_init, LEVEL_ERROR,
			    "failed to initialize the up/return MODCOD definition file\n");
			return false;
		}
	}

	return true;
}


bool BlockDvbTal::Upward::initOutput(void)
{
	auto output = Output::Get();

	this->probe_st_received_modcod = output->registerProbe<int>("Down_Forward_modcod.Received_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_st_rejected_modcod = output->registerProbe<int>("Down_Forward_modcod.Rejected_modcod",
	                                                            "modcod index",
	                                                            true, SAMPLE_LAST);
	this->probe_sof_interval = output->registerProbe<float>("Perf.SOF_interval",
	                                                        "ms", true,
	                                                        SAMPLE_LAST);

	this->probe_st_l2_from_sat =
		output->registerProbe<int>("Throughputs.L2_from_SAT.total",
		                           "Kbits/s", true, SAMPLE_AVG);
	this->l2_from_sat_bytes = 0;
	return true;
}


bool BlockDvbTal::Upward::onRcvDvbFrame(DvbFrame *dvb_frame)
{
	uint8_t msg_type = dvb_frame->getMessageType();
	bool corrupted = dvb_frame->isCorrupted();

	LOG(this->log_receive, LEVEL_INFO,
			    "Receive a frame of type %d\n", dvb_frame->getMessageType());

	// get ACM parameters that will be transmited to GW in SAC  TODO check it
	if(IS_CN_CAPABLE_FRAME(msg_type) && this->state == TalState::running)
	{
		double cni = dvb_frame->getCn();
		LOG(this->log_receive, LEVEL_INFO,
				    "Read a C/N of %f for packet of type %d\n",
				    cni, dvb_frame->getMessageType());
		this->setRequiredCniInput(this->tal_id, cni);
	}

	switch(msg_type)
	{
		case MSG_TYPE_BBFRAME:
		{
			if(this->state != TalState::running)
			{
				LOG(this->log_receive, LEVEL_NOTICE,
				    "Ignore received BBFrames while not logged\n");
				delete dvb_frame;
				return true;
			}

			NetBurst *burst{nullptr};
			DvbS2Std *std = (DvbS2Std *)this->reception_std;

			// Update stats
			this->l2_from_sat_bytes += dvb_frame->getMessageLength();
			this->l2_from_sat_bytes -= sizeof(T_DVB_HDR);

			// Set the real modcod of the ST
			std->setRealModcod(this->getCurrentModcodIdInput(this->tal_id));

			if(!std->onRcvFrame(dvb_frame,
			                    this->tal_id,
			                    &burst))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "failed to handle the reception of "
				    "BB frame (len = %u)\n",
				    dvb_frame->getMessageLength());
				goto error;
			}

			NetBurst::const_iterator it;
			if(burst)
			{
				for(auto&& packet : *burst)
				{
					if(packet->getDstTalId() == this->tal_id && this->is_scpc)
					{
						uint32_t opaque = 0;
						if(!this->pkt_hdl->getHeaderExtensions(packet,
						                                       "deencodeCniExt",
						                                       &opaque))
						{
							LOG(this->log_receive, LEVEL_ERROR,
							    "error when trying to read header extensions\n");
							goto error;
						}
						if(opaque != 0)
						{
							// This is the C/N0 value evaluated by the GW and transmitted
							// via GSE extensions
							this->setRequiredCniOutput(this->gw_id, ncntoh(opaque));
							break;
						}
					}
				}
			}

			if(!corrupted)
			{
				// update MODCOD probes
				this->probe_st_received_modcod->put(std->getReceivedModcod());
				this->probe_st_rejected_modcod->put(0);
			}
			else
			{
				this->probe_st_rejected_modcod->put(std->getReceivedModcod());
				this->probe_st_received_modcod->put(0);
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
				LOG(this->log_receive, LEVEL_ERROR,
				    "on start of frame failed\n");
				goto error;
			}
			// fall through
		case MSG_TYPE_TTP:
			LOG(this->log_receive, LEVEL_INFO,
			    "SF#%u: received SOF or TTP in state %s\n",
			    this->super_frame_counter, stateDescription(this->state));

			if(this->state == TalState::running)
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
				LOG(this->log_receive, LEVEL_ERROR,
				    "on receive logon resp failed\n");
				goto error;
			}
			break;

		// messages sent by current or another ST for the NCC --> ignore
		case MSG_TYPE_SAC:
		case MSG_TYPE_SESSION_LOGON_REQ:
			if (!this->disable_control_plane)
			{
				delete dvb_frame;
				break;
			}
			// else fallthrough

		case MSG_TYPE_SALOHA_CTRL:
			if(!this->shareFrame(dvb_frame))
			{
				LOG(this->log_receive, LEVEL_ERROR,
				    "Unable to transmit Control frame "
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


bool BlockDvbTal::Upward::onStartOfFrame(DvbFrame *dvb_frame)
{
	// update the frame numerotation
	this->super_frame_counter = reinterpret_cast<Sof *>(dvb_frame)->getSuperFrameNumber();

	return true;
}


bool BlockDvbTal::Upward::onRcvLogonResp(DvbFrame *dvb_frame)
{
	T_LINK_UP *link_is_up;
	LogonResponse *logon_resp = (LogonResponse *)(dvb_frame);
	// Retrieve the Logon Response frame
	if(logon_resp->getMac() != this->mac_id)
	{
		LOG(this->log_receive, LEVEL_INFO,
		    "SF#%u Loggon_resp for mac=%d, not %d\n",
		    this->super_frame_counter, logon_resp->getMac(),
		    this->mac_id);
		delete dvb_frame;
		return true;
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
		return false;
	}
	link_is_up->group_id = this->group_id;
	link_is_up->tal_id = this->tal_id;

	if(!this->enqueueMessage((void **)(&link_is_up),
	                         sizeof(T_LINK_UP),
	                         to_underlying(InternalMessageType::msg_link_up)))
	{
		LOG(this->log_receive, LEVEL_ERROR,
		    "SF#%u: failed to send link up message to upper layer",
		    this->super_frame_counter);
		delete link_is_up;
		return false;
	}
	LOG(this->log_receive, LEVEL_DEBUG,
	    "SF#%u Link is up msg sent to upper layer\n",
	    this->super_frame_counter);

	// Set the state to "running"
	this->state = TalState::running;
	LOG(this->log_receive, LEVEL_NOTICE,
	    "SF#%u: logon succeeded, running as group %u and logon"
	    " %u\n", this->super_frame_counter,
	    this->group_id, this->tal_id);

  // TODO: Should we disable it when disable_control_plane???
	// Add the st id
	if(!this->addInputTerminal(this->tal_id, this->s2_modcod_def))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle FMT for ST %u, "
		    "won't send logon response\n", this->tal_id);
		return false;
	}

	return true;
}


void BlockDvbTal::Upward::updateStats(void)
{
	if(!this->doSendStats())
	{
		return;
	}

	this->probe_st_l2_from_sat->put(
		this->l2_from_sat_bytes * 8 / this->stats_period_ms);
	this->l2_from_sat_bytes = 0;
	// send all probes
	// in upward because this block has less events to handle => more time
	Output::Get()->sendProbes();

	// reset stat context for next frame
}
