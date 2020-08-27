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
 * @file EntityGwPhy.cpp
 * @brief Entity physical gateway process
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
 *                         :
 *                         :
 *              Block Interconnect Upward
 *                         |
 *                  Sat Carrier Eth
 *                         |
 *                     eth nic 2
 *
 * </pre>
 *
 */


#include "EntityGwPhy.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"
#include "BlockInterconnect.h"


vector<string> EntityGwPhy::generateUsage(const string &progname) const
{
	vector<string> usage({
		progname + " " + this->getType() + " [-h] -i instance_id -a ip_address "
			"-w interconnect_addr -c conf_path [-f output_folder] [-r remote_address "
			"[-l logs_port] [-s stats_port]]",
		"\t-h                       print this message",
		"\t-a <ip_address>          set the IP address for emulation; this is the address",
		"\t                         this gateway should listen to for messages from the",
		"\t                         satellite",
		"\t-i <instance>            set the instance id",
		"\t-w <interconnect_addr>   set the interconnect IP address; this is the address",
		"\t                         this gateway should listen to for messages from the",
		"\t                         gw_net_acc part",
		"\t-c <conf_path>           specify the configuration folder path",
		"\t-f <output_folder>       activate and specify the folder for logs and probes",
		"\t                         files",
		"\t-r <remote_address>      activate and specify the address for logs and probes",
		"\t                         socket messages",
		"\t-l <logs_port>           specify the port for logs socket messages",
		"\t-s <stats_port>          specify the port for probes socket messages"});
	return usage;
}

bool EntityGwPhy::parseSpecificArguments(int argc, char **argv,
	string &name,
	string &conf_path,
	string &output_folder, string &remote_address,
	unsigned short &stats_port, unsigned short &logs_port)
{
	int opt;

	name = this->getType();

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-hi:a:u:w:c:f:r:l:s:")) != EOF)
	{
		switch(opt)
		{
		case 'i':
			// get instance id
			instance_id = atoi(optarg);
			name += optarg;
			break;
		case 'a':
			// get local IP address
			this->ip_address = optarg;
			break;
		case 'w':
			// Get the interconnect IP address
			this->interconnect_address = optarg;
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
			this->getType());
		return false;
	}

	if(this->interconnect_address.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: missing mandatory interconnect address option",
			this->getType());
		return false;
	}

	return true;
}

bool EntityGwPhy::createSpecificBlocks()
{
	struct sc_specific specific;

	Block *block_phy_layer;
	Block *block_sat_carrier;
	Block *block_interconnect;

	// instantiate all blocs
	block_interconnect = Rt::createBlock<BlockInterconnectUpward,
		       BlockInterconnectUpward::Upward,
		       BlockInterconnectUpward::Downward,
		       const string &>
		       ("InterconnectUpward", NULL, this->interconnect_address);
	if(!block_interconnect)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the InterconnectUpward block",
			this->getType());
		return false;
	}

	block_phy_layer = Rt::createBlock<BlockPhysicalLayer,
		    BlockPhysicalLayer::Upward,
		    BlockPhysicalLayer::Downward,
		    tal_id_t>("PhysicalLayer", block_interconnect, this->instance_id);
	if(!block_phy_layer)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the PhysicalLayer block",
		        this->getType());
		return false;;
	}
	specific.ip_addr = this->ip_address;
	specific.tal_id = this->instance_id;
	block_sat_carrier = Rt::createBlock<BlockSatCarrier,
		      BlockSatCarrier::Upward,
		      BlockSatCarrier::Downward,
		      struct sc_specific>("SatCarrier",
					  block_phy_layer,
					  specific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block",
			this->getType());
		return false;
	}

	return true;
}
