/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
 * Copyright © 2016 CNES
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
 * @file gw.cpp
 * @brief Gateway (GW) process
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 * @author Aurelien DELRIEU <adelrieu@toulouse.viveris.com>
 * @author Joaquin MUGUERZA <jmuguerza@toulouse.viveris.com>
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


#include "BlockLanAdaptation.h"
#include "BlockDvbNcc.h"
#include "BlockSatCarrier.h"
#include "BlockEncap.h"
#include "BlockPhysicalLayer.h"
#include "Plugin.h"
#include "OpenSandConf.h"

#include <opensand_conf/ConfigurationFile.h>

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


/**
 * Argument treatment
 */
bool init_process(int argc, char **argv, 
				  string &ip_addr,
                  string &emu_iface, 
                  string &lan_iface,
                  string &conf_path,
                  tal_id_t &instance_id)
{
	// TODO remove lan_iface and handle bridging in daemon
	int opt;
	bool output_enabled = true;
	bool output_stdout = false;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-hqdi:a:n:l:c:")) != EOF)
	{
		switch(opt)
		{
		case 'q':
			// disable output
			output_enabled = false;
			break;
		case 'd':
			// enable output debug
			output_stdout = true;;
			break;
		case 'i':
			// get instance id
			instance_id = atoi(optarg);
			break;
		case 'a':
			// get local IP address
			ip_addr = optarg;
			break;
		case 'n':
			// get local interface name
			emu_iface = optarg;
			break;
		case 'l':
			// get lan interface name
			lan_iface = optarg;
			break;
		case 'c':
			// get the configuration path
			conf_path = optarg;
			break;
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [[-q] [-d] -i instance_id -a ip_address "
				"-n emu_iface -l lan_iface -c conf_path\n",
			        argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
			fprintf(stderr, "\t-q                   disable output\n");
			fprintf(stderr, "\t-d                   enable output debug events\n");
			fprintf(stderr, "\t-a <ip_address>      set the IP address for emulation\n");
			fprintf(stderr, "\t-n <emu_iface>       set the emulation interface name\n");
			fprintf(stderr, "\t-l <lan_iface>       set the ST lan interface name\n");
			fprintf(stderr, "\t-i <instance>        set the instance id\n");
			fprintf(stderr, "\t-c <conf_path>       specify the configuration path\n");
			Output::init(true);
			Output::enableStdlog();
			return false;
		}
	}

	// output initialisation
	Output::init(output_enabled);
	if(output_stdout)
	{
		Output::enableStdlog();
	}

	DFLTLOG(LEVEL_NOTICE,
	        "starting output\n");

	if(ip_addr.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory IP address option");
		return false;
	}

	if(emu_iface.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory emulation interface name option");
		return false;
	}

	if(lan_iface.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory lan interface name option");
		return false;
	}

	if(conf_path.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory configuration path option");
		return false;
	}
	return true;
}


int main(int argc, char **argv)
{
	const char *progname = argv[0];
	struct sched_param param;
	bool init_ok;
	string ip_addr;
	string emu_iface;
	string lan_iface;
	tal_id_t mac_id = 0;
	struct sc_specific specific;

	string topology_file;
	string global_file;
	string default_file;
	string conf_path;
	string plugin_conf_path;

	Block *block_lan_adaptation;
	Block *block_encap;
	Block *block_dvb;
	Block *block_phy_layer;
	Block *block_sat_carrier;

	vector<string> conf_files;
	map<string, log_level_t> levels;
	map<string, log_level_t> spec_level;

	OutputEvent *status;

	int is_failure = 1;

	// retrieve arguments on command line
	init_ok = init_process(argc, argv, ip_addr, emu_iface, lan_iface, conf_path, mac_id);

  plugin_conf_path = conf_path + string("plugins/");

	status = Output::registerEvent("Status");
	if(!init_ok)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: failed to init the process\n", progname);
		goto quit;
	}

	// increase the realtime responsiveness of the process
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &param);

	topology_file = conf_path + string(CONF_TOPOLOGY);
	global_file = conf_path + string(CONF_GLOBAL_FILE);
	default_file = conf_path + string(CONF_DEFAULT_FILE);

	conf_files.push_back(topology_file.c_str());
	conf_files.push_back(global_file.c_str());
	conf_files.push_back(default_file.c_str());
	// Load configuration files content
	if(!Conf::loadConfig(conf_files))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load configuration files, quit\n",
		        progname);
		goto quit;
	}
	
	OpenSandConf::loadConfig();
	
	// read all packages debug levels
	if(!Conf::loadLevels(levels, spec_level))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load default levels, quit\n",
		        progname);
		goto quit;
	}
	Output::setLevels(levels, spec_level);

	// load the plugins
	if(!Plugin::loadPlugins(true, plugin_conf_path))
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot load the plugins\n", progname);
		goto quit;
	}

	// instantiate all blocs
	block_lan_adaptation = Rt::createBlock<BlockLanAdaptation,
	                                       BlockLanAdaptation::Upward,
	                                       BlockLanAdaptation::Downward,
	                                       string>("LanAdaptation", NULL, lan_iface);
	if(!block_lan_adaptation)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the LanAdaptation block\n",
		        progname);
		goto release_plugins;
	}
	block_encap = Rt::createBlock<BlockEncap,
	                              BlockEncap::Upward,
	                              BlockEncap::Downward,
	                              tal_id_t>("Encap", block_lan_adaptation, mac_id);
	if(!block_encap)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the Encap block\n", progname);
		goto release_plugins;
	}

	block_dvb = Rt::createBlock<BlockDvbNcc,
	                            BlockDvbNcc::Upward,
	                            BlockDvbNcc::Downward,
	                            tal_id_t>("Dvb", block_encap, mac_id);
	if(!block_dvb)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the DvbNcc block\n", progname);
		goto release_plugins;
	}

	block_phy_layer = Rt::createBlock<BlockPhysicalLayer,
																		BlockPhysicalLayer::Upward,
																		BlockPhysicalLayer::Downward>("PhysicalLayer",
																		                              block_dvb, mac_id);
	if(block_phy_layer == NULL)
	{
		DFLTLOG(LEVEL_CRITICAL,
						"%s: cannot create the PhysicalLayer block\n",
						progname);
		goto release_plugins;
	}

	specific.ip_addr = ip_addr;
	specific.emu_iface = emu_iface; // TODO emu_iface in struct
	specific.tal_id = mac_id;
	block_sat_carrier = Rt::createBlock<BlockSatCarrier,
	                                    BlockSatCarrier::Upward,
	                                    BlockSatCarrier::Downward,
	                                    struct sc_specific>("SatCarrier",
	                                                        block_phy_layer,
	                                                        specific);
	if(!block_sat_carrier)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the SatCarrier block\n", progname);
		goto release_plugins;
	}

	DFLTLOG(LEVEL_DEBUG,
	        "All blocks are created, start\n");

	// make the GW alive
	if(!Rt::init())
	{
		goto release_plugins;
	}
	if(!Output::finishInit())
	{
		DFLTLOG(LEVEL_NOTICE,
		        "%s: failed to init the output => disable it\n",
		        progname);
	}

	Output::sendEvent(status, "Blocks initialized");
	if(!Rt::run())
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot run process loop\n",
		        progname);
	}

	Output::sendEvent(status, "Simulation stopped");

	// everything went fine, so report success
	is_failure = 0;

	// cleanup before GW stops
release_plugins:
	Plugin::releasePlugins();
quit:
	DFLTLOG(LEVEL_NOTICE,
	        "%s: GW process stopped with exit code %d\n",
	        progname, is_failure);
	return is_failure;
}
