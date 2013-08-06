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
#define DVB_TIMER_ADJUST(x)  ((long)x/10)*10


/// ST MAC layer UL/DL throughput statistics context - updated each frame
typedef struct _TalStatContext
{
	int dlOutgoingThroughput;    ///< DL throughput received on the AIR IF by the ST (kbits/s)
	int *ulIncomingThroughput;   ///< UL throughput received on the terrestrial interface by the ST (kbits/s)
	int *ulOutgoingThroughput;   ///< UL throughput sent on the AIR IF by the ST (kbits/s)
} TalStatContext;

/// ST MAC layer statistics context counters
typedef struct _TalStatCounter
{
	int dlOutgoingCells;  ///< DL throughput received on the AIR IF by the ST (kbits/s)
	int *ulIncomingCells; ///< UL throughput received on the terrestrial interface by the ST (kbits/s)
	int *ulOutgoingCells; ///< UL throughput sent on the AIR IF by the ST (kbits/s)
} TalStatCounter;



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
	long m_groupId;
	/// the logon ID sent by NCC (only valid in state ef state_running,
	/// should be the same as ef mac_d)
	long m_talId;

	/// the DAMA agent
	DamaAgent *dama_agent;


	/* carrier IDs */

	long m_carrierIdDvbCtrl; ///< carrier id for DVB control frames emission
	long m_carrierIdLogon;   ///< carrier id for Logon req  emission
	long m_carrierIdData;    ///< carrier id for traffic emission

	/* DVB-RCS/S2 emulation */

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	std::list<DvbFrame *> complete_dvb_frames;

	/// The capacity request
	CapacityRequest capacity_request;
	/// The received TTP
	Ttp ttp;

	float m_bbframe_dropped_rate;
	int m_bbframe_dropped;
	int m_bbframe_received;


	/// Length of an output encapsulation packet (in bytes)
	int out_encap_packet_length;
	/// Type of output encapsulation packet
	long out_encap_packet_type;
	/// Length of an input encapsulation packet (in bytes)
	int in_encap_packet_length;

	int m_fixedBandwidth;   ///< fixed bandwidth (CRA) in kbits/s


	/* Timers and their values */

	event_id_t logon_timer;  ///< Upon each m_logonTimer event retry logon
	event_id_t frame_timer;  ///< Upon each m_frameTimer event is a frame
	/// The sf counter
	long super_frame_counter;
	/// the frame number WITHIN the current superframe
	long frame_counter; // is now from 1 (1st frame)
	                    // to m_frames_per_superframe (last frame)


	/* Fifos */

	/// map of FIFOs per MAX priority to manage different queues
	std::map<unsigned int, DvbFifo *> dvb_fifos;
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


	/* Probes */

	TalStatContext m_statContext;
	TalStatCounter m_statCounters;  ///< counter for UL/DL throughput stats


 public:

	BlockDvbTal(const string &name, tal_id_t mac_id);
	virtual ~BlockDvbTal();

	class DvbTalUpward: public Upward
	{
	  public:
		DvbTalUpward(Block &bl):
			Upward(bl)
		{};

		bool onInit(void);
	};

	class DvbTalDownward: public Downward
	{
	  public:
		DvbTalDownward(Block &bl):
			Downward(bl)
		{};

		bool onInit(void);
	};

  protected:

	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);
	bool onInit();

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
	bool initMacFifo(std::vector<std::string>&);

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
	bool initDownwardTimers();

	/**
	 * @brief Initialize the output
	 *
	 * @return  true on success, false otherwise
	 */
	bool initOutput(const std::vector<std::string>&);

	bool onStartOfFrame(unsigned char *ip_buf, long l_len);
	int processOnFrameTick();

	// DVB frame from lower layer

	/**
	 * Manage the receipt of the DVB Frames
	 *
	 * @param ip_buf the data buffer
	 * @param i_len the length of the buffer
	 * @return true on success, false otherwise
	 */
	bool onRcvDvbFrame(unsigned char *ip_buf, long l_len);

	/**
	 * Manage logon response: inform dama and upper layer that the link is now up and running
	 * @param ip_buf the pointer to the longon response message
	 * @param l_len the lenght of the message
	 * @return true on success, false otherwise
	 */
	bool onRcvLogonResp(unsigned char *ip_buf, long l_len);

	// UL DVB frames emission
	/**
	 * This method send a Logon Req message
	 *
	 * @return true on success, false otherwise
	 */
	bool sendLogonReq();
	bool sendCR();

	void deletePackets();

	// statistics management
	void updateStatsOnFrameAndEncap();
	void updateStatsOnFrame();
	void resetStatsCxt();

	// communication with QoS Server:
	bool connectToQoSServer();
	static void closeQosSocket(int sig);

	// output probes and events
	Event *event_login_sent;
	Event *event_login_complete;

	Probe<int> **probe_st_terminal_queue_size;
	Probe<int> **probe_st_real_in_thr;
	Probe<int> **probe_st_real_out_thr;
	Probe<int> *probe_st_phys_out_thr;
	Probe<int> *probe_st_rbdc_req_size;
	Probe<int> *probe_st_vbdc_req_size;
	Probe<int> *probe_st_cra;
	Probe<int> *probe_st_alloc_size;
	Probe<int> *probe_st_unused_capacity;
	Probe<float> *probe_st_bbframe_drop_rate;
	Probe<int> *probe_st_real_modcod;
	Probe<int> *probe_st_used_modcod;

};

#endif
