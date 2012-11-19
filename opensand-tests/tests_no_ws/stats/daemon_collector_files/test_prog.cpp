#include "Output.h"

#include <stdio.h>
#include <unistd.h>

#include <opensand_conf/uti_debug.h>

unsigned char dbgLevel_default = 4;

int main(int argc, char* argv[])
{
	bool output_enabled = true;
	event_level min_level = LEVEL_DEBUG;

	puts("Initializing");

	Output::init(output_enabled, min_level);

	Probe<int32_t> *int32_last_probe =
	    Output::register_probe<int32_t>("int32_last_probe", true, SAMPLE_LAST);
	Output::register_probe<int32_t>("int32_max_probe", true, SAMPLE_MAX);
	Output::register_probe<int32_t>("int32_min_probe", true, SAMPLE_MIN);
	Output::register_probe<int32_t>("int32_avg_probe", true, SAMPLE_AVG);
	Output::register_probe<int32_t>("int32_sum_probe", true, SAMPLE_SUM);
	Output::register_probe<int32_t>("int32_dis_probe", false, SAMPLE_LAST);

	Probe<float> *float_probe =
	    Output::register_probe<float>("float_probe", true, SAMPLE_LAST);
	Output::register_probe<double>("double_probe", true, SAMPLE_LAST);

	Output::register_event("debug_event", LEVEL_DEBUG);
	Event* info_event = Output::register_event("info_event", LEVEL_INFO);

	puts("Finishing init");
	if(!Output::finish_init()) {
		fprintf(stderr, "Init failed (see syslog for details)\n");
		return 1;
	}

	puts("Entering main loop");

	int32_t val = 42;
	float float_val = 10;
	for(int i = 0 ; i < 20 ; i++)
	{
		printf("Putting values %d and %f\n", val, float_val);

		int32_last_probe->put(val);
		float_probe->put(float_val);

		Output::sendProbes();

		if((val % 10) == 0)
		{
			puts("Sending an event");
			Output::sendEvent(info_event, "Hello, %s.", "World");
			sleep(1);
		}

		sleep(1);

		val++;
		float_val = float_val * 5 - 4.2;
	}

	return 0;
}
