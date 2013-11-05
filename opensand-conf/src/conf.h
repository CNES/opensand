/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
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


// configuration file
#include "ConfigurationFile.h"

// configure defines
// TODO remove if not necessary
//#include "config.h"

////////////////////
// section global //
////////////////////

#define GLOBAL_SECTION            "global"

#define LAN_ADAPTATION_SCHEME_LIST "lan_adaptation_schemes"
#define UP_RETURN_ENCAP_SCHEME_LIST "up_return_encap_schemes"
#define DOWN_FORWARD_ENCAP_SCHEME_LIST "down_forward_encap_schemes"
#define POSITION                  "pos"
#define PROTO                     "proto"
#define ENCAP_NAME                "encap"

#define SATELLITE_TYPE            "satellite_type"

#define SAT_DELAY                 "delay"

#define DVB_F_DURATION            "frame_duration"


#define DOWN_FORWARD_MODCOD_DEF   "down_forward_modcod_def"
#define DOWN_FORWARD_MODCOD_SIMU  "down_forward_modcod_simu"
#define UP_RETURN_MODCOD_DEF      "up_return_modcod_def"
#define UP_RETURN_MODCOD_SIMU     "up_return_modcod_simu"

#define DVB_SCENARIO_REFRESH      "dvb_scenario_refresh"

/////////////////////////
//   section dvb_rcs   //
/////////////////////////

#define DVB_GLOBAL_SECTION        "dvb_rcs"

#define DVB_SIZE_FIFO             "max_fifo"

/////////////////////////
// section dvb_rcs_tal //
/////////////////////////

#define DVB_TAL_SECTION           "dvb_rcs_tal"

#define FIFO_LIST                 "fifos"
#define FIFO_PRIO                 "priority"

#define FIFO_TYPE                 "type"
#define FIFO_SIZE                 "size_max"
#define FIFO_PVC                  "pvc"
#define FIFO_CR_TYPE              "cr_type"
#define DVB_TYPE                  "dvb_type"
#define DVB_RT_BANDWIDTH          "rt_fixed_bandwidth"
#define DVB_SIMU_COL              "simulation_column"
#define COLUMN_LIST               "columns"
#define COLUMN_NBR                "column_nbr"
#define DVB_CAR_ID_CTRL           "carrier_id_dvb_ctrl"
#define DVB_CAR_ID_LOGON          "carrier_id_logon"
#define DVB_CAR_ID_DATA           "carrier_id_data"
#define DVB_OBR_PERIOD_DATA       "obr_period"
#define DAMA_ALGO                 "dama_algorithm"


/////////////////////////
// section dvb_rcs_sat //
/////////////////////////

#define SPOT_ID                   "spot_id"
#define TAL_ID                    "tal_id"

#define SAT_DVB_SECTION           "dvb_rcs_sat"
#define SPOT_LIST                 "spots"
#define CTRL_ID                   "ctrl_id"
#define DATA_IN_ID                "data_in_id"
#define DATA_OUT_GW_ID            "data_out_gw_id"
#define DATA_OUT_ST_ID            "data_out_st_id"
#define LOG_ID                    "log_id"

#define SAT_RAND_SEED             "seed"

#define SAT_SIMU_COL_SECTION      "simulation_column"
#define COLUMN_LIST               "columns"
#define COLUMN_NBR                "column_nbr"
#define SAT_SWITCH_SECTION        "sat_switching_table"
#define SWITCH_LIST               "switchs"
#define DEFAULT_SPOT             "default_spot"

#define SAT_ERR_GENERATOR         "error_generator"
#define SAT_ERR_GENERATOR_NONE    "none"
#define SAT_ERR_GENERATOR_DEFAULT "default"
#define SAT_ERR_BER               "error_generator_ber"
#define SAT_ERR_MEAN              "error_generator_mean"
#define SAT_ERR_DELTA             "error_generator_delta"


//////////////////////
// bloc_dvb_rcs_ncc //
//////////////////////

#define DVB_MAC_SECTION         "mac_layer"
#define DVB_FPF                 "frames_per_superframe"

#define DVB_NCC_SECTION         "dvb_rcs_ncc"
#define DVB_NCC_DAMA_ALGO       "dama_algorithm"
#define DVB_CTRL_CAR            "carrier_id_dvb_ctrl"
#define DVB_SOF_CAR             "carrier_id_sof"
#define DVB_DATA_CAR            "carrier_id_data"
#define DVB_EVENT_FILE          "event_file"
#define DVB_STAT_FILE           "stat_file"
#define DVB_SIMU_FILE           "simu_file"
#define DVB_SIMU_MODE           "simulation"
#define DVB_SIMU_RANDOM         "simu_random"


//////////////////////////
//  Band configuration  //
//////////////////////////

#define UP_RETURN_BAND        "up_return_band"
#define DOWN_FORWARD_BAND     "down_forward_band"

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
//    PEP section   //
//////////////////////

#define NCC_SECTION_PEP      "pep"
#define DVB_NCC_ALLOC_DELAY  "pep_alloc_delay"
#define PEP_DAMA_PORT        "pep_to_dama_port"

//////////////////
// DAMA CONTROL //
//////////////////

#define DC_SECTION_NCC              "dvb_rcs_ncc"
#define DC_CRA_DECREASE             "cra_decrease"
#define DC_FREE_CAP                 "fca"
#define DC_RBDC_TIMEOUT             "rbdc_timeout"
#define DC_SECTION_MAC_LAYER        "mac_layer"

//////////
// DAMA //
//////////

#define DA_MAC_LAYER_SECTION    "mac_layer"

#define DA_TAL_SECTION          "dvb_rcs_tal"
#define DA_MAX_RBDC_DATA        "max_rbdc"
#define DA_RBDC_TIMEOUT_DATA    "rbdc_timeout"
#define DA_MAX_VBDC_DATA        "max_vbdc"
#define DA_MSL_DURATION         "msl_duration"
#define DA_CR_RULE              "cr_rule_output_fifo_only"

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
#define PHYSICAL_LAYER_SECTION    "physical_layer"
#define ENABLE                    "enable"
#define MODEL_LIST                "models"
#define ATTENUATION_MODEL_TYPE    "attenuation_model_type"
#define MINIMAL_CONDITION_TYPE    "minimal_condition_type"
#define ERROR_INSERTION_TYPE      "error_insertion_type"
#define NOMINAL_CONDITION         "nominal_condition"
#define GRANULARITY               "granularity"

/////////////////
// SAT Carrier //
/////////////////

#define SATCAR_SECTION      "sat_carrier"
#define CARRIER_LIST        "carriers"
#define CARRIER_ID          "id"
#define CARRIER_IP          "ip_address"
#define CARRIER_PORT        "port"
#define CARRIER_UP          "up"
#define CARRIER_DOWN        "down"
#define CARRIER_MULTICAST   "ip_multicast"
#define CARRIER_DISABLED    "disabled_on"
#define SOCKET_TYPE         "socket_type"
#define UDP                 "SOCK_DGRAM"

/////////////////
//    Debug    //
/////////////////
#define SECTION_DEBUG "debug"

#endif
