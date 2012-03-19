/**
 * @file msg_dvb_rcs.cpp
 * @brief This files defines messages type numbers for DVB-S/RCS related packets
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#include "msg_dvb_rcs.h"

mgl_memory_pool g_memory_pool_dvb_rcs(MSG_BBFRAME_SIZE_MAX, 10000, (const char *)"gmp_dvb_rcs");

