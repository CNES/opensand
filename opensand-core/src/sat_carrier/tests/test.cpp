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
 * @file test.cpp
 * @brief sat carrier test source
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 *
 *
 */


#include "TestSatCarriers.h"
#include "OpenSandModelConf.h"

#include <opensand_rt/Rt.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>



/**
 * Argument treatment
 */
bool init_process(int argc, char **argv, std::string &ip_addr)
{
	int opt;

	/* setting environment agent parameters */
	while((opt = getopt(argc, argv, "-hqda:n:")) != EOF)
	{
		switch(opt)
		{
			case 'a':
				/// get local IP address
				ip_addr = optarg;
				break;
			case 'h':
			case '?':
				fprintf(stderr, "usage: %s [-h] [[-q] [-d] -a ip_address]\n",
					argv[0]);
				fprintf(stderr, "\t-h              print this message\n");
				fprintf(stderr, "\t-a <ip_address> set the IP address\n");

				fprintf(stderr, "usage printed on stderr\n");
				return false;
		}
	}

	if(ip_addr.size() == 0)
	{
		fprintf(stderr, "missing mandatory IP address option");
		return false;
	}

	return true;
}


int main(int argc, char **argv)
{
	const char *progname = argv[0];
	std::string ip_addr;

	std::vector<std::string> conf_files;

	int is_failure = 1;
	struct sc_specific specific;

	auto Conf = OpenSandModelConf::Get();

	// retrieve arguments on command line
	if(!init_process(argc, argv, ip_addr))
	{
		fprintf(stderr, "%s: failed to init the process\n", progname);
		goto quit;
	}

	specific.ip_addr = ip_addr;

	Conf->createModels();
	// Load configuration files content
	if(!Conf->readInfrastructure("test_topology.conf"))
	{
		fprintf(stderr, "%s: cannot load configuration files, quit\n", progname);
		goto quit;
	}

	try
	{
		Rt::Rt::createBlock<TestSatCarriers>("TestSatCarriers", specific);
	}
	catch (const std::bad_alloc&)
	{
		fprintf(stderr, "%s: cannot create the SatCarrier block\n", progname);
		goto quit;
	}

	// make the SAT alive
	if(!Rt::Rt::init())
	{
		goto quit;
	}

	if(!Rt::Rt::run())
	{
		fprintf(stderr, "cannot run process loop\n");
	}


	// everything went fine, so report success
	is_failure = 0;

quit:
	return is_failure;
}
