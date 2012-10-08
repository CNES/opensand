#include "EnvPlane.h"

#include <stdio.h>
#include <unistd.h>

#include <opensand_conf/uti_debug.h>

unsigned char dbgLevel_default = 4;

int main(int argc, char* argv[]) {
	bool env_plane_enabled = true;
	event_level min_level = LEVEL_DEBUG;

	puts("Initializing");

	EnvPlane::init(env_plane_enabled, min_level);

	Probe<int32_t>* int32_last_probe = EnvPlane::register_probe<int32_t>("int32_last_probe", true, SAMPLE_LAST);
	EnvPlane::register_probe<int32_t>("int32_max_probe", true, SAMPLE_MAX);
	EnvPlane::register_probe<int32_t>("int32_min_probe", true, SAMPLE_MIN);
	EnvPlane::register_probe<int32_t>("int32_avg_probe", true, SAMPLE_AVG);
	EnvPlane::register_probe<int32_t>("int32_sum_probe", true, SAMPLE_SUM);
	EnvPlane::register_probe<int32_t>("int32_dis_probe", false, SAMPLE_LAST);

	Probe<float>* float_probe = EnvPlane::register_probe<float>("float_probe", true, SAMPLE_LAST);
	EnvPlane::register_probe<double>("double_probe", true, SAMPLE_LAST);

	EnvPlane::register_event("debug_event", LEVEL_DEBUG);
	Event* info_event = EnvPlane::register_event("info_event", LEVEL_INFO);

	puts("Finishing init");
	if (!EnvPlane::finish_init()) {
		fprintf(stderr, "Init failed (see syslog for details)\n");
		return 1;
	}

	puts("Entering main loop");

	int32_t val = 42;
	float float_val = 10;
	for (int i = 0 ; i < 20 ; i++) {
		printf("Putting values %d and %f\n", val, float_val);

		int32_last_probe->put(val);
		float_probe->put(float_val);

		EnvPlane::send_probes();

		if ((val % 10) == 0) {
			puts("Sending an event");
			EnvPlane::send_event(info_event, "Hello, %s.", "World");
			sleep(1);
		}

		sleep(1);

		val++;
		float_val = float_val * 5 - 4.2;
	}

	return 0;
}
