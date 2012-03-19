/*  Cartouche AD  */

/* SYSTEM RESOURCES */
#include <time.h>
#include <sys/time.h>

/* PROJECT RESOURCES */
#include "Time_e.h"


/* Internal time reference (in seconds) */
static T_UINT32 _zero;

/* Init internal time reference */
T_ERROR TIME_Init()
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	_zero = ts.tv_sec;

	return C_ERROR_OK;
}


/* Get current time in seconds */
T_TIME TIME_GetTime()
{
	struct timeval ts;
	gettimeofday(&ts, NULL);
	return (double) (ts.tv_sec - _zero) + ((double) ts.tv_usec / 1000000.);

	/* !CB higher precision
	   struct timespec ts;

	   clock_gettime(CLOCK_REALTIME, &ts);
	   return (double)(ts.tv_sec - _zero) + ((double)ts.tv_nsec / 1000000000.);
	 */
}
