#include <opensand_output/Output.h>

#include <stdio.h>
#include <unistd.h>


int main(int argc, char* argv[])
{
	bool output_enabled = true;
	log_level_t min_level = LEVEL_DEBUG;

	puts("Initializing");

	Output::init(output_enabled);

	Probe<int32_t> *int32_last_probe =
	    Output::registerProbe<int32_t>("int32_last_probe", true, SAMPLE_LAST);
	Output::registerProbe<int32_t>("int32_max_probe", true, SAMPLE_MAX);
	Output::registerProbe<int32_t>("int32_min_probe", true, SAMPLE_MIN);
	Output::registerProbe<int32_t>("int32_avg_probe", true, SAMPLE_AVG);
	Output::registerProbe<int32_t>("int32_sum_probe", true, SAMPLE_SUM);
	Output::registerProbe<int32_t>("int32_dis_probe", false, SAMPLE_LAST);

	Probe<float> *float_probe =
	    Output::registerProbe<float>("float_probe", true, SAMPLE_LAST);
	Output::registerProbe<double>("double_probe", true, SAMPLE_LAST);

	OutputLog* log = Output::registerLog(LEVEL_INFO, "log");

	puts("Finishing init");
	if(!Output::finishInit()) {
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
			puts("Sending a log");
			Output::sendLog(log, LEVEL_INFO, "Hello, %s.", "World");
			sleep(1);
		}

		sleep(1);

		val++;
		float_val = float_val * 5 - 4.2;
	}

	return 0;
}
