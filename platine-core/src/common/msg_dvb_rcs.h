/**
 * @file msg_dvb_rcs.h
 * @brief This file defines message type for DVB-S/RCS related packets
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
*/

#ifndef MSG_DVB_RCS_H
#define MSG_DVB_RCS_H

#include "platine_margouilla/mgl_memorypool.h"


const long msg_dvb = 300;
const long msg_link_up = 302;
const long msg_dvb_up = 303;
const long msg_encap_burst = 304;

const unsigned long MSG_DVB_RCS_SIZE_MAX = 1200;
const unsigned long MSG_BBFRAME_SIZE_MAX = 8100; //7154;


/// This message is used by dvb rcs layer to advertise the upper layer
/// that the link is up
typedef struct
{
	long group_id;  /// The id of the station
	long tal_id;
} T_LINK_UP;


/// DVB RCS messages from DVB to lower
extern mgl_memory_pool g_memory_pool_dvb_rcs;

#endif
