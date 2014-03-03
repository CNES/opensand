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
 * @file BlockDvbTal.h
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal,
 *        compatible with Legacy Dama agent.
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 */

#ifndef BLOCK_DVB_TAL_H
#define BLOCK_DVB_TAL_H

#include "BlockDvb.h"

#include "DamaAgent.h"
#include "UnitConverter.h"
#include "OpenSandFrames.h"
#include "OpenSandCore.h"

#include <opensand_conf/conf.h>
#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>


#include <errno.h>
//#include <stdarg.h>       // for va_* macros (ANSI format)
#include <netdb.h>        // for h_errno and hstrerror
#include <arpa/inet.h>    // for inet_ntoa
#include <sys/socket.h>
#include <signal.h>
// BEGIN STAT
#include <linux/param.h>
#include <sys/times.h>
// END STAT


// Adjust timer to linux timer precision (10 ms):
// e.g., if a frame lasts 53 ms, but we wake up every 50 ms
// so as to consume all allocated bandwidth during a superframe
// TODO find a way to detect if the linux kernel contain the RT patch
//      to disable this macro consequently
//      see https://rt.wiki.kernel.org/index.php/RT_PREEMPT_HOWTO#Runtime_detection_of_an_RT-PREEMPT_Kernel
//      BUT do we really need that ? Do wee need to add it on the other timers ?
#define DVB_TIMER_ADJUST(x)  ((long)x/10)*10


/**
 * @class BlockDvbTal
 * @brief This bloc implements a DVB-S/RCS stack for a Terminal,
 *        compatible with Legacy Dama agent.
 *
 *
 *      ^           |
 *      | encap     | encap packets with QoS
 *      | packets   v
 *    ------------------
 *   |                  |
 *   |  DVB-RCS Tal     |
 *   |  Dama Agent      |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame
 *            v
 *
 */
class BlockDvbTal: public BlockDvb
{

 private:

	/// is true if the init is done
	bool init_ok;

	/// the current state of the ST
	enum
	{
		state_null,            /**< non-existant state */
		state_off,
		state_initializing,    /**< The ST is begin started */
		state_wait_logon_resp, /**< The ST is not logged yet */
		state_running,         /**< The ST is operational */
	} _state;

	// the MAC ID of the ST (as specified in configuration)
	int mac_id;
	/// the group ID sent by NCC (only valid in state ef state_running)
	group_id_t group_id;
	/// the logon ID sent by NCC (only valid in state ef state_running,
	/// should be the same as ef mac_d)
	tal_id_t tal_id;

	/// the DAMA agent
	DamaAgent *dama_agent;


	/* carrier IDs */

	uint8_t carrier_id_ctrl;  ///< carrier id for DVB control frames emission
	uint8_t carrier_id_logon; ///< carrier id for Logon req  emission
	uint8_t carrier_id_data;  ///< carrier id for traffic emission

	/* DVB-RCS/S2 emulation */

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	std::list<DvbFrame *> complete_dvb_frames;

	/// Length of an output encapsulation packet (in bytes)
	int out_encap_packet_length;
	/// Type of output encapsulation packet
	long out_encap_packet_type;
	/// Length of an input encapsulation packet (in bytes)
	int in_encap_packet_length;

	rate_kbps_t fixed_bandwidth;   ///< fixed bandwidth (CRA) in kbits/s
	rate_kbps_t max_rbdc_kbps;     ///< Maximum RBDC in kbits/s
	vol_kb_t max_vbdc_kb;          ///< Maximum VBDC in kbits

	/// The C/N0 for downlink in scenario
	double cni;

	/* Timers and their values */

	event_id_t logon_timer;  ///< Upon each m_logonTimer event retry logon
	event_id_t frame_timer;  ///< Upon each m_frameTimer event is a frame

	/// whether this is the first frame
	bool first;


	/* Fifos */

	/// map of FIFOs per MAX priority to manage different queues
	fifos_t dvb_fifos;
	/// the default MAC fifo index = fifo with the smallest priority
	unsigned int default_fifo_id;
	/// the number of PVCs
	unsigned int nbr_pvc;


	/* QoS Server / Policy Enforcement Point (PEP) on ST side */

	static int qos_server_sock;           ///< The socket for the QoS Server
	std::string qos_server_host;   ///< The hostname of the QoS Server
	int qos_server_port;           ///< The TCP port of the QoS Server
	event_id_t qos_server_timer;   ///< The timer for connection retry to QoS Server


	/* OBR */

	/// OBR period -in number of frames- and Obr slot
	/// position within the multi-frame,
	int m_obrPeriod;
	int m_obrSlotFrame;

 public:

	BlockDvbTal(const string &name, tal_id_t mac_id);
	virtual ~BlockDvbTal();

	class Upward: public DvbUpward
	{
	  public:
		Upward(Block *const bl, tal_id_t UNUSED(mac_id)):
			DvbUpward(bl)
		{};
	};


	class Downward: public DvbDownward
	{
	  public:
		Downward(Block *const bl, tal_id_t UNUSED(mac_id)):
			DvbDownward(bl)
		{};
		bool onInit(void);
	};

  protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit(void);

 private:

	// initialization methods


	/**
	 * @brief Initialize the transmission mode
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMode();

	/**
	 * Read configuration for the parameters
	 *
	 * @return  true on success, false otherwise
	 */
	bool initParameters();

	/**
	 * Read configuration for the carrier ID
	 *
	 * @return  true on success, false otherwise
	 */
	bool initCarrierId();

	/**
	 * Read configuration for the MAC FIFOs
	 *
	 * @return  true on success, false otherwise
	 */
	bool initMacFifo();

	/**
	 * Read configuration for the OBR period
	 *
	 *
	 * @return  true on success, false otherwise
	 */
	bool initObr();

	/**
	 * Read configuration for the DAMA algorithm
	 *
	 * @return  true on success, false otherwise
	 */
	bool initDama();

	/**
	 * Read configuration for the QoS Server
	 *
	 * @return  true on success, false otherwise
	 */
	bool initQoSServer();

	/**
	 * @brief Initialize the timers
	 *
	 *
	 * @return  true on success, false otherwise
	 */
	bool initTimers();

	/**
	 * @brief Initialize the output
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput();

	bool onStartOfFrame(DvbFrame *dvb_frame);
	int processOnFrameTick();

	// DVB frame from lower layer

	/**
	 * Manage the receipt of the DVB Frames
	 *
	 * @param dvb_frame  The DVB frame
	 * @return true on success, false otherwise
	 */
	bool onRcvDvbFrame(DvbFrame *dvb_frame);

	/**
	 * Manage logon response: inform dama and upper layer that the link is now up and running
	 *
	 * @param dvb_frame  The frame containing the logon response
	 * @return true on success, false otherwise
	 */
	bool onRcvLogonResp(DvbFrame *dvb_frame);

	// UL DVB frames emission
	/**
	 * This method send a Logon Req message
	 *
	 * @return true on success, false otherwise
	 */
	bool sendLogonReq();
	bool sendSAC();

	void deletePackets();

	// statistics update
	void updateStats();
	void resetStatsCxt();

	// communication with QoS Server:
	bool connectToQoSServer();
	static void closeQosSocket(int sig);

	// Output events
	Event *event_login_sent;
	Event *event_login_complete;

	// Output probes and stats

		// Queue sizes
	map<unsigned int, Probe<int> *> probe_st_queue_size;
	map<unsigned int, Probe<int> *> probe_st_queue_size_kb;
		// Rates
				// Layer 2 to SAT
	map<unsigned int, Probe<int> *> probe_st_l2_to_sat_before_sched;
	int *l2_to_sat_cells_before_sched;
	map<unsigned int, Probe<int> *> probe_st_l2_to_sat_after_sched;
	int *l2_to_sat_cells_after_sched;
	Probe<int> *probe_st_l2_to_sat_total;
	int l2_to_sat_total_cells;
				// PHY to SAT
	Probe<int> *probe_st_phy_to_sat;
				// Layer 2 from SAT
	Probe<int> *probe_st_l2_from_sat;
	int l2_from_sat_bytes;
				// PHY from SAT
	Probe<int> *probe_st_phy_from_sat;
	int phy_from_sat_bytes;
		// Physical layer information
	Probe<int> *probe_st_real_modcod;
	Probe<int> *probe_st_received_modcod;
	Probe<int> *probe_st_rejected_modcod;
		// Stability
	Probe<float> *probe_sof_interval;
};

#endif

