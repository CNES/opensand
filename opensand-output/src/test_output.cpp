/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2019 TAS
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

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#define PUT_IN_PROBE(probe, action, val) do \
{ \
  bool active = val != 0; \
  if (action == 'e') { \
    probe->enable(active); \
  } else if(active) { \
    probe->put(val); \
  } \
} while(0)


int main(int argc, char* argv[])
{
  bool output_enabled = true;
  log_level_t min_level = LEVEL_DEBUG;

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
    }
    if(strcmp(argv[2], "nodebug") == 0)
    {
      min_level = LEVEL_INFO;
    }
  }

  puts("init");
  fflush(stdout);

  auto output = Output::Get();
  if(!output)
  {
    puts("init_error");
    fflush(stdout);
    return 1;
  }

  if (output_enabled)
  {
    output->configureRemoteOutput(argv[1], 58008, 58008);
  }

  std::shared_ptr<Probe<int32_t>> int32_last_probe =
    output->registerProbe<int32_t>("testing.int32_last_probe", "µF", true, SAMPLE_LAST);
  std::shared_ptr<Probe<int32_t>> int32_max_probe =
    output->registerProbe<int32_t>("testing.int32_max_probe", "mm/s", true, SAMPLE_MAX);
  std::shared_ptr<Probe<int32_t>> int32_min_probe =
    output->registerProbe<int32_t>("testing.int32_min_probe", "m²", true, SAMPLE_MIN);
  std::shared_ptr<Probe<int32_t>> int32_avg_probe =
    output->registerProbe<int32_t>("testing.int32_avg_probe", true, SAMPLE_AVG);
  std::shared_ptr<Probe<int32_t>> int32_sum_probe =
    output->registerProbe<int32_t>("testing.int32_sum_probe", true, SAMPLE_SUM);
  std::shared_ptr<Probe<int32_t>> int32_dis_probe =
    output->registerProbe<int32_t>(false, SAMPLE_LAST, "testing.int32_%s_probe", "dis");

  std::shared_ptr<Probe<float>> float_probe =
    output->registerProbe<float>("testing.float_probe", true, SAMPLE_LAST);
  std::shared_ptr<Probe<double>> double_probe =
    output->registerProbe<double>("testing.double_probe", true, SAMPLE_LAST);

  puts("fin_init");
  fflush(stdout);

  output->finalizeConfiguration();

  auto info = output->registerLog(LEVEL_INFO, "info");
  auto debug = output->registerLog(min_level, "debug");


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
        fflush(stdout);
        return 0;
    }

    PUT_IN_PROBE(int32_last_probe, action, values[0]);
    PUT_IN_PROBE(int32_max_probe, action, values[1]);
    PUT_IN_PROBE(int32_min_probe, action, values[2]);
    PUT_IN_PROBE(int32_avg_probe, action, values[3]);
    PUT_IN_PROBE(int32_sum_probe, action, values[4]);
    PUT_IN_PROBE(int32_dis_probe, action, values[5]);

    PUT_IN_PROBE(float_probe, action, float_val);
    PUT_IN_PROBE(double_probe, action, double_val);

    switch (action)
    {
      case 's':
        puts("send");
        fflush(stdout);
        output->sendProbes();
        break;

      case 'd':
        puts("debug");
        fflush(stdout);
        LOG(debug, LEVEL_DEBUG, "This is a debug %s message.", "log");
        break;

      case 'i':
        puts("info");
        fflush(stdout);
        LOG(info, LEVEL_INFO, "This is %s info log message.", "the");
        break;
        
      case 't':
        puts("default log");
        fflush(stdout);
        output->sendLog(LEVEL_ERROR, "This is a default log message%s", ".");
        break;

      case 'e':
        puts("enable/disable probes");
        fflush(stdout);
        output->finalizeConfiguration();
        break;
    }
  }
}
