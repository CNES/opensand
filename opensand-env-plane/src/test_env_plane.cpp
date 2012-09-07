#include "EnvPlane.h"

#include <stdio.h>
#include <unistd.h>

#include <opensand_conf/uti_debug.h>

#define PUT_IN_PROBE(probe, val) do { if(val != 0) { probe->put(val); } } while(0)

unsigned char dbgLevel_default = 4;

int main(int argc, char* argv[]) {
	bool env_plane_enabled = true;
	event_level min_level = LEVEL_DEBUG;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <socket path> [disable|nodebug]\n", argv[0]);
		exit(1);
	}

	if (argc >= 3) {
		if (strcmp(argv[2], "disable") == 0) {
			env_plane_enabled = false;
		} else if (strcmp(argv[2], "nodebug") == 0) {
			min_level = LEVEL_INFO;
		}
	}

	puts("init"); fflush(stdout);

	EnvPlane::init(env_plane_enabled, min_level, argv[1]);

	Probe<int32_t>* int32_last_probe = EnvPlane::register_probe<int32_t>("int32_last_probe", true, SAMPLE_LAST);
	Probe<int32_t>* int32_max_probe = EnvPlane::register_probe<int32_t>("int32_max_probe", true, SAMPLE_MAX);
	Probe<int32_t>* int32_min_probe = EnvPlane::register_probe<int32_t>("int32_min_probe", true, SAMPLE_MIN);
	Probe<int32_t>* int32_avg_probe = EnvPlane::register_probe<int32_t>("int32_avg_probe", true, SAMPLE_AVG);
	Probe<int32_t>* int32_sum_probe = EnvPlane::register_probe<int32_t>("int32_sum_probe", true, SAMPLE_SUM);
	Probe<int32_t>* int32_dis_probe = EnvPlane::register_probe<int32_t>("int32_dis_probe", false, SAMPLE_LAST);

	Probe<float>* float_probe = EnvPlane::register_probe<float>("float_probe", true, SAMPLE_LAST);
	Probe<double>* double_probe = EnvPlane::register_probe<double>("double_probe", true, SAMPLE_LAST);

	Event* debug_event = EnvPlane::register_event("debug_event", LEVEL_DEBUG);
	Event* info_event = EnvPlane::register_event("info_event", LEVEL_INFO);

	puts("fin_init"); fflush(stdout);
	if (!EnvPlane::finish_init())
		return 1;

	puts("start"); fflush(stdout);

	for (;;) {
		int32_t values[6];
		float float_val;
		double double_val;
		char action;

		if (scanf("%d %d %d %d %d %d %f %lf %c", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5],
			&float_val, &double_val, &action) < 9) {
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

		switch (action) {
			case 's':
				puts("send"); fflush(stdout);
				EnvPlane::send_probes();
			break;

			case 'd':
				puts("debug"); fflush(stdout);
				EnvPlane::send_event(debug_event, "This is the debug %s message.", "event");
			break;

			case 'i':
				puts("info"); fflush(stdout);
				EnvPlane::send_event(info_event, "This is %s info event message.", "the");
			break;
		}
	}
}
