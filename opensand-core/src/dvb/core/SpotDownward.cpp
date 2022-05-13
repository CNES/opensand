/*
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
 * @file SpotDownward.cpp
 * @brief Downward spot related functions for DVB NCC block
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#include "SpotDownward.h"

#include "ForwardSchedulingS2.h"
#include "DamaCtrlRcs2Legacy.h"
#include "DamaCtrlRcs2.h"
#include "Scheduling.h"
#include "SlottedAlohaNcc.h"
#include "SvnoRequest.h"
#include "FileSimulator.h"
#include "RandomSimulator.h"
#include "OpenSandModelConf.h"


SpotDownward::SpotDownward(spot_id_t spot_id,
                           tal_id_t mac_id,
                           time_ms_t fwd_down_frame_duration,
                           time_ms_t ret_up_frame_duration,
                           time_ms_t stats_period,
                           EncapPlugin::EncapPacketHandler *pkt_hdl,
                           StFmtSimuList *input_sts,
                           StFmtSimuList *output_sts):
	DvbChannel(),
	DvbFmt(),
	dama_ctrl(NULL),
	scheduling(),
	fwd_frame_counter(0),
	ctrl_carrier_id(),
	sof_carrier_id(),
	data_carrier_id(),
	spot_id(spot_id),
	mac_id(mac_id),
	is_tal_scpc(),
	dvb_fifos(),
	default_fifo_id(0),
	complete_dvb_frames(),
	categories(),
	terminal_affectation(),
	default_category(NULL),
	up_return_pkt_hdl(NULL),
	fwd_fmt_groups(),
	ret_fmt_groups(),
	cni(100),
	pep_cmd_apply_timer(),
	request_simu(NULL),
	event_file(NULL),
	simulate(none_simu),
	probe_gw_queue_size(NULL),
	probe_gw_queue_size_kb(NULL),
	probe_gw_queue_loss(NULL),
	probe_gw_queue_loss_kb(NULL),
	probe_gw_l2_to_sat_before_sched(NULL),
	probe_gw_l2_to_sat_after_sched(NULL),
	probe_gw_l2_to_sat_total(),
	l2_to_sat_total_bytes(),
	probe_frame_interval(NULL),
	probe_sent_modcod(NULL),
	log_request_simulation(NULL),
	event_logon_resp(NULL)
{
	this->fwd_down_frame_duration_ms = fwd_down_frame_duration;
	this->ret_up_frame_duration_ms = ret_up_frame_duration;
	this->stats_period_ms = stats_period;
	this->pkt_hdl = pkt_hdl;
	this->input_sts = input_sts;
	this->output_sts = output_sts;

	this->log_request_simulation = Output::Get()->registerLog(LEVEL_WARNING,
	                                                          "Spot_%d.Dvb.RequestSimulation",
	                                                          this->spot_id);

}

SpotDownward::~SpotDownward()
{
	this->categories.clear();

	delete this->dama_ctrl;

	for (auto& it : this->scheduling)
	{
		delete it.second;
	}
	this->scheduling.clear();

	this->complete_dvb_frames.clear();

	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for (auto& it : this->fwd_fmt_groups)
	{
		delete it.second;
	}
	// delete FMT groups here because they may be present in many carriers
	// TODO do something to avoid groups here
	for (auto& it : this->ret_fmt_groups)
	{
		delete it.second;
	}

	// delete fifos
	for (auto& it1 : this->dvb_fifos)
	{
		for (auto& it2 : it1.second)
		{
			delete it2.second;
		}
		it1.second.clear();
	}
	this->dvb_fifos.clear();

	this->terminal_affectation.clear();

	// delete probes
	delete this->probe_gw_queue_size;
	delete this->probe_gw_queue_size_kb;
	delete this->probe_gw_queue_loss;
	delete this->probe_gw_queue_loss_kb;
	delete this->probe_gw_l2_to_sat_before_sched;
	delete this->probe_gw_l2_to_sat_after_sched;
}


void SpotDownward::generateConfiguration()
{
	RequestSimulator::generateConfiguration();

	auto Conf = OpenSandModelConf::Get();

	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("ncc_simulation", "Simulated Requests", {"None", "Random", "File"});
	types->addEnumType("fifo_access_type", "Access Type", {"ACM", "VCM0", "VCM1", "VCM2", "VCM3"});
	types->addEnumType("dama_algorithm", "DAMA Algorithm", {"Legacy",});

	auto conf = Conf->getOrCreateComponent("network", "Network", "The DVB layer configuration");
	auto fifos = conf->addList("fifos", "FIFOs", "fifo")->getPattern();
	fifos->addParameter("priority", "Priority", types->getType("int"));
	fifos->addParameter("name", "Name", types->getType("string"));
	fifos->addParameter("capacity", "Capacity", types->getType("int"))->setUnit("packets");
	fifos->addParameter("access_type", "Access Type", types->getType("fifo_access_type"));
	auto simulation = conf->addParameter("simulation",
	                                     "Simulated Requests",
	                                     types->getType("ncc_simulation"),
	                                     "Should OpenSAND simulate extraneous requests?");
	auto parameter = conf->addParameter("simulation_file",
	                                    "Simulation Trace File",
	                                    types->getType("string"),
	                                    "Path to a file containing requests traces; or stdin");
	Conf->setProfileReference(parameter, simulation, "File");

	parameter = conf->addParameter("simulation_nb_station",
	                               "Simulated Station ID",
	                               types->getType("int"),
	                               "Numbered > 31");
	Conf->setProfileReference(parameter, simulation, "Random");
	parameter = conf->addParameter("simulation_rt_bandwidth",
	                               "RT Bandwidth",
	                               types->getType("int"));
	parameter->setUnit("kbps");
	Conf->setProfileReference(parameter, simulation, "Random");
	parameter = conf->addParameter("simulation_max_rbdc",
	                               "Simulated Maximal RBDC",
	                               types->getType("int"));
	parameter->setUnit("kbps");
	Conf->setProfileReference(parameter, simulation, "Random");
	parameter = conf->addParameter("simulation_max_vbdc",
	                               "Simulated Maximal VBDC",
	                               types->getType("int"));
	parameter->setUnit("kb");
	Conf->setProfileReference(parameter, simulation, "Random");
	parameter = conf->addParameter("simulation_mean_requests",
	                               "Simulated Mean Requests",
	                               types->getType("int"));
	parameter->setUnit("kbps");
	Conf->setProfileReference(parameter, simulation, "Random");
	parameter = conf->addParameter("simulation_amplitude_request",
	                               "Simulated Amplitude Request",
	                               types->getType("int"));
	parameter->setUnit("kbps");
	Conf->setProfileReference(parameter, simulation, "Random");

	conf->addParameter("fca", "FCA", types->getType("int"));
	conf->addParameter("dama_algorithm", "DAMA Algorithm", types->getType("dama_algorithm"));
}


bool SpotDownward::onInit(void)
{
	if(!this->initPktHdl(RETURN_UP_ENCAP_SCHEME_LIST,
	                     &this->up_return_pkt_hdl))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed get packet handler\n");
		return false;
	}

	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// Initialization of the modcod def
	if(!this->initModcodDefFile(MODCOD_DEF_S2,
	                            &this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward link definition MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS2,
	                            &this->rcs_modcod_def,
	                            this->req_burst_length))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the return link definition MODCOD file\n");
		return false;
	}

	// Get the carrier Ids
	if(!this->initCarrierIds())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the carrier IDs part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initMode())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the mode part of the "
		    "initialisation\n");
		return false;
	}

	this->initStatsTimer(this->fwd_down_frame_duration_ms);

	if(!this->initRequestSimulation())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the request simulation part of "
		    "the initialisation\n");
		return false;
	}

	// get and launch the dama algorithm
	if(!this->initDama())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the DAMA part of the "
		    "initialisation\n");
		return false;
	}

	if(!this->initOutput())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to complete the initialization of "
		    "statistics\n");
		return false;
	}

	// everything went fine
	return true;
}


bool SpotDownward::initMode(void)
{
	TerminalCategoryDama *cat;
	TerminalCategories<TerminalCategoryDama>::iterator cat_it;

	// initialize scheduling
	// depending on the satellite type
	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotForwardCarriers(this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into forward down frequency plan\n",
		    this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "forward down frequency plan",
	                                         TDM,
	                                         this->fwd_down_frame_duration_ms,
	                                         this->s2_modcod_def,
	                                         this->categories,
	                                         this->terminal_affectation,
	                                         &this->default_category,
	                                         this->fwd_fmt_groups))
	{
		return false;
	}


	// check that there is at least DVB fifos for VCM carriers
	for(cat_it = this->categories.begin();
	    cat_it != this->categories.end(); ++cat_it)
	{
		bool is_vcm_carriers = false;
		bool is_acm_carriers = false;
		bool is_vcm_fifo = false;
		fifos_t fifos;
		std::string label;
		Scheduling *schedule;

		cat = (*cat_it).second;
		label = cat->getLabel();
		if(!this->initFifo(fifos))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed initialize fifos for category %s\n", label.c_str());
			return false;
		}
		this->dvb_fifos.insert(pair<string, fifos_t>(label, fifos));

		// check if there is VCM carriers in this category
		vector<CarriersGroupDama *>::iterator carrier_it;
		vector<CarriersGroupDama *> carriers_group;
		carriers_group = (*cat_it).second->getCarriersGroups();
		for(carrier_it = carriers_group.begin();
		    carrier_it != carriers_group.end();
		    ++carrier_it)
		{
			vector<CarriersGroupDama *> vcm_carriers;
			vector<CarriersGroupDama *>::iterator vcm_it;
			vcm_carriers = (*carrier_it)->getVcmCarriers();
			if(vcm_carriers.size() > 1)
			{
				is_vcm_carriers = true;
			}
			else
			{
				is_acm_carriers = true;
			}
		}

		for(fifos_t::iterator it = fifos.begin();
		    it != fifos.end(); ++it)
		{
			if((*it).second->getAccessType() == access_vcm)
			{
				is_vcm_fifo = true;
				break;
			}
		}
		if(is_vcm_carriers && !is_vcm_fifo)
		{
			if(!is_acm_carriers)
			{
				LOG(this->log_init_channel, LEVEL_CRITICAL,
				    "There is VCM carriers in category %s but no VCM FIFOs, "
				    "as there is no other carriers, "
				    "terminals in this category "
				    "won't be able to send any trafic. "
				    "Please check your configuration",
				    (*cat_it).second->getLabel().c_str());
				return false;
			}
			else
			{
				LOG(this->log_init_channel, LEVEL_WARNING,
				    "There is VCM carriers in category %s but no VCM FIFOs, "
				    "the VCM carriers won't be used",
				    (*cat_it).second->getLabel().c_str());
			}
		}

		schedule =  new ForwardSchedulingS2(this->fwd_down_frame_duration_ms,
		                                    this->pkt_hdl,
		                                    this->dvb_fifos.at(label),
		                                    this->output_sts,
		                                    this->s2_modcod_def,
		                                    cat, this->spot_id,
		                                    true, this->mac_id, "");
		if(!schedule)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed initialize forward scheduling for category %s\n", label.c_str());
			return false;
		}
		this->scheduling.emplace(label, schedule);
	}

	return true;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownward::initDama(void)
{
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	rate_kbps_t fca_kbps;
	string dama_algo;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	TerminalCategoryDama *dc_default_category = NULL;

	auto Conf = OpenSandModelConf::Get();
	auto ncc = Conf->getProfileData()->getComponent("network");

	// Retrieving the free capacity assignement parameter
	int fca;
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("fca"), fca))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing FCA parameter\n");
		return false;
	}

	fca_kbps = fca;
	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);


	if(!Conf->getSynchroPeriod(sync_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Missing synchronisation period\n");
		return false;
	}
	sync_period_frame = (time_frame_t)round((double)sync_period_ms /
	                                        (double)this->ret_up_frame_duration_ms);
	rbdc_timeout_sf = sync_period_frame + 1;

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "rbdc_timeout = %d superframes computed from sync period %d superframes\n",
	    rbdc_timeout_sf, sync_period_frame);

	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotReturnCarriers(this->mac_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into return up frequency plan\n",
		    this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "return up frequency plan",
	                                         DAMA,
	                                         this->ret_up_frame_duration_ms,
	                                         this->rcs_modcod_def,
	                                         dc_categories,
	                                         dc_terminal_affectation,
	                                         &dc_default_category,
	                                         this->ret_fmt_groups))
	{
		return false;
	}


	// check if there is DAMA carriers
	if(dc_categories.size() == 0)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "No TDM carrier, won't allocate DAMA\n");
		// Also disable request simulation
		this->simulate = none_simu;
		return true;
	}

	// dama algorithm
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("dama_algorithm"), dama_algo))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section 'ncc': missing parameter 'dama_algorithm'\n");
		return false;
	}

	if(!this->up_return_pkt_hdl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "up return pkt hdl has not been initialized first.\n");
		return false;
	}

	/* select the specified DAMA algorithm */
	if(dama_algo == "Legacy")
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "creating Legacy DAMA controller\n");
		this->dama_ctrl = new DamaCtrlRcs2Legacy(this->spot_id);
	}
	else
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "section 'ncc': bad value '%s' for "
		    "parameter 'dama_algorithm'\n",
		    dama_algo.c_str());
		return false;
	}

	if(!this->dama_ctrl)
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to create the DAMA controller\n");
		return false;
	}

	// Initialize the DamaCtrl parent class
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration_ms,
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                this->input_sts,
	                                this->rcs_modcod_def,
	                                (this->simulate == none_simu) ?
	                                false : true))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		goto release_dama;
	}

	if(!this->dama_ctrl->init())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		goto release_dama;
	}
	this->dama_ctrl->setRecordFile(this->event_file);

	return true;

release_dama:
	delete this->dama_ctrl;
	return false;
}


bool SpotDownward::initCarrierIds(void)
{
	auto Conf = OpenSandModelConf::Get();

	OpenSandModelConf::spot_infrastructure carriers;
	if (!Conf->getSpotInfrastructure(this->mac_id, carriers)) {
	LOG(this->log_init_channel, LEVEL_ERROR,
	    "couldn't create spot infrastructure for gw %d",
	    this->mac_id);
	return false;
	}

	this->ctrl_carrier_id = carriers.ctrl_in.id;
	this->sof_carrier_id = carriers.ctrl_in.id;
	this->data_carrier_id = carriers.data_in_gw.id;

	return true;
}


bool SpotDownward::initFifo(fifos_t &fifos)
{
	auto Conf = OpenSandModelConf::Get();
	auto ncc = Conf->getProfileData()->getComponent("network");

	for (auto& item : ncc->getList("fifos")->getItems())
	{
		auto fifo_item = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		int fifo_prio;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("priority"), fifo_prio))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo priority from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}
		unsigned int fifo_priority = fifo_prio;

		std::string fifo_name;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("name"), fifo_name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo name from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}

		int fifo_capa;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("capacity"), fifo_capa))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo capacity from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}
		vol_pkt_t fifo_size = fifo_capa;

		std::string fifo_access_type;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("access_type"), fifo_access_type))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo access type from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}

		DvbFifo *fifo = new DvbFifo(fifo_priority, fifo_name, fifo_access_type, fifo_size);

		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "access type %d\n",
		    fifo->getPriority(),
		    fifo->getName().c_str(),
		    fifo->getMaxSize(),
		    fifo->getAccessType());

		// the default FIFO is the last one = the one with the smallest
		// priority actually, the IP plugin should add packets in the
		// default FIFO if the DSCP field is not recognize, default_fifo_id
		// should not be used this is only used if traffic categories
		// configuration and fifo configuration are not coherent.
		this->default_fifo_id = std::max(this->default_fifo_id,
		                                 fifo->getPriority());

		fifos.insert(pair<unsigned int, DvbFifo *>(fifo->getPriority(), fifo));
	}

	return true;

err_fifo_release:
	for(fifos_t::iterator it = fifos.begin();
	    it != fifos.end(); ++it)
	{
		delete (*it).second;
	}
	fifos.clear();
	return false;
}

bool SpotDownward::initRequestSimulation(void)
{
	auto Conf = OpenSandModelConf::Get();
	auto ncc = Conf->getProfileData()->getComponent("network");

	std::string str_config;
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation"), str_config))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "cannot load simulation mode from section ncc\n");
		return false;
	}

	// TODO for stdin use FileEvent for simu_timer ?
	if(str_config == "File")
	{
		std::string simulation_file;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_file"), simulation_file))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulation trace file from section ncc\n");
			return false;
		}

		this->simulate = file_simu;
		this->request_simu = new FileSimulator(this->spot_id,
		                                       this->mac_id,
		                                       &this->event_file,
		                                       simulation_file);
	}
	else if(str_config == "Random")
	{
		int simu_st;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_nb_station"), simu_st))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulated station ID from section ncc\n");
			return false;
		}
		int simu_rt;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_rt_bandwidth"), simu_rt))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulated RT bandwidth from section ncc\n");
			return false;
		}
		int simu_rbdc;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_max_rbdc"), simu_rbdc))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulated maximal RBDC from section ncc\n");
			return false;
		}
		int simu_vbdc;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_max_vbdc"), simu_vbdc))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulated maximal RBDC from section ncc\n");
			return false;
		}
		int simu_cr;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_mean_requests"), simu_cr))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulated mean capacity request from section ncc\n");
			return false;
		}
		int simu_interval;
		if(!OpenSandModelConf::extractParameterData(ncc->getParameter("simulation_amplitude_request"), simu_interval))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot load simulated station ID from section ncc\n");
			return false;
		}

		this->simulate = random_simu;
		this->request_simu = new RandomSimulator(this->spot_id,
		                                         this->mac_id,
		                                         &this->event_file,
		                                         simu_st,
		                                         simu_rt,
		                                         simu_rbdc,
		                                         simu_vbdc,
		                                         simu_cr,
		                                         simu_interval);
	}
	else
	{
		this->simulate = none_simu;
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "no event simulation\n");
	}

	return true;
}

bool SpotDownward::initOutput(void)
{
	auto output = Output::Get();
	// Events
	this->event_logon_resp = output->registerEvent("Spot_%d.DVB.logon_response",
	                                               this->spot_id);

	this->probe_gw_queue_size = new map<string, ProbeListPerId>();
	this->probe_gw_queue_size_kb = new map<string, ProbeListPerId>();
	this->probe_gw_queue_loss = new map<string, ProbeListPerId>();
	this->probe_gw_queue_loss_kb = new map<string, ProbeListPerId>();
	this->probe_gw_l2_to_sat_before_sched = new map<string, ProbeListPerId>();
	this->probe_gw_l2_to_sat_after_sched = new map<string, ProbeListPerId>();
	char probe_name[128];

	for(map<string, fifos_t>::iterator it1 = this->dvb_fifos.begin();
		it1 != this->dvb_fifos.end(); it1++)
	{
		string cat_label = it1->first;
		for(fifos_t::iterator it2 = it1->second.begin();
			it2 != it1->second.end(); ++it2)
		{
			string fifo_name = ((*it2).second)->getName();
			unsigned int id = (*it2).first;

			std::shared_ptr<Probe<int>> probe_temp;

			snprintf(probe_name, sizeof(probe_name),
			         "Spot_%d.%s.Queue size.packets.%s",
			         spot_id, cat_label.c_str(), fifo_name.c_str());
			probe_temp = output->registerProbe<int>(probe_name, "Packets", true, SAMPLE_LAST);
			(*this->probe_gw_queue_size)[cat_label].emplace(id, probe_temp);

			snprintf(probe_name, sizeof(probe_name),
			         "Spot_%d.%s.Queue size.capacity.%s",
			         spot_id, cat_label.c_str(), fifo_name.c_str());
			probe_temp = output->registerProbe<int>(probe_name, "kbits", true, SAMPLE_LAST);
			(*this->probe_gw_queue_size_kb)[cat_label].emplace(id, probe_temp);

			snprintf(probe_name, sizeof(probe_name),
			         "Spot_%d.%s.Throughputs.L2_to_SAT_before_sched.%s",
			         spot_id, cat_label.c_str(), fifo_name.c_str());
			probe_temp = output->registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_AVG);
			(*this->probe_gw_l2_to_sat_before_sched)[cat_label].emplace(id, probe_temp);

			snprintf(probe_name, sizeof(probe_name),
			         "Spot_%d.%s.Throughputs.L2_to_SAT_after_sched.%s",
			         spot_id, cat_label.c_str(), fifo_name.c_str());
			probe_temp = output->registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_AVG);
			(*this->probe_gw_l2_to_sat_after_sched)[cat_label].emplace(id, probe_temp);

			snprintf(probe_name, sizeof(probe_name),
			         "Spot_%d.%s.Queue loss.packets.%s",
			         spot_id, cat_label.c_str(), fifo_name.c_str());
			probe_temp = output->registerProbe<int>(probe_name, "Packets", true, SAMPLE_SUM);
			(*this->probe_gw_queue_loss)[cat_label].emplace(id, probe_temp);

			snprintf(probe_name, sizeof(probe_name),
			         "Spot_%d.%s.Queue loss.rate.%s",
			         spot_id, cat_label.c_str(), fifo_name.c_str());
			probe_temp = output->registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_SUM);
			(*this->probe_gw_queue_loss_kb)[cat_label].emplace(id, probe_temp);
		}
		snprintf(probe_name, sizeof(probe_name),
		         "Spot_%d.%s.Throughputs.L2_to_SAT_after_sched.total",
		         spot_id, cat_label.c_str());
		this->probe_gw_l2_to_sat_total[cat_label] =
			output->registerProbe<int>(probe_name, "Kbits/s", true, SAMPLE_AVG);

	}

	return true;
}


bool SpotDownward::handleSalohaAcks(const std::list<DvbFrame *> *ack_frames)
{
	for (auto&& ack : *ack_frames)
	{
		this->complete_dvb_frames.push_back(ack);
	}
	return true;
}


bool SpotDownward::handleEncapPacket(NetPacket *packet)
{
	qos_t fifo_priority = packet->getQos();
	string cat_label;
	map<string, fifos_t>::iterator fifos_it;
	tal_id_t dst_tal_id;

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: store one encapsulation "
	    "packet\n", this->super_frame_counter);

	dst_tal_id = packet->getDstTalId();
	// category of the packet
	if(this->terminal_affectation.find(dst_tal_id) != this->terminal_affectation.end())
	{
		if(this->terminal_affectation.at(dst_tal_id) == NULL)
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "No category associated to terminal %u, cannot handle packet\n",
			    dst_tal_id);
			return false;
		}
		cat_label = this->terminal_affectation.at(dst_tal_id)->getLabel();
	}
	else
	{
		if(!this->default_category)
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "No default category for terminal %u, cannot handle packet\n",
			    dst_tal_id);
			return false;
		}
		cat_label = this->default_category->getLabel();
	}

	// find the FIFO associated to the IP QoS (= MAC FIFO id)
	// else use the default id
	fifos_it = this->dvb_fifos.find(cat_label);
	if(fifos_it == this->dvb_fifos.end())
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "No fifo found for this category %s", cat_label.c_str());
		return false;
	}
	if(fifos_it->second.find(fifo_priority) == fifos_it->second.end())
	{
		fifo_priority = this->default_fifo_id;
	}

	if(!this->pushInFifo(fifos_it->second[fifo_priority], packet, 0))
	{
		// a problem occured, we got memory allocation error
		// or fifo full and we won't empty fifo until next
		// call to onDownwardEvent => return
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "SF#%u: unable to store received "
		    "encapsulation packet (see previous errors)\n",
		    this->super_frame_counter);
		return false;
	}

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: encapsulation packet is "
	    "successfully stored\n",
	    this->super_frame_counter);

	return true;
}


bool SpotDownward::handleLogonReq(const LogonRequest *logon_req)
{
	uint16_t mac = logon_req->getMac();
	bool is_scpc = logon_req->getIsScpc();
	if(is_scpc)
	{
		this->is_tal_scpc.push_back(mac);
	}

	// Inform the Dama controller (for its own context)
	if(!is_scpc && this->dama_ctrl && 
	   !this->dama_ctrl->hereIsLogon(logon_req))
	{
		return false;
	}

	// send the corresponding event
	event_logon_resp->sendEvent("Logon response send to ST%u on spot %u",
	                            mac, this->spot_id);

	LOG(this->log_send_channel, LEVEL_DEBUG,
	    "SF#%u: logon response sent to lower layer\n",
	    this->super_frame_counter);

	return true;
}


bool SpotDownward::handleLogoffReq(const DvbFrame *dvb_frame)
{
	// TODO	Logoff *logoff = dynamic_cast<Logoff *>(dvb_frame);
	Logoff *logoff = (Logoff *)dvb_frame;

	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->delInputTerminal(logoff->getMac()))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to delete the ST with ID %d from FMT simulation\n",
		    logoff->getMac());
		delete dvb_frame;
		return false;
	}
	if(!this->delOutputTerminal(logoff->getMac()))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to delete the ST with ID %d from FMT simulation\n",
		    logoff->getMac());
		delete dvb_frame;
		return false;
	}

	if(this->dama_ctrl)
	{
		this->dama_ctrl->hereIsLogoff(logoff);
	}
	LOG(this->log_receive_channel, LEVEL_DEBUG,
	    "SF#%u: logoff request from %d\n",
	    this->super_frame_counter, logoff->getMac());

	delete dvb_frame;
	return true;
}



bool SpotDownward::buildTtp(Ttp *ttp)
{
	return this->dama_ctrl->buildTTP(ttp);
}

void SpotDownward::updateStatistics(void)
{
	if(!this->doSendStats())
	{
		return;
	}

	// Update stats on the GW
	if(this->dama_ctrl)
	{
		this->dama_ctrl->updateStatistics(this->stats_period_ms);
	}

	mac_fifo_stat_context_t fifo_stat;
	// MAC fifos stats
	for(map<string, fifos_t>::iterator it1 = this->dvb_fifos.begin();
		it1 != this->dvb_fifos.end(); it1++)
	{
		string cat_label = it1->first;
		for(fifos_t::iterator it2 = it1->second.begin();
		    it2 != it1->second.end(); ++it2)
		{
			(*it2).second->getStatsCxt(fifo_stat);

			this->l2_to_sat_total_bytes[cat_label] += fifo_stat.out_length_bytes;

			(*this->probe_gw_l2_to_sat_before_sched)[cat_label][(*it2).first]->put(
				fifo_stat.in_length_bytes * 8.0 / this->stats_period_ms);

			(*this->probe_gw_l2_to_sat_after_sched)[cat_label][(*it2).first]->put(
				fifo_stat.out_length_bytes * 8.0 / this->stats_period_ms);

			// Mac fifo stats
			(*this->probe_gw_queue_size)[cat_label][(*it2).first]->put(fifo_stat.current_pkt_nbr);
			(*this->probe_gw_queue_size_kb)[cat_label][(*it2).first]->put(
						fifo_stat.current_length_bytes * 8 / 1000);
			(*this->probe_gw_queue_loss)[cat_label][(*it2).first]->put(fifo_stat.drop_pkt_nbr);
			(*this->probe_gw_queue_loss_kb)[cat_label][(*it2).first]->put(fifo_stat.drop_bytes * 8);
		}
		this->probe_gw_l2_to_sat_total[cat_label]->put(this->l2_to_sat_total_bytes[cat_label] * 8 /
	                                                   this->stats_period_ms);
		this->l2_to_sat_total_bytes[cat_label] = 0;
	}
}


bool SpotDownward::checkDama()
{
	if(!this->dama_ctrl)
	{
		// stop here
		return true;
	}
	return false;
}


bool SpotDownward::handleFrameTimer(time_sf_t super_frame_counter)
{
	// Upate the superframe counter
	this->super_frame_counter = super_frame_counter;

	// run the allocation algorithms (DAMA)
	this->dama_ctrl->runOnSuperFrameChange(this->super_frame_counter);

	list<DvbFrame *> msgs;
	list<DvbFrame *>::iterator msg;

	// handle simulated terminals
	if(!this->request_simu)
	{
		return true;
	}

	if(!this->request_simu->simulation(&msgs, this->super_frame_counter))
	{
		this->request_simu->stopSimulation();
		this->simulate = none_simu;

		LOG(this->log_request_simulation, LEVEL_ERROR,
		    "failed to simulate");
		return false;
	}

	for(msg = msgs.begin(); msg != msgs.end(); ++msg)
	{
		uint8_t msg_type = (*msg)->getMessageType();
		switch(msg_type)
		{
			case MSG_TYPE_SAC:
			{
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "simulate message type SAC");

				Sac *sac = (Sac *)(*msg);
				tal_id_t tal_id = sac->getTerminalId();
				// add CNI in SAC here as we have access to the data
				sac->setAcm(this->getRequiredCniOutput(tal_id));
				if(!this->handleSac(*msg))
				{
					return false;
				}

				break;
			}
			case MSG_TYPE_SESSION_LOGON_REQ:
			{
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "simulate message session logon request");

				LogonRequest *logon_req = (LogonRequest*)(*msg);
				tal_id_t st_id = logon_req->getMac();

				// check for column in FMT simulation list
				if(!this->addInputTerminal(st_id, this->rcs_modcod_def ))
				{
					LOG(this->log_request_simulation, LEVEL_ERROR,
					    "failed to register simulated ST with MAC "
					    "ID %u\n", st_id);
					return false;
				}
				if(!this->addOutputTerminal(st_id, this->s2_modcod_def))
				{
					LOG(this->log_request_simulation, LEVEL_ERROR,
					    "failed to register simulated ST with MAC "
					    "ID %u\n", st_id);
					return false;
				}
				if(!this->dama_ctrl->hereIsLogon(logon_req))
				{
					return false;
				}
				break;
			}
			case MSG_TYPE_SESSION_LOGOFF:
			{
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "simulate message logoff");

				// TODO remove Terminals
				Logoff *logoff = (Logoff*)(*msg);
				if(!this->dama_ctrl->hereIsLogoff(logoff))
				{
					return false;
				}
				break;
			}
			default:
				LOG(this->log_request_simulation, LEVEL_WARNING,
					"default");
				break;
		}
	}

	return true;
}


bool SpotDownward::handleFwdFrameTimer(time_sf_t fwd_frame_counter)
{
	this->fwd_frame_counter = fwd_frame_counter;
	this->updateStatistics();

	if(!this->addCniExt())
	{
		LOG(this->log_send_channel, LEVEL_ERROR,
		    "fail to add CNI extension");
		return false;
	}

	// schedule encapsulation packets
	// TODO In regenerative mode we should schedule in frame_timer ??
	// do not schedule on all categories, in regenerative we only schedule on the GW category
	for (auto&& it : this->scheduling)
	{
		uint32_t remaining_alloc_sym = 0;
		std::string label = it.first;
		Scheduling *scheduler = it.second;

		if(!scheduler->schedule(this->fwd_frame_counter,
		                        getCurrentTime(),
		                        &this->complete_dvb_frames,
		                        remaining_alloc_sym))
		{
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "failed to schedule encapsulation "
			    "packets stored in DVB FIFO for category %s\n",
			    label.c_str());
			return false;
		}

		LOG(this->log_receive_channel, LEVEL_INFO,
		    "SF#%u: %u symbols remaining after "
		    "scheduling in category %s\n", this->super_frame_counter,
		    remaining_alloc_sym, label.c_str());
	}

	return true;

}

void SpotDownward::updateFmt(void)
{
	if(!this->dama_ctrl)
	{
		// stop here
		return;
	}

	// for each terminal in DamaCtrl update required FMTs
	this->dama_ctrl->updateRequiredFmts();
}

uint8_t SpotDownward::getCtrlCarrierId(void) const
{
	return this->ctrl_carrier_id;
}

uint8_t SpotDownward::getSofCarrierId(void) const
{
	return this->sof_carrier_id;
}

uint8_t SpotDownward::getDataCarrierId(void) const
{
	return this->data_carrier_id;
}

list<DvbFrame *> &SpotDownward::getCompleteDvbFrames(void)
{
	return this->complete_dvb_frames;
}

event_id_t SpotDownward::getPepCmdApplyTimer(void)
{
	return this->pep_cmd_apply_timer;
}

void SpotDownward::setPepCmdApplyTimer(event_id_t pep_cmd_a_timer)
{
	this->pep_cmd_apply_timer = pep_cmd_a_timer;
}

bool SpotDownward::handleSac(const DvbFrame *dvb_frame)
{
	Sac *sac = (Sac *)dvb_frame;

	if(!this->dama_ctrl->hereIsSAC(sac))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle SAC frame\n");
		delete dvb_frame;
		return false;
	}

	return true;
}


bool SpotDownward::applyPepCommand(PepRequest *pep_request)
{
	if(this->dama_ctrl->applyPepCommand(pep_request))
	{
		LOG(this->log_receive_channel, LEVEL_NOTICE,
		    "PEP request successfully "
		    "applied in DAMA\n");
	}
	else
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to apply PEP request "
		    "in DAMA\n");
		return false;
	}

	return true;
}


bool SpotDownward::applySvnoCommand(SvnoRequest *svno_request)
{
	svno_request_type_t req_type = svno_request->getType();
	band_t band = svno_request->getBand();
	string cat_label = svno_request->getLabel();
	rate_kbps_t new_rate_kbps = svno_request->getNewRate();
	TerminalCategories<TerminalCategoryDama> *cat;
	time_ms_t frame_duration_ms;

	switch(band)
	{
		case FORWARD:
			cat = &this->categories;
			frame_duration_ms = this->fwd_down_frame_duration_ms;
			break;

		case RETURN:
			cat = this->dama_ctrl->getCategories();
			frame_duration_ms = this->ret_up_frame_duration_ms;
			break;

		default:
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Wrong SVNO band %u\n", band);
			return false;
	}

	switch(req_type)
	{
		case SVNO_REQUEST_ALLOCATION:
			return this->allocateBand(frame_duration_ms, cat_label, new_rate_kbps, *cat);
			break;

		case SVNO_REQUEST_RELEASE:
			return this->releaseBand(frame_duration_ms, cat_label, new_rate_kbps, *cat);
			break;

		default:
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Wrong SVNO request type %u\n", req_type);
			return false;
	}

	return true;
}


/**
 * Add a CNI extension in the next GSE packet header
 * Only for SCPC
 * @return false if failed, true if succeed
 */
// TODO At the moment, the CNI is only sent when it changes and with the
//      current MODCOD that can leads to CNI not transmitted. This can be
//      correct either with a timer (base en acm_period) that may call
//      setCniInputHasChanged on all SCPC terminals or using the most
//      robust MODCOD to transmit packets with CNI extension
bool SpotDownward::addCniExt(void)
{
	std::list<tal_id_t> list_st;
	std::map<std::string, fifos_t>::iterator dvb_fifos_it;

	// Create list of first packet from FIFOs
	for(dvb_fifos_it = this->dvb_fifos.begin();
	    dvb_fifos_it != this->dvb_fifos.end();
	    ++dvb_fifos_it)
	{
		fifos_t::iterator fifos_it;
		fifos_t fifos = (*dvb_fifos_it).second;
		for(fifos_it = fifos.begin();
		    fifos_it != fifos.end();
		    ++fifos_it)
		{
			DvbFifo *fifo = (*fifos_it).second;
			vector<MacFifoElement *> queue = fifo->getQueue();
			vector<MacFifoElement *>::iterator queue_it;

			for(queue_it = queue.begin();
			    queue_it != queue.end();
			    ++queue_it)
			{
				MacFifoElement* elem = (*queue_it);
				NetPacket *packet = (NetPacket*)elem->getElem();
				tal_id_t tal_id = packet->getDstTalId();
				NetPacket *extension_pkt = NULL;

				list<tal_id_t>::iterator it = std::find(this->is_tal_scpc.begin(),
				                                        this->is_tal_scpc.end(),
				                                        tal_id);
				if(it != this->is_tal_scpc.end() &&
				   this->getCniInputHasChanged(tal_id))
				{
					list_st.push_back(tal_id);
					// we could make specific SCPC function
					if(!this->setPacketExtension(this->pkt_hdl,
						                         elem, fifo,
						                         packet,
						                         &extension_pkt,
						                         this->mac_id,
						                         tal_id,
						                         "encodeCniExt",
						                         this->super_frame_counter,
						                         true))
					{
						return false;
					}

					LOG(this->log_send_channel, LEVEL_DEBUG,
					    "SF #%d: packet belongs to FIFO #%d\n",
					    this->super_frame_counter, (*fifos_it).first);
					// Delete old packet
					delete packet;
				}
			}
		}
	}

	// try to send empty packet if no packet has been found for a terminal
	for(set<tal_id_t>::const_iterator st_it = this->input_sts->begin();
	    st_it != this->input_sts->end(); ++st_it)
	{
		tal_id_t tal_id = *st_it;
		list<tal_id_t>::iterator it = std::find(list_st.begin(),
		                                        list_st.end(),
		                                        tal_id);
		list<tal_id_t>::iterator it_scpc = std::find(this->is_tal_scpc.begin(),
		                                             this->is_tal_scpc.end(),
		                                             tal_id);

		if(it_scpc != this->is_tal_scpc.end() && it == list_st.end()
		   && this->getCniInputHasChanged(tal_id))
		{
			NetPacket *extension_pkt = NULL;
			string cat_label;
			map<string, fifos_t>::iterator fifos_it;

			// first get the relevant category for the packet to find appropriate fifo
			if(this->terminal_affectation.find(tal_id) != this->terminal_affectation.end())
			{
				if(this->terminal_affectation.at(tal_id) == NULL)
				{
					LOG(this->log_send_channel, LEVEL_ERROR,
					    "No category associated to terminal %u, "
					    "cannot send CNI for SCPC carriers\n",
					    tal_id);
					return false;
				}
				cat_label = this->terminal_affectation.at(tal_id)->getLabel();
			}
			else
			{
				if(!this->default_category)
				{
					LOG(this->log_send_channel, LEVEL_ERROR,
					    "No default category for terminal %u, "
					    "cannot send CNI for SCPC carriers\n",
					    tal_id);
					return false;
				}
				cat_label = this->default_category->getLabel();
			}
			// find the FIFO associated to the IP QoS (= MAC FIFO id)
			// else use the default id
			fifos_it = this->dvb_fifos.find(cat_label);
			if(fifos_it == this->dvb_fifos.end())
			{
				LOG(this->log_send_channel, LEVEL_ERROR,
				    "No fifo found for this category %s unable to send CNI for SCPC carriers",
				    cat_label.c_str());
				return false;
			}

			// set packet extension to this new empty packet
			if(!this->setPacketExtension(this->pkt_hdl,
				                         NULL,
				                         // highest priority fifo
				                         ((*fifos_it).second)[0],
				                         nullptr,
				                         &extension_pkt,
				                         this->mac_id,
				                         tal_id,
				                         "encodeCniExt",
				                         this->super_frame_counter,
				                         true))
			{
				return false;
			}

			LOG(this->log_send_channel, LEVEL_DEBUG,
			    "SF #%d: adding empty packet into FIFO NM\n",
			    this->super_frame_counter);
		}
	}

	return true;
}
