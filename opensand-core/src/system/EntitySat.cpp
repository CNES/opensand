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
 * @file EntitySat.cpp
 * @brief Entity satellite process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Mathias Ettinger <mathias.ettinger@viveris.fr>
 *
 * SE uses the following stack of mgl blocs installed over 1 NIC:
 *
 * <pre>
 *
 *                +---+
 *                |   |
 *            Encap/Desencap
 *                |   |
 *               Dvb Sat
 *                |   |
 *           Sat Carrier Eth
 *                |   |
 *               eth nic
 *
 * </pre>
 *
 */


#include "EntitySat.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "BlockDvbSatTransp.h"
#include "BlockSatCarrier.h"
#include "Plugin.h"
#include "OpenSandConf.h"


vector<string> EntitySat::generateUsage(const string &progname) const
{
	vector<string> usage({
		progname + " " + this->getType() + " [-h] -a ip_address -c conf_path "
			"[-f output_folder] [-r remote_address [-l logs_port] [-s stats_port]]",
		"\t-h                       print this message",
		"\t-a <ip_address>          set the IP address for emulation; this is the address",
		"\t                         this satellite should listen to for messages from other",
		"\t                         entities",
		"\t-c <conf_path>           specify the configuration folder path",
		"\t-f <output_folder>       activate and specify the folder for logs and probes",
		"\t                         files",
		"\t-r <remote_address>      activate and specify the address for logs and probes",
		"\t                         socket messages",
		"\t-l <logs_port>           specify the port for logs socket messages",
		"\t-s <stats_port>          specify the port for probes socket messages"});
	return usage;
}

bool EntitySat::parseSpecificArguments(int argc, char **argv,
	string &name,
	string &conf_path,
	string &output_folder, string &remote_address,
	unsigned short &stats_port, unsigned short &logs_port)
{
	int opt;

	name = this->getType();

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-ha:c:f:r:l:s:")) != EOF)
	{
		switch(opt)
		{
		case 'a':
			// get local IP address
			this->ip_address = optarg;
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

	return true;
}

bool EntitySat::createSpecificBlocks()
{
	struct sc_specific specific;

	Block *block_dvb;
	Block *block_sat_carrier;

	// instantiate all blocs
	block_dvb = Rt::createBlock<BlockDvbSatTransp,
	BlockDvbSatTransp::UpwardTransp,
	BlockDvbSatTransp::DownwardTransp>("Dvb", NULL);
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbSat block",
			this->getType());
		return false;
	}

	specific.ip_addr = this->ip_address;
	block_sat_carrier = Rt::createBlock<BlockSatCarrier,
	      BlockSatCarrier::Upward,
	      BlockSatCarrier::Downward,
	      struct sc_specific>("SatCarrier",
		  block_dvb,
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
