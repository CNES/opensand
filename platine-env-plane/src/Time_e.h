#ifndef Time_e
#   define Time_e

#   include <sys/types.h>
#   include <sys/times.h>
#   include <limits.h>

#   include "Types_e.h"
#   include "Error_e.h"

typedef T_DOUBLE T_TIME;

/* Init internal time reference */
T_ERROR TIME_Init(void);

/* Get current time in seconds */
T_TIME TIME_GetTime(void);

/* Get current time in tick (struct tms *ptr_buffer) */
#      define TIME_GetTimeTick(ptr_buffer) times(ptr_buffer)

/* Get a time difference in second (struct tms *ptr_bufferEnd,struct tms *ptr_bufferBegin) */
#      define TIME_GetTimeDiff(ptr_bufferEnd,ptr_bufferBegin) \
((T_TIME)((((T_DOUBLE)(ptr_bufferEnd)->tms_utime - (T_DOUBLE)(ptr_bufferBegin)->tms_utime) \
           + ((T_DOUBLE)(ptr_bufferEnd)->tms_stime - (T_DOUBLE)(ptr_bufferBegin)->tms_stime)) \
          / (T_DOUBLE)CLK_TCK))

#endif /* Time_e */
