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
#include <map>

#include "OpenSandCore.h"
#include "SarpTable.h"
#include <opensand_output/OutputLog.h>


namespace OpenSANDConf {
	class MetaModel;
	class DataModel;
}


class OpenSandModelConf
{
	public:
		struct carrier_socket {
			uint16_t id;
			std::string address;
			uint16_t port;
			bool is_multicast;
			unsigned int udp_stack;
			unsigned int udp_rmem;
			unsigned int udp_wmem;
		};

		struct spot_infrastructure {
			OpenSandModelConf::carrier_socket ctrl_out;
			OpenSandModelConf::carrier_socket ctrl_in;
			OpenSandModelConf::carrier_socket logon_out;
			OpenSandModelConf::carrier_socket logon_in;
			OpenSandModelConf::carrier_socket data_out_st;
			OpenSandModelConf::carrier_socket data_in_st;
			OpenSandModelConf::carrier_socket data_out_gw;
			OpenSandModelConf::carrier_socket data_in_gw;
		};

		struct carrier {
			access_type_t access_type;
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

		void createModels ();
		std::shared_ptr<OpenSANDConf::MetaModel> getProfileModel () const;

		bool writeTopologyModel (const std::string& filename) const;
		bool writeInfrastructureModel (const std::string& filename) const;
		bool writeProfileModel (const std::string& filename) const;

		bool readTopology (const std::string& filename);
		bool readInfrastructure (const std::string& filename);
		bool readProfile (const std::string& filename);

		component_t getComponentType() const;
		bool getComponentType(std::string &type, tal_id_t &id) const;
		bool getLocalStorage(bool &enabled, std::string &output_folder) const;
		bool getRemoteStorage(bool &enabled, std::string &address, unsigned short &stats_port, unsigned short &logs_port) const;
		bool getGwIds(std::vector<tal_id_t> &gws) const;
		bool logLevels(std::map<std::string, log_level_t> &levels, std::map<std::string, log_level_t> &specific) const;
		bool getSarp(SarpTable &sarp_table) const;
		bool getGwWithTalId(tal_id_t terminal_id, tal_id_t &gw_id) const;
		bool getGwWithCarrierId(unsigned int carrier_id, tal_id_t &gw) const;
		bool isGw(tal_id_t gw_id) const;
		bool getScpcEncapStack(std::vector<std::string> &encap_stack) const;
		bool getSpotInfrastructure(tal_id_t gw_id, OpenSandModelConf::spot_infrastructure &carriers) const;
		bool getSpotReturnCarriers(tal_id_t gw_id, OpenSandModelConf::spot &spot) const;
		bool getSpotForwardCarriers(tal_id_t gw_id, OpenSandModelConf::spot &spot) const;
		bool getTerminalAffectation(spot_id_t &default_spot_id,
		                            std::string &default_category_name,
		                            std::map<tal_id_t, std::pair<spot_id_t, std::string>> &terminal_categories) const;

	private:
		OpenSandModelConf();

		std::shared_ptr<OpenSANDConf::MetaModel> topology_model;
		std::shared_ptr<OpenSANDConf::MetaModel> infrastructure_model;
		std::shared_ptr<OpenSANDConf::MetaModel> profile_model;

		std::shared_ptr<OpenSANDConf::DataModel> topology;
		std::shared_ptr<OpenSANDConf::DataModel> infrastructure;
		std::shared_ptr<OpenSANDConf::DataModel> profile;

		bool getSpotCarriers(uint16_t gw_id, OpenSandModelConf::spot &spot, bool forward) const;
};


#endif  // OPENSAND_MODEL_CONF_H
