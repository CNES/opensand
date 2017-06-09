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
 * @brief Gateway Physical (GW-PHY) process
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
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

#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"
#include "BlockInterconnectUpward.h"
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
                  tal_id_t &instance_id,
                  uint16_t &port_up,
                  uint16_t &port_down,
                  string &ip_top,
                  string &conf_path)
{
	// TODO remove lan_iface and handle bridging in daemon
	int opt;
	bool output_enabled = true;
	bool output_stdout = false;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-hqdi:a:n:t:u:w:c:")) != EOF)
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
		case 't':
			// get the GW_LAN_ACC ip address
			ip_top = optarg;
			break;
		case 'u':
			// Get the upward connection port
			port_up = (uint16_t) atoi(optarg);
			break;
		case 'w':
			// Get the downward connection port
			port_down = (uint16_t) atoi(optarg);
			break;
		case 'c':
			// get the configuration path
			conf_path = optarg;
			break;
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [-q] [-d] -i instance_id -a ip_address "
			        "-n emu_iface -t ip_address -u upward_port -d downward_port -c conf_path\n",
			        argv[0]);
			fprintf(stderr, "\t-h                   print this message\n");
			fprintf(stderr, "\t-q                   disable output\n");
			fprintf(stderr, "\t-d                   enable output debug events\n");
			fprintf(stderr, "\t-a <ip_address>      set the IP address for emulation\n");
			fprintf(stderr, "\t-n <emu_iface>       set the emulation interface name\n");
			fprintf(stderr, "\t-i <instance>        set the instance id\n");
			fprintf(stderr, "\t-t <ip_address>      set the IP address of top GW\n");
			fprintf(stderr, "\t-u <upward_port>     set the upward port\n");
			fprintf(stderr, "\t-w <donwward_port>   set the downward port\n");
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

	if(ip_top.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory gw_lan_acc IP address option");
		return false;
	}

	if(conf_path.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory configuration path option");
		return false;
	}

	if(port_up == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory port upward option");
		return false;
	}

	if(port_down == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory port downward option");
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
	tal_id_t mac_id = 0;
	struct sc_specific specific;
	uint16_t port_up = 0;
	uint16_t port_down = 0;
	string ip_top;
	struct icu_specific spec_icu;

	string conf_path;
	string topology_file;
	string global_file;
	string default_file;
	string plugin_conf_path;

	Block *block_phy_layer;
	Block *block_sat_carrier;
	Block *block_interconnect;

	vector<string> conf_files;
	map<string, log_level_t> levels;
	map<string, log_level_t> spec_level;

	OutputEvent *status;

	int is_failure = 1;

	// retrieve arguments on command line
	init_ok = init_process(argc, argv, ip_addr, emu_iface, mac_id,
	                       port_up, port_down, ip_top, conf_path);

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

	spec_icu.ip_addr = ip_top;
	spec_icu.port_upward = port_up;
	spec_icu.port_downward = port_down;

	block_interconnect = Rt::createBlock<BlockInterconnectUpward,
	                                     BlockInterconnectUpward::Upward,
	                                     BlockInterconnectUpward::Downward,
	                                     struct icu_specific>
	                                     ("InterconnectUpward", NULL, spec_icu);
	if(!block_interconnect)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the InterconnectUpward block\n", progname);
		goto release_plugins;
	}

	block_phy_layer = Rt::createBlock<BlockPhysicalLayer,
																		BlockPhysicalLayer::Upward,
																		BlockPhysicalLayer::Downward,
	                                  tal_id_t>("PhysicalLayer",
	                                            block_interconnect,
																		          mac_id);
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
