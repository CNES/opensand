/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
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
 * @file OpenSandModelConf.h
 * @brief GLobal interface for configuration file reading
 * @author Bénédicte MOTTO / <bmotto@toulouse.viveris.com>
 * @author Joaquin Muguerza / <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU / <adelrieu@toulouse.viveris.com>
 * @author Mathias ETTINGER / <mathias.ettinger@viveris.fr>
 */

#include <sstream>
#include <utility>

#include <opensand_conf/Configuration.h>

#include "OpenSandModelConf.h"
#include "MacAddress.h"
#include "SarpTable.h"
#include "FmtDefinition.h"
#include "FmtDefinitionTable.h"


const std::map<std::string, log_level_t> levels_map{
	{"debug", LEVEL_DEBUG},
	{"info", LEVEL_INFO},
	{"notice", LEVEL_NOTICE},
	{"warning", LEVEL_WARNING},
	{"error", LEVEL_ERROR},
	{"critical", LEVEL_CRITICAL},
};


OpenSandModelConf::OpenSandModelConf():
	topology_model(nullptr),
	infrastructure_model(nullptr),
	profile_model(nullptr),
	topology(nullptr),
	infrastructure(nullptr),
	profile(nullptr)
{
}


OpenSandModelConf::~OpenSandModelConf()
{
}


std::shared_ptr<OpenSandModelConf> OpenSandModelConf::Get()
{
	static std::shared_ptr<OpenSandModelConf> instance{new OpenSandModelConf()};
	return instance;
}


void OpenSandModelConf::createModels()
{
	infrastructure_model = std::make_shared<OpenSANDConf::MetaModel>("1.0.0");
	infrastructure_model->getRoot()->setDescription("infrastructure");
	auto types = infrastructure_model->getTypesDefinition();
	types->addEnumType("log_level", "Log Level", {"debug", "info", "notice", "warning", "error", "critical"});
	types->addEnumType("entity_type", "Entity Type", {"Gateway", "Gateway Net Access", "Gateway Phy", "Satellite", "Terminal"});

	auto log_levels = infrastructure_model->getRoot()->addComponent("logs", "Logs");
	log_levels->addComponent("init", "init")->addParameter("level", "Log Level", types->getType("log_level"));
	log_levels->addComponent("lan_adaptation", "lan_adaptation")->addParameter("level", "Log Level", types->getType("log_level"));
	log_levels->addComponent("encap", "encap")->addParameter("level", "Log Level", types->getType("log_level"));
	log_levels->addComponent("dvb", "dvb")->addParameter("level", "Log Level", types->getType("log_level"));
	log_levels->addComponent("physical_layer", "physical_layer")->addParameter("level", "Log Level", types->getType("log_level"));
	log_levels->addComponent("sat_carrier", "sat_carrier")->addParameter("level", "Log Level", types->getType("log_level"));
	auto extra_logs = log_levels->addList("extra_levels", "Levels", "levels")->getPattern();
	extra_logs->addParameter("name", "Log Name", types->getType("string"));
	extra_logs->addParameter("level", "Log Level", types->getType("log_level"));

	auto storage = infrastructure_model->getRoot()->addComponent("storage", "Storage");
	auto local_storage = storage->addParameter("enable_local", "Enable Storage to Local Filesystem", types->getType("bool"));
	auto path_storage = storage->addParameter("path_local", "Folder for Storage", types->getType("string"));
	infrastructure_model->setReference(path_storage, local_storage);
	auto expected = std::dynamic_pointer_cast<OpenSANDConf::DataValue<bool>>(path_storage->getReferenceData());
	expected->set(true);

	auto collector_storage = storage->addParameter("enable_collector", "Enable Storage to OpenSAND Collector", types->getType("bool"));
	auto collector_address = storage->addParameter("collector_address", "IP address of the Collector", types->getType("string"));
	infrastructure_model->setReference(collector_address, collector_storage);
	expected = std::dynamic_pointer_cast<OpenSANDConf::DataValue<bool>>(collector_address->getReferenceData());
	expected->set(true);

	auto collector_logs = storage->addParameter("collector_logs", "Port of the Collector Listening for Logs", types->getType("int"));
	infrastructure_model->setReference(collector_logs, collector_storage);
	expected = std::dynamic_pointer_cast<OpenSANDConf::DataValue<bool>>(collector_logs->getReferenceData());
	expected->set(true);
	collector_logs->setAdvanced(true);

	auto collector_probes = storage->addParameter("collector_probes", "Port of the Collector Listening for Probes", types->getType("int"));
	infrastructure_model->setReference(collector_probes, collector_storage);
	expected = std::dynamic_pointer_cast<OpenSANDConf::DataValue<bool>>(collector_probes->getReferenceData());
	expected->set(true);
	collector_probes->setAdvanced(true);

	auto entity = infrastructure_model->getRoot()->addComponent("entity", "Emulated Entity");
	auto entity_type = entity->addParameter("entity_type", "Entity Type", types->getType("entity_type"));
	std::vector<std::tuple<std::string, std::string>> entity_id_loop{
		{"gw", "Gateway"},
		{"gw_phy", "Gateway Phy"},
		{"gw_net_acc", "Gateway Net Access"},
		{"st", "Terminal"},
	};
	for (auto& item : entity_id_loop) {
		auto entity_id = entity->addParameter("entity_id_" + std::get<0>(item), std::get<1>(item) + " ID", types->getType("int"));
		infrastructure_model->setReference(entity_id, entity_type);
		auto expected = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(entity_id->getReferenceData());
		expected->set(std::get<1>(item));
	}

	auto infra = infrastructure_model->getRoot()->addComponent("infrastructure", "Infrastructure");
	auto satellite = infra->addComponent("satellite", "Satellite");
	satellite->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address this satellite should listen on for messages from ground entities");

	auto gateways = infra->addList("gateways", "Gateways", "gateway")->getPattern();
	gateways->addParameter("entity_id", "Entity ID", types->getType("int"));
	gateways->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address this gateway should listen on for messages from the satellite");
	gateways->addParameter("tap_iface", "TAP Interface", types->getType("string"), "Name of the TAP interface used by this gateway");
	gateways->addParameter("mac_address", "MAC Address", types->getType("string"), "MAC address this gateway routes traffic to");
	gateways->addParameter("ctrl_multicast_address", "Multicast IP Address (Control Messages)", types->getType("string"))->setAdvanced(true);
	gateways->addParameter("data_multicast_address", "Multicast IP Address (Data)", types->getType("string"))->setAdvanced(true);
	gateways->addParameter("ctrl_out_port", "Port (Control Messages Out)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("ctrl_in_port", "Port (Control Messages In)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("logon_out_port", "Port (Logon Messages Out)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("logon_in_port", "Port (Logon Messages In)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_out_st_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_in_st_port", "Port (Data In ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_out_gw_port", "Port (Data Out GW)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_in_gw_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("udp_stack", "UDP Stack", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("udp_rmem", "UDP RMem", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("udp_wmem", "UDP WMem", types->getType("int"))->setAdvanced(true);

	auto split_gateways_list = infra->addList("split_gateways", "Split Gateways", "split_gateway");
	split_gateways_list->setAdvanced(true);
	auto split_gateways = split_gateways_list->getPattern();
	split_gateways->addParameter("entity_id", "Entity ID", types->getType("int"));
	split_gateways->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address the physical layer gateway should listen on for messages from the satellite");
	split_gateways->addParameter("interco_net_access", "Interconnection Address (Net Access)", types->getType("string"), "Address the net access gateway should listen on for messages from the physical layer gateway");
	split_gateways->addParameter("interco_phy", "Interconnection Address (Phy)", types->getType("string"), "Address the physical layer gateway should listen on for messages from the net access gateway");
	split_gateways->addParameter("tap_iface", "TAP Interface", types->getType("string"), "Name of the TAP interface used by the net access gateway");
	split_gateways->addParameter("mac_address", "MAC Address", types->getType("string"), "MAC address the net access gateway routes traffic to");
	split_gateways->addParameter("ctrl_multicast_address", "Multicast IP Address (Control Messages)", types->getType("string"))->setAdvanced(true);
	split_gateways->addParameter("data_multicast_address", "Multicast IP Address (Data)", types->getType("string"))->setAdvanced(true);
	split_gateways->addParameter("ctrl_out_port", "Port (Control Messages Out)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("ctrl_in_port", "Port (Control Messages In)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("logon_out_port", "Port (Logon Messages Out)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("logon_in_port", "Port (Logon Messages In)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("data_out_st_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("data_in_st_port", "Port (Data In ST)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("data_out_gw_port", "Port (Data Out GW)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("data_in_gw_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("udp_stack", "UDP Stack", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("udp_rmem", "UDP RMem", types->getType("int"))->setAdvanced(true);
	split_gateways->addParameter("udp_wmem", "UDP WMem", types->getType("int"))->setAdvanced(true);

	auto terminals = infra->addList("terminals", "Terminals", "terminal")->getPattern();
	terminals->addParameter("entity_id", "Entity ID", types->getType("int"));
	terminals->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address this satellite terminal should listen on for messages from the satellite");
	terminals->addParameter("tap_iface", "TAP Interface", types->getType("string"), "Name of the TAP interface used by this satellite terminal");
	terminals->addParameter("mac_address", "MAC Address", types->getType("string"), "MAC address this satellite terminal routes traffic to");

	auto sarp = infrastructure_model->getRoot()->addComponent("sarp", "Sarp");
	sarp->setAdvanced(true);
	sarp->addParameter("default_gw", "Default Gateway", types->getType("int"), "Default Gateway ID for a packet destination when the MAC address is not found in the SARP Table; use -1 to drop such packets");

	auto ncc = infrastructure_model->getRoot()->addComponent("ncc", "NCC");
	ncc->setAdvanced(true);
	ncc->addParameter("pep_port", "PEP DAMA Port", types->getType("int"));
	ncc->addParameter("svno_port", "SVNO Port", types->getType("int"));


	topology_model = std::make_shared<OpenSANDConf::MetaModel>("1.0.0");
	topology_model->getRoot()->setDescription("topology");
	types = topology_model->getTypesDefinition();
	types->addEnumType("burst_length", "DVB-RCS2 Burst Length", {"536 sym", "1616 sym"});
	types->addEnumType("forward_type", "Forward Carrier Type", {"ACM", "VCM"});
	types->addEnumType("return_type", "Return Carrier Type", {"DAMA", "ALOHA", "SCPC"});
	types->addEnumType("carrier_group", "Carrier Group", {"Standard", "Premium", "Professional", "SVNO1", "SVNO2", "SVNO3", "SNO"});

	auto frequency_plan = topology_model->getRoot()->addComponent("frequency_plan", "Spots / Frequency Plan");
	auto spots = frequency_plan->addList("spots", "Spots", "spot_detail")->getPattern();
	auto spot_assignment = spots->addComponent("assignments", "Spot Assignment");
	spot_assignment->addParameter("spot_id", "Spot ID", types->getType("int"));
	spot_assignment->addParameter("gateway_id", "Gateway ID", types->getType("int"), "ID of the gateway this spot belongs to");
	auto roll_offs = spots->addComponent("roll_off", "Roll Off");
	roll_offs->addParameter("forward", "Forward Band Roll Off", types->getType("double"), "Usually 0.35, 0.25 or 0.2 for DVB-S2");
	roll_offs->addParameter("return", "Return Band Roll Off", types->getType("double"), "Usually 0.2 for DVB-RCS2");
	auto forward_band = spots->addList("forward_band", "Forward Band", "fwd_band")->getPattern();
	// forward_band->addParameter("carrier_id", "Carrier ID", types->getType("int"));
	forward_band->addParameter("symbol_rate", "Symbol Rate", types->getType("double"))->setUnit("Mbauds");
	auto band_type = forward_band->addParameter("type", "Type", types->getType("forward_type"));
	forward_band->addParameter("wave_form", "Wave Form IDs", types->getType("string"), "Supported Wave Forms. Use ';' separator for unique IDs, '-' separator for all the IDs between bounds");
	forward_band->addParameter("group", "Group", types->getType("carrier_group"));
	auto ratio = forward_band->addParameter("ratio", "Ratio", types->getType("string"), "Separate temporal division ratios by ','; you should also specify as many wave form IDs also separated by ','");
	topology_model->setReference(ratio, band_type);
	auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(ratio->getReferenceData());
	expected_str->set("VCM");
	auto return_band = spots->addList("return_band", "Return Band", "rtn_band")->getPattern();
	// return_band->addParameter("carrier_id", "Carrier ID", types->getType("int"));
	return_band->addParameter("symbol_rate", "Symbol Rate", types->getType("double"))->setUnit("Mbauds");
	return_band->addParameter("type", "Type", types->getType("return_type"));
	return_band->addParameter("wave_form", "Wave Forms", types->getType("string"));
	return_band->addParameter("group", "Group", types->getType("carrier_group"));

	auto st_assignment = topology_model->getRoot()->addComponent("st_assignment", "Satellite Terminal Assignment");
	auto defaults = st_assignment->addComponent("defaults", "Default Settings");
	defaults->addParameter("default_spot", "Spot", types->getType("int"));
	defaults->addParameter("default_group", "Group", types->getType("carrier_group"));
	auto assignments = st_assignment->addList("assignments", "Additional Assignments", "assigned")->getPattern();
	assignments->addParameter("terminal_id", "Terminal ID", types->getType("int"));
	assignments->addParameter("spot_id", "Spot ID", types->getType("int"));
	assignments->addParameter("group", "Group", types->getType("carrier_group"));

	auto wave_forms = topology_model->getRoot()->addComponent("wave_forms", "Wave Forms");
	auto dvb_s2 = wave_forms->addList("dvb_s2", "DVB-S2 Wave Forms", "dvb_s2_waveforms")->getPattern();
	dvb_s2->addParameter("id", "Wave Form ID", types->getType("int"));
	dvb_s2->addParameter("modulation", "Modulation", types->getType("modulation"));
	dvb_s2->addParameter("coding", "Coding Rate", types->getType("coding"));
	dvb_s2->addParameter("efficiency", "Spectral Efficiency", types->getType("double"));
	dvb_s2->addParameter("threshold", "Required Es/N0", types->getType("double"))->setUnit("dB");
	auto dvb_rcs2 = wave_forms->addList("dvb_rcs2", "DVB-RCS2 Wave Forms", "dvb_rcs2_waveforms")->getPattern();
	dvb_rcs2->addParameter("id", "Wave Form ID", types->getType("int"));
	dvb_rcs2->addParameter("modulation", "Modulation", types->getType("modulation"));
	dvb_rcs2->addParameter("coding", "Coding Rate", types->getType("coding"));
	dvb_rcs2->addParameter("efficiency", "Spectral Efficiency", types->getType("double"));
	dvb_rcs2->addParameter("threshold", "Required Es/N0", types->getType("double"))->setUnit("dB");
	dvb_rcs2->addParameter("burst_length", "Burst Length", types->getType("burst_length"));

	auto advanced = topology_model->getRoot()->addComponent("advanced_settings", "Advanced Settings");
	advanced->setAdvanced(true);
	auto links = advanced->addComponent("links", "Links");
	links->addParameter("forward_duration", "Forward link frame duration", types->getType("int"))->setUnit("ms"); 
	links->addParameter("forward_margin", "Forward link ACM loop margin", types->getType("double"))->setUnit("dB");
	// auto forward_encap = links->addList("forward_encap_schemes", "Forward link Encapsulation Schemes", "forward_encap_scheme")->getPattern();
	links->addParameter("return_duration", "Return link frame duration", types->getType("int"))->setUnit("ms"); 
	links->addParameter("return_margin", "Return link ACM loop margin", types->getType("double"))->setUnit("dB");
	// auto return_encap = links->addList("return_encap_schemes", "Forward link Encapsulation Schemes", "return_encap_scheme")->getPattern();
	auto schedulers = advanced->addComponent("schedulers", "Schedulers");
	schedulers->addParameter("burst_length", "DVB-RCS2 Burst Length", types->getType("burst_length"));
	schedulers->addParameter("crdsa_frame", "CRDSA Frame", types->getType("int"))->setUnit("DVB-RCS2 SuperFrames");
	schedulers->addParameter("crdsa_delay", "CRDSA Max Satellite Delay", types->getType("int"))->setUnit("ms");
	schedulers->addParameter("pep_allocation", "PEP Allocation Delay", types->getType("int"))->setUnit("ms");
	auto timers = advanced->addComponent("timers", "Timers");
	timers->addParameter("statistics", "Statistics Timer", types->getType("int"))->setUnit("ms");
	timers->addParameter("synchro", "Sync Period", types->getType("int"))->setUnit("ms");
	timers->addParameter("acm_refresh", "ACM Refresh Period", types->getType("int"))->setUnit("ms");
	auto delay = advanced->addComponent("delay", "Delay");
	delay->addParameter("fifo_size", "Buffer Size", types->getType("int"),
	                    "Amount of packets that can be stored at once in the "
	                    "delay FIFO before being sent through the physical "
	                    "channels; acts as default values for the Gateways "
	                    "channels FIFO sizes if not specified")->setUnit("packets");
	delay->addParameter("delay_timer", "Timer", types->getType("int"));

	profile_model = std::make_shared<OpenSANDConf::MetaModel>("1.0.0");
	profile_model->getRoot()->setDescription("profile");
}


std::shared_ptr<OpenSANDConf::DataComponent> OpenSandModelConf::getProfileData(const std::string &path) const
{
	if (profile == nullptr)
	{
		return nullptr;
	}

	if (path.empty())
	{
		return profile->getRoot();
	}

	return std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(profile->getItemByPath(path));
}


std::shared_ptr<OpenSANDConf::MetaTypesList> OpenSandModelConf::getModelTypesDefinition() const
{
	if (profile_model == nullptr)
	{
		return nullptr;
	}

	return profile_model->getTypesDefinition();
}


std::shared_ptr<OpenSANDConf::MetaComponent> OpenSandModelConf::getOrCreateComponent(
		const std::string& id,
		const std::string& name,
		std::shared_ptr<OpenSANDConf::MetaComponent> from)
{
	return getOrCreateComponent(id, name, "", from);
}


std::shared_ptr<OpenSANDConf::MetaComponent> OpenSandModelConf::getOrCreateComponent(
		const std::string& id,
		const std::string& name,
		const std::string& description,
		std::shared_ptr<OpenSANDConf::MetaComponent> from)
{
	if (from == nullptr && profile_model == nullptr)
	{
		createModels();
	}

	auto parent = from == nullptr ? profile_model->getRoot() : from;
	auto child = parent->getComponent(id);
	if (child == nullptr) {
		child = parent->addComponent(id, name, description);
	}
	return child;
}


std::shared_ptr<OpenSANDConf::MetaComponent> OpenSandModelConf::getComponentByPath(
		const std::string &path,
		std::shared_ptr<OpenSANDConf::MetaModel> from)
{
	if (from == nullptr && profile_model == nullptr)
	{
		createModels();
	}

	auto element = (from == nullptr ? profile_model : from)->getItemByPath(path);
	return std::dynamic_pointer_cast<OpenSANDConf::MetaComponent>(element);
}


void OpenSandModelConf::setProfileReference(std::shared_ptr<OpenSANDConf::MetaParameter> parameter,
                                            std::shared_ptr<OpenSANDConf::MetaParameter> referee,
											const std::string &expected_value)
{
	if (profile_model == nullptr)
	{
		return;
	}

	profile_model->setReference(parameter, referee);
	auto expected = parameter->getReferenceData();
	std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(expected)->set(expected_value);
}


void OpenSandModelConf::setProfileReference(std::shared_ptr<OpenSANDConf::MetaParameter> parameter,
                                            std::shared_ptr<OpenSANDConf::MetaParameter> referee,
											bool expected_value)
{
	if (profile_model == nullptr)
	{
		return;
	}

	profile_model->setReference(parameter, referee);
	auto expected = parameter->getReferenceData();
	std::dynamic_pointer_cast<OpenSANDConf::DataValue<bool>>(expected)->set(expected_value);
}


bool OpenSandModelConf::writeTopologyModel(const std::string& filename) const
{
	if (topology_model == nullptr)
	{
		return false;
	}

	return OpenSANDConf::toXSD(topology_model, filename);
}


bool OpenSandModelConf::writeInfrastructureModel(const std::string& filename) const
{
	if (infrastructure_model == nullptr)
	{
		return false;
	}

	return OpenSANDConf::toXSD(infrastructure_model, filename);
}


bool OpenSandModelConf::writeProfileModel(const std::string& filename) const
{
	if (profile_model == nullptr)
	{
		return false;
	}

	return OpenSANDConf::toXSD(profile_model, filename);
}


bool OpenSandModelConf::readTopology(const std::string& filename)
{
	if (topology_model == nullptr)
	{
		createModels();
	}

	topology = OpenSANDConf::fromXML(topology_model, filename);
	return topology != nullptr;
}


bool OpenSandModelConf::readInfrastructure(const std::string& filename)
{
	if (infrastructure_model == nullptr)
	{
		createModels();
	}

	infrastructure = OpenSANDConf::fromXML(infrastructure_model, filename);
	return infrastructure != nullptr;
}


bool OpenSandModelConf::readProfile(const std::string& filename)
{
	if (profile_model == nullptr)
	{
		createModels();
	}

	profile = OpenSANDConf::fromXML(profile_model, filename);
	return profile != nullptr;
}


component_t OpenSandModelConf::getComponentType() const
{
	if (infrastructure == nullptr) {
		return unknown_compo;
	}

	std::string component_type;
	extractParameterData(infrastructure->getRoot()->getComponent("entity")->getParameter("entity_type"), component_type);

	if (component_type == "Satellite") {
		return satellite;
	} else if (component_type == "Terminal") {
		return terminal;
	} else if (component_type == "Gateway" || component_type == "Gateway Net Access" || component_type == "Gateway Phy") {
		return gateway;
	} else {
		return unknown_compo;
	}
}


bool OpenSandModelConf::getComponentType(std::string &type, tal_id_t &id) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto entity = infrastructure->getRoot()->getComponent("entity");
	std::string component_type;
	extractParameterData(entity->getParameter("entity_type"), component_type);

	if (component_type == "Satellite") {
		type = "sat";
		return true;
	} else if (component_type == "Terminal") {
		type = "st";
	} else if (component_type == "Gateway") {
		type = "gw";
	} else if (component_type == "Gateway Net Access") {
		type = "gw_net_acc";
	} else if (component_type == "Gateway Phy") {
		type = "gw_phy";
	} else {
		return false;
	}

	int entity_id;
	if (!extractParameterData(entity->getParameter("entity_id_" + type), entity_id)) {
		return false;
	}

	id = entity_id;
	return true;
}


bool OpenSandModelConf::getSatInfrastructure(std::string &ip_address) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto satellite = infrastructure->getRoot()->getComponent("infrastructure")->getComponent("satellite");
	return extractParameterData(satellite->getParameter("emu_address"), ip_address);
}


bool OpenSandModelConf::getGwInfrastructure(tal_id_t id, std::string &ip_address, std::string &tap_iface) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	for (auto& gateway_item : infrastructure->getRoot()->getComponent("infrastructure")->getList("gateways")->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(gateway_item);

		int gateway_id = -id;
		if (!extractParameterData(gateway->getParameter("entity_id"), gateway_id)) {
			continue;
		}

		if (gateway_id == id) {
			if (!extractParameterData(gateway->getParameter("emu_address"), ip_address)) {
				return false;
			}
			if (!extractParameterData(gateway->getParameter("tap_iface"), tap_iface)) {
				return false;
			}
			return true;
		}
	}

	return false;
}


bool OpenSandModelConf::getSplitGwInfrastructure(tal_id_t id, std::string& ip_address,
                                                 std::string &interconnect_phy,
                                                 std::string &interconnect_net_access,
                                                 std::string &tap_iface) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	for (auto& gateway_item : infrastructure->getRoot()->getComponent("infrastructure")->getList("split_gateways")->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(gateway_item);

		int gateway_id = -id;
		if (!extractParameterData(gateway->getParameter("entity_id"), gateway_id)) {
			continue;
		}

		if (gateway_id == id) {
			if (!extractParameterData(gateway->getParameter("emu_address"), ip_address)) {
				return false;
			}
			if (!extractParameterData(gateway->getParameter("interco_phy"), interconnect_phy)) {
				return false;
			}
			if (!extractParameterData(gateway->getParameter("interco_net_access"), interconnect_net_access)) {
				return false;
			}
			if (!extractParameterData(gateway->getParameter("tap_iface"), tap_iface)) {
				return false;
			}
			return true;
		}
	}

	return false;
}


bool OpenSandModelConf::getStInfrastructure(tal_id_t id, std::string &ip_address, std::string &tap_iface) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	for (auto& terminal_item : infrastructure->getRoot()->getComponent("infrastructure")->getList("terminals")->getItems()) {
		auto terminal = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(terminal_item);

		int terminal_id = -id;
		if (!extractParameterData(terminal->getParameter("entity_id"), terminal_id)) {
			continue;
		}

		if (terminal_id == id) {
			if (!extractParameterData(terminal->getParameter("emu_address"), ip_address)) {
				return false;
			}
			if (!extractParameterData(terminal->getParameter("tap_iface"), tap_iface)) {
				return false;
			}
			return true;
		}
	}

	return false;
}


bool OpenSandModelConf::getLocalStorage(bool &enabled, std::string &output_folder) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto storage = infrastructure->getRoot()->getComponent("storage");
	if (!extractParameterData(storage->getParameter("enable_local"), enabled)) {
		return false;
	}
	if (enabled && !extractParameterData(storage->getParameter("path_local"), output_folder)) {
		return false;
	}
	return true;
}


bool OpenSandModelConf::getRemoteStorage(bool &enabled, std::string &address, unsigned short &stats_port, unsigned short &logs_port) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto storage = infrastructure->getRoot()->getComponent("storage");
	if (!extractParameterData(storage->getParameter("enable_collector"), enabled)) {
		return false;
	}
	if (!enabled) {
		return true;
	}

	if (!extractParameterData(storage->getParameter("collector_address"), address)) {
		return false;
	}

	int stats = 5361;
	extractParameterData(storage->getParameter("collector_probes"), stats);

	int logs = 5362;
	extractParameterData(storage->getParameter("collector_logs"), logs);

	stats_port = stats;
	logs_port = logs;
	return true;
}


bool OpenSandModelConf::getGwIds(std::vector<tal_id_t> &gws) const
{
	if (infrastructure != nullptr) {
		return false;
	}

	for (auto& entity_element : infrastructure->getRoot()->getList("gateways")->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id;
		if (!extractParameterData(gateway->getParameter("entity_id"), gateway_id)) {
			return false;
		}
		gws.push_back(gateway_id);
	}

	for (auto& entity_element : infrastructure->getRoot()->getList("split_gateways")->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id;
		if (!extractParameterData(gateway->getParameter("entity_id"), gateway_id)) {
				return false;
		}
		gws.push_back(gateway_id);
	}

	return true;
}


bool OpenSandModelConf::logLevels(std::map<std::string, log_level_t> &levels,
                                  std::map<std::string, log_level_t> &specific) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto log_levels = infrastructure->getRoot()->getComponent("logs");
	static std::vector<std::string> log_names_loop{
		{"init"},
		{"lan_adaptation"},
		{"encap"},
		{"dvb"},
		{"physical_layer"},
		{"sat_carrier"},
	};
	for (auto& log_name : log_names_loop) {
		std::string log_level;
		if (!extractParameterData(log_levels->getComponent(log_name)->getParameter("level"), log_level)) {
			return false;
		}
		auto iterator = levels_map.find(log_level);
		if (iterator == levels_map.end()) {
			return false;
		}
		levels[log_name] = iterator->second;
	}

	for (auto& log_item : log_levels->getList("extra_levels")->getItems()) {
		auto log = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(log_item);
		std::string log_name;
		if (!extractParameterData(log->getParameter("name"), log_name)) {
			return false;
		}
		std::string log_level;
		if (!extractParameterData(log->getParameter("level"), log_level)) {
			return false;
		}
		auto iterator = levels_map.find(log_level);
		if (iterator == levels_map.end()) {
			return false;
		}
		specific[log_name] = iterator->second;
	}

	return true;
}


bool OpenSandModelConf::getSarp(SarpTable& sarp_table) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	int default_gw = -1;
	extractParameterData(infrastructure->getRoot()->getComponent("sarp")->getParameter("default_gw"), default_gw);
	sarp_table.setDefaultTal(default_gw);

	static std::vector<std::string> list_names{"gateways", "split_gateways", "terminals"};
	for (auto& list_name : list_names) {
		for (auto& entity_element : infrastructure->getRoot()->getList(list_name)->getItems()) {
			auto entity = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);

			int entity_id;
			if (!extractParameterData(entity->getParameter("entity_id"), entity_id)) {
				return false;
			}

			std::string mac_address;
			if (!extractParameterData(entity->getParameter("mac_address"), mac_address)) {
				return false;
			}

			sarp_table.add(new MacAddress(mac_address), entity_id);
		}
	}

	return true;
}


bool OpenSandModelConf::getNccPorts(int &pep_tcp_port, int &svno_tcp_port) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	// Default values
	pep_tcp_port = 4998;
	svno_tcp_port = 4999;

	auto ncc = infrastructure->getRoot()->getComponent("ncc");
	extractParameterData(ncc->getParameter("pep_port"), pep_tcp_port);
	extractParameterData(ncc->getParameter("svno_port"), svno_tcp_port);
	return true;
}


bool OpenSandModelConf::getS2WaveFormsDefinition(std::vector<fmt_definition_parameters> &fmt_definitions) const
{
	if(topology == nullptr) {
		return false;
	}

	auto waveforms = topology->getRoot()->getComponent("wave_forms")->getList("dvb_s2");
	for (auto& waveform_item : waveforms->getItems()) {
		auto waveform = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(waveform_item);

		int scheme_number;
		if (!extractParameterData(waveform->getParameter("id"), scheme_number)) {
			return false;
		}
		if (scheme_number <= 0) {
			return false;
		}

		std::string modulation;
		if (!extractParameterData(waveform->getParameter("modulation"), modulation)) {
			return false;
		}

		std::string coding;
		if (!extractParameterData(waveform->getParameter("coding"), coding)) {
			return false;
		}

		double spectral_efficiency;
		if (!extractParameterData(waveform->getParameter("efficiency"), spectral_efficiency)) {
			return false;
		}

		double threshold;
		if (!extractParameterData(waveform->getParameter("threshold"), threshold)) {
			return false;
		}

		fmt_definition_parameters params{scheme_number,
		                                 modulation,
		                                 coding,
		                                 spectral_efficiency,
		                                 threshold};
		fmt_definitions.push_back(params);
	}

	return true;
}


bool OpenSandModelConf::getRcs2WaveFormsDefinition(std::vector<fmt_definition_parameters> &fmt_definitions, vol_sym_t req_burst_length) const
{
	if(topology == nullptr) {
		return false;
	}

	auto waveforms = topology->getRoot()->getComponent("wave_forms")->getList("dvb_rcs2");
	for (auto& waveform_item : waveforms->getItems()) {
		auto waveform = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(waveform_item);

		std::string burst_length;
		if (!extractParameterData(waveform->getParameter("burst_length"), burst_length)) {
			return false;
		}

		std::stringstream parser(burst_length);
		vol_sym_t defined_burst_length;
		parser >> defined_burst_length;
		if (parser.fail()) {
			return false;
		}

		if (defined_burst_length != req_burst_length) {
			continue;
		}

		int scheme_number;
		if (!extractParameterData(waveform->getParameter("id"), scheme_number)) {
			return false;
		}
		if (scheme_number <= 0) {
			return false;
		}

		std::string modulation;
		if (!extractParameterData(waveform->getParameter("modulation"), modulation)) {
			return false;
		}

		std::string coding;
		if (!extractParameterData(waveform->getParameter("coding"), coding)) {
			return false;
		}

		double spectral_efficiency;
		if (!extractParameterData(waveform->getParameter("efficiency"), spectral_efficiency)) {
			return false;
		}

		double threshold;
		if (!extractParameterData(waveform->getParameter("threshold"), threshold)) {
			return false;
		}

		fmt_definition_parameters params{scheme_number,
		                                 modulation,
		                                 coding,
		                                 spectral_efficiency,
		                                 threshold};
		fmt_definitions.push_back(params);
	}

	return true;
}


bool OpenSandModelConf::getRcs2BurstLength(vol_sym_t &length_sym) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("schedulers");
	std::string burst_length;
	if (!extractParameterData(schedulers->getParameter("burst_length"), burst_length)) {
		return false;
	}

	std::stringstream parser(burst_length);
	parser >> length_sym;
	return !parser.fail();
}


bool OpenSandModelConf::getSuperframePerSlottedAlohaFrame(time_sf_t &sf_per_saframe) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("schedulers");
	int value;
	if (!extractParameterData(schedulers->getParameter("crdsa_frame"), value)) {
		return false;
	}

	sf_per_saframe = value;
	return true;
}


bool OpenSandModelConf::getCrdsaMaxSatelliteDelay(time_ms_t &sat_delay) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("schedulers");
	int value;
	if (!extractParameterData(schedulers->getParameter("crdsa_delay"), value)) {
		return false;
	}

	sat_delay = value;
	return true;
}


bool OpenSandModelConf::getPepAllocationDelay(int &pep_allocation_delay) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("schedulers");
	return extractParameterData(schedulers->getParameter("pep_allocation"), pep_allocation_delay);
}


template<typename T>
bool getAdvancedLinksParameter(std::shared_ptr<OpenSANDConf::DataModel> model, const std::string& parameter, T &result)
{
	if (model == nullptr) {
		return false;
	}

	auto schedulers = model->getRoot()->getComponent("advanced_settings")->getComponent("links");
	return OpenSandModelConf::extractParameterData(schedulers->getParameter(parameter), result);
}


bool OpenSandModelConf::getReturnFrameDuration(time_ms_t &frame_duration) const
{
	int value;
	if (!getAdvancedLinksParameter(topology, "return_duration", value)) {
		return false;
	}

	frame_duration = value;
	return true;
}


bool OpenSandModelConf::getForwardFrameDuration(time_ms_t &frame_duration) const
{
	int value;
	if (!getAdvancedLinksParameter(topology, "forward_duration", value)) {
		return false;
	}

	frame_duration = value;
	return true;
}


bool OpenSandModelConf::getReturnAcmLoopMargin(double &margin) const
{
	return getAdvancedLinksParameter(topology, "return_margin", margin);
}


bool OpenSandModelConf::getForwardAcmLoopMargin(double &margin) const
{
	return getAdvancedLinksParameter(topology, "forward_margin", margin);
}


bool OpenSandModelConf::getStatisticsPeriod(time_ms_t &period) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("timers");
	int value;
	if (!extractParameterData(schedulers->getParameter("statistics"), value)) {
		return false;
	}

	period = value;
	return true;
}


bool OpenSandModelConf::getSynchroPeriod(time_ms_t &period) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("timers");
	int value;
	if (!extractParameterData(schedulers->getParameter("synchro"), value)) {
		return false;
	}

	period = value;
	return true;
}


bool OpenSandModelConf::getAcmRefreshPeriod(time_ms_t &period) const
{
	if (topology == nullptr) {
		return false;
	}

	auto schedulers = topology->getRoot()->getComponent("advanced_settings")->getComponent("timers");
	int value;
	if (!extractParameterData(schedulers->getParameter("acm_refresh"), value)) {
		return false;
	}

	period = value;
	return true;
}


bool OpenSandModelConf::getDelayBufferSize(std::size_t &size) const
{
	if (topology == nullptr) {
		return false;
	}

	auto delay = topology->getRoot()->getComponent("advanced_settings")->getComponent("delay");
	int value;
	if (!extractParameterData(delay->getParameter("fifo_size"), value)) {
		return false;
	}

	size = value;
	return true;
}


bool OpenSandModelConf::getDelayTimer(time_ms_t &period) const
{
	if (topology == nullptr) {
		return false;
	}

	auto delay = topology->getRoot()->getComponent("advanced_settings")->getComponent("delay");
	int value;
	if (!extractParameterData(delay->getParameter("delay_timer"), value)) {
		return false;
	}

	period = value;
	return true;
}


bool OpenSandModelConf::getGwWithTalId(uint16_t tal_id, uint16_t &gw_id) const
{
	if (topology == nullptr) {
		return false;
	}

	auto st_assignments = topology->getRoot()->getComponent("st_assignment");
	auto assigned_spot = st_assignments->getComponent("defaults")->getParameter("default_spot");

	for (auto& assignment : st_assignments->getList("assignments")->getItems()) {
		auto st_assignment = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(assignment);
		int st_id;
		if (!extractParameterData(st_assignment->getParameter("terminal_id"), st_id)) {
			return false;
		}
		if (tal_id == st_id) {
			assigned_spot = st_assignment->getParameter("spot_id");
		}
	}

	int assigned_spot_id;
	if (!extractParameterData(assigned_spot, assigned_spot_id)) {
		return false;
	}
	return getGwWithCarrierId(assigned_spot_id * 10, gw_id);
}


bool OpenSandModelConf::getGwWithCarrierId(unsigned int car_id, uint16_t &gw) const
{
	if (topology == nullptr) {
		return false;
	}

	unsigned int assigned_spot_id = car_id / 10;

	for (auto& spot : topology->getRoot()->getComponent("frequency_plan")->getList("spots")->getItems()) {
		auto gw_assignment = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(spot)->getComponent("assignments");
		int spot_id;
		if (!extractParameterData(gw_assignment->getParameter("spot_id"), spot_id)) {
			return false;
		}
		if (assigned_spot_id == spot_id) {
			int assigned_gw;
			if (!extractParameterData(gw_assignment->getParameter("gateway_id"), assigned_gw)) {
				return false;
			}

			gw = assigned_gw;
			return true;
		}
	}

	return false;
}


bool OpenSandModelConf::isGw(uint16_t gw_id) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	for (auto& entity_element : infrastructure->getRoot()->getList("gateways")->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id = -gw_id;
		extractParameterData(gateway->getParameter("entity_id"), gateway_id);
		if (gateway_id == gw_id) {
			return true;
		}
	}

	for (auto& entity_element : infrastructure->getRoot()->getList("split_gateways")->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id = -gw_id;
		extractParameterData(gateway->getParameter("entity_id"), gateway_id);
		if (gateway_id == gw_id) {
			return true;
		}
	}

	return false;
}


bool OpenSandModelConf::getScpcEncapStack(std::vector<std::string> &encap_stack) const
{
	// encap_stack = std::vector<std::string>{"RLE", "GSE"};
	encap_stack = std::vector<std::string>{"GSE"};
	return true;
}


bool OpenSandModelConf::getSpotInfrastructure(uint16_t gw_id, spot_infrastructure &carriers) const
{
	if (infrastructure == nullptr || topology == nullptr) {
		return false;
	}

	auto satellite = infrastructure->getRoot()->getComponent("satellite");
	std::string satellite_address;
	if (!extractParameterData(satellite->getParameter("emu_address"), satellite_address)) {
		return false;
	}

	std::shared_ptr<OpenSANDConf::DataComponent> gateway = nullptr;
	for (auto& entity_element : infrastructure->getRoot()->getList("gateways")->getItems()) {
		auto possible_gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id;
		if (!extractParameterData(possible_gateway->getParameter("entity_id"), gateway_id)) {
			return false;
		}
		if (gateway_id == gw_id) {
			gateway = possible_gateway;
		}
	}

	for (auto& entity_element : infrastructure->getRoot()->getList("split_gateways")->getItems()) {
		auto possible_gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id;
		if (!extractParameterData(possible_gateway->getParameter("entity_id"), gateway_id)) {
			return false;
		}
		if (gateway_id == gw_id) {
			if (gateway != nullptr) {
				return false;
			}
			gateway = possible_gateway;
		}
	}

	if (gateway == nullptr) {
		return false;
	}

	for (auto& spot : topology->getRoot()->getComponent("frequency_plan")->getList("spots")->getItems()) {
		auto gw_assignment = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(spot)->getComponent("assignments");
		int assigned_gw;
		if (!extractParameterData(gw_assignment->getParameter("gateway_id"), assigned_gw)) {
			return false;
		}
		if (assigned_gw == gw_id) {
			int spot_id;
			if (!extractParameterData(gw_assignment->getParameter("spot_id"), spot_id)) {
				return false;
			}
			uint16_t carrier_id = spot_id * 10;

			std::string gateway_address;
			if (!extractParameterData(gateway->getParameter("emu_address"), gateway_address)) {
				return false;
			}

			std::string ctrl_multicast_address = "239.137.194." + std::to_string(220 + spot_id * 2);
			extractParameterData(gateway->getParameter("ctrl_multicast_address"), ctrl_multicast_address);
			std::string data_multicast_address = "239.137.194." + std::to_string(221 + spot_id * 2);
			extractParameterData(gateway->getParameter("data_multicast_address"), data_multicast_address);

			int ctrl_out_port = 55000 + carrier_id;
			extractParameterData(gateway->getParameter("ctrl_out_port"), ctrl_out_port);
			int ctrl_in_port = 55001 + carrier_id;
			extractParameterData(gateway->getParameter("ctrl_in_port"), ctrl_in_port);
			int logon_out_port = 55002 + carrier_id;
			extractParameterData(gateway->getParameter("logon_out_port"), logon_out_port);
			int logon_in_port = 55003 + carrier_id;
			extractParameterData(gateway->getParameter("logon_in_port"), logon_in_port);
			int data_out_st_port = 55004 + carrier_id;
			extractParameterData(gateway->getParameter("data_out_st_port"), data_out_st_port);
			int data_in_st_port = 55005 + carrier_id;
			extractParameterData(gateway->getParameter("data_in_st_port"), data_in_st_port);
			int data_out_gw_port = 55006 + carrier_id;
			extractParameterData(gateway->getParameter("data_out_gw_port"), data_out_gw_port);
			int data_in_gw_port = 55007 + carrier_id;
			extractParameterData(gateway->getParameter("data_in_gw_port"), data_in_gw_port);

			int udp_stack = 5;
			extractParameterData(gateway->getParameter("udp_stack"), udp_stack);
			int udp_rmem = 1048580;
			extractParameterData(gateway->getParameter("udp_rmem"), udp_rmem);
			int udp_wmem = 1048580;
			extractParameterData(gateway->getParameter("udp_wmem"), udp_wmem);

			int fifo_sizes = 0;
			// TODO
			// extractParameterData(gateway->getParameter("fifos_size"), fifo_sizes);
			bool individual_fifos = false;
			// TODO
			// extractParameterData(gateway->getParameter("individual_fifo_sizes"), individual_fifos);

			int ctrl_out_fifo_size = fifo_sizes;
			int ctrl_in_fifo_size = fifo_sizes;
			int logon_out_fifo_size = fifo_sizes;
			int logon_in_fifo_size = fifo_sizes;
			int data_out_st_fifo_size = fifo_sizes;
			int data_in_st_fifo_size = fifo_sizes;
			int data_out_gw_fifo_size = fifo_sizes;
			int data_in_gw_fifo_size = fifo_sizes;
			if (individual_fifos) {
				// TODO
				// extractParameterData(gateway->getParameter("ctrl_out_fifo_size"), ctrl_out_fifo_size);
				// extractParameterData(gateway->getParameter("ctrl_in_fifo_size"), ctrl_in_fifo_size);
				// extractParameterData(gateway->getParameter("logon_out_fifo_size"), logon_out_fifo_size);
				// extractParameterData(gateway->getParameter("logon_in_fifo_size"), logon_in_fifo_size);
				// extractParameterData(gateway->getParameter("data_out_st_fifo_size"), data_out_st_fifo_size);
				// extractParameterData(gateway->getParameter("data_in_st_fifo_size"), data_in_st_fifo_size);
				// extractParameterData(gateway->getParameter("data_out_gw_fifo_size"), data_out_gw_fifo_size);
				// extractParameterData(gateway->getParameter("data_in_gw_fifo_size"), data_in_gw_fifo_size);
			}

			carriers.ctrl_out = carrier_socket{
			    carrier_id + 0,
			    ctrl_multicast_address,
			    ctrl_out_port,
			    true,
			    ctrl_out_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.ctrl_in = carrier_socket{
			    carrier_id + 1,
			    satellite_address,
			    ctrl_in_port,
			    false,
			    ctrl_in_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.logon_out = carrier_socket{
			    carrier_id + 2,
			    gateway_address,
			    logon_out_port,
			    false,
			    logon_out_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.logon_in = carrier_socket{
			    carrier_id + 3,
			    satellite_address,
			    logon_in_port,
			    false,
			    logon_in_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.data_out_st = carrier_socket{
			    carrier_id + 4,
			    data_multicast_address,
			    data_out_st_port,
			    true,
			    data_out_st_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.data_in_st = carrier_socket{
			    carrier_id + 5,
			    satellite_address,
			    data_in_st_port,
			    false,
			    data_in_st_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.data_out_gw = carrier_socket{
			    carrier_id + 6,
			    gateway_address,
			    data_out_gw_port,
			    false,
			    data_out_gw_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};
			carriers.data_in_gw = carrier_socket{
			    carrier_id + 7,
			    satellite_address,
			    data_in_gw_port,
			    false,
			    data_in_gw_fifo_size,
			    udp_stack,
			    udp_rmem,
			    udp_wmem
			};

			return true;
		}
	}
	return false;
}


bool OpenSandModelConf::getSpotForwardCarriers(uint16_t gw_id, OpenSandModelConf::spot &spot) const
{
	return getSpotCarriers(gw_id, spot, true);
}


bool OpenSandModelConf::getSpotReturnCarriers(uint16_t gw_id, OpenSandModelConf::spot &spot) const
{
	return getSpotCarriers(gw_id, spot, false);
}

bool OpenSandModelConf::getSpotCarriers(uint16_t gw_id, OpenSandModelConf::spot &spot, bool forward) const
{
	const std::string roll_off_parameter = forward ? "forward" : "return";
	const std::string band_parameter = roll_off_parameter + "_band";

	if (topology == nullptr) {
		return false;
	}

	std::shared_ptr<OpenSANDConf::DataComponent> selected_spot = nullptr;
	for (auto& spot_topology_item : topology->getRoot()->getComponent("frequency_plan")->getList("spots")->getItems()) {
		auto spot_topology = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(spot_topology_item);
		auto spot_assignment = spot_topology->getComponent("assignments");
		int gateway_id;
		if (!extractParameterData(spot_assignment->getParameter("gateway_id"), gateway_id)) {
			return false;
		}
		if (gateway_id == gw_id) {
			selected_spot = spot_assignment;
			break;
		}
	}

	if (selected_spot == nullptr) {
		return false;
	}

	int spot_id;
	if (!extractParameterData(selected_spot->getComponent("assignments")->getParameter("spot_id"), spot_id)) {
		return false;
	}

	if (!extractParameterData(selected_spot->getComponent("roll_off")->getParameter(roll_off_parameter), spot.roll_off)) {
		return false;
	}

	freq_khz_t total_bandwidth = 0;
	std::vector<OpenSandModelConf::carrier> carriers;
	for (auto& carrier_item : selected_spot->getList(band_parameter)->getItems()) {
		auto carrier = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(carrier_item);

		double symbol_rate;
		if (!extractParameterData(carrier->getParameter("symbol_rate"), symbol_rate)) {
			return false;
		}

		std::string category;
		if (!extractParameterData(carrier->getParameter("group"), category)) {
			return false;
		}

		std::string access_type;
		if (!extractParameterData(carrier->getParameter("type"), access_type)) {
			return false;
		}

		std::string format_ids;
		if (!extractParameterData(carrier->getParameter("wave_form"), format_ids)) {
			return false;
		}

		std::string ratio = "1000";
		if (access_type == "VCM" && !extractParameterData(carrier->getParameter("ratio"), ratio)) {
			return false;
		}

		std::vector<std::string> tokens;
		tokenize(ratio, tokens, ",;- ");

		std::vector<std::string> formats;
		tokenize(format_ids, formats, ",");

		if (tokens.size() != formats.size()) {
			return false;
		}

		unsigned int total_ratio = 0;
		std::map<std::string, unsigned int> format_ratios;
		auto iterator = formats.begin();
		for (auto& token : tokens) {
			std::stringstream parser(token);
			unsigned int value;
			parser >> value;
			if (parser.fail()) {
				return false;
			}
			format_ratios[*iterator++] = value;
			total_ratio += value;
		}

		// Ensure all ratios sum up to 1000 so we can force 1 carrier per carrier
		for (auto& format_ratio : format_ratios) {
			format_ratio.second = 1000 * format_ratio.second / total_ratio;
		}

		freq_khz_t bandwidth = symbol_rate * (spot.roll_off + 1) / 1000;
		total_bandwidth += bandwidth;
		carriers.push_back(OpenSandModelConf::carrier{strToAccessType(access_type), category, symbol_rate, format_ratios, bandwidth});
	}

	spot.bandwidth_khz = total_bandwidth;
	spot.carriers = carriers;

	return true;
}


bool OpenSandModelConf::getTerminalAffectation(spot_id_t &default_spot_id,
                                               std::string &default_category_name,
                                               std::map<tal_id_t, std::pair<spot_id_t, std::string>> &terminal_categories) const
{
	if (topology == nullptr) {
		return false;
	}

	auto assignments = topology->getRoot()->getComponent("st_assignment");

	int spot_id;
	if (!extractParameterData(assignments->getParameter("default_spot"), spot_id)) {
		return false;
	}
	default_spot_id = spot_id;

	std::string category;
	if (!extractParameterData(assignments->getParameter("default_group"), category)) {
		return false;
	}
	default_category_name = category;


	for (auto& terminal_assignment : assignments->getList("assignments")->getItems()) {
		auto terminal = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(terminal_assignment);

		int terminal_id;
		if (!extractParameterData(terminal->getParameter("terminal_id"), terminal_id)) {
			return false;
		}
		if (!extractParameterData(terminal->getParameter("spot_id"), spot_id)) {
			return false;
		}
		if (!extractParameterData(terminal->getParameter("group"), category)) {
			return false;
		}
		auto result = terminal_categories.emplace(terminal_id, std::make_pair(spot_id, category));
		if (!result.second) {
			return false;
		}
	}

	return true;
}
