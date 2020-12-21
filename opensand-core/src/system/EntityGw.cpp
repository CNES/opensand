/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2020 TAS
 * Copyright © 2020 CNES
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
 * @file EntityGw.cpp
 * @brief Entity gateway process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 *
 * Gateway uses the following stack of blocks installed over 2 NICs
 * (nic1 on user network side and nic2 on satellite network side):
 *
 * <pre>
 *
 *                     eth nic 1
 *                         |
 *                   Lan Adaptation  ---------
 *                         |                  |
 *                   Encap/Desencap      IpMacQoSInteraction
 *                         |                  |
 *                      Dvb Ncc  -------------
 *                 [Dama Controller]
 *                         |
 *                  Sat Carrier Eth
 *                         |
 *                     eth nic 2
 *
 * </pre>
 *
 */


#include "EntityGw.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "BlockLanAdaptation.h"
#include "BlockDvbNcc.h"
#include "BlockSatCarrier.h"
#include "BlockEncap.h"
#include "BlockPhysicalLayer.h"

vector<string> EntityGw::generateUsage(const string &progname) const
{
	vector<string> usage({
		progname + " " + this->getType() + " [-h] -i instance_id -a ip_address "
			"-t tap_iface -c conf_path [-f output_folder] [-r remote_address "
			"[-l logs_port] [-s stats_port]]",
		"\t-h                       print this message",
		"\t-a <ip_address>          set the IP address for emulation; this is the address",
		"\t                         this gateway should listen to for messages from the",
		"\t                         satellite",
		"\t-t <tap_iface>           set the GW TAP interface name",
		"\t-i <instance>            set the instance id",
		"\t-c <conf_path>           specify the configuration folder path",
		"\t-f <output_folder>       activate and specify the folder for logs and probes",
		"\t                         files",
		"\t-r <remote_address>      activate and specify the address for logs and probes",
		"\t                         socket messages",
		"\t-l <logs_port>           specify the port for logs socket messages",
		"\t-s <stats_port>          specify the port for probes socket messages"});
	return usage;
}

bool EntityGw::parseSpecificArguments(int argc, char **argv,
	string &name,
	string &conf_path,
	string &output_folder, string &remote_address,
	unsigned short &stats_port, unsigned short &logs_port)
{
	int opt;

	name = this->getType();

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-hi:a:t:c:f:r:l:s:")) != EOF)
	{
		switch(opt)
		{
		case 'i':
			// get instance id
			this->instance_id = atoi(optarg);
			name += optarg;
			break;
		case 'a':
			// get local IP address
			this->ip_address = optarg;
			break;
		case 't':
			// get TAP interface name
			this->tap_iface = optarg;
			break;
		case 'c':
			// get the configuration path
			conf_path = optarg;
			break;
		case 'f':
			output_folder = optarg;
			break;
		case 'r':
			remote_address = optarg;
			break;
		case 'l':
			logs_port = atoi(optarg);
			break;
		case 's':
			stats_port = atoi(optarg);
			break;
		case 'h':
		case '?':
			return false;
		}
	}

	if(this->ip_address.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: missing mandatory IP address option",
            this->getType().c_str());
		return false;
	}

	if(this->tap_iface.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: missing mandatory TAP interface name option",
            this->getType().c_str());
		return false;
	}

	return true;
}

bool EntityGw::createSpecificBlocks()
{
	struct la_specific laspecific;
	struct sc_specific scspecific;

	Block *block_lan_adaptation;
	Block *block_encap;
	Block *block_dvb;
	Block *block_phy_layer;
	Block *block_sat_carrier;

	// instantiate all blocs
	laspecific.tap_iface = this->tap_iface;
	block_lan_adaptation = Rt::createBlock<BlockLanAdaptation,
		 BlockLanAdaptation::Upward,
		 BlockLanAdaptation::Downward,
		 struct la_specific>("LanAdaptation", NULL, laspecific);
	if(!block_lan_adaptation)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the LanAdaptation block",
		        this->getType().c_str());
		return false;
	}
	block_encap = Rt::createBlock<BlockEncap,
		BlockEncap::Upward,
		BlockEncap::Downward,
		tal_id_t>("Encap", block_lan_adaptation, this->instance_id);
	if(!block_encap)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the Encap block",
            this->getType().c_str());
		return false;
	}

	block_dvb = Rt::createBlock<BlockDvbNcc,
		BlockDvbNcc::Upward,
		BlockDvbNcc::Downward,
		tal_id_t>("Dvb", block_encap, this->instance_id);
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbNcc block",
            this->getType().c_str());
		return false;
	}

	block_phy_layer = Rt::createBlock<BlockPhysicalLayer,
		BlockPhysicalLayer::Upward,
		BlockPhysicalLayer::Downward,
		tal_id_t>("PhysicalLayer", block_dvb, this->instance_id);
	if(!block_phy_layer)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the PhysicalLayer block",
		        this->getType().c_str());
		return false;
	}

	scspecific.ip_addr = this->ip_address;
	scspecific.tal_id = this->instance_id;
	block_sat_carrier = Rt::createBlock<BlockSatCarrier,
		BlockSatCarrier::Upward,
		BlockSatCarrier::Downward,
		struct sc_specific>("SatCarrier",
		                    block_phy_layer,
		                    scspecific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block",
            this->getType().c_str());
		return false;
	}
	return true;
}
