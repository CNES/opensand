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
 * @file EntityGwNetAcc.cpp
 * @brief Entity network/access gateway process
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
 *           Block Interconnect Downward
 *                         :
 *
 * </pre>
 *
 */


#include "EntityGwNetAcc.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "BlockInterconnect.h"
#include "BlockLanAdaptation.h"
#include "BlockDvbNcc.h"
#include "BlockEncap.h"


vector<string> EntityGwNetAcc::generateUsage(const string &progname) const
{
	vector<string> usage({
		progname + " " + this->getType() + " [-h] -i instance_id "
			"-t tap_iface -w interconnect_addr -c conf_path "
			"[-f output_folder] [-r remote_address [-l logs_port] [-s stats_port]]",
		"\t-h                       print this message",
		"\t-t <tap_iface>           set the GW TAP interface name",
		"\t-i <instance>            set the instance id",
		"\t-w <interconnect_addr>   set the interconnect IP address; this is the address",
		"\t                         this gateway should listen to for messages from the",
		"\t                         gw_phy part",
		"\t-c <conf_path>           specify the configuration folder path",
		"\t-f <output_folder>       activate and specify the folder for logs and probes",
		"\t                         files",
		"\t-r <remote_address>      activate and specify the address for logs and probes",
		"\t                         socket messages",
		"\t-l <logs_port>           specify the port for logs socket messages",
		"\t-s <stats_port>          specify the port for probes socket messages"});
	return usage;
}

bool EntityGwNetAcc::parseSpecificArguments(int argc, char **argv,
	string &name,
	string &conf_path,
	string &output_folder, string &remote_address,
	unsigned short &stats_port, unsigned short &logs_port)
{
	int opt;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-hi:t:u:w:c:f:r:l:s:")) != EOF)
	{
		switch(opt)
		{
		case 'i':
			// get instance id
			this->instance_id = atoi(optarg);
			name += optarg;
			break;
		case 't':
			// get TAP interface name
			this->tap_iface = optarg;
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

	if(this->tap_iface.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: missing mandatory TAP interface name option",
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

bool EntityGwNetAcc::createSpecificBlocks()
{
	struct la_specific spec_la;

	Block *block_lan_adaptation;
	Block *block_encap;
	Block *block_dvb;
	Block *block_interconnect;

	// instantiate all blocs
	spec_la.tap_iface = this->tap_iface;
	block_lan_adaptation = Rt::createBlock<BlockLanAdaptation,
			 BlockLanAdaptation::Upward,
			 BlockLanAdaptation::Downward,
			 struct la_specific>("LanAdaptation", NULL, spec_la);
	if(!block_lan_adaptation)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the LanAdaptation block",
		        this->getType());
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
			this->getType());
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
			this->getType());
		return false;
	}

	block_interconnect = Rt::createBlock<BlockInterconnectDownward,
		       BlockInterconnectDownward::Upward,
		       BlockInterconnectDownward::Downward,
		       const string &>
		       ("InterconnectDownward", block_dvb, this->interconnect_address);
	if(!block_interconnect)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the InterconnectDownward block",
			this->getType());
		return false;
	}

	return true;
}
