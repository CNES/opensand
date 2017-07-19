/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2016 TAS
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
 * @file conf.h
 * @brief Configuration options
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * @author Julien Bernard <jbernard@toulouse.viveris.com>
 */

#ifndef CONF__H
#define CONF__H

#include "Configuration.h"


////////////////////
// section global //
////////////////////

#define GLOBAL_SECTION            "global"
#define COMMON_SECTION            "common"

#define LAN_ADAPTATION_SCHEME_LIST     "lan_adaptation_schemes"
#define RETURN_UP_ENCAP_SCHEME_LIST    "return_up_encap_schemes"
#define FORWARD_DOWN_ENCAP_SCHEME_LIST "forward_down_encap_schemes"
#define POSITION                  "pos"
#define PROTO                     "proto"
#define ENCAP_NAME                "encap"

#define SATELLITE_TYPE            "satellite_type"

#define RET_UP_CARRIER_DURATION     "return_up_carrier_duration"
#define FWD_DOWN_CARRIER_DURATION   "forward_down_carrier_duration"

#define STATS_TIMER               "statistics_timer"

#define SYNC_PERIOD               "sync_period"


/////////////////////////
//   section dvb_rcs   //
/////////////////////////

#define DVB_GLOBAL_SECTION        "dvb_rcs"

/////////////////////////
// section dvb_rcs_tal //
/////////////////////////

#define DVB_TAL_SECTION           "dvb_rcs_tal"

#define FIFO_LIST                 "layer2_fifos"
#define FIFO_PRIO                 "priority"

#define FIFO_NAME                 "name"
#define FIFO_SIZE                 "size_max"
#define FIFO_ACCESS_TYPE          "access_type"
#define DVB_TYPE                  "dvb_type"
#define CRA                       "constant_rate_allocation"
#define DAMA_ALGO                 "dama_agent_algorithm"


///////////////////////////
// section sat switching //
///////////////////////////

#define TAL_ID                    "tal_id"
#define ID                        "id"
#define GW                        "gw"
#define SPOT                      "spot"

#define SPOT_TABLE_SECTION        "spot_table"
#define TERMINAL_LIST			  "terminals"
#define DEFAULT_SPOT              "default_spot"

#define GW_TABLE_SECTION          "gw_table"
#define DEFAULT_GW                "default_gw"

//////////////////////
// bloc_dvb_rcs_ncc //
//////////////////////

#define DVB_NCC_SECTION         "dvb_rcs_ncc"
#define DVB_NCC_DAMA_ALGO       "dama_algorithm"
#define DVB_EVENT_FILE          "event_file"
#define DVB_SIMU_FILE           "simu_file"
#define DVB_SIMU_MODE           "simulation"
#define DVB_SIMU_RANDOM         "simu_random"


//////////////////////////
//  Band configuration  //
//////////////////////////

#define RETURN_UP_BAND        "return_up_band"
#define FORWARD_DOWN_BAND     "forward_down_band"

#define BANDWIDTH             "bandwidth"
#define ROLL_OFF              "roll_off"

#define CARRIERS_DISTRI_LIST  "carriers_distribution"
#define CATEGORY              "category"
#define RATIO                 "ratio"
#define SYMBOL_RATE           "symbol_rate"
#define FMT_GROUP             "fmt_group"
#define ACCESS_TYPE           "access_type"

#define TAL_AFF_LIST          "tal_affectations"
#define DEFAULT_AFF           "tal_default_affectation"

#define FMT_GROUP_LIST        "fmt_groups"
#define GROUP_ID              "id"
#define FMT_ID                "fmt_id"


//////////////////////
// QoS PEP section  //
//////////////////////

#define NCC_SECTION_PEP      "qos_pep"
#define DVB_NCC_ALLOC_DELAY  "pep_alloc_delay"
#define PEP_DAMA_PORT        "pep_to_dama_port"


////////////////////
// SVNO section  //
////////////////////

#define NCC_SECTION_SVNO     "svno_interface"
#define SVNO_NCC_PORT        "svno_to_ncc_port"

//////////////////
// DAMA CONTROL //
//////////////////

#define DC_SECTION_NCC              "dvb_rcs_ncc"
#define DC_FREE_CAP                 "fca"

//////////
// DAMA //
//////////

#define DA_TAL_SECTION          "dvb_rcs_tal"
#define DA_MAX_RBDC_DATA        "max_rbdc"
#define DA_MAX_VBDC_DATA        "max_vbdc"
#define DA_MSL_DURATION         "msl_duration"

/////////////////////////
//    slotted aloha    //
/////////////////////////

#define SALOHA_SECTION                 "slotted_aloha"
#define SALOHA_FPSAF                   "superframes_per_slotted_aloha_frame"
#define SALOHA_NB_MAX_PACKETS          "nb_max_packets"
#define SALOHA_TIMEOUT                 "timeout"
#define SALOHA_NB_MAX_RETRANSMISSIONS  "nb_max_retransmissions"
#define SALOHA_BACKOFF_ALGORITHM       "backoff_algorithm"
#define SALOHA_CW_MAX                  "cw_max"
#define SALOHA_BACKOFF_MULTIPLE        "backoff_multiple"
#define SALOHA_NB_REPLICAS             "nb_replicas"
#define SALOHA_ALGO                    "algorithm"
#define SALOHA_SIMU_LIST               "simulation_traffic"
#define SALOHA_RATIO                   "ratio"
#define SALOHA_MAXDELAY                "max_satdelay"


/////////////////
/// dama SCPC ///
/////////////////

#define SCPC_SECTION                    "scpc"
#define IS_SCPC							"is_scpc"
#define SCPC_C_DURATION                 "scpc_carrier_duration"

///////////////////
/// access type ///
///////////////////

#define ACCESS_DAMA                  "DAMA"
#define ACCESS_VCM                  "VCM"
#define ACCESS_ACM                  "ACM"
#define ACCESS_SCPC                 "SCPC"
#define ACCESS_ALOHA                "ALOHA"

//////////
// QOS //
//////////

#define SECTION_QOS_AGENT    "qos_agent"
#define ST_NAME              "st_name"
#define QOS_SERVER_HOST      "st_address"
#define QOS_SERVER_PORT      "qos_server_port"


//////////////////
//     SARP     //
//////////////////

#define SARP_SECTION      "sarp"
#define IPV4_LIST         "ipv4"
#define IPV6_LIST         "ipv6"
#define ETH_LIST          "ethernet"
#define TERMINAL_ADDR     "addr"
#define TERMINAL_IP_MASK  "mask"
#define MAC_ADDR          "mac"
#define DEFAULT           "default"

////////////////////
// Physical Layer //
////////////////////

#define LINK                      "link"

#define UPLINK_PHYSICAL_LAYER_SECTION    "uplink_physical_layer"
#define DOWNLINK_PHYSICAL_LAYER_SECTION  "downlink_physical_layer"
#define PHYSICAL_LAYER_SECTION           "physical_layer"
#define SAT_PHYSICAL_LAYER_SECTION       "sat_physical_layer"
#define ENABLE                    "enable"
#define MODEL_LIST                "models"
#define ATTENUATION_MODEL_TYPE    "attenuation_model_type"
#define MINIMAL_CONDITION_TYPE    "minimal_condition_type"
#define ERROR_INSERTION_TYPE      "error_insertion_type"
#define CLEAR_SKY_CONDITION       "clear_sky_condition"
#define MODCOD_DEF_S2                     "modcod_def_s2"
#define FORWARD_DOWN_MODCOD_TIME_SERIES   "forward_down_modcod_time_series"
#define MODCOD_DEF_RCS                    "modcod_def_rcs"
#define RETURN_UP_MODCOD_TIME_SERIES      "return_up_modcod_time_series"
#define LOOP_ON_FILE                      "loop_on_file"
#define RETURN_UP_ACM_LOOP_MARGIN         "return_up_acm_loop_margin"
#define FORWARD_DOWN_ACM_LOOP_MARGIN      "forward_down_acm_loop_margin"

#define GENERATE_TIME_SERIES_PATH         "generate_time_series_path"

#define ACM_PERIOD_REFRESH        "acm_period_refresh"


/////////////////
// SAT Carrier //
/////////////////

#define SATCAR_SECTION      "sat_carrier"
#define CARRIER_LIST        "carriers"
#define CARRIER_ID          "id"
#define CARRIER_IP          "ip_address"
#define CARRIER_TYPE        "type"
#define CARRIER_PORT        "port"
#define CARRIER_MULTICAST   "ip_multicast"

#define DVB_CTRL_CAR        "carrier_id_dvb_ctrl"
#define DVB_SOF_CAR         "carrier_id_sof"
#define DVB_DATA_CAR        "carrier_id_data"
#define DVB_IN_DATA_CAR     "carrier_id_in_data"

#define DVB_CAR_ID_CTRL     "carrier_id_dvb_ctrl"
#define DVB_CAR_ID_LOGON    "carrier_id_logon"
#define DVB_CAR_ID_DATA     "carrier_id_data"

#define LOGON_IN            "logon_in"
#define LOGON_OUT           "logon_out"
#define CTRL_IN             "ctrl_in"
#define CTRL_OUT            "ctrl_out"
#define DATA_IN_ST          "data_in_st"
#define DATA_OUT_ST         "data_out_st"
#define DATA_IN_GW          "data_in_gw"
#define DATA_OUT_GW         "data_out_gw"
#define DISABLED_GW         "gw"
#define DISABLED_ST         "st"
#define DISABLED_NONE       "none"

#define SPOT_LIST           "spot"
#define NO_GW               10
#define GW_LIST             "gw"
#define CTRL_ID             "ctrl_id"
#define DATA_IN_ID          "data_in_id"
#define DATA_OUT_GW_ID      "data_out_gw_id"
#define DATA_OUT_ST_ID      "data_out_st_id"
#define LOG_ID              "log_id"

//////////////////////////
//      sat_delays      //
//////////////////////////
#define SAT_DELAY_SECTION          "delay"
#define GLOBAL_SAT_DELAY_SECTION   "global_delay"
#define GLOBAL_CONSTANT_DELAY      "global_constant_delay"
#define DELAY                      "delay"
#define SAT_DELAY_CONF             "delay_conf"
#define REFRESH_PERIOD_MS          "refresh_period"
#define DELAY_TYPE                 "delay_type"
#define CONSTANT_DELAY             "ConstantDelay"

//////////////////////////
//       advanced       //
//////////////////////////
#define ADV_SECTION      "advanced"
#define DELAY_BUFFER     "delay_buffer"
#define DELAY_TIMER      "delay_timer"
#define UDP_RMEM         "udp_rmem"
#define UDP_WMEM         "udp_wmem"
#define UDP_STACK        "udp_stack"

/////////////////
//    Debug    //
/////////////////
#define SECTION_DEBUG  "debug"
#define LEVEL_LIST     "levels"
#define LOG_NAME       "name"
#define LOG_LEVEL      "level"
/*#define INIT           "init"
#define LAN_ADAPTATION "lan_adaptation"
#define ENCAP          "encap"
#define DVB            "dvb"
#define SAT_CARRIER    "sat_carrier"
#define PHYS           "physical_layer"*/

/////////////
// OTHERS  //
///////////// 
#define ENCODE_CNI_EXT  "encodeCniExt"
#endif
