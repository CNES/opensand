/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2012 TAS
 * Copyright © 2012 CNES
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
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
#include "config.h"

////////////////////
// section Global //
////////////////////

#define GLOBAL_SECTION            "global"

#define SAT_ETH_IFACE             "satellite_eth_interface"
#define IP_OPTION_LIST            "ip_options"
#define OPTION_NAME               "name"
#define UP_RETURN_ENCAP_SCHEME_LIST "up_return_encap_schemes"
#define DOWN_FORWARD_ENCAP_SCHEME_LIST "down_forward_encap_schemes"
#define POSITION                  "pos"
#define ENCAP_NAME                "encap"

#define SATELLITE_TYPE            "satellite_type"
#define TRANSPARENT_SATELLITE     "transparent"
#define REGENERATIVE_SATELLITE    "regenerative"

#define SAT_DELAY                 "delay"

#define DVB_F_DURATION            "frame_duration"

#define DVB_SCENARIO              "dvb_scenario"
#define DVB_SCENARIO_REFRESH      "dvb_scenario_refresh"

#define BANDWIDTH                 "bandwidth"

//////////////////////
// section macLayer //
//////////////////////

#define DVB_MAC_LAYER_SECTION     "mac_layer"

#define DVB_FRMS_PER_SUPER        "frames_per_superframe"
#define DVB_FRM_DURATION          "frame_duration"

/////////////////////////
//   section Dvb_rcs   //
/////////////////////////

#define DVB_GLOBAL_SECTION        "dvb_rcs"

/////////////////////////
// section Dvb_rcs_tal //
/////////////////////////

#define DVB_TAL_SECTION           "dvb_rcs_tal"

#define FIFO_LIST                 "fifos"
#define FIFO_ID                   "id"
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
#define DVB_DAMA_ALGO             "dama_algorithm"


/////////////////////////
// section Dvb_rcs_sat //
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

#define SAT_ERR_GENERATOR         "error_generator"
#define SAT_ERR_GENERATOR_NONE    "none"
#define SAT_ERR_GENERATOR_DEFAULT "default"
#define SAT_ERR_BER               "error_generator_ber"
#define SAT_ERR_MEAN              "error_generator_mean"
#define SAT_ERR_DELTA             "error_generator_delta"


//////////////////////
// bloc_dvb_rcs_ncc //
//////////////////////

#define DVB_GW_MAC_ID           (0L)
#define DVB_MAC_SECTION         "mac_layer"
#define DVB_MEDIUM_RATE         "transmission_rate"
#define DVB_FPF                 "frames_per_superframe"

#define DVB_NCC_SECTION         "dvb_rcs_ncc"
#define DVB_NCC_DAMA_ALGO       "dama_algorithm"
#define DVB_CTRL_CAR            "carrier_id_dvb_ctrl"
#define DVB_SOF_CAR             "carrier_id_sof"
#define DVB_DATA_CAR            "carrier_id_data"
#define DVB_SIZE_FIFO           "max_fifo"
#define DVB_EVENT_FILE          "event_file"
#define DVB_STAT_FILE           "stat_file"
#define DVB_SIMU_FILE           "simu_file"
#define DVB_SIMU_MODE           "simulation"
#define DVB_SIMU_RANDOM         "simu_random"

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
#define DC_MAX_RBDC                 "max_rbdc"
#define DC_MIN_VBDC                 "min_vbdc"
#define DC_SECTION_MAC_LAYER        "mac_layer"
#define DC_CARRIER_TRANS_RATE       "carrier_transmission_rate"
#define DC_CARRIER_NUMBER           "carrier_number"

//////////
// DAMA //
//////////

#define DA_MAC_LAYER_SECTION    "mac_layer"
#define DA_CARRIER_TRANS_RATE   "carrier_transmission_rate"

#define DA_TAL_SECTION          "dvb_rcs_tal"
#define DA_RT_BANDWIDTH         "rt_fixed_bandwidth"
#define DA_MAX_RBDC_DATA        "max_rbdc"
#define DA_RBDC_TIMEOUT_DATA    "rbdc_timeout"
#define DA_MAX_VBDC_DATA        "max_vbdc"
#define DA_MSL_DURATION         "msl_duration"
#define DA_CR_RULE              "cr_rule_output_fifo_only"

//////////
// QOS //
//////////

#define TUNTAP_BUFSIZE 1514 // ethernet header + mtu, crc not included

#define SECTION_QOS_AGENT    "qos_agent"
#define ST_NAME              "st_name"
#define QOS_SERVER_HOST      "st_address"
#define QOS_SERVER_PORT      "qos_server_port"

#define SECTION_IPQOS   "ip_qos"

#define SECTION_CLASS    "service_class"
#define CLASS_LIST       "classes"
#define CLASS_ID         "id"
#define CLASS_NAME       "name"
#define CLASS_SCHED_PRIO "scheduler_priority"
#define CLASS_MAC_ID     "mac_queue_id"

#define SECTION_CATEGORY    "traffic_category"
#define CATEGORY_LIST       "categories"
#define CATEGORY_ID         "id"
#define CATEGORY_NAME       "name"
#define CATEGORY_SERVICE    "class"
#define KEY_DEF_CATEGORY    "default_category"


//////////////////
// IP dedicated //
//////////////////

#define IPD_SECTION_V4    "ip_dedicated_v4"
#define IPD_SECTION_V6    "ip_dedicated_v6"
#define TERMINAL_LIST     "terminals"
#define TERMINAL_ADDR     "addr"
#define TERMINAL_IP_MASK  "mask"

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
#define INTERFACE_NAME_SIZE 16
#define SOCKET_TYPE         "socket_type"
#define UDP                 "SOCK_DGRAM"

/////////////////
//    Debug    //
/////////////////
#define SECTION_DEBUG "debug"

#endif
