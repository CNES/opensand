/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2015 TAS
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
 * @file SatGw.h
 * @brief This bloc implements satellite spots
 * @author Bénédicte Motto <bmotto@toulouse.viveris.com>
 *
 */

#ifndef SAT_GW_H
#define SAT_GW_H

#include "DvbFifo.h"
#include "DvbFrame.h"
#include "DvbChannel.h"
#include "Scheduling.h"
#include "FmtSimulation.h"
#include "StFmtSimu.h"
#include "TerminalCategoryDama.h"
#include "TimeSeriesGenerator.h"

#include <opensand_output/OutputLog.h>

#include <sys/times.h>
#include <map>
#include <list>


using std::list;
using std::map;


/**
 * @class SatSpot
 * @brief A DVB-RCS/S2 spot for the satellite emulator
 */
class SatGw: public DvbFmt 
{

 private:
	tal_id_t gw_id;            ///< Internal identifier of a gw
	spot_id_t spot_id;         ///< Internal identifier of a spot
	
	uint8_t data_in_st_id;     ///< Carrier ID associated with Data from the ST
	uint8_t data_in_gw_id;     ///< Carrier ID associated with Data from the GW

	DvbFifo *control_fifo;     ///<  Fifo associated with Control carrier
	DvbFifo *logon_fifo;       ///<  Fifo associated with Logons
	DvbFifo *data_out_gw_fifo; ///<  Fifo associated with Data for the GW
	DvbFifo *data_out_st_fifo; ///<  Fifo associated with Data for the ST

	/// the list of complete DVB-RCS/BB frames for ST that were not sent yet
	list<DvbFrame *> complete_st_dvb_frames;
	/// the list of complete DVB-RCS/BB frames fot GW that were not sent yet
	list<DvbFrame *> complete_gw_dvb_frames;

	/// The downlink scheduling for regenerative satellite toward ST
	Scheduling *st_scheduling;
	/// The downlink scheduling for regenerative satellite toward GW
	Scheduling *gw_scheduling;

	/// timer used to awake the block every second in order to retrieve
	/// the modcods
	event_id_t scenario_timer;

	// statistics

	/// Amount of layer 2 data received from ST
	vol_bytes_t l2_from_st_bytes;
	/// Amount of layer 2 data received from GW
	vol_bytes_t l2_from_gw_bytes;

	/// Mutex to protect access to spot element
	RtMutex gw_mutex;
	
	/// time series generator for input
	TimeSeriesGenerator *input_series;

	/// time series generator for output
	TimeSeriesGenerator *output_series;

	// Output probes and stats
	typedef map<unsigned int, Probe<int> *> ProbeListPerSpot;

		// Queue sizes
	ProbeListPerSpot probe_sat_output_gw_queue_size;
	ProbeListPerSpot probe_sat_output_gw_queue_size_kb;
	ProbeListPerSpot probe_sat_output_st_queue_size;
	ProbeListPerSpot probe_sat_output_st_queue_size_kb;
		// Rates
	ProbeListPerSpot probe_sat_l2_from_st;
	ProbeListPerSpot probe_sat_l2_to_st;
	ProbeListPerSpot probe_sat_l2_from_gw;
	ProbeListPerSpot probe_sat_l2_to_gw;

	// Output Log
	OutputLog *log_init;
	OutputLog *log_receive;

 public:

	/**
	 * @brief Create spot
	 *
	 * @param gw_id              The gw id
	 * @param spot_id            The spot id
	 * @param log_id             The carrier id for logon packets
	 * @param ctrl_id            The carrier id for control frames
	 * @param data_in_st_id      The carrier id for incoming GW data
	 * @param data_in_gw_id      The carrier id for incoming GW data
	 * @param data_out_st_id     The carrier id for outgoing terminal data
	 * @param data_out_gw_id     The carrier id for outgoing GW data
	 * @param fifo_size          The size of data FIFOs
	 */
	SatGw(tal_id_t gw_id,
	      spot_id_t spot_id,
	      uint8_t log_id,
	      uint8_t ctrl_id,
	      uint8_t data_in_st_id,
	      uint8_t data_in_gw_id,
	      uint8_t data_out_st_id,
	      uint8_t data_out_gw_id,
	      size_t fifo_size);
	~SatGw();

	bool init();

	/**
	 * initialize the scheduling attribute
	 *
	 * @param fwd_timer_ms    The timer for forward scheduling
	 * @param pkt_hdl         The packet handler
	 * @param fwd_modcod_def  The FMT defintion table associated
	 * @param st_category     The related terminal category for ST
	 * @param gw_category     The related terminal category for GW
	 * @return true on success, false otherwise
	 */
	bool initScheduling(time_ms_t fwd_timer_ms,
	                    const EncapPlugin::EncapPacketHandler *pkt_hdl,
	                    const TerminalCategoryDama *const st_category,
	                    const TerminalCategoryDama *const gw_category);
	
	/**
	 * @brief set scenario timer configuration
	 * 
	 * @param scenario_timer the scenario timer
	 */
	void initScenarioTimer(event_id_t scenario_timer);

	/**
	 * @brief Read configuration for the different files and open them
	 *
	 * @param return_link_standard the return link standard (DVB-RCS or DVB-RCS2)
	 * @return  true on success, false otherwise
	 */
	bool initModcodSimu(return_link_standard_t return_link_standard);
	
	/**
	 * @brief Initialize the ACM loop margins
	 *        Called in regenerative satellite as it handles
	 *        downlink MODCODs
	 *
	 * @return  true on success, false otherwise
	 */
	bool initAcmLoopMargin(void);

	/**
	 *  @brief Initialize the time series generators
	 *
	 *  @return  true on success, false otherwise
	 */
	bool initSeriesGenerator(void);

	/**
	 * @brief initialize probes
	 *
	 * @return true on success, false otherwise
	 */ 
	bool initProbes();

	/**
	 * @brief Schedule packets emission
	 *        Call scheduling @ref schedule function
	 *
	 * @param current_superframe_sf the current superframe (for logging)
	 * @param current_time          the current time
	 *
	 * @return true on success, false otherwise
	 */
	bool schedule(const time_sf_t current_superframe_sf,
	              time_ms_t current_time);

	/**
	 * Add a terminal to StFmtSimuList
	 *
	 * @param tal_id the new terminal id
	 * @return true on success, false otherwise
	 */ 
	bool addTerminal(tal_id_t tal_id);
	
	/**
	 * @brief  Add a new line in the MODCOD time series generator file
	 *
	 *  @return true on success, false otherwise
	 */
	bool updateSeriesGenerator(void);


	/**
	 * Update fmt
	 *
	 * @param dvb_frame The dvb frame
	 * @param pkt_hdl  The packet handler
	 * 
	 * @return true on success , false otherwise
	 */ 
	bool updateFmt(DvbFrame *dvb_frame,
	               EncapPlugin::EncapPacketHandler *pkt_hdl);

	/**
	 * Handle Sac
	 * 
	 * @return true on success, false otherwise
	 */ 
	bool handleSac(DvbFrame *dvb_frame);

	/**
	 * @brief handle probes
	 *
	 * @return true on success, false otherwise
	 */ 
	bool updateProbes(time_ms_t stats_period_ms);
	
	/**
	 * @brief Get the gw ID
	 *
	 * @return the gw ID
	 */
	uint16_t getGwId(void) const;

	/**
	 * @brief Get the input data ST carrier ID
	 *
	 * @return the input data ST carrier ID
	 */
	uint8_t getDataInStId(void) const;

	/**
	 * @brief Get the input data GW carrier ID
	 *
	 * @return the input data GW carrier ID
	 */
	uint8_t getDataInGwId(void) const;

	/**
	 * @brief Get the output data ST FIFO
	 *
	 * @return the output data ST FIFO
	 */
	DvbFifo *getDataOutStFifo(void) const;

	/**
	 * @brief Get the output data GW FIFO
	 *
	 * @return the output data GW FIFO
	 */
	DvbFifo *getDataOutGwFifo(void) const;

	/**
	 * @brief Get the control FIFO
	 *
	 * @return the control FIFO
	 */
	DvbFifo *getControlFifo(void) const;

 	/**
	 * @brief Get the control carrier ID
 	 *
	 * @return the control carrier ID
 	 */
	uint8_t getControlCarrierId(void) const;

	/**
	 * @brief Get the logon FIFO
	 *
	 * @return the logon FIFO
	 */
	DvbFifo *getLogonFifo(void) const;

	/**
	 * @brief Get the complete DVB frames list for ST
	 *
	 * @return the list of complete DVB frames fot ST
	 */
	list<DvbFrame *> &getCompleteStDvbFrames(void);

	/**
	 * @brief Get the complete DVB frames list for GW
	 *
	 * @return the list of complete DVB frames for GW
	 */
	list<DvbFrame *> &getCompleteGwDvbFrames(void);

	/**
	 * @brief Update the amount of layer 2 data received from ST
	 *
	 * @param  bytes  The amount of layer 2 data received
	 */
	void updateL2FromSt(vol_bytes_t bytes);

	/**
	 * @brief Update the amount of layer 2 data received from GW
	 *
	 * @param  bytes  The amount of layer 2 data received
	 */
	void updateL2FromGw(vol_bytes_t bytes);

	/**
	 * @brief Get and reset the amount of layer 2 data received from ST
	 *
	 * @return  The amount of layer 2 data received
	 */
	vol_bytes_t getL2FromSt(void);

	/**
	 * @brief Get and reset the amount of layer 2 data received from GW
	 *
	 * @return  The amount of layer 2 data received
	 */
	vol_bytes_t getL2FromGw(void);

	/**
	 * @brief Go to the first step in adaptive physical layer scenario
	 *        Update current MODCODs IDs of all STs in the list
	 *
	 * @return true on success, false otherwise
	 */
	bool goFirstScenarioStep();

	/**
	 * @brief Go to next step in adaptive physical layer scenario
	 *        Update current MODCODs IDs of all STs in the list
	 *
	 * @param duration     duration before the next step
	 * @return true on success, false otherwise
	 */
	bool goNextScenarioStep(double &duration);

	/**
	 * @brief Get the spot id
	 *
	 * @return ths spot id
	 */
	spot_id_t getSpotId(void);

	/**
	 * @brief get the output modcod definition table
	 */
	FmtDefinitionTable* getOutputModcodDef(void);
	
	/**
	 * @ brief get the gateway scenario timer
	 */ 
	event_id_t getScenarioTimer(void);

	void print(void); /// For debug
};


/// The map of satellite spots
typedef map<pair<spot_id_t, uint8_t>, SatGw *> sat_gws_t;

#endif
