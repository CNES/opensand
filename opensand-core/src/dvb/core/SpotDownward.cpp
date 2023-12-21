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
#include "DvbFifo.h"
#include "FifoElement.h"

#include <errno.h>
#include <opensand_output/OutputEvent.h>


SpotDownward::SpotDownward(spot_id_t spot_id,
                           tal_id_t mac_id,
                           time_us_t fwd_down_frame_duration,
                           time_us_t ret_up_frame_duration,
                           time_ms_t stats_period,
                           StackPlugin *upper_encap,
                           std::shared_ptr<EncapPlugin::EncapPacketHandler> pkt_hdl,
                           std::shared_ptr<StFmtSimuList> input_sts,
                           std::shared_ptr<StFmtSimuList> output_sts):
	DvbChannel{upper_encap},
	DvbFmt(),
	dama_ctrl(nullptr),
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
	default_category(nullptr),
	fwd_fmt_groups(),
	ret_fmt_groups(),
	cni(100),
	pep_cmd_apply_timer(),
	request_simu(nullptr),
	event_file(nullptr),
	simulate(none_simu),
	probe_gw_l2_to_sat_total(),
	l2_to_sat_total_bytes(),
	probe_frame_interval(nullptr),
	probe_sent_modcod(nullptr),
	log_request_simulation(nullptr),
	event_logon_resp(nullptr)
{
	this->fwd_down_frame_duration = fwd_down_frame_duration;
	this->ret_up_frame_duration = ret_up_frame_duration;
	this->stats_period_ms = stats_period;
	this->pkt_hdl = pkt_hdl;
	this->input_sts = input_sts;
	this->output_sts = output_sts;

	this->log_request_simulation = Output::Get()->registerLog(LEVEL_WARNING, Format("Spot_%d.Dvb.RequestSimulation", this->spot_id));
}


SpotDownward::~SpotDownward()
{
	this->categories.clear();
	this->scheduling.clear();
	this->complete_dvb_frames.clear();
	this->fwd_fmt_groups.clear();
	this->ret_fmt_groups.clear();

	// delete fifos
	for (auto &&[label, fifos] : this->dvb_fifos)
	{
		fifos->clear();
	}
	this->dvb_fifos.clear();

	this->terminal_affectation.clear();
}

void SpotDownward::generateConfiguration(std::shared_ptr<OpenSANDConf::MetaParameter> disable_ctrl_plane)
{
	RequestSimulator::generateConfiguration();

	auto Conf = OpenSandModelConf::Get();

	auto types = Conf->getModelTypesDefinition();
	types->addEnumType("ncc_simulation", "Simulated Requests", {"None", "Random", "File"});
	types->addEnumType("gw_fifo_access_type", "Access Type", {"ACM", "VCM0", "VCM1", "VCM2", "VCM3"});
	types->addEnumType("dama_algorithm", "DAMA Algorithm", {"Legacy",});

	auto conf = Conf->getOrCreateComponent("network", "Network", "The DVB layer configuration");
	auto fifos = conf->addList("gw_fifos", "FIFOs to send messages to Terminals", "gw_fifo")->getPattern();
	fifos->addParameter("priority", "Priority", types->getType("ubyte"));
	fifos->addParameter("name", "Name", types->getType("string"));
	fifos->addParameter("capacity", "Capacity", types->getType("ushort"))->setUnit("packets");
	fifos->addParameter("access_type", "Access Type", types->getType("gw_fifo_access_type"));
	auto simulation = conf->addParameter("simulation",
	                                     "Simulated Requests",
	                                     types->getType("ncc_simulation"),
	                                     "Should OpenSAND simulate extraneous requests?");
	Conf->setProfileReference(simulation, disable_ctrl_plane, false);
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

	auto fca = conf->addParameter("fca", "FCA", types->getType("uint"));
	Conf->setProfileReference(fca, disable_ctrl_plane, false);
	auto dama_algo = conf->addParameter("dama_algorithm", "DAMA Algorithm", types->getType("dama_algorithm"));
	Conf->setProfileReference(dama_algo, disable_ctrl_plane, false);
}


bool SpotDownward::onInit()
{
	if(!this->initModcodDefinitionTypes())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize MOCODS definitions types\n");
		return false;
	}

	// Initialization of the modcod def
	if(!this->initModcodDefFile(MODCOD_DEF_S2, this->s2_modcod_def))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the forward link definition MODCOD file\n");
		return false;
	}
	if(!this->initModcodDefFile(MODCOD_DEF_RCS2,
	                            this->rcs_modcod_def,
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

	this->initStatsTimer(this->fwd_down_frame_duration);

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


bool SpotDownward::initMode()
{
	// initialize scheduling
	// depending on the satellite type
	OpenSandModelConf::spot current_spot;
	if (!OpenSandModelConf::Get()->getSpotForwardCarriers(this->spot_id, current_spot))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "there is no gateways with value: "
			"%d into forward down frequency plan\n",
		    this->mac_id);
		return false;
	}

	if(!this->initBand<TerminalCategoryDama>(current_spot,
	                                         "forward down frequency plan",
	                                         AccessType::TDM,
	                                         this->fwd_down_frame_duration,
	                                         this->s2_modcod_def,
	                                         this->categories,
	                                         this->terminal_affectation,
	                                         this->default_category,
	                                         this->fwd_fmt_groups))
	{
		return false;
	}


	// check that there is at least DVB fifos for VCM carriers
	for (auto&& cat_it: this->categories)
	{
		bool is_vcm_carriers = false;
		bool is_acm_carriers = false;
		bool is_vcm_fifo = false;
		std::shared_ptr<fifos_t> fifos = std::make_shared<fifos_t>();
		std::string label;

		std::shared_ptr<TerminalCategoryDama> cat = cat_it.second;
		label = cat->getLabel();
		if(!this->initFifo(fifos))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed initialize fifos for category %s\n", label.c_str());
			return false;
		}
		this->dvb_fifos.emplace(label, fifos);

		// check if there is VCM carriers in this category
		for (auto&& carriers: cat->getCarriersGroups())
		{
			if(carriers.getVcmCarriers().size() > 1)
			{
				is_vcm_carriers = true;
			}
			else
			{
				is_acm_carriers = true;
			}
		}

		for (auto&& [qos, fifo]: *fifos)
		{
			if(fifo->getAccessType() == ForwardAccessType::vcm)
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
				    label.c_str());
				return false;
			}
			else
			{
				LOG(this->log_init_channel, LEVEL_WARNING,
				    "There is VCM carriers in category %s but no VCM FIFOs, "
				    "the VCM carriers won't be used",
				    label.c_str());
			}
		}

		try
		{
			auto schedule =  std::make_unique<ForwardSchedulingS2>(this->fwd_down_frame_duration,
		                                                           this->pkt_hdl,
		                                                           this->dvb_fifos.at(label),
		                                                           this->output_sts,
		                                                           this->s2_modcod_def,
		                                                           cat, this->spot_id,
		                                                           true, this->mac_id, "");
			this->scheduling.emplace(label, std::move(schedule));
		}
		catch (const std::bad_alloc&)
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "failed initialize forward scheduling for category %s\n", label.c_str());
			return false;
		}
	}

	return true;
}


// TODO this function is NCC part but other functions are related to GW,
//      we could maybe create two classes inside the block to keep them separated
bool SpotDownward::initDama()
{
	time_ms_t sync_period_ms;
	time_frame_t sync_period_frame;
	time_sf_t rbdc_timeout_sf;
	std::string dama_algo;

	TerminalCategories<TerminalCategoryDama> dc_categories;
	TerminalMapping<TerminalCategoryDama> dc_terminal_affectation;
	std::shared_ptr<TerminalCategoryDama> dc_default_category = nullptr;

	auto Conf = OpenSandModelConf::Get();

	// Skip if the control plane is disabled
	bool ctrl_plane_disabled;
	Conf->getControlPlaneDisabled(ctrl_plane_disabled);
	if (ctrl_plane_disabled)
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Control plane disabled: skipping DAMA initialization");
		return true;
	}

	auto ncc = Conf->getProfileData()->getComponent("network");

	// Retrieving the free capacity assignement parameter
	rate_kbps_t fca_kbps;
	if(!OpenSandModelConf::extractParameterData(ncc->getParameter("fca"), fca_kbps))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "missing FCA parameter\n");
		return false;
	}

	LOG(this->log_init_channel, LEVEL_NOTICE,
	    "fca = %d kb/s\n", fca_kbps);

	if(!Conf->getSynchroPeriod(sync_period_ms))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Missing synchronisation period\n");
		return false;
	}
	sync_period_frame = time_frame_t(round(std::chrono::duration_cast<std::chrono::duration<double>>(sync_period_ms) /
	                                       std::chrono::duration_cast<std::chrono::duration<double>>(this->ret_up_frame_duration)));
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
	                                         AccessType::DAMA,
	                                         this->ret_up_frame_duration,
	                                         this->rcs_modcod_def,
	                                         dc_categories,
	                                         dc_terminal_affectation,
	                                         dc_default_category,
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

	/* select the specified DAMA algorithm */
	if(dama_algo == "Legacy")
	{
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "creating Legacy DAMA controller\n");
		this->dama_ctrl = std::make_unique<DamaCtrlRcs2Legacy>(this->spot_id);
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
	if(!this->dama_ctrl->initParent(this->ret_up_frame_duration,
	                                rbdc_timeout_sf,
	                                fca_kbps,
	                                dc_categories,
	                                dc_terminal_affectation,
	                                dc_default_category,
	                                this->input_sts,
	                                &this->rcs_modcod_def,
	                                this->simulate != none_simu))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "Dama Controller Initialization failed.\n");
		return false;
	}

	if(!this->dama_ctrl->init())
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "failed to initialize the DAMA controller\n");
		return false;
	}
	this->dama_ctrl->setRecordFile(this->event_file);

	return true;
}


bool SpotDownward::initCarrierIds()
{
	auto Conf = OpenSandModelConf::Get();

	OpenSandModelConf::spot_infrastructure carriers;
	if (!Conf->getSpotInfrastructure(this->spot_id, carriers))
	{
		LOG(this->log_init_channel, LEVEL_ERROR,
		    "couldn't create spot infrastructure for gw %d",
		    this->mac_id);
		return false;
	}

	Component entity_type = Conf->getEntityType(mac_id);
	if (entity_type == Component::gateway)
	{
		this->ctrl_carrier_id = carriers.ctrl_in_gw.id;
		this->sof_carrier_id = carriers.ctrl_in_gw.id;
		this->data_carrier_id = carriers.data_in_gw.id;
	}
	else if (entity_type == Component::satellite)
	{
		this->ctrl_carrier_id = carriers.ctrl_out_st.id;
		this->sof_carrier_id = carriers.ctrl_out_st.id;
		this->data_carrier_id = carriers.data_out_st.id;
	}
	else
	{
		LOG(log_init_channel, LEVEL_ERROR, "Cannot instantiate a SpotDownward with mac_id %d which is not a gateway nor a satellite");
		return false;
	}

	return true;
}


bool SpotDownward::initFifo(std::shared_ptr<fifos_t> fifos)
{
	unsigned int default_fifo_prio = 0;
	std::map<std::string, int> fifo_ids;

	auto Conf = OpenSandModelConf::Get();
	auto ncc = Conf->getProfileData()->getComponent("network");
	for(auto& item : ncc->getList("qos_classes")->getItems())
	{
		auto category = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);
		std::string fifo_name;
		if(!OpenSandModelConf::extractParameterData(category->getParameter("fifo"), fifo_name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "Section network, missing QoS class FIFO name parameter\n");
			return false;
		}

		auto priority = fifo_ids.find(fifo_name);
		if(priority == fifo_ids.end())
		{
			fifo_ids.emplace(fifo_name, fifo_ids.size());
		}
	}

	for (auto& item : ncc->getList("gw_fifos")->getItems())
	{
		auto fifo_item = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);

		qos_t fifo_priority;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("priority"), fifo_priority))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo priority from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}

		std::string fifo_name;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("name"), fifo_name))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo name from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}

		auto fifo_it = fifo_ids.find(fifo_name);
		if (fifo_it == fifo_ids.end())
		{
			fifo_it = fifo_ids.emplace(fifo_name, fifo_ids.size()).first;
		}
		auto fifo_id = fifo_it->second;

		vol_pkt_t fifo_size;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("capacity"), fifo_size))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo capacity from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}

		std::string fifo_access_type;
		if(!OpenSandModelConf::extractParameterData(fifo_item->getParameter("access_type"), fifo_access_type))
		{
			LOG(this->log_init_channel, LEVEL_ERROR,
			    "cannot get fifo access type from section 'ncc, fifos'\n");
			goto err_fifo_release;
		}

		auto fifo = std::make_unique<DvbFifo>(fifo_priority, fifo_name, fifo_access_type, fifo_size);

		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Fifo priority = %u, FIFO name %s, size %u, "
		    "access type %d\n",
		    fifo->getPriority(),
		    fifo->getName(),
		    fifo->getMaxSize(),
		    fifo->getAccessType());

		// the default FIFO is the last one = the one with the smallest
		// priority actually, the IP plugin should add packets in the
		// default FIFO if the DSCP field is not recognize, default_fifo_id
		// should not be used this is only used if traffic categories
		// configuration and fifo configuration are not coherent.
		if (fifo->getPriority() > default_fifo_prio)
		{
			default_fifo_prio = fifo->getPriority();
			this->default_fifo_id = fifo_id;
		}

		fifos->emplace(fifo_id, std::move(fifo));
	}

	return true;

err_fifo_release:
	fifos->clear();
	return false;
}

bool SpotDownward::initRequestSimulation()
{
	auto Conf = OpenSandModelConf::Get();

	// Skip if the control plane is disabled
	bool ctrl_plane_disabled;
	Conf->getControlPlaneDisabled(ctrl_plane_disabled);
	if (ctrl_plane_disabled)
	{
		this->simulate = none_simu;
		LOG(this->log_init_channel, LEVEL_NOTICE,
		    "Control plane disabled: skipping event simulation initialization");
		return true;
	}

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
		this->request_simu = std::make_unique<FileSimulator>(this->spot_id,
		                                                     this->mac_id,
		                                                     this->event_file,
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
		this->request_simu = std::make_unique<RandomSimulator>(this->spot_id,
		                                                       this->mac_id,
		                                                       this->event_file,
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

bool SpotDownward::initOutput()
{
	auto output = Output::Get();

	// generate probes prefix
	bool is_sat = OpenSandModelConf::Get()->getComponentType() == Component::satellite;
	std::string prefix = generateProbePrefix(spot_id, Component::gateway, is_sat);

	// Events
	this->event_logon_resp = output->registerEvent(prefix + "DVB.logon_response");

	for (auto &&[cat_label, fifos]: dvb_fifos)
	{
		for (auto &&[qos_id, fifo]: *fifos)
		{
			std::string fifo_name = fifo->getName();

			std::shared_ptr<Probe<int>> probe_temp;
			std::string name;

			probe_temp = output->registerProbe<int>(prefix + cat_label + ".Queue size.packets." + fifo_name,
			                                        "Packets", true, SAMPLE_LAST);
			this->probe_gw_queue_size[cat_label].emplace(qos_id, probe_temp);

			probe_temp = output->registerProbe<int>(prefix + cat_label + ".Queue size.capacity." + fifo_name,
			                                        "kbits", true, SAMPLE_LAST);
			this->probe_gw_queue_size_kb[cat_label].emplace(qos_id, probe_temp);

			probe_temp = output->registerProbe<int>(prefix + cat_label + ".Throughputs.L2_to_SAT_before_sched." + fifo_name,
			                                        "Kbits/s", true, SAMPLE_AVG);
			this->probe_gw_l2_to_sat_before_sched[cat_label].emplace(qos_id, probe_temp);

			probe_temp = output->registerProbe<int>(prefix + cat_label + ".Throughputs.L2_to_SAT_after_sched." + fifo_name,
			                                        "Kbits/s", true, SAMPLE_AVG);
			this->probe_gw_l2_to_sat_after_sched[cat_label].emplace(qos_id, probe_temp);

			probe_temp = output->registerProbe<int>(prefix + cat_label + ".Queue loss.packets." + fifo_name,
			                                        "Packets", true, SAMPLE_SUM);
			this->probe_gw_queue_loss[cat_label].emplace(qos_id, probe_temp);

			probe_temp = output->registerProbe<int>(prefix + cat_label + ".Queue loss.rate." + fifo_name,
			                                        "Kbits/s", true, SAMPLE_SUM);
			this->probe_gw_queue_loss_kb[cat_label].emplace(qos_id, probe_temp);
		}
		this->probe_gw_l2_to_sat_total[cat_label] =
		    output->registerProbe<int>(prefix + cat_label + ".Throughputs.L2_to_SAT_after_sched.total",
		                               "Kbits/s", true, SAMPLE_AVG);
	}

	return true;
}


bool SpotDownward::handleSalohaAcks(Rt::Ptr<std::list<Rt::Ptr<DvbFrame>>> ack_frames)
{
	for (auto&& ack: *ack_frames)
	{
		this->complete_dvb_frames.push_back(std::move(ack));
	}
	return true;
}


bool SpotDownward::handleEncapPacket(Rt::Ptr<NetPacket> packet)
{
	qos_t fifo_priority = packet->getQos();
	std::string cat_label;
	tal_id_t dst_tal_id;

	LOG(this->log_receive_channel, LEVEL_INFO,
	    "SF#%u: store one encapsulation "
	    "packet\n", this->super_frame_counter);

	dst_tal_id = packet->getDstTalId();
	// category of the packet
	if(this->terminal_affectation.find(dst_tal_id) != this->terminal_affectation.end())
	{
		if(this->terminal_affectation.at(dst_tal_id) == nullptr)
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
	auto fifos_it = this->dvb_fifos.find(cat_label);
	if(fifos_it == this->dvb_fifos.end())
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "No fifo found for this category %s", cat_label.c_str());
		return false;
	}

	auto fifo_it = fifos_it->second->find(fifo_priority);
	if(fifo_it == fifos_it->second->end())
	{
		fifo_it = fifos_it->second->find(this->default_fifo_id);
	}

	if(!this->pushInFifo(*(fifo_it->second), std::move(packet), time_ms_t::zero()))
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


bool SpotDownward::handleLogonReq(Rt::Ptr<LogonRequest> logon_req)
{
	uint16_t mac = logon_req->getMac();
	bool is_scpc = logon_req->getIsScpc();
	if(is_scpc)
	{
		this->is_tal_scpc.push_back(mac);
	}

	// Inform the Dama controller (for its own context)
	if(!is_scpc && this->dama_ctrl && 
	   !this->dama_ctrl->hereIsLogon(std::move(logon_req)))
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


bool SpotDownward::handleLogoffReq(Rt::Ptr<DvbFrame> dvb_frame)
{
	auto logoff = dvb_frame_upcast<Logoff>(std::move(dvb_frame));

	// unregister the ST identified by the MAC ID found in DVB frame
	if(!this->delInputTerminal(logoff->getMac()))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to delete the ST with ID %d from FMT simulation\n",
		    logoff->getMac());
		return false;
	}
	if(!this->delOutputTerminal(logoff->getMac()))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to delete the ST with ID %d from FMT simulation\n",
		    logoff->getMac());
		return false;
	}

	if(this->dama_ctrl)
	{
		this->dama_ctrl->hereIsLogoff(std::move(logoff));
	}
	LOG(this->log_receive_channel, LEVEL_DEBUG,
	    "SF#%u: logoff request from %d\n",
	    this->super_frame_counter, logoff->getMac());

	return true;
}


bool SpotDownward::buildTtp(Ttp& ttp)
{
	return this->dama_ctrl->buildTTP(ttp);
}


void SpotDownward::updateStatistics()
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

	for (auto &&[cat_label, fifos]: dvb_fifos)
	{
		for (auto &&[qos_id, fifo]: *fifos)
		{
			fifo->getStatsCxt(fifo_stat);

			this->l2_to_sat_total_bytes[cat_label] += fifo_stat.out_length_bytes;

			this->probe_gw_l2_to_sat_before_sched[cat_label][qos_id]->put(
			    time_ms_t(fifo_stat.in_length_bytes * 8) / this->stats_period_ms);

			this->probe_gw_l2_to_sat_after_sched[cat_label][qos_id]->put(
			    time_ms_t(fifo_stat.out_length_bytes * 8) / this->stats_period_ms);

			// Mac fifo stats
			this->probe_gw_queue_size[cat_label][qos_id]->put(fifo_stat.current_pkt_nbr);
			this->probe_gw_queue_size_kb[cat_label][qos_id]->put(
			    fifo_stat.current_length_bytes * 8 / 1000);
			this->probe_gw_queue_loss[cat_label][qos_id]->put(fifo_stat.drop_pkt_nbr);
			this->probe_gw_queue_loss_kb[cat_label][qos_id]->put(fifo_stat.drop_bytes * 8);
		}
		this->probe_gw_l2_to_sat_total[cat_label]->put(
			time_ms_t(this->l2_to_sat_total_bytes[cat_label] * 8) / this->stats_period_ms);
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

	// handle simulated terminals
	if(!this->request_simu)
	{
		return true;
	}

	std::list<Rt::Ptr<DvbFrame>> msgs;
	if(!this->request_simu->simulation(msgs, this->super_frame_counter))
	{
		this->request_simu->stopSimulation();
		this->simulate = none_simu;

		LOG(this->log_request_simulation, LEVEL_ERROR,
		    "failed to simulate");
		return false;
	}

	for(auto&& msg: msgs)
	{
		EmulatedMessageType msg_type = msg->getMessageType();
		switch(msg_type)
		{
			case EmulatedMessageType::Sac:
			{
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "simulate message type SAC");

				auto sac = dvb_frame_upcast<Sac>(std::move(msg));
				tal_id_t tal_id = sac->getTerminalId();
				// add CNI in SAC here as we have access to the data
				sac->setAcm(this->getRequiredCniOutput(tal_id));
				if(!this->handleSac(dvb_frame_downcast(std::move(sac))))
				{
					return false;
				}

				break;
			}
			case EmulatedMessageType::SessionLogonReq:
			{
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "simulate message session logon request");

				auto logon_req = dvb_frame_upcast<LogonRequest>(std::move(msg));
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
				if(!this->dama_ctrl->hereIsLogon(std::move(logon_req)))
				{
					return false;
				}
				break;
			}
			case EmulatedMessageType::SessionLogoff:
			{
				LOG(this->log_request_simulation, LEVEL_INFO,
				    "simulate message logoff");

				// TODO remove Terminals
				if(!this->dama_ctrl->hereIsLogoff(dvb_frame_upcast<Logoff>(std::move(msg))))
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
	for (auto&& [label, scheduler]: this->scheduling)
	{
		uint32_t remaining_alloc_sym = 0;
		if(!scheduler->schedule(this->fwd_frame_counter,
		                        this->complete_dvb_frames,
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

void SpotDownward::updateFmt()
{
	if(!this->dama_ctrl)
	{
		// stop here
		return;
	}

	// for each terminal in DamaCtrl update required FMTs
	this->dama_ctrl->updateRequiredFmts();
}

uint8_t SpotDownward::getCtrlCarrierId() const
{
	return this->ctrl_carrier_id;
}

uint8_t SpotDownward::getSofCarrierId() const
{
	return this->sof_carrier_id;
}

uint8_t SpotDownward::getDataCarrierId() const
{
	return this->data_carrier_id;
}

std::list<Rt::Ptr<DvbFrame>> &SpotDownward::getCompleteDvbFrames()
{
	return this->complete_dvb_frames;
}

Rt::event_id_t SpotDownward::getPepCmdApplyTimer()
{
	return this->pep_cmd_apply_timer;
}

void SpotDownward::setPepCmdApplyTimer(Rt::event_id_t pep_cmd_a_timer)
{
	this->pep_cmd_apply_timer = pep_cmd_a_timer;
}

bool SpotDownward::handleSac(Rt::Ptr<DvbFrame> dvb_frame)
{
	if(!this->dama_ctrl->hereIsSAC(dvb_frame_upcast<Sac>(std::move(dvb_frame))))
	{
		LOG(this->log_receive_channel, LEVEL_ERROR,
		    "failed to handle SAC frame\n");
		return false;
	}

	return true;
}


bool SpotDownward::applyPepCommand(std::unique_ptr<PepRequest> pep_request)
{
	if(this->dama_ctrl->applyPepCommand(std::move(pep_request)))
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


bool SpotDownward::applySvnoCommand(std::unique_ptr<SvnoRequest> svno_request)
{
	svno_request_type_t req_type = svno_request->getType();
	band_t band = svno_request->getBand();
	std::string cat_label = svno_request->getLabel();
	rate_kbps_t new_rate_kbps = svno_request->getNewRate();
	TerminalCategories<TerminalCategoryDama> *cat;
	time_us_t frame_duration;

	switch(band)
	{
		case FORWARD:
			cat = &this->categories;
			frame_duration = this->fwd_down_frame_duration;
			break;

		case RETURN:
			cat = this->dama_ctrl->getCategories();
			frame_duration = this->ret_up_frame_duration;
			break;

		default:
			LOG(this->log_receive_channel, LEVEL_ERROR,
			    "Wrong SVNO band %u\n", band);
			return false;
	}

	switch(req_type)
	{
		case SVNO_REQUEST_ALLOCATION:
			return this->allocateBand(frame_duration, cat_label, new_rate_kbps, *cat);
			break;

		case SVNO_REQUEST_RELEASE:
			return this->releaseBand(frame_duration, cat_label, new_rate_kbps, *cat);
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
bool SpotDownward::addCniExt()
{
	std::list<tal_id_t> list_st;

	// Create list of first packet from FIFOs
	for(auto&& [label, fifos]: this->dvb_fifos)
	{
		for(auto&& [qos, fifo] : *fifos)
		{
			for(auto elem_it = fifo->wbegin(); elem_it != fifo->wend(); ++elem_it)
			{
				std::unique_ptr<FifoElement>& elem = *elem_it;
				Rt::Ptr<NetPacket> packet = elem->releaseElem<NetPacket>();
				tal_id_t tal_id = packet->getDstTalId();

				auto it = std::find(this->is_tal_scpc.begin(), this->is_tal_scpc.end(), tal_id);
				if(it != this->is_tal_scpc.end() && this->getCniInputHasChanged(tal_id))
				{
					list_st.push_back(tal_id);
					vol_bytes_t packet_length = packet->getTotalLength();
					// we could make specific SCPC function
					packet = this->setPacketExtension(this->pkt_hdl,
						                              std::move(packet),
						                              this->mac_id,
						                              tal_id,
						                              "encodeCniExt",
						                              this->super_frame_counter,
						                              true);
					if (!packet)
					{
						fifo->erase(elem_it);
						fifo->decreaseFifoSize(packet_length);
						return false;
					}
					else
					{
						vol_bytes_t new_length = packet->getTotalLength();
						if (new_length > packet_length)
						{
							fifo->increaseFifoSize(new_length - packet_length);
						}
						if (packet_length > new_length)
						{
							fifo->decreaseFifoSize(packet_length - new_length);
						}
					}

					LOG(this->log_send_channel, LEVEL_DEBUG,
					    "SF #%d: packet belongs to FIFO #%d\n",
					    this->super_frame_counter, qos);
				}

				// Put packet back into the fifo element
				elem->setElem(std::move(packet));
			}
		}
	}

	// try to send empty packet if no packet has been found for a terminal
	for (auto&& tal_id : *this->input_sts)
	{
		auto it = std::find(list_st.begin(), list_st.end(), tal_id);
		auto it_scpc = std::find(this->is_tal_scpc.begin(), this->is_tal_scpc.end(), tal_id);

		if(it_scpc != this->is_tal_scpc.end() && it == list_st.end() && this->getCniInputHasChanged(tal_id))
		{
			std::string cat_label;

			// first get the relevant category for the packet to find appropriate fifo
			if(this->terminal_affectation.find(tal_id) != this->terminal_affectation.end())
			{
				if(this->terminal_affectation.at(tal_id) == nullptr)
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
			auto fifos_it = this->dvb_fifos.find(cat_label);
			if(fifos_it == this->dvb_fifos.end())
			{
				LOG(this->log_send_channel, LEVEL_ERROR,
				    "No fifo found for this category %s unable to send CNI for SCPC carriers",
				    cat_label.c_str());
				return false;
			}

			// set packet extension to this new empty packet
			Rt::Ptr<NetPacket> scpc_packet = this->setPacketExtension(this->pkt_hdl,
				                                                      Rt::make_ptr<NetPacket>(nullptr),
				                                                      this->mac_id,
				                                                      tal_id,
				                                                      "encodeCniExt",
				                                                      this->super_frame_counter,
				                                                      true);
			if (!scpc_packet)
			{
				return false;
			}

			fifos_t &fifos = *(fifos_it->second);
			// highest priority fifo
			fifos[0]->push(std::move(scpc_packet), time_ms_t::zero());
			LOG(this->log_send_channel, LEVEL_DEBUG,
			    "SF #%d: adding empty packet into FIFO NM\n",
			    this->super_frame_counter);
		}
	}

	return true;
}
