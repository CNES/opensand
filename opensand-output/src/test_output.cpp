/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
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
 * @file test_output.cpp
 * @brief Part of the output test framework.
 * @author Vincent Duvert <vduvert@toulouse.viveris.com>
 */


#include "Output.h"

#include <stdio.h>
#include <unistd.h>

#include <opensand_conf/uti_debug.h>

#define PUT_IN_PROBE(probe, val) do \
{ if(val != 0) { probe->put(val); } } while(0)

unsigned char dbgLevel_default = 4;

int main(int argc, char* argv[])
{
	bool output_enabled = true;
	event_level_t min_level = LEVEL_DEBUG;

	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s <socket path> [disable|nodebug]\n", argv[0]);
		exit(1);
	}

	if(argc >= 3)
	{
		if(strcmp(argv[2], "disable") == 0)
		{
			output_enabled = false;
		} else if(strcmp(argv[2], "nodebug") == 0)
		{
			min_level = LEVEL_INFO;
		}
	}

	puts("init");
	fflush(stdout);

	Output::init(output_enabled, min_level, argv[1]);

	Probe<int32_t> *int32_last_probe =
		Output::registerProbe<int32_t>("int32_last_probe", "µF", true, SAMPLE_LAST);
	Probe<int32_t> *int32_max_probe =
		Output::registerProbe<int32_t>("int32_max_probe", "mm/s", true, SAMPLE_MAX);
	Probe<int32_t> *int32_min_probe =
		Output::registerProbe<int32_t>("int32_min_probe", "m²", true, SAMPLE_MIN);
	Probe<int32_t> *int32_avg_probe =
		Output::registerProbe<int32_t>("int32_avg_probe", true, SAMPLE_AVG);
	Probe<int32_t> *int32_sum_probe =
		Output::registerProbe<int32_t>("int32_sum_probe", true, SAMPLE_SUM);
	Probe<int32_t> *int32_dis_probe =
		Output::registerProbe<int32_t>("int32_dis_probe", false, SAMPLE_LAST);

	Probe<float> *float_probe =
		Output::registerProbe<float>("float_probe", true, SAMPLE_LAST);
	Probe<double> *double_probe =
		Output::registerProbe<double>("double_probe", true, SAMPLE_LAST);

	Event* debug_event = Output::registerEvent("debug_event", LEVEL_DEBUG);
	Event* info_event = Output::registerEvent("info_event", LEVEL_INFO);

	puts("fin_init");
	fflush(stdout);
	if(!Output::finishInit())
		return 1;

	puts("start");
	fflush(stdout);

	for(;;)
	{
		int32_t values[6];
		float float_val;
		double double_val;
		char action;

		if(scanf("%d %d %d %d %d %d %f %lf %c", &values[0], &values[1],
		   &values[2], &values[3], &values[4], &values[5],
		   &float_val, &double_val, &action) < 9)
		{
				puts("quit");
				return 0;
		}

		PUT_IN_PROBE(int32_last_probe, values[0]);
		PUT_IN_PROBE(int32_max_probe, values[1]);
		PUT_IN_PROBE(int32_min_probe, values[2]);
		PUT_IN_PROBE(int32_avg_probe, values[3]);
		PUT_IN_PROBE(int32_sum_probe, values[4]);
		PUT_IN_PROBE(int32_dis_probe, values[5]);

		PUT_IN_PROBE(float_probe, float_val);
		PUT_IN_PROBE(double_probe, double_val);

		switch (action)
		{
			case 's':
				puts("send");
				fflush(stdout);
				Output::sendProbes();
			break;

			case 'd':
				puts("debug");
				fflush(stdout);
				Output::sendEvent(debug_event, "This is the debug %s message.",
				                  "event");
			break;

			case 'i':
				puts("info");
				fflush(stdout);
				Output::sendEvent(info_event, "This is %s info event message.",
				                  "the");
			break;
		}
	}
}
