/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
 * Copyright © 2019 CNES
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
 * @brief Gateway Network Access (GW-NET-ACC) process
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
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

#include "BlockInterconnect.h"
#include "BlockLanAdaptation.h"
#include "BlockDvbNcc.h"
#include "BlockEncap.h"
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
                  string &lan_iface,
                  tal_id_t &instance_id,
                  string &interconnect_iface,
                  string &interconnect_addr,
                  string &conf_path)
{
	// TODO remove lan_iface and handle bridging in daemon
	int opt;
	bool output_enabled = true;
	bool output_stdout = false;
	bool stop = false;
	string lib_external_output_path = "";
	char entity[10];
	
	/* setting environment agent parameters */
	while(!stop && (opt = getopt(argc, argv, "-hqdi:l:u:w:c:e:")) != EOF)
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
		case 'l':
			// get lan interface name
			lan_iface = optarg;
			break;
		case 'u':
			// Get the interconnect interface name
			interconnect_iface = optarg;
			break;
		case 'w':
			// Get the interconnect IP address
			interconnect_addr = optarg;
			break;
		case 'c':
			// get the configuration path
			conf_path = optarg;
			break;
		case 'e':
			// get library external path
			lib_external_output_path = optarg;
			break;
		case 'h':
		case '?':
			fprintf(stderr, "usage: %s [-h] [[-q] [-d] -i instance_id "
			        "-l lan_iface -u interconnect_iface -w interconnect_addr -c conf_path -e lib_ext_output_path\n",
			        argv[0]);
			fprintf(stderr, "\t-h                       print this message\n");
			fprintf(stderr, "\t-q                       disable output\n");
			fprintf(stderr, "\t-d                       enable output debug events\n");
			fprintf(stderr, "\t-l <lan_iface>           set the ST lan interface name\n");
			fprintf(stderr, "\t-i <instance>            set the instance id\n");
			fprintf(stderr, "\t-u <interconnect_iface>  set the interconnect interface name\n");
			fprintf(stderr, "\t-w <interconnect_addr>   set the interconnect IP address\n");
			fprintf(stderr, "\t-c <conf_path>           specify the configuration path\n");
			fprintf(stderr, "\t-e <lib_ext_output_path> specify the external output library path\n");
			stop = true;
			break;
		}
	}

	if(lib_external_output_path != "")
	{
		sprintf(entity, "gw%d", instance_id);
		// external output initialization
		if(!Output::initExt(output_enabled, (const char *)entity, lib_external_output_path.c_str()))
		{
			stop = true;
			fprintf(stderr, "Unable to initialize external output library\n");
		}
	}
	else
	{
		// output initialization
		if(!Output::init(output_enabled)) 
		{
			stop = true;
			fprintf(stderr, "Unable to initialize output library\n");
		}
	}
	if(output_stdout)
	{
		Output::enableStdlog();
	}
	if(stop)
	{
		return false;
	}

	DFLTLOG(LEVEL_NOTICE,
	        "starting output\n");

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
	
	if(interconnect_iface.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory interconnect interface option");
		return false;
	}
	return true;
	
	if(interconnect_addr.size() == 0)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "missing mandatory interconnect address option");
		return false;
	}
}


int main(int argc, char **argv)
{
	const char *progname = argv[0];
	struct sched_param param;
	bool init_ok;
	string lan_iface;
	tal_id_t mac_id = 0;
	string interconnect_iface;
	string interconnect_addr;
	struct ic_specific spec_ic;

	string conf_path;
	string topology_file;
	string global_file;
	string default_file;
	string plugin_conf_path;

	Block *block_lan_adaptation;
	Block *block_encap;
	Block *block_dvb;
	Block *block_interconnect;

	vector<string> conf_files;
	map<string, log_level_t> levels;
	map<string, log_level_t> spec_level;

	OutputEvent *status;

	int is_failure = 1;

	// retrieve arguments on command line
	init_ok = init_process(argc, argv, lan_iface, mac_id,
	                       interconnect_iface, interconnect_addr, conf_path);

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
	if(!Plugin::loadPlugins(false, plugin_conf_path))
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

	spec_ic.interconnect_iface = interconnect_iface;
	spec_ic.interconnect_addr = interconnect_addr;

	block_interconnect = Rt::createBlock<BlockInterconnectDownward,
	                                     BlockInterconnectDownward::Upward,
	                                     BlockInterconnectDownward::Downward,
	                                     struct ic_specific>
	                                     ("InterconnectDownward", block_dvb, spec_ic);
	if(!block_interconnect)
	{
		DFLTLOG(LEVEL_CRITICAL,
		        "%s: cannot create the InterconnectDownward block\n", progname);
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
	Output::close();
	return is_failure;
}
