/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/**
 * @file TestInterconnectBlock.h
 * @author Cyrille Gaillardet <cgaillardet@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 * @author Joaquin Muguerza <jmuguerza@toulouse.viveris.fr>
 * @brief This test check that we can read a file on a channel then
 *        transmit content to lower block, the bottom block transmit it
 *        to the following channel that will forward it to the top and
 *        compare output
 *
 *
 *        file
 *          |
 *  +-------+-----------------------+
 *  | +-----+-----+   +-----------+ |
 *  | |     |     |   |  compare  | |
 *  | |     |     |Top|     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----|-----+   +-----+-----+ |
 *  +-------|---------------+-------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----+-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     | Interconnect  |     | |
 *  | |     |   Downward    |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  +-------|-----------------------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----+-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     | Interconnect  |     | |
 *  | |     |    Upward     |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  +-------|-----------------------+
 *          |               |
 *  +-------+---------------+-------+
 *  | +-----|-----+   +-----+-----+ |
 *  | |     |     |   |     |     | |
 *  | |     |    Bottom     |     | |
 *  | |     |     |   |     |     | |
 *  | +-----+-----+   +-----+-----+ |
 *  |       +---------------+       |
 *  +-------------------------------+
 */

#include "TestInterconnectBlock.h"

#include "opensand_rt/Rt.h"

#include <TestBlockInterconnectUpward.h>
#include <TestBlockInterconnectDownward.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <Config.h>

#if ENABLE_TCMALLOC
#include <heap-checker.h>
#endif

using std::ostringstream;

/**
 * @brief Print usage of the test application
 */
static void usage(void)
{
	std::cerr << "Test interconnect block" << std::endl
	          << "usage: test_interconnect -i input_file -o output_file"
              << " -u upward_port -d downward_port"
              << std::endl;
}


int main(int argc, char **argv)
{
	int ret = 0;
	Output::init(false);
	Output::enableStdlog();
	Block *top;
	string error;
    struct top_specific spec_top;
    struct icd_specific spec_icd;
	int args_used;

	/* parse program arguments, print the help message in case of failure */
	if(argc <= 1 || argc > 9)
	{
		usage();
		return 1;
	}

	for(argc--, argv++; argc > 0; argc -= args_used, argv += args_used)
	{
		args_used = 1;

		if(!strcmp(*argv, "-h") || !strcmp(*argv, "--help"))
		{
			/* print help */
			usage();
			return 1;
		}
		else if(!strcmp(*argv, "-i"))
		{
			/* get the name of the input file */
			spec_top.input_file = argv[1];
			args_used++;
		}
		else if(!strcmp(*argv, "-o"))
		{
			/* get the name of the output file */
			spec_top.output_file = argv[1];
			args_used++;
		}
		else if(!strcmp(*argv, "-u"))
		{
			/* get the upward port */
			spec_icd.port_upward = (uint16_t) atoi(argv[1]);
			args_used++;
		}
		else if(!strcmp(*argv, "-d"))
		{
			/* get the downward port */
			spec_icd.port_downward = (uint16_t) atoi(argv[1]);
			args_used++;
		}
		else
		{
			usage();
			return 1;
		}
	}
	std::cout << "Launch test" << std::endl;

    spec_top.must_wait = true;

    top = Rt::createBlock<TopBlock, TopBlock::Upward,
	                      TopBlock::Downward, struct top_specific>
                          ("top", NULL, spec_top);

    spec_icd.top_block = (TopBlock *) top;
   
    Rt::createBlock<TestBlockInterconnectDownward,
                    TestBlockInterconnectDownward::Upward,
                    TestBlockInterconnectDownward::Downward,
                    struct icd_specific>
                    ("interconnect_downward", top, spec_icd);
	
    std::cout << "Start loop, please wait..." << std::endl;
	Output::finishInit();
	if(!Rt::run(true))
	{
		ret = 1;
		std::cerr << "Unable to run" << std::endl;
	}
	else
	{
		std::cout << "Successfull" << std::endl;
	}

	return ret;
}
