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
 * @file gw.cpp
 * @brief Gateway Physical (GW-PHY) process
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.com>
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

#include "BlockSatCarrier.h"
#include "BlockPhysicalLayer.h"
#include "BlockInterconnect.h"
#include "Plugin.h"
#include "OpenSandConf.h"

#include <opensand_conf/ConfigurationFile.h>

#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>


/**
 * Argument treatment
 */
bool init_process(int argc, char **argv,
                  std::string &ip_addr,
                  tal_id_t &instance_id,
                  std::string &interconnect_addr,
                  std::string &conf_path)
{
  int opt;
  std::string output_folder = "";
  std::string remote_address = "";
  unsigned short stats_port = 12345;
  unsigned short logs_port = 23456;
  bool stop = false;
  std::string entity{"gw_phy"};

  /* setting environment agent parameters */
  while(!stop && (opt = getopt(argc, argv, "-hi:a:u:w:c:f:r:l:s:")) != EOF)
  {
    switch(opt)
    {
    case 'i':
      // get instance id
      instance_id = atoi(optarg);
      entity += optarg;
      break;
    case 'a':
      // get local IP address
      ip_addr = optarg;
      break;
    case 'w':
      // Get the interconnect IP address
      interconnect_addr = optarg;
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
      std::cerr << "usage: " << argv[0] << " [-h] -i instance_id -a ip_address "
                   "-w interconnect_addr -c conf_path [-f output_folder] [-r remote_address "
                   "[-l logs_port] [-s stats_port]]\n"
                   "\t-h                       print this message\n"
                   "\t-a <ip_address>          set the IP address for emulation; this is the address\n"
                   "\t                         this gateway should listen to for messages from the\n"
                   "\t                         satellite\n"
                   "\t-i <instance>            set the instance id\n"
                   "\t-w <interconnect_addr>   set the interconnect IP address; this is the address\n"
                   "\t                         this gateway should listen to for messages from the\n"
                   "\t                         gw_net_acc part\n"
                   "\t-c <conf_path>           specify the configuration folder path\n"
                   "\t-f <output_folder>       activate and specify the folder for logs and probes\n"
                   "\t                         files\n"
                   "\t-r <remote_address>      activate and specify the address for logs and probes\n"
                   "\t                         socket messages\n"
                   "\t-l <logs_port>           specify the port for logs socket messages\n"
                   "\t-s <stats_port>          specify the port for probes socket messages\n";
      stop = true;
      break;
    }
  }

  if (!output_folder.empty())
  {
    stop = stop || !Output::Get()->configureLocalOutput(output_folder, entity);
  }

  if (!remote_address.empty())
  {
    stop = stop || !Output::Get()->configureRemoteOutput(remote_address, stats_port, logs_port);
  }

  if(stop)
  {
    return false;
  }

  DFLTLOG(LEVEL_NOTICE,
          "starting output\n");

  if(ip_addr.size() == 0)
  {
    DFLTLOG(LEVEL_CRITICAL,
            "missing mandatory IP address option");
    return false;
  }

  if(conf_path.size() == 0)
  {
    DFLTLOG(LEVEL_CRITICAL,
            "missing mandatory configuration path option");
    return false;
  }

  if(interconnect_addr.size() == 0)
  {
    DFLTLOG(LEVEL_CRITICAL,
            "missing mandatory interconnect address option");
    return false;
  }

  return true;
}


int main(int argc, char **argv)
{
  const char *progname = argv[0];
  struct sched_param param;
  bool init_ok;
  std::string ip_addr;
  tal_id_t mac_id = 0;
  struct sc_specific specific;
  std::string interconnect_addr;
  struct ic_specific spec_ic;

  std::string conf_path;
  std::string topology_file;
  std::string global_file;
  std::string default_file;
  std::string plugin_conf_path;

  Block *block_phy_layer;
  Block *block_sat_carrier;
  Block *block_interconnect;

  vector<std::string> conf_files;
  map<std::string, log_level_t> levels;
  map<std::string, log_level_t> spec_level;

  std::shared_ptr<OutputEvent> status;

  int is_failure = 1;

  // retrieve arguments on command line
  init_ok = init_process(argc, argv, ip_addr, mac_id,
                         interconnect_addr, conf_path);

  plugin_conf_path = conf_path + "/" + std::string("plugins/");

  status = Output::Get()->registerEvent("Status");
  if(!init_ok)
  {
    DFLTLOG(LEVEL_CRITICAL,
            "%s: failed to init the process\n", progname);
    goto quit;
  }

  // increase the realtime responsiveness of the process
  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &param);

  topology_file = conf_path + "/" + std::string(CONF_TOPOLOGY);
  global_file = conf_path + "/" + std::string(CONF_GLOBAL_FILE);
  default_file = conf_path + "/" + std::string(CONF_DEFAULT_FILE);

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
  // Output::setLevels(levels, spec_level);

  // load the plugins
  if(!Plugin::loadPlugins(true, plugin_conf_path))
  {
    DFLTLOG(LEVEL_CRITICAL,
            "%s: cannot load the plugins\n", progname);
    goto quit;
  }

  // instantiate all blocs
  spec_ic.interconnect_addr = interconnect_addr;

  block_interconnect = Rt::createBlock<BlockInterconnectUpward,
                                       BlockInterconnectUpward::Upward,
                                       BlockInterconnectUpward::Downward,
                                       struct ic_specific>
                                       ("InterconnectUpward", NULL, spec_ic);
  if(!block_interconnect)
  {
    DFLTLOG(LEVEL_CRITICAL,
            "%s: cannot create the InterconnectUpward block\n", progname);
    goto release_plugins;
  }

  block_phy_layer = Rt::createBlock<BlockPhysicalLayer,
                                    BlockPhysicalLayer::Upward,
                                    BlockPhysicalLayer::Downward,
                                    tal_id_t>("PhysicalLayer", block_interconnect, mac_id);
  if(!block_phy_layer)
  {
    DFLTLOG(LEVEL_CRITICAL,
            "%s: cannot create the PhysicalLayer block\n",
            progname);
    goto release_plugins;
  }
  specific.ip_addr = ip_addr;
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

  Output::Get()->finalizeConfiguration();

  status->sendEvent("Blocks initialized");
  if(!Rt::run())
  {
    DFLTLOG(LEVEL_CRITICAL,
            "%s: cannot run process loop\n",
            progname);
  }

  status->sendEvent("Simulation stopped");

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
