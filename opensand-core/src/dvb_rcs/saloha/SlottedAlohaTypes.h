/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
 * @file SlottedAlohaTypes.h
 * @brief Types for Slotted Aloha
 * @author Vincent WINKEL <vincent.winkel@thalesaleniaspace.com> <winkel@live.fr>
*/

#ifndef SALOHA_TYPES_H
#define SALOHA_TYPES_H

//Control signal types
#define SALOHA_CTRL_ERR 0
#define SALOHA_CTRL_ACK 1

//<ID,Seq,PDU_nb,QoS> identifiants constants
#define SALOHA_ID_ID 0
#define SALOHA_ID_SEQ 1
#define SALOHA_ID_PDU_NB 2
#define SALOHA_ID_QOS 3

//Set to true if debug logs, false otherwise
//// TODO REMOVE
#define SALOHA_DEBUG true

#include "SlottedAlohaPacketData.h"
#include "SlottedAlohaPacketCtrl.h"

#include <set>
#include <vector>
#include <map>
#include <sstream>

using std::string;
using std::vector;
using std::ostringstream;
using std::istringstream;
using std::map;
using std::vector;
using std::set;


// Types definitions used for Slotted Aloha processing
// TODO need everything ???
// TODO rename !

typedef Data                                 sa_id_t;
typedef vector<uint16_t>                     sa_vector_id_t;

typedef set<uint8_t>                         sa_set_int_t;
// TODO DvbFifo !!!
typedef vector<SlottedAlohaPacketData *>               sa_vector_data_t;
typedef map<uint8_t, sa_vector_data_t>        sa_map_vector_data_t;

typedef set<sa_id_t>                         sa_set_id_t;
typedef vector<sa_vector_data_t>             sa_vector_vector_data_t;
typedef map<sa_id_t, SlottedAlohaPacketData *>         sa_map_data_t;
typedef map<uint16_t, sa_id_t>               sa_map_id_t;
typedef map<uint16_t, sa_map_id_t>           sa_map_map_id_t;
typedef map<uint16_t, sa_map_vector_data_t>  sa_map_map_vector_data_t;
// TODO use DvbFifo instead of vectors for SA FIFOs

#endif

