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
#include "CarrierType.h"


const std::map<std::string, log_level_t> levels_map{
	{"debug", LEVEL_DEBUG},
	{"info", LEVEL_INFO},
	{"notice", LEVEL_NOTICE},
	{"warning", LEVEL_WARNING},
	{"error", LEVEL_ERROR},
	{"critical", LEVEL_CRITICAL},
};


OpenSandModelConf::OpenSandModelConf():
	topology_model{nullptr},
	infrastructure_model{nullptr},
	profile_model{nullptr},
	topology{nullptr},
	infrastructure{nullptr},
	profile{nullptr}
{
	this->log = Output::Get()->registerLog(LEVEL_WARNING, "Configuration");
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
	types->addEnumType("isl_type", "Type of ISL", {"LanAdaptation", "Interconnect", "None"});

	auto entity = infrastructure_model->getRoot()->addComponent("entity", "Emulated Entity");
	auto entity_type = entity->addParameter("entity_type", "Entity Type", types->getType("entity_type"));
	entity_type->setReadOnly(true);

	{
		auto satellite_regen = entity->addComponent("entity_sat", "Satellite", "Specific infrastructure information for a Satellite");
		infrastructure_model->setReference(satellite_regen, entity_type);
		auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(satellite_regen->getReferenceData());
		expected_str->set("Satellite");
		satellite_regen->addParameter("entity_id", "Satellite ID", types->getType("int"));
		satellite_regen->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address this satellite should listen on for messages from ground entities");
		auto regen_level = satellite_regen->addParameter("regen_level", "Regeneration Level", types->getType("sat_regen_level"));

		auto isl_settings = satellite_regen->addList("isl_settings", "Inter-Satellite Link settings", "isl")->getPattern();
		isl_settings->addParameter("linked_sat_id", "Linked satellite ID", types->getType("int"), "ID of the other satellite to which this ISL is linked");
		auto isl_type = isl_settings->addParameter("isl_type", "ISL Type", types->getType("isl_type"));

		// Interconnect params
		auto interco_params = isl_settings->addComponent("interconnect_params", "Interconnect",
		                                                 "Interconnect ISL parameters (only for Transparent or BBFrame regeneration)");
		infrastructure_model->setReference(interco_params, isl_type);
		interco_params->getReferenceData()->fromString("Interconnect");
		interco_params->addParameter("interconnect_address",
		                             "Interconnection Address",
		                             types->getType("string"),
		                             "Address this satellite should listen on for "
		                             "messages from other satellites");
		interco_params->addParameter("interconnect_remote",
		                             "Remote Interconnection Address",
		                             types->getType("string"),
		                             "Address other satellites should listen on for "
		                             "messages from this satellite");
		interco_params->addParameter("upward_data_port", "Data Port (Upward)", types->getType("int"));
		interco_params->addParameter("downward_data_port", "Data Port (Downward)", types->getType("int"));
		interco_params->addParameter("upward_sig_port", "Signalisation Port (Upward)", types->getType("int"));
		interco_params->addParameter("downward_sig_port", "Signalisation Port (Downward)", types->getType("int"));
		interco_params->addParameter("interco_udp_stack", "UDP Stack (Interconnect)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_rmem", "UDP RMem (Interconnect)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_wmem", "UDP WMem (Interconnect)", types->getType("int"))->setAdvanced(true);

		// LanAdaptation params
		auto lan_params = isl_settings->addComponent("lan_adaptation", "Lan Adaptation",
		                                             "Lan Adaptation ISL parameters (available only for IP regeneration)");
		infrastructure_model->setReference(lan_params, isl_type);
		lan_params->getReferenceData()->fromString("LanAdaptation");
		lan_params->addParameter("tap_name", "TAP Name", types->getType("string"), "Name of the TAP interface");
	}

	{
		auto gateway = entity->addComponent("entity_gw", "Gateway", "Specific infrastructure information for a Gateway");
		infrastructure_model->setReference(gateway, entity_type);
		auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(gateway->getReferenceData());
		expected_str->set("Gateway");
		gateway->addParameter("entity_id", "Gateway ID", types->getType("int"));
		gateway->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address this gateway should listen on for messages from the satellite");
		gateway->addParameter("tap_iface", "TAP Interface", types->getType("string"), "Name of the TAP interface used by this gateway");
		gateway->addParameter("mac_address", "MAC Address", types->getType("string"), "MAC address this gateway routes traffic to");
		gateway->addParameter("ctrl_multicast_address", "Multicast IP Address (Control Messages)", types->getType("string"))->setAdvanced(true);
		gateway->addParameter("data_multicast_address", "Multicast IP Address (Data)", types->getType("string"))->setAdvanced(true);
		gateway->addParameter("ctrl_out_st_port", "Port (Control Messages Out ST)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("ctrl_out_gw_port", "Port (Control Messages Out GW)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("ctrl_in_st_port", "Port (Control Messages In ST)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("ctrl_in_gw_port", "Port (Control Messages In GW)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("logon_out_port", "Port (Logon Messages Out)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("logon_in_port", "Port (Logon Messages In)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("data_out_st_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("data_in_st_port", "Port (Data In ST)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("data_out_gw_port", "Port (Data Out GW)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("data_in_gw_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("udp_stack", "UDP Stack", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("udp_rmem", "UDP RMem", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("udp_wmem", "UDP WMem", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("pep_port", "PEP DAMA Port", types->getType("int"))->setAdvanced(true);
		gateway->addParameter("svno_port", "SVNO Port", types->getType("int"))->setAdvanced(true);
	}

	{
		auto gateway_net_acc = entity->addComponent("entity_gw_net_acc", "Gateway Net Access", "Specific infrastructure information for a split Gateway (Net Access)");
		infrastructure_model->setReference(gateway_net_acc, entity_type);
		auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(gateway_net_acc->getReferenceData());
		expected_str->set("Gateway Net Access");
		gateway_net_acc->addParameter("entity_id", "Gateway ID", types->getType("int"));
		gateway_net_acc->addParameter("tap_iface", "TAP Interface", types->getType("string"), "Name of the TAP interface used by this gateway");
		gateway_net_acc->addParameter("mac_address", "MAC Address", types->getType("string"), "MAC address this gateway routes traffic to");
		auto interco_params = gateway_net_acc->addComponent("interconnect_params", "Interconnect", "Interconnect parameters");
		interco_params->addParameter("interconnect_address",
		                             "Interconnection Address",
		                             types->getType("string"),
		                             "Address the net access gateway should listen on for "
		                             "messages from the physical layer gateway");
		interco_params->addParameter("interconnect_remote",
		                             "Remote Interconnection Address",
		                             types->getType("string"),
		                             "Address the physical layer gateway is listening on for "
		                             "messages from this net access gateway");
		interco_params->addParameter("upward_data_port", "Data Port (Upward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("upward_sig_port", "Signalisation Port (Upward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("downward_data_port", "Data Port (Downward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("downward_sig_port", "Signalisation Port (Downward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_stack", "UDP Stack (Interconnect)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_rmem", "UDP RMem (Interconnect)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_wmem", "UDP WMem (Interconnect)", types->getType("int"))->setAdvanced(true);
		gateway_net_acc->addParameter("pep_port", "PEP DAMA Port", types->getType("int"))->setAdvanced(true);
		gateway_net_acc->addParameter("svno_port", "SVNO Port", types->getType("int"))->setAdvanced(true);
	}

	{
		auto gateway_phy = entity->addComponent("entity_gw_phy", "Gateway Phy", "Specific infrastructure information for a split Gateway (Phy)");
		infrastructure_model->setReference(gateway_phy, entity_type);
		auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(gateway_phy->getReferenceData());
		expected_str->set("Gateway Phy");
		gateway_phy->addParameter("entity_id", "Gateway ID", types->getType("int"));
		auto interco_params = gateway_phy->addComponent("interconnect_params", "Interconnect", "Interconnect parameters");
		interco_params->addParameter("interconnect_address",
		                             "Interconnection Address",
		                             types->getType("string"),
		                             "Address the physical layer gateway should listen on for "
		                             "messages from the net access gateway");
		interco_params->addParameter("interconnect_remote",
		                             "Remote Interconnection Address",
		                             types->getType("string"),
		                             "Address the net access gateway is listening on for "
		                             "messages from this physical layer gateway");
		interco_params->addParameter("upward_data_port", "Data Port (Upward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("upward_sig_port", "Signalisation Port (Upward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("downward_data_port", "Data Port (Downward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("downward_sig_port", "Signalisation Port (Downward)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_stack", "UDP Stack (Interconnect)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_rmem", "UDP RMem (Interconnect)", types->getType("int"))->setAdvanced(true);
		interco_params->addParameter("interco_udp_wmem", "UDP WMem (Interconnect)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("emu_address", "Emulation Address", types->getType("string"), "Address this gateway should listen on for messages from the satellite");
		gateway_phy->addParameter("ctrl_multicast_address", "Multicast IP Address (Control Messages)", types->getType("string"))->setAdvanced(true);
		gateway_phy->addParameter("data_multicast_address", "Multicast IP Address (Data)", types->getType("string"))->setAdvanced(true);
		gateway_phy->addParameter("ctrl_out_st_port", "Port (Control Messages Out ST)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("ctrl_out_gw_port", "Port (Control Messages Out GW)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("ctrl_in_st_port", "Port (Control Messages In ST)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("ctrl_in_gw_port", "Port (Control Messages In GW)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("logon_out_port", "Port (Logon Messages Out)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("logon_in_port", "Port (Logon Messages In)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("data_out_st_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("data_in_st_port", "Port (Data In ST)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("data_out_gw_port", "Port (Data Out GW)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("data_in_gw_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("udp_stack", "UDP Stack (Satellite)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("udp_rmem", "UDP RMem (Satellite)", types->getType("int"))->setAdvanced(true);
		gateway_phy->addParameter("udp_wmem", "UDP WMem (Satellite)", types->getType("int"))->setAdvanced(true);
	}

	{
		auto terminal = entity->addComponent("entity_st", "Terminal", "Specific infrastructure information for a Terminal");
		infrastructure_model->setReference(terminal, entity_type);
		auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(terminal->getReferenceData());
		expected_str->set("Terminal");
		terminal->addParameter("entity_id", "Terminal ID", types->getType("int"));
		terminal->addParameter("emu_address", "Emulation Address", types->getType("string"),
		                       "Address this satellite terminal should listen on for messages from the satellite");
		terminal->addParameter("tap_iface", "TAP Interface", types->getType("string"),
		                       "Name of the TAP interface used by this satellite terminal");
		terminal->addParameter("mac_address", "MAC Address", types->getType("string"),
		                       "MAC address this satellite terminal routes traffic to");
		terminal->addParameter("qos_server_host", "QoS server Host Agent", types->getType("string"))->setAdvanced(true);
		terminal->addParameter("qos_server_port", "QoS server Host Port", types->getType("int"))->setAdvanced(true);
	}

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

	auto infra = infrastructure_model->getRoot()->addComponent("infrastructure", "Infrastructure");
	infra->setAdvanced(true);
	infra->setReadOnly(true);

	auto satellites = infra->addList("satellites", "Satellites", "satellite")->getPattern();
	satellites->addParameter("entity_id", "Entity ID", types->getType("int"));
	satellites->addParameter("emu_address", "Emulation Address", types->getType("string"),
	                         "Address this satellite should listen on for messages from ground entities");

	auto gateways = infra->addList("gateways", "Gateways", "gateway")->getPattern();
	gateways->addParameter("entity_id", "Entity ID", types->getType("int"));
	gateways->addParameter("emu_address", "Emulation Address", types->getType("string"),
	                       "Address this gateway should listen on for messages from the satellite");
	gateways->addParameter("mac_address", "MAC Address", types->getType("string"), "MAC address this gateway routes traffic to");
	gateways->addParameter("ctrl_multicast_address", "Multicast IP Address (Control Messages)", types->getType("string"))->setAdvanced(true);
	gateways->addParameter("data_multicast_address", "Multicast IP Address (Data)", types->getType("string"))->setAdvanced(true);
	gateways->addParameter("ctrl_out_st_port", "Port (Control Messages Out ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("ctrl_out_gw_port", "Port (Control Messages Out GW)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("ctrl_in_st_port", "Port (Control Messages In ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("ctrl_in_gw_port", "Port (Control Messages In GW)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("logon_out_port", "Port (Logon Messages Out)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("logon_in_port", "Port (Logon Messages In)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_out_st_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_in_st_port", "Port (Data In ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_out_gw_port", "Port (Data Out GW)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("data_in_gw_port", "Port (Data Out ST)", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("udp_stack", "UDP Stack", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("udp_rmem", "UDP RMem", types->getType("int"))->setAdvanced(true);
	gateways->addParameter("udp_wmem", "UDP WMem", types->getType("int"))->setAdvanced(true);

	auto terminals = infra->addList("terminals", "Terminals", "terminal")->getPattern();
	terminals->addParameter("entity_id", "Entity ID", types->getType("int"));
	terminals->addParameter("emu_address", "Emulation Address", types->getType("string"),
	                        "Address this satellite terminal should listen on for messages from the satellite");
	terminals->addParameter("mac_address", "MAC Address", types->getType("string"),
	                        "MAC address this satellite terminal routes traffic to");

	infra->addParameter("default_gw", "Default Gateway", types->getType("int"),
	                    "Default Gateway ID for a packet destination when the MAC "
	                    "address is not found in the SARP Table; use -1 to drop "
	                    "such packets")->setAdvanced(true);


	topology_model = std::make_shared<OpenSANDConf::MetaModel>("1.0.0");
	topology_model->getRoot()->setDescription("topology");
	types = topology_model->getTypesDefinition();
	types->addEnumType("burst_length", "DVB-RCS2 Burst Length", {"536 sym", "1616 sym"});
	types->addEnumType("forward_type", "Forward Carrier Type", {"ACM", "VCM"});
	types->addEnumType("return_type", "Return Carrier Type", {"DAMA", "ALOHA", "SCPC"});
	types->addEnumType("carrier_group", "Carrier Group", {"Standard", "Premium", "Professional", "SVNO1", "SVNO2", "SVNO3", "SNO"});
	types->addEnumType("modulation", "Modulation", {"BPSK", "Pi/2BPSK", "QPSK", "8PSK", "16APSK", "16QAM", "32APSK"});
	types->addEnumType("coding", "Coding", {"1/4", "1/3", "2/5", "1/2", "3/5", "2/3", "3/4", "4/5", "5/6", "6/7", "8/9", "9/10"});
	types->addEnumType("sat_regen_level", "Regeneration Level for Satellite", {"Transparent", "BBFrame", "IP"});

	auto frequency_plan = topology_model->getRoot()->addComponent("frequency_plan", "Spots / Frequency Plan");
	auto spots = frequency_plan->addList("spots", "Spots", "spot")->getPattern();
	auto spot_assignment = spots->addComponent("assignments", "Spot Assignment");
	spot_assignment->addParameter("gateway_id", "Gateway ID", types->getType("int"),
	                              "ID of the gateway this spot belongs to; note that "
	                              "only one spot must be managed by a given gateway");
	spot_assignment->addParameter("sat_id_gw", "Satellite ID for the gateway", types->getType("int"),
	                              "ID of the satellite to which the gateway is connected");
	spot_assignment->addParameter("sat_id_st", "Satellite ID for the terminals", types->getType("int"),
	                              "ID of the satellite to which the terminals are connected");
	spot_assignment->addParameter("forward_regen_level", "Forward channel regeneration level", types->getType("sat_regen_level"),
	                              "Regeneration level for the forward channel (gateway -> terminal)");
	spot_assignment->addParameter("return_regen_level", "Return channel regeneration level", types->getType("sat_regen_level"),
	                              "Regeneration level for the return channel (terminal -> gateway)");
	auto roll_offs = spots->addComponent("roll_off", "Roll Off");
	roll_offs->addParameter("forward", "Forward Band Roll Off", types->getType("double"), "Usually 0.35, 0.25 or 0.2 for DVB-S2");
	roll_offs->addParameter("return", "Return Band Roll Off", types->getType("double"), "Usually 0.2 for DVB-RCS2");
	auto forward_band = spots->addList("forward_band", "Forward Band", "fwd_band")->getPattern();
	forward_band->addParameter("symbol_rate", "Symbol Rate", types->getType("double"))->setUnit("Bauds");
	auto band_type = forward_band->addParameter("type", "Type", types->getType("forward_type"));
	forward_band->addParameter("wave_form", "Wave Form IDs", types->getType("string"),
	                           "Supported Wave Forms. Use ';' separator for unique IDs, "
	                           "'-' separator for all the IDs between bounds");
	forward_band->addParameter("group", "Group", types->getType("carrier_group"));
	auto ratio = forward_band->addParameter("ratio", "Ratio", types->getType("string"),
	                                        "Separate temporal division ratios by ','; you should "
	                                        "also specify as many wave form IDs also separated by ','");
	topology_model->setReference(ratio, band_type);
	auto expected_str = std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(ratio->getReferenceData());
	expected_str->set("VCM");
	auto return_band = spots->addList("return_band", "Return Band", "rtn_band")->getPattern();
	return_band->addParameter("symbol_rate", "Symbol Rate", types->getType("double"))->setUnit("Bauds");
	return_band->addParameter("type", "Type", types->getType("return_type"));
	return_band->addParameter("wave_form", "Wave Forms", types->getType("string"));
	return_band->addParameter("group", "Group", types->getType("carrier_group"));

	auto st_assignment = topology_model->getRoot()->addComponent("st_assignment", "Satellite Terminal Assignment");
	auto defaults = st_assignment->addComponent("defaults", "Default Settings");
	defaults->addParameter("default_gateway", "Gateway", types->getType("int"),
	                       "ID of the gateway terminals should connect to by default; since a gateway manages only "
	                       "one spot, this also defines the spot terminals belong to by default");
	defaults->addParameter("default_group", "Group", types->getType("carrier_group"));
	auto assignments = st_assignment->addList("assignments", "Additional Assignments", "assigned")->getPattern();
	assignments->setAdvanced(true);
	assignments->setDescription("Additional terminal assignments that does not fit the default values");
	assignments->addParameter("terminal_id", "Terminal ID", types->getType("int"));
	assignments->addParameter("gateway_id", "Gateway ID", types->getType("int"));
	assignments->addParameter("group", "Group", types->getType("carrier_group"));

	auto wave_forms = topology_model->getRoot()->addComponent("wave_forms", "Wave Forms");
	wave_forms->setReadOnly(true);
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
	links->addParameter("forward_duration", "Forward link frame duration", types->getType("double"))->setUnit("ms");
	links->addParameter("forward_margin", "Forward link ACM loop margin", types->getType("double"))->setUnit("dB");
	// auto forward_encap = links->addList("forward_encap_schemes", "Forward link Encapsulation Schemes", "forward_encap_scheme")->getPattern();
	links->addParameter("return_duration", "Return link frame duration", types->getType("double"))->setUnit("ms");
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


std::shared_ptr<OpenSANDConf::MetaComponent> OpenSandModelConf::getOrCreateComponent(const std::string& id,
                                                                                     const std::string& name,
                                                                                     std::shared_ptr<OpenSANDConf::MetaComponent> from)
{
	return getOrCreateComponent(id, name, "", from);
}


std::shared_ptr<OpenSANDConf::MetaComponent> OpenSandModelConf::getOrCreateComponent(const std::string& id,
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


std::shared_ptr<OpenSANDConf::MetaComponent> OpenSandModelConf::getComponentByPath(const std::string &path,
                                                                                   std::shared_ptr<OpenSANDConf::MetaModel> from)
{
	if (from == nullptr && profile_model == nullptr)
	{
		createModels();
	}

	auto element = (from == nullptr ? profile_model : from)->getItemByPath(path);
	return std::dynamic_pointer_cast<OpenSANDConf::MetaComponent>(element);
}


void OpenSandModelConf::setProfileReference(std::shared_ptr<OpenSANDConf::MetaElement> parameter,
                                            std::shared_ptr<OpenSANDConf::MetaParameter> referee,
                                            const char *expected_value)
{
	if (profile_model == nullptr)
	{
		return;
	}

	if (profile_model->setReference(parameter, referee))
	{
		auto expected = parameter->getReferenceData();
		std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(expected)->set(expected_value);
	}
}


void OpenSandModelConf::setProfileReference(std::shared_ptr<OpenSANDConf::MetaElement> parameter,
                                            std::shared_ptr<OpenSANDConf::MetaParameter> referee,
                                            const std::string &expected_value)
{
	if (profile_model == nullptr)
	{
		return;
	}

	if (profile_model->setReference(parameter, referee))
	{
		auto expected = parameter->getReferenceData();
		std::dynamic_pointer_cast<OpenSANDConf::DataValue<std::string>>(expected)->set(expected_value);
	}
}


void OpenSandModelConf::setProfileReference(std::shared_ptr<OpenSANDConf::MetaElement> parameter,
                                            std::shared_ptr<OpenSANDConf::MetaParameter> referee,
                                            bool expected_value)
{
	if (profile_model == nullptr)
	{
		return;
	}

	if (profile_model->setReference(parameter, referee))
	{
		auto expected = parameter->getReferenceData();
		std::dynamic_pointer_cast<OpenSANDConf::DataValue<bool>>(expected)->set(expected_value);
	}
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
	if (topology == nullptr)
	{
		LOG(log, LEVEL_ERROR, "parse error when reading topology file");
		return false;
	}

	if (infrastructure == nullptr)
	{
		LOG(log, LEVEL_ERROR, "building spot topology requires reading infrastructure file first");
		return false;
	}
	
	spots_topology.clear();
	auto spot_list = topology->getRoot()->getComponent("frequency_plan")->getList("spots");
	bool ok = true;
	for (auto &&spot_item: spot_list->getItems()) {
		auto spot_assignement = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(spot_item)->getComponent("assignments");
		int gw_id, sat_id_gw, sat_id_st;
		std::string forward_str, return_str;
		ok &= extractParameterData(spot_assignement, "gateway_id", gw_id);
		ok &= extractParameterData(spot_assignement, "sat_id_gw", sat_id_gw);
		ok &= extractParameterData(spot_assignement, "sat_id_st", sat_id_st);
		ok &= extractParameterData(spot_assignement, "forward_regen_level", forward_str);
		ok &= extractParameterData(spot_assignement, "return_regen_level", return_str);

		SpotTopology spot_topo{};
		spot_topo.spot_id = gw_id;
		spot_topo.gw_id = gw_id;
		spot_topo.sat_id_gw = sat_id_gw;
		spot_topo.sat_id_st = sat_id_st;
		spot_topo.forward_regen_level = strToRegenLevel(forward_str);
		spot_topo.return_regen_level = strToRegenLevel(return_str);
		spots_topology[gw_id] = spot_topo;
	}
	if (!ok) {
		LOG(log, LEVEL_ERROR, "A problem occurred while extracting spot assignments");
		return false;
	}

	std::unordered_set<int> terminal_ids{};
	auto terminals = infrastructure->getRoot()->getComponent("infrastructure")->getList("terminals");
	for (auto& entity_element : terminals->getItems()) {
		auto st = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int st_id;
		if (extractParameterData(st, "entity_id", st_id)) {
			terminal_ids.insert(st_id);
		}
	}
	
	auto st_assignments = topology->getRoot()->getComponent("st_assignment");
	for (auto& assignment_item : st_assignments->getList("assignments")->getItems()) {
		auto st_assignment = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(assignment_item);
		int st_id;
		int spot_id;
		if (!extractParameterData(st_assignment, "terminal_id", st_id)) {
			return false;
		}
		auto assigned_spot = st_assignment->getParameter("gateway_id");
		if (!extractParameterData(assigned_spot, spot_id)) {
			return false;
		}
		if (spots_topology.find(spot_id) == spots_topology.end())
		{
			LOG(log, LEVEL_ERROR,
			    "ST%d is assigned to the spot %d, which was not found in the configuration",
			    st_id, spot_id);
			return false;
		}
		spots_topology[spot_id].st_ids.insert(st_id);

		auto non_default_st = terminal_ids.find(st_id);
		if (non_default_st != terminal_ids.end())
		{
			terminal_ids.erase(non_default_st);
		}
	}

	if (terminal_ids.size())
	{
		auto assigned_spot = st_assignments->getComponent("defaults")->getParameter("default_gateway");
		int default_spot;
		if (!extractParameterData(assigned_spot, default_spot)) {
			return false;
		}
		if (spots_topology.find(default_spot) == spots_topology.end())
		{
			LOG(log, LEVEL_ERROR,
			    "Some ST are not assigned a spot and should be assigned to the default "
				"spot (%d), but it was not found in the configuration",
				default_spot);
			return false;
		}

		auto& sts = spots_topology[default_spot].st_ids;
		std::copy(terminal_ids.begin(), terminal_ids.end(), std::inserter(sts, sts.end()));
	}

	return true;
}


bool OpenSandModelConf::readInfrastructure(const std::string& filename)
{
	if (infrastructure_model == nullptr)
	{
		createModels();
	}

	entities_type.clear();
	infrastructure = OpenSANDConf::fromXML(infrastructure_model, filename);
	if (infrastructure == nullptr) {
		LOG(log, LEVEL_ERROR, "parse error when reading infrastructure file");
		return false;
	}

	auto gws = infrastructure->getRoot()->getComponent("infrastructure")->getList("gateways");
	for (auto& entity_element : gws->getItems()) {
		auto gateway = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int gateway_id;
		if (extractParameterData(gateway, "entity_id", gateway_id)) {
			entities_type[gateway_id] = Component::gateway;
		}
	}
	auto satellites = infrastructure->getRoot()->getComponent("infrastructure")->getList("satellites");
	for (auto& entity_element : satellites->getItems()) {
		auto sat = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int sat_id;
		if (extractParameterData(sat, "entity_id", sat_id)) {
			entities_type[sat_id] = Component::satellite;
		}
	}
	auto terminals = infrastructure->getRoot()->getComponent("infrastructure")->getList("terminals");
	for (auto& entity_element : terminals->getItems()) {
		auto st = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);
		int st_id;
		if (extractParameterData(st, "entity_id", st_id)) {
			entities_type[st_id] = Component::terminal;
		}
	}

	return true;
}


bool OpenSandModelConf::readProfile(const std::string& filename)
{
	if (profile_model == nullptr)
	{
		createModels();
	}

	profile = OpenSANDConf::fromXML(profile_model, filename);
	if (profile == nullptr)
	{
		LOG(log, LEVEL_ERROR, "parse error when reading profile file");
		return false;
	}

	return true;
}


Component OpenSandModelConf::getComponentType() const
{
	if (infrastructure == nullptr) {
		return Component::unknown;
	}

	std::string component_type;
	extractParameterData(infrastructure->getRoot()->getComponent("entity"), "entity_type", component_type);

	if (component_type == "Satellite") {
		return Component::satellite;
	} else if (component_type == "Terminal") {
		return Component::terminal;
	} else if (component_type == "Gateway" || component_type == "Gateway Net Access" || component_type == "Gateway Phy") {
		return Component::gateway;
	} else {
		return Component::unknown;
	}
}


bool OpenSandModelConf::getComponentType(std::string &type, tal_id_t &id) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto entity = infrastructure->getRoot()->getComponent("entity");
	std::string component_type;
	extractParameterData(entity, "entity_type", component_type);

	if (component_type == "Satellite") {
		type = "sat";
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
	if (!extractParameterData(entity->getComponent("entity_" + type), "entity_id", entity_id)) {
		return false;
	}

	id = entity_id;
	return true;
}


bool OpenSandModelConf::getSatInfrastructure(std::string &ip_address, std::vector<IslConfig> &isls_cfg) const
{
	isls_cfg.clear();
	if (infrastructure == nullptr) {
		return false;
	}

	std::string type;
	tal_id_t id;
	if (!this->getComponentType(type, id)) {
		return false;
	}

	if (type != "sat") {
		return false;
	}

	auto satellite = infrastructure->getRoot()->getComponent("entity")->getComponent("entity_" + type);
	if (!extractParameterData(satellite, "emu_address", ip_address))
	{
		return false;
	}

	auto isls = std::dynamic_pointer_cast<OpenSANDConf::DataList>(
	    infrastructure->getItemByPath("entity/entity_" + type + "/isl_settings"));

	for (auto &&isl_item: isls->getItems())
	{
		IslConfig isl_cfg;
		auto isl = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(isl_item);

		int linked_sat_id;
		if (!extractParameterData(isl, "linked_sat_id", linked_sat_id))
		{
			LOG(this->log, LEVEL_ERROR,
			    "The linked satellite ID for ISL was not found in the infrastructure configuration");
			return false;
		}
		isl_cfg.linked_sat_id = linked_sat_id;

		std::string isl_type;
		if (!extractParameterData(isl, "isl_type", isl_type))
		{
			LOG(this->log, LEVEL_ERROR,
			    "The ISL type was not found in the infrastructure configuration");
			return false;
		}
		if (isl_type == "None")
		{
			isl_cfg.type = IslType::None;
		}
		else if (isl_type == "LanAdaptation")
		{
			isl_cfg.type = IslType::LanAdaptation;
			auto lan_adaptation_params = isl->getComponent("lan_adaptation");
			if (!extractParameterData(lan_adaptation_params, "tap_name", isl_cfg.tap_iface))
			{
				LOG(this->log, LEVEL_ERROR,
				    "Error extracting ISL LanAdaptation settings from the infrastructure configuration");
				return false;
			}
		}
		else if (isl_type == "Interconnect")
		{
			isl_cfg.type = IslType::Interconnect;
			auto interconnect_params = isl->getComponent("interconnect_params");
			if (!extractParameterData(interconnect_params, "interconnect_address", isl_cfg.interco_addr))
			{
				LOG(this->log, LEVEL_ERROR,
				    "Error extracting ISL Interconnect settings from the infrastructure configuration");
				return false;
			}
		}
		else
		{
			LOG(this->log, LEVEL_ERROR,
			    "The ISL type %s is not supported", isl_type.c_str());
			return false;
		}
		isls_cfg.push_back(isl_cfg);
	}

	return true;
}


bool OpenSandModelConf::getGroundInfrastructure(std::string &ip_address, std::string &tap_iface) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	std::string type;
	tal_id_t id;
	if (!this->getComponentType(type, id)) {
		return false;
	}

	auto entity = infrastructure->getRoot()->getComponent("entity")->getComponent("entity_" + type);
	if (type == "st" || type == "gw") {
		if (!extractParameterData(entity, "emu_address", ip_address)) {
			return false;
		}
		return extractParameterData(entity, "tap_iface", tap_iface);
	} else if (type == "gw_net_acc") {
		auto isl_params = entity->getComponent("interconnect_params");
		if (!extractParameterData(isl_params, "interconnect_address", ip_address))
		{
			return false;
		}
		return extractParameterData(entity, "tap_iface", tap_iface);
	} else if (type == "gw_phy") {
		if (!extractParameterData(entity, "emu_address", ip_address)) {
			return false;
		}
		auto isl_params = entity->getComponent("interconnect_params");
		return extractParameterData(isl_params, "interconnect_address", tap_iface);
	} else {
		return false;
	}

	return false;
}


bool OpenSandModelConf::getLocalStorage(bool &enabled, std::string &output_folder) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto storage = infrastructure->getRoot()->getComponent("storage");
	if (!extractParameterData(storage, "enable_local", enabled)) {
		return false;
	}
	if (enabled && !extractParameterData(storage, "path_local", output_folder)) {
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
	if (!extractParameterData(storage, "enable_collector", enabled)) {
		return false;
	}
	if (!enabled) {
		return true;
	}

	if (!extractParameterData(storage, "collector_address", address)) {
		return false;
	}

	int stats = 5361;
	extractParameterData(storage, "collector_probes", stats);

	int logs = 5362;
	extractParameterData(storage, "collector_logs", logs);

	stats_port = stats;
	logs_port = logs;
	return true;
}


bool OpenSandModelConf::logLevels(std::map<std::string, log_level_t> &levels) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto log_levels = infrastructure->getRoot()->getComponent("logs");
	static std::vector<std::string> log_names_loop{
		"init",
		"lan_adaptation",
		"encap",
		"dvb",
		"physical_layer",
		"sat_carrier",
	};
	for (auto& log_name : log_names_loop) {
		std::string log_level;
		if (!extractParameterData(log_levels->getComponent(log_name), "level", log_level)) {
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
		if (!extractParameterData(log, "name", log_name)) {
			return false;
		}
		std::string log_level;
		if (!extractParameterData(log, "level", log_level)) {
			return false;
		}
		auto iterator = levels_map.find(log_level);
		if (iterator == levels_map.end()) {
			return false;
		}
		levels[log_name] = iterator->second;
	}

	return true;
}


inline std::unique_ptr<MacAddress> make_unique_mac(std::string address)
{
	return std::unique_ptr<MacAddress>{new MacAddress{address}};
}


bool OpenSandModelConf::getSarp(SarpTable& sarp_table) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto infra = infrastructure->getRoot()->getComponent("infrastructure");

	int default_gw = -1;
	extractParameterData(infra, "default_gw", default_gw);
	sarp_table.setDefaultTal(default_gw);

	// Broadcast
	sarp_table.add(make_unique_mac("ff:ff:ff:ff:ff:ff"), 31);
	// Multicast
	sarp_table.add(make_unique_mac("33:33:**:**:**:**"), 31);
	sarp_table.add(make_unique_mac("01:00:5E:**:**:**"), 31);

	static const std::vector<std::string> list_names{"gateways", "terminals"};
	for (auto& list_name : list_names) {
		for (auto& entity_element : infra->getList(list_name)->getItems()) {
			auto entity = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(entity_element);

			int entity_id;
			if (!extractParameterData(entity, "entity_id", entity_id)) {
				return false;
			}

			std::string mac_address;
			if (!extractParameterData(entity, "mac_address", mac_address)) {
				return false;
			}

			sarp_table.add(make_unique_mac(mac_address), entity_id);
		}
	}

	return true;
}


bool OpenSandModelConf::getNccPorts(int &pep_tcp_port, int &svno_tcp_port) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	std::string type;
	tal_id_t id;
	if (!this->getComponentType(type, id)) {
		return false;
	}

	if (type != "gw" && type != "gw_net_acc") {
		return false;
	}

	// Default values
	pep_tcp_port = 4998;
	svno_tcp_port = 4999;

	auto ncc = infrastructure->getRoot()->getComponent("entity")->getComponent("entity_" + type);
	extractParameterData(ncc, "pep_port", pep_tcp_port);
	extractParameterData(ncc, "svno_port", svno_tcp_port);
	return true;
}


bool OpenSandModelConf::getQosServerHost(std::string &qos_server_host_agent, int &qos_server_host_port) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	std::string type;
	tal_id_t id;
	if (!this->getComponentType(type, id)) {
		return false;
	}

	if (type != "st") {
		return false;
	}

	// Default values
	qos_server_host_agent = "127.0.0.1";
	qos_server_host_port = 4000;

	auto qos_server = infrastructure->getRoot()->getComponent("entity")->getComponent("entity_" + type);
	extractParameterData(qos_server, "qos_server_host", qos_server_host_agent);
	extractParameterData(qos_server, "qos_server_port", qos_server_host_port);
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
		if (!extractParameterData(waveform, "id", scheme_number)) {
			return false;
		}
		if (scheme_number <= 0) {
			return false;
		}

		std::string modulation;
		if (!extractParameterData(waveform, "modulation", modulation)) {
			return false;
		}

		std::string coding;
		if (!extractParameterData(waveform, "coding", coding)) {
			return false;
		}

		double spectral_efficiency;
		if (!extractParameterData(waveform, "efficiency", spectral_efficiency)) {
			return false;
		}

		double threshold;
		if (!extractParameterData(waveform, "threshold", threshold)) {
			return false;
		}

		fmt_definition_parameters params{static_cast<unsigned>(scheme_number),
		                                 modulation,
		                                 coding,
		                                 static_cast<float>(spectral_efficiency),
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
		if (!extractParameterData(waveform, "burst_length", burst_length)) {
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
		if (!extractParameterData(waveform, "id", scheme_number)) {
			return false;
		}
		if (scheme_number <= 0) {
			return false;
		}

		std::string modulation;
		if (!extractParameterData(waveform, "modulation", modulation)) {
			return false;
		}

		std::string coding;
		if (!extractParameterData(waveform, "coding", coding)) {
			return false;
		}

		double spectral_efficiency;
		if (!extractParameterData(waveform, "efficiency", spectral_efficiency)) {
			return false;
		}

		double threshold;
		if (!extractParameterData(waveform, "threshold", threshold)) {
			return false;
		}

		fmt_definition_parameters params{static_cast<unsigned>(scheme_number),
		                                 modulation,
		                                 coding,
		                                 static_cast<float>(spectral_efficiency),
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
	if (!extractParameterData(schedulers, "burst_length", burst_length)) {
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
	if (!extractParameterData(schedulers, "crdsa_frame", value)) {
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
	if (!extractParameterData(schedulers, "crdsa_delay", value)) {
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
	return extractParameterData(schedulers, "pep_allocation", pep_allocation_delay);
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
	double value;
	if (!getAdvancedLinksParameter(topology, "return_duration", value)) {
		return false;
	}

	frame_duration = value;
	return true;
}


bool OpenSandModelConf::getForwardFrameDuration(time_ms_t &frame_duration) const
{
	double value;
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
	if (!extractParameterData(schedulers, "statistics", value)) {
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
	if (!extractParameterData(schedulers, "synchro", value)) {
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
	if (!extractParameterData(schedulers, "acm_refresh", value)) {
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
	if (!extractParameterData(delay, "fifo_size", value)) {
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
	if (!extractParameterData(delay, "delay_timer", value)) {
		return false;
	}

	period = value;
	return true;
}


bool OpenSandModelConf::getControlPlaneDisabled(bool &disabled) const
{
	if (profile == nullptr)
		return false;

	auto elem = profile->getItemByPath("control_plane/disable_control_plane");
	return extractParameterData(std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(elem), disabled);
}


bool OpenSandModelConf::getGwWithTalId(uint16_t tal_id, uint16_t &gw_id) const
{
	if (topology == nullptr) {
		return false;
	}

	auto st_assignments = topology->getRoot()->getComponent("st_assignment");
	auto assigned_spot = st_assignments->getComponent("defaults")->getParameter("default_gateway");

	for (auto& assignment : st_assignments->getList("assignments")->getItems()) {
		auto st_assignment = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(assignment);
		int st_id;
		if (!extractParameterData(st_assignment, "terminal_id", st_id)) {
			return false;
		}
		if (tal_id == st_id) {
			assigned_spot = st_assignment->getParameter("gateway_id");
		}
	}

	int assigned_spot_id;
	if (!extractParameterData(assigned_spot, assigned_spot_id)) {
		return false;
	}

	gw_id = assigned_spot_id;
	return true;
}


bool OpenSandModelConf::getGwWithCarrierId(unsigned int car_id, uint16_t &gw) const
{
	if (topology == nullptr) {
		return false;
	}

	gw = car_id / 10;

	// Check the spot exists, fail in case of multiple spots configured for a gateway
	unsigned int amount_found = 0;
	for (auto& spot : topology->getRoot()->getComponent("frequency_plan")->getList("spots")->getItems()) {
		auto gw_assignment = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(spot)->getComponent("assignments");
		int gw_id;
		if (!extractParameterData(gw_assignment, "gateway_id", gw_id)) {
			return false;
		}
		if (gw == gw_id) {
			++amount_found;
		}
	}

	return amount_found == 1;
}


bool OpenSandModelConf::isGw(uint16_t gw_id) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	auto entity = entities_type.find(gw_id);
	return entity != entities_type.end() && entity->second == Component::gateway;
}


std::unordered_set<tal_id_t> OpenSandModelConf::getSatellites() const
{
	std::unordered_set<tal_id_t> satellites{};
	for (auto&& [entity_id, entity_type] : this->entities_type)
	{
		if (entity_type == Component::satellite)
		{
			satellites.insert(entity_id);
		}
	}

	return satellites;
}


Component OpenSandModelConf::getEntityType(tal_id_t tal_id) const
{
	if (infrastructure == nullptr) {
		return Component::unknown;
	}
	auto entity_it = entities_type.find(tal_id);
	if (entity_it == entities_type.end()) {
		return Component::unknown;
	}
	return entity_it->second;
}

bool OpenSandModelConf::getScpcEnabled(bool &scpc_enabled) const
{
	auto param = std::dynamic_pointer_cast<OpenSANDConf::DataParameter>(
	    profile->getItemByPath("access/settings/scpc_enabled"));
	if (!extractParameterData(param, scpc_enabled))
	{
		LOG(log, LEVEL_ERROR, "Failed to get scpc_enabled parameter in the "
		                      "profile config (root/access/settings/scpc_enabled)");
		return false;
	}
	return true;
}

bool OpenSandModelConf::getScpcEncapStack(std::vector<std::string> &encap_stack) const
{
	// encap_stack = std::vector<std::string>{"RLE", "GSE"};
	encap_stack = std::vector<std::string>{"GSE"};
	return true;
}


std::shared_ptr<OpenSANDConf::DataComponent> getEntityById(std::shared_ptr<OpenSANDConf::DataList> list, int id)
{
	for(auto &item: list->getItems()) {
		auto entity = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);
		auto entity_id_param = entity->getParameter("entity_id")->getData();
		if (!entity_id_param) return nullptr;
		auto entity_id = std::dynamic_pointer_cast<OpenSANDConf::DataValue<int>>(entity_id_param)->get();
		if(id == entity_id) {
			return entity;
		}
	}
	return nullptr;
}


std::shared_ptr<OpenSANDConf::DataComponent> getSpotById(std::shared_ptr<OpenSANDConf::DataComponent> topo, int id)
{
	for(auto &item: topo->getComponent("frequency_plan")->getList("spots")->getItems()) {
		auto spot = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(item);
		auto assignments = spot->getComponent("assignments");
		auto assigned_gw_param = assignments->getParameter("gateway_id")->getData();
		if(!assigned_gw_param) return nullptr;
		auto assigned_gw = std::dynamic_pointer_cast<OpenSANDConf::DataValue<int>>(assigned_gw_param)->get();
		if(assigned_gw == id) {
			return spot;
		}
	}
	return nullptr;
}


bool OpenSandModelConf::getSpotInfrastructure(uint16_t gw_id, spot_infrastructure &carriers) const
{
	if (infrastructure == nullptr || topology == nullptr) {
		return false;
	}

	auto infra = infrastructure->getRoot()->getComponent("infrastructure");

	std::shared_ptr<OpenSANDConf::DataComponent> gateway = getEntityById(infra->getList("gateways"), gw_id);
	if (gateway == nullptr) {
		LOG(this->log, LEVEL_ERROR,
		    "The gateway %d was not found in the infrastructure configuration", gw_id);
		return false;
	}

	auto topo = topology->getRoot();
	auto delay = topo->getComponent("advanced_settings")->getComponent("delay");

	int default_fifos_size = 10000;
	extractParameterData(delay, "fifo_size", default_fifos_size);

	auto spot = getSpotById(topo, gw_id);
	if (spot == nullptr) {
		LOG(this->log, LEVEL_ERROR,
		    "The spot associated with the gateway %d was not found in the infrastructure configuration", gw_id);
		return false;
	}
		
	uint16_t carrier_id = gw_id * 10;

	std::string gateway_address;
	if (!extractParameterData(gateway, "emu_address", gateway_address)) {
		return false;
	}

	int assigned_sat_for_gw;
	if(!extractParameterData(spot->getComponent("assignments"), "sat_id_gw", assigned_sat_for_gw)) {
		return false;
	}
	int assigned_sat_for_st;
	if(!extractParameterData(spot->getComponent("assignments"), "sat_id_st", assigned_sat_for_st)) {
		return false;
	}

	std::shared_ptr<OpenSANDConf::DataComponent> satellite_gw = getEntityById(infra->getList("satellites"), assigned_sat_for_gw);
	if(satellite_gw == nullptr) {
		LOG(this->log, LEVEL_ERROR,
		    "The GW %d is assigned to the satellite %d, which was not found "
		    "in the infrastructure configuration",
		    gw_id, assigned_sat_for_gw);
		return false;
	}
	std::shared_ptr<OpenSANDConf::DataComponent> satellite_st = getEntityById(infra->getList("satellites"), assigned_sat_for_st);
	if(satellite_st == nullptr) {
		LOG(this->log, LEVEL_ERROR,
		    "The STs of spot %d are assigned to the satellite %d, which was not found "
		    "in the infrastructure configuration",
		    gw_id, assigned_sat_for_st);
		return false;
	}

	std::string satellite_st_address;
	if(!extractParameterData(satellite_st, "emu_address", satellite_st_address)) {
		return false;
	}
	std::string satellite_gw_address;
	if(!extractParameterData(satellite_gw, "emu_address", satellite_gw_address)) {
		return false;
	}

	std::string ctrl_multicast_address = "239.137.194." + std::to_string(220 + gw_id * 2);
	extractParameterData(gateway, "ctrl_multicast_address", ctrl_multicast_address);
	std::string data_multicast_address = "239.137.194." + std::to_string(221 + gw_id * 2);
	extractParameterData(gateway, "data_multicast_address", data_multicast_address);

	int logon_in_port = 55000 + carrier_id;
	extractParameterData(gateway, "logon_in_port", logon_in_port);
	int logon_out_port = 55001 + carrier_id;
	extractParameterData(gateway, "logon_out_port", logon_out_port);
	int ctrl_in_st_port = 55002 + carrier_id;
	extractParameterData(gateway, "ctrl_in_st_port", ctrl_in_st_port);
	int ctrl_out_gw_port = 55003 + carrier_id;
	extractParameterData(gateway, "ctrl_out_gw_port", ctrl_out_gw_port);
	int ctrl_in_gw_port = 55004 + carrier_id;
	extractParameterData(gateway, "ctrl_in_gw_port", ctrl_in_gw_port);
	int ctrl_out_st_port = 55005 + carrier_id;
	extractParameterData(gateway, "ctrl_out_st_port", ctrl_out_st_port);
	int data_in_st_port = 55006 + carrier_id;
	extractParameterData(gateway, "data_in_st_port", data_in_st_port);
	int data_out_gw_port = 55007 + carrier_id;
	extractParameterData(gateway, "data_out_gw_port", data_out_gw_port);
	int data_in_gw_port = 55008 + carrier_id;
	extractParameterData(gateway, "data_in_gw_port", data_in_gw_port);
	int data_out_st_port = 55009 + carrier_id;
	extractParameterData(gateway, "data_out_st_port", data_out_st_port);

	int udp_stack = 5;
	extractParameterData(gateway, "udp_stack", udp_stack);
	int udp_rmem = 1048580;
	extractParameterData(gateway, "udp_rmem", udp_rmem);
	int udp_wmem = 1048580;
	extractParameterData(gateway, "udp_wmem", udp_wmem);

	int fifo_sizes = default_fifos_size;
	extractParameterData(gateway, "fifos_size", fifo_sizes);  // TODO: add this to conf file?
	bool individual_fifos = false;
	extractParameterData(gateway, "individual_fifo_sizes", individual_fifos);  // TODO: add this to conf file?

	int ctrl_out_gw_fifo_size = fifo_sizes;
	int ctrl_in_gw_fifo_size = fifo_sizes;
	int ctrl_out_st_fifo_size = fifo_sizes;
	int ctrl_in_st_fifo_size = fifo_sizes;
	int logon_out_fifo_size = fifo_sizes;
	int logon_in_fifo_size = fifo_sizes;
	int data_out_st_fifo_size = fifo_sizes;
	int data_in_st_fifo_size = fifo_sizes;
	int data_out_gw_fifo_size = fifo_sizes;
	int data_in_gw_fifo_size = fifo_sizes;
	int isl_in_fifo_size = fifo_sizes;
	int isl_out_fifo_size = fifo_sizes;
	if (individual_fifos) {
		// TODO: add these to conf file?
		 extractParameterData(gateway, "ctrl_out_gw_fifo_size", ctrl_out_gw_fifo_size);
		 extractParameterData(gateway, "ctrl_in_gw_fifo_size", ctrl_in_gw_fifo_size);
		 extractParameterData(gateway, "ctrl_out_st_fifo_size", ctrl_out_st_fifo_size);
		 extractParameterData(gateway, "ctrl_in_st_fifo_size", ctrl_in_st_fifo_size);
		 extractParameterData(gateway, "logon_out_fifo_size", logon_out_fifo_size);
		 extractParameterData(gateway, "logon_in_fifo_size", logon_in_fifo_size);
		 extractParameterData(gateway, "data_out_st_fifo_size", data_out_st_fifo_size);
		 extractParameterData(gateway, "data_in_st_fifo_size", data_in_st_fifo_size);
		 extractParameterData(gateway, "data_out_gw_fifo_size", data_out_gw_fifo_size);
		 extractParameterData(gateway, "data_in_gw_fifo_size", data_in_gw_fifo_size);
		 extractParameterData(gateway, "isl_in_fifo_size", isl_in_fifo_size);
		 extractParameterData(gateway, "isl_out_fifo_size", isl_out_fifo_size);
	}

	carriers.logon_in = carrier_socket{
	    carrier_id + CarrierType::LOGON_IN,
	    satellite_st_address,
	    static_cast<uint16_t>(logon_in_port),
	    false,
	    static_cast<std::size_t>(logon_in_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.logon_out = carrier_socket{
	    carrier_id + CarrierType::LOGON_OUT,
	    gateway_address,
	    static_cast<uint16_t>(logon_out_port),
	    false,
	    static_cast<std::size_t>(logon_out_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.ctrl_in_st = carrier_socket{
	    carrier_id + CarrierType::CTRL_IN_ST,
	    satellite_st_address,
	    static_cast<uint16_t>(ctrl_in_st_port),
	    false,
	    static_cast<std::size_t>(ctrl_in_st_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.ctrl_out_gw = carrier_socket{
	    carrier_id + CarrierType::CTRL_OUT_GW,
	    gateway_address,
	    static_cast<uint16_t>(ctrl_out_gw_port),
	    false,
	    static_cast<std::size_t>(ctrl_out_gw_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.ctrl_in_gw = carrier_socket{
	    carrier_id + CarrierType::CTRL_IN_GW,
	    satellite_gw_address,
	    static_cast<uint16_t>(ctrl_in_gw_port),
	    false,
	    static_cast<std::size_t>(ctrl_in_gw_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.ctrl_out_st = carrier_socket{
	    carrier_id + CarrierType::CTRL_OUT_ST,
	    ctrl_multicast_address,
	    static_cast<uint16_t>(ctrl_out_st_port),
	    true,
	    static_cast<std::size_t>(ctrl_out_st_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.data_in_st = carrier_socket{
	    carrier_id + CarrierType::DATA_IN_ST,
	    satellite_st_address,
	    static_cast<uint16_t>(data_in_st_port),
	    false,
	    static_cast<std::size_t>(data_in_st_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.data_out_gw = carrier_socket{
	    carrier_id + CarrierType::DATA_OUT_GW,
	    gateway_address,
	    static_cast<uint16_t>(data_out_gw_port),
	    false,
	    static_cast<std::size_t>(data_out_gw_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.data_in_gw = carrier_socket{
	    carrier_id + CarrierType::DATA_IN_GW,
	    satellite_gw_address,
	    static_cast<uint16_t>(data_in_gw_port),
	    false,
	    static_cast<std::size_t>(data_in_gw_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};
	carriers.data_out_st = carrier_socket{
	    carrier_id + CarrierType::DATA_OUT_ST,
	    data_multicast_address,
	    static_cast<uint16_t>(data_out_st_port),
	    true,
	    static_cast<std::size_t>(data_out_st_fifo_size),
	    static_cast<unsigned>(udp_stack),
	    static_cast<unsigned>(udp_rmem),
	    static_cast<unsigned>(udp_wmem)
	};

	return true;
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
		if (!extractParameterData(spot_assignment, "gateway_id", gateway_id)) {
			return false;
		}
		if (gateway_id == gw_id) {
			selected_spot = spot_topology;
			break;
		}
	}

	if (selected_spot == nullptr) {
		return false;
	}

	if (!extractParameterData(selected_spot->getComponent("roll_off"), roll_off_parameter, spot.roll_off)) {
		return false;
	}

	freq_khz_t total_bandwidth = 0;
	std::vector<OpenSandModelConf::carrier> carriers;
	for (auto& carrier_item : selected_spot->getList(band_parameter)->getItems()) {
		auto carrier = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(carrier_item);

		double symbol_rate;
		if (!extractParameterData(carrier, "symbol_rate", symbol_rate)) {
			return false;
		}

		std::string category;
		if (!extractParameterData(carrier, "group", category)) {
			return false;
		}

		std::string access_type;
		if (!extractParameterData(carrier, "type", access_type)) {
			return false;
		}

		std::string format_ids;
		if (!extractParameterData(carrier, "wave_form", format_ids)) {
			return false;
		}

		std::string ratio = "1000";
		if (access_type == "VCM" && !extractParameterData(carrier, "ratio", ratio)) {
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

bool OpenSandModelConf::getInterconnectCarrier(bool upward,
                                               std::string &remote,
                                               unsigned int &data_port,
                                               unsigned int &sig_port,
                                               unsigned int &udp_stack,
                                               unsigned int &udp_rmem,
                                               unsigned int &udp_wmem,
											   std::size_t isl_index) const
{
	if (infrastructure == nullptr) {
		return false;
	}

	std::string type;
	tal_id_t id;
	if (!this->getComponentType(type, id)) {
		return false;
	}

	if (type != "gw_net_acc" && type != "gw_phy" && type != "sat")
	{
		return false;
	}

	std::string direction = upward ? "upward_" : "downward_";

	std::shared_ptr<OpenSANDConf::DataComponent> interco_params;
	if (type == "sat")
	{
		auto isl_settings = infrastructure->getRoot()
		                        ->getComponent("entity")
		                        ->getComponent("entity_" + type)
		                        ->getList("isl_settings")
		                        ->getItems();
		if (isl_settings.size() <= isl_index)
		{
			LOG(log, LEVEL_ERROR, "ISL configuration #%d requested but this satellite only have %d.",
			    isl_index, isl_settings.size());
			return false;
		}
		interco_params = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(isl_settings[isl_index])
		                     ->getComponent("interconnect_params");
	}
	else
	{
		interco_params = infrastructure->getRoot()
		                     ->getComponent("entity")
		                     ->getComponent("entity_" + type)
		                     ->getComponent("interconnect_params");
	}
	if (!extractParameterData(interco_params, "interconnect_remote", remote))
	{
		return false;
	}

	int data_port_value = upward ? 4500 : 4501;
	extractParameterData(interco_params, direction + "data_port", data_port_value);
	data_port = data_port_value;
	int sig_port_value = upward ? 4502 : 4503;
	extractParameterData(interco_params, direction + "sig_port", sig_port_value);
	sig_port = sig_port_value;

	int udp_stack_value = 5;
	extractParameterData(interco_params, "interco_udp_stack", udp_stack_value);
	udp_stack = udp_stack_value;
	int udp_rmem_value = 1048580;
	extractParameterData(interco_params, "interco_udp_rmem", udp_rmem_value);
	udp_rmem = udp_rmem_value;
	int udp_wmem_value = 1048580;
	extractParameterData(interco_params, "interco_udp_wmem", udp_wmem_value);
	udp_wmem = udp_wmem_value;

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
	auto defaults = assignments->getComponent("defaults");

	int spot_id;
	if (!extractParameterData(defaults, "default_gateway", spot_id)) {
		return false;
	}
	default_spot_id = spot_id;

	std::string category;
	if (!extractParameterData(defaults, "default_group", category)) {
		return false;
	}
	default_category_name = category;


	for (auto& terminal_assignment : assignments->getList("assignments")->getItems()) {
		auto terminal = std::dynamic_pointer_cast<OpenSANDConf::DataComponent>(terminal_assignment);

		int terminal_id;
		if (!extractParameterData(terminal, "terminal_id", terminal_id)) {
			return false;
		}
		if (!extractParameterData(terminal, "gateway_id", spot_id)) {
			return false;
		}
		if (!extractParameterData(terminal, "group", category)) {
			return false;
		}
		auto result = terminal_categories.emplace(terminal_id, std::make_pair(spot_id, category));
		if (!result.second) {
			return false;
		}
	}

	return true;
}


bool OpenSandModelConf::getDefaultSpotId(spot_id_t &default_spot_id) const
{
	if (topology == nullptr) {
		return false;
	}

	auto assignments = topology->getRoot()->getComponent("st_assignment");
	auto defaults = assignments->getComponent("defaults");

	int spot_id;
	if (!extractParameterData(defaults, "default_gateway", spot_id)) {
		return false;
	}
	default_spot_id = spot_id;

	return true;
}


const std::unordered_map<spot_id_t, SpotTopology> &OpenSandModelConf::getSpotsTopology() const
{
	return spots_topology;
}

RegenLevel OpenSandModelConf::getRegenLevel() const {
	assert(infrastructure != nullptr);
	auto entity = infrastructure->getRoot()->getComponent("entity");
	assert(entity != nullptr);
	auto sat = entity->getComponent("entity_sat");
	assert(sat != nullptr);

	std::string regen_level;
	extractParameterData(sat, "regen_level", regen_level);

	if (regen_level == "Transparent") {
		return RegenLevel::Transparent;
	} else if (regen_level == "BBFrame") {
		return RegenLevel::BBFrame;
	} else if (regen_level == "IP") {
		return RegenLevel::IP;
	}
	LOG(this->log, LEVEL_ERROR,
	    "The regen level \"%s\" is not supported", regen_level.c_str());
	return RegenLevel::Unknown;
}
