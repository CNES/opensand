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

#ifndef OPENSAND_MODEL_CONF_H
#define OPENSAND_MODEL_CONF_H


#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <opensand_conf/MetaComponent.h>
#include <opensand_conf/DataComponent.h>
#include <opensand_conf/DataParameter.h>
#include <opensand_conf/DataValue.h>
#include <opensand_output/Output.h>

#include "OpenSandCore.h"
#include "SpotComponentPair.h"


namespace OpenSANDConf {
	class MetaModel;
	class MetaTypesList;
	class MetaElement;
	class MetaParameter;
	class DataModel;
}
class SarpTable;


using MetaComponentPtr = std::shared_ptr<OpenSANDConf::MetaComponent>;


class OpenSandModelConf
{
public:
	struct fmt_definition_parameters {
		unsigned int id;
		std::string modulation_type;
		std::string coding_type;
		float spectral_efficiency;
		double required_es_no;
	};

	struct carrier_socket {
		uint16_t id;
		std::string address;
		uint16_t port;
		bool is_multicast;
		std::size_t fifo_size;
		unsigned int udp_stack;
		unsigned int udp_rmem;
		unsigned int udp_wmem;
	};

	struct spot_infrastructure {
		OpenSandModelConf::carrier_socket logon_in;
		OpenSandModelConf::carrier_socket logon_out;
		OpenSandModelConf::carrier_socket ctrl_in_st;
		OpenSandModelConf::carrier_socket ctrl_out_gw;
		OpenSandModelConf::carrier_socket ctrl_in_gw;
		OpenSandModelConf::carrier_socket ctrl_out_st;
		OpenSandModelConf::carrier_socket data_in_st;
		OpenSandModelConf::carrier_socket data_out_gw;
		OpenSandModelConf::carrier_socket data_in_gw;
		OpenSandModelConf::carrier_socket data_out_st;
	};

	struct carrier {
		AccessType access_type;
		std::string category;
		rate_symps_t symbol_rate;
		std::map<std::string, unsigned int> format_ratios;
		freq_khz_t bandwidth_khz;
	};

	struct spot {
		freq_khz_t bandwidth_khz;
		double roll_off;
		std::vector<OpenSandModelConf::carrier> carriers;
	};

	static std::shared_ptr<OpenSandModelConf> Get();
	~OpenSandModelConf();

	void createModels();
	std::shared_ptr<OpenSANDConf::DataComponent> getProfileData(const std::string &path="") const;
	std::shared_ptr<OpenSANDConf::MetaTypesList> getModelTypesDefinition() const;
	MetaComponentPtr getOrCreateComponent(const std::string &id,
	                                      const std::string &name,
	                                      MetaComponentPtr from=nullptr);
	MetaComponentPtr getOrCreateComponent(const std::string &id,
	                                      const std::string &name,
	                                      const std::string &description,
	                                      MetaComponentPtr from=nullptr);
	MetaComponentPtr getComponentByPath(const std::string &path,
	                                    std::shared_ptr<OpenSANDConf::MetaModel> model=nullptr);
	void setProfileReference(std::shared_ptr<OpenSANDConf::MetaElement> parameter,
	                         std::shared_ptr<OpenSANDConf::MetaParameter> referee,
	                         const char *expected_value);
	void setProfileReference(std::shared_ptr<OpenSANDConf::MetaElement> parameter,
	                         std::shared_ptr<OpenSANDConf::MetaParameter> referee,
	                         const std::string &expected_value);
	void setProfileReference(std::shared_ptr<OpenSANDConf::MetaElement> parameter,
	                         std::shared_ptr<OpenSANDConf::MetaParameter> referee,
	                         bool expected_value);

	bool writeTopologyModel(const std::string& filename) const;
	bool writeInfrastructureModel(const std::string& filename) const;
	bool writeProfileModel(const std::string& filename) const;

	bool readTopology(const std::string& filename);
	bool readInfrastructure(const std::string& filename);
	bool readProfile(const std::string& filename);

	template<typename T>
	static bool extractParameterData(std::shared_ptr<const OpenSANDConf::DataParameter> parameter, T &result);

	template <typename T>
	static bool extractParameterData(std::shared_ptr<const OpenSANDConf::DataComponent> component,
	                                 const std::string &parameter,
	                                 T &result);

	Component getComponentType() const;
	bool getComponentType(std::string &type, tal_id_t &id) const;
	bool getSatInfrastructure(std::string &ip_address, std::vector<IslConfig> &cfg) const;
	/**
	 * @brief: get infrastructure informations for ground entities
	 *
	 * @param: ip_address    Emulation Network IP address (except for Gateway Net Access:
	 *                       interconnection network IP) this entity is listening on.
	 * @param: tap_iface     Name of the tap interface to communicate from/with the real
	 *                       network (except for Gateway Phy: interconnection network IP).
	 */
	bool getGroundInfrastructure(std::string &ip_address, std::string &tap_iface) const;
	bool getLocalStorage(bool &enabled, std::string &output_folder) const;
	bool getRemoteStorage(bool &enabled,
	                      std::string &address,
	                      uint16_t &stats_port,
	                      uint16_t &logs_port) const;
	bool logLevels(std::map<std::string, log_level_t> &levels) const;
	bool getSarp(SarpTable &sarp_table) const;
	bool getNccPorts(uint16_t &pep_tcp_port, uint16_t &svno_tcp_port) const;
	bool getQosServerHost(std::string &qos_server_host_agent, uint16_t &qos_server_host_port) const;
	bool getS2WaveFormsDefinition(std::vector<fmt_definition_parameters> &fmt_definitions) const;
	bool getRcs2WaveFormsDefinition(std::vector<fmt_definition_parameters> &fmt_definitions,
	                                vol_sym_t req_burst_length) const;
	bool getRcs2BurstLength(vol_sym_t &length_sym) const;
	bool getSuperframePerSlottedAlohaFrame(time_sf_t &sf_per_saframe) const;
	bool getCrdsaMaxSatelliteDelay(time_ms_t &sat_delay) const;
	bool getPepAllocationDelay(int &pep_allocation_delay) const;
	bool getReturnFrameDuration(time_us_t &frame_duration) const;
	bool getForwardFrameDuration(time_us_t &frame_duration) const;
	bool getReturnAcmLoopMargin(double &margin) const;
	bool getForwardAcmLoopMargin(double &margin) const;
	bool getStatisticsPeriod(time_ms_t &period) const;
	bool getSynchroPeriod(time_ms_t &period) const;
	bool getAcmRefreshPeriod(time_ms_t &period) const;
	bool getDelayBufferSize(std::size_t &size) const;
	bool getDelayTimer(time_ms_t &period) const;
	bool getControlPlaneDisabled(bool &disabled) const;
	bool getGwWithTalId(tal_id_t terminal_id, tal_id_t &gw_id) const;
	bool getGwWithCarrierId(unsigned int carrier_id, tal_id_t &gw) const;
	bool isGw(tal_id_t gw_id) const;
	std::unordered_set<tal_id_t> getSatellites() const;
	Component getEntityType(tal_id_t tal_id) const;
	bool getScpcEnabled(bool &scpc_enabled) const;
	bool getSpotInfrastructure(tal_id_t gw_id, OpenSandModelConf::spot_infrastructure &carriers) const;
	bool getSpotReturnCarriers(tal_id_t gw_id, OpenSandModelConf::spot &spot) const;
	bool getSpotForwardCarriers(tal_id_t gw_id, OpenSandModelConf::spot &spot) const;
	bool getInterconnectCarrier(bool upward_connection,
	                            std::string &remote_address,
	                            uint16_t &data_port,
	                            uint16_t &sig_port,
	                            unsigned int &udp_stack,
	                            unsigned int &udp_rmem,
	                            unsigned int &udp_wmem,
								std::size_t isl_index = 0) const;
	bool getTerminalAffectation(spot_id_t &default_spot_id,
	                            std::string &default_category_name,
	                            std::map<tal_id_t, std::pair<spot_id_t, std::string>> &terminal_categories) const;

	bool getDefaultSpotId(spot_id_t &default_spot_id) const;
	const std::unordered_map<spot_id_t, SpotTopology> &getSpotsTopology() const;
	RegenLevel getRegenLevel() const;

private:
	OpenSandModelConf();

	std::shared_ptr<OpenSANDConf::MetaModel> topology_model;
	std::shared_ptr<OpenSANDConf::MetaModel> infrastructure_model;
	std::shared_ptr<OpenSANDConf::MetaModel> profile_model;

	std::shared_ptr<OpenSANDConf::DataModel> topology;
	std::shared_ptr<OpenSANDConf::DataModel> infrastructure;
	std::shared_ptr<OpenSANDConf::DataModel> profile;

	std::shared_ptr<OutputLog> log;
	
	std::unordered_map<tal_id_t, Component> entities_type;
	std::unordered_map<spot_id_t, SpotTopology> spots_topology;

	bool getSpotCarriers(uint16_t gw_id, OpenSandModelConf::spot &spot, bool forward) const;
};


template<typename T>
bool OpenSandModelConf::extractParameterData(std::shared_ptr<const OpenSANDConf::DataParameter> parameter,
                                             T &result)
{
	if (parameter == nullptr)
	{
		return false;
	}

	auto data = std::dynamic_pointer_cast<OpenSANDConf::DataValue<T>>(parameter->getData());
	if (data == nullptr || !data->isSet())
	{
		return false;
	}

	result = data->get();
	return true;
}


template<typename T>
bool OpenSandModelConf::extractParameterData(std::shared_ptr<const OpenSANDConf::DataComponent> component,
                                             const std::string& parameter,
                                             T &result)
{
	if (component == nullptr)
	{
		DFLTLOG(LEVEL_ERROR,
		        "Conf: Trying to extract parameter %s from NULL component",
		        parameter.c_str());
		return false;
	}

	auto path = component->getPath();
	DFLTLOG(LEVEL_INFO,
	        "Conf: Extracting %s parameter from component %s",
	        parameter.c_str(), path.c_str());

	if (!extractParameterData(component->getParameter(parameter), result))
	{
		DFLTLOG(LEVEL_NOTICE,
		        "Conf: Extracting %s/%s failed, default value used instead",
		        path.c_str(), parameter.c_str());
		return false;
	}

	std::stringstream s;
	s << result;
	std::string result_string = s.str();
	DFLTLOG(LEVEL_DEBUG, "Conf: Extracted value %s", result_string);

	return true;
}


#endif  // OPENSAND_MODEL_CONF_H
