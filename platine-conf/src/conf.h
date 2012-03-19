/**
 * @file conf.h
 * @brief Configuration options
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard / Viveris Technologies
 *
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

#define GLOBAL_SECTION           "Global"

#define IN_ENCAP_SCHEME          "InputEncapScheme"
#define OUT_ENCAP_SCHEME         "OutputEncapScheme"
#define ENCAP_ATM_AAL5           "ATM_AAL5"
#define ENCAP_MPEG_ULE           "MPEG_ULE"
#define ENCAP_MPEG_ATM_AAL5      "MPEG_ATM_AAL5"
#define ENCAP_GSE                "GSE"
#define ENCAP_GSE_ATM_AAL5       "GSE_ATM_AAL5"
#define ENCAP_GSE_MPEG_ULE       "GSE_MPEG_ULE"
#define ENCAP_ATM_AAL5_ROHC      "ATM_AAL5_ROHC"
#define ENCAP_MPEG_ULE_ROHC      "MPEG_ULE_ROHC"
#define ENCAP_MPEG_ATM_AAL5_ROHC "MPEG_ATM_AAL5_ROHC"
#define ENCAP_GSE_ROHC           "GSE_ROHC"
#define ENCAP_GSE_ATM_AAL5_ROHC  "GSE_ATM_AAL5_ROHC"
#define ENCAP_GSE_MPEG_ULE_ROHC  "GSE_MPEG_ULE_ROHC"

#define SATELLITE_TYPE           "satelliteType"
#define TRANSPARENT_SATELLITE    "transparent"
#define REGENERATIVE_SATELLITE   "regenerative"

#define SAT_DELAY                "delay"

#define PACK_THRES               "packingThreshold"
#define GSE_QOS_NBR              "gseQosNumber"
#define DFLT_PACK_THRES          10 // default packing threshold in milliseconds
#define DFLT_GSE_QOS_NBR         5  // default QoS number for GSE encapsulation and desencapsulation
#define NB_MAX_ST                (1941)
#define DVB_F_DURATION           "frame_duration"

#define DVB_SCENARIO             "dvb_scenario"
#define DVB_SCENARIO_REFRESH     "dvb_scenario_refresh"

#define BANDWIDTH                "bandwidth"

//////////////////////
// section macLayer //
//////////////////////

#define DVB_MAC_LAYER_SECTION    "macLayer"

#define DVB_FRMS_PER_SUPER       "frames_per_superframe"
#define DVB_FRM_DURATION         "frame_duration"


/////////////////////////
// section Dvb_rcs_tal //
/////////////////////////

#define DVB_TAL_SECTION          "Dvb_rcs_tal"

#define DVB_TYPE                 "DvbType"
#define DVB_RT_BANDWIDTH         "RTFixedBandwidth"
#define DVB_MAC_ID               "DvbMacId"
#define DVB_SIMU_COL             "SimuColumnNbr"
#define DVB_CAR_ID_CTRL          "CarrierIdDvbCtrl"
#define DVB_CAR_ID_LOGON         "CarrierIdLogon"
#define DVB_CAR_ID_DATA          "CarrierIdData"
#define DVB_OBR_PERIOD_DATA      "ObrPeriod"
#define DVB_DAMA_ALGO            "DamaAlgorithm"


/////////////////////////
// section Dvb_rcs_sat //
/////////////////////////

#define SAT_DVB_SECTION          "Dvb_rcs_sat"

#define SAT_RAND_SEED            "seed"

#define SAT_TABLE_SPOT_FMT       "%ld %ld %ld %ld %ld %ld"
#define SAT_TABLE_SPOT_NB_ITEM   (6)

#define SAT_SIMU_COL_SECTION     "SimulationColumn"
#define SAT_SIMU_COL_FMT         "%ld %ld"
#define SAT_SIMU_COL_NB_ITEM     (2)
#define SAT_SWITCH_SECTION       "SatSwitchingTable"
#define SAT_TABLE_SWITCH_FMT     "%ld %ld"
#define SAT_TABLE_SWITCH_NB_ITEM (2)

#define SAT_ERR_GENERATOR        "error_generator"
#define SAT_ERR_GENERATOR_NONE   "none"
#define SAT_ERR_GENERATOR_DEFAULT "default"
#define SAT_ERR_BER              "error_generator_ber"
#define SAT_ERR_MEAN             "error_generator_mean"
#define SAT_ERR_DELTA            "error_generator_delta"


//////////////////////
// bloc_dvb_rcs_ncc //
//////////////////////

#define OUT_ST_ENCAP_SCHEME     "OutputSTEncapScheme"
#define DVB_GW_MAC_ID           (0L)
#define DVB_MAC_SECTION         "macLayer"
#define DVB_MEDIUM_RATE         "transmission_rate"
#define DVB_FPF                 "frames_per_superframe"

#define DVB_NCC_SECTION         "Dvb_rcs_ncc"
#define DVB_NCC_DAMA_ALGO       "dama_algorithm"
#define DVB_CTRL_CAR            "CarrierIdDvbCtrl"
#define DVB_SOF_CAR             "CarrierIdSOF"
#define DVB_DATA_CAR            "CarrierIdData"
#define DVB_SIZE_FIFO           "max_fifo"

//////////////////////
//    PEP section   //
//////////////////////

#define NCC_SECTION_PEP      "Pep"
#define DVB_NCC_ALLOC_DELAY  "pep_alloc_delay"
#define PEP_DAMA_PORT        "pep_to_dama_port"

//////////////////
// DAMA CONTROL //
//////////////////

#define DC_SECTION_NCC              "Dvb_rcs_ncc"
#define DC_CRA_DECREASE             "cra_decrease"
#define DC_YES                      "yes"
#define DC_NO                       "no"
#define DC_FREE_CAP                 "fca"
#define DC_RBDC_TIMEOUT             "rbdc_timeout"
#define DC_MAX_RBDC                 "max_rbdc"
#define DC_MIN_VBDC                 "min_vbdc"
#define DC_SECTION_MAC_LAYER        "macLayer"
#define DC_CARRIER_TRANS_RATE       "carrier_transmission_rate"
#define DC_CARRIER_NUMBER           "carrier_number"
#define DC_GAC_PORT                 "gac_port"

#define DC_DFLT_RBDC_TIMEOUT        16
#define DC_DFLT_CARRIER_TRANS_RATE  4096
#define DC_DFLT_GAC_PORT            5555



//////////
// DAMA //
//////////

#define DA_MAC_LAYER_SECTION    "macLayer"
#define DA_FRM_DURATION         "frame_duration"
#define DA_CARRIER_TRANS_RATE   "carrier_transmission_rate"

#define DA_TAL_SECTION          "Dvb_rcs_tal"
#define DA_RT_BANDWIDTH         "RTFixedBandwidth"
#define DA_OBR_PERIOD_DATA      "ObrPeriod"
#define DA_MAX_RBDC_DATA        "MaxRbdc"
#define DA_RBDC_TIMEOUT_DATA    "RbdcTimeout"
#define DA_MAX_VBDC_DATA        "MaxVbdc"
#define DA_MSL_DURATION         "MSLDuration"
#define DA_CR_RULE              "CrRuleOutputFifoOnly"

// default parameter values used by NCC
#define DC_DFLT_DAMA_BHV        SACT_builtin
#define DC_DFLT_CRA_DECREASE    1
#define DC_DFLT_FREE_CAP        100 // in kbits/s
#define DC_DFLT_MIN_VBDC        0   // in kbits/s
#define DC_DFLT_CARRIER_NUMBER  1

// default parameter values used by ST
#define DA_DFLT_RT_BANDWIDTH    0    // in kbits/s
#define DA_DFLT_OBR_PERIOD_DATA 16   // in frame number
#define DA_DFLT_MAX_RBDC_DATA   8192 // in kbits/s
#define DA_DFLT_MAX_VBDC_DATA   8192 // in number of time-slots
#define DA_DFLT_MSL_DURATION    23   // in frame number
#define DA_DFLT_CR_RULE         "no" // output fifo size only = no --> both
                                     // input and output DLB fifos size are
                                     // taken into account in CR

// default parameter values used by NCC and ST
#define DFLT_RBDC_TIMEOUT       16   // in frame number
#define DFLT_CARRIER_TRANS_RATE 4096 // in kbits/s
#define DFLT_FRMS_PER_SUPER     10
#define DFLT_FRM_DURATION       53   // in ms

//////////
// QOS //
//////////

#define TUNTAP_BUFSIZE 1514 // ethernet header + mtu, crc not included

#define SECTION_QOS_AGENT    "QoSAgent"
#define ST_NAME              "st_name"
#define QOS_SERVER_HOST      "st_address"
#define QOS_SERVER_PORT      "qos_server_port"
#define QOS_SERVER_DFLT_HOST "192.168.21.5"
#define QOS_SERVER_DFLT_PORT 12000

#define SECTION_IPQOS   "IPQoS"

#define SECTION_CLASS    "ServiceClass"
#define SCHEDULER_TYPE   "SchedulerType"
#define QOS_TREE_MODE    "QoSTreeMode"
#define MAPPING_SIG      "mapping_sig"
#define SIG_PORT       "sig_port"

#define SECTION_CATEGORY    "TrafficCategory"
#define KEY_DEF_CATEGORY    "DefaultCategory"


//////////////////
// IP dedicated //
//////////////////

#define IPD_SECTION_V4 "ip_dedicated_v4"
#define IPD_SECTION_V6 "ip_dedicated_v6"



/////////////////
// SAT Carrier //
/////////////////

#define SATCAR_SECTION "SatCarrier"
#define INTERFACE_NAME_SIZE 16
#define SOCKET_TYPE    "type"
#define UDP            "SOCK_DGRAM"
#define ETHERNET       "SOCK_RAW"
#define IPADDR         "addr"

#endif
