/**
 * @file bloc_dvb_rcs_sat.h
 * @brief This bloc implements a DVB-S/RCS stack for a Satellite
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 * <pre>
 *
 *                  ^
 *                  | DVB Frame / BBFrame
 *                  v
 *           ------------------
 *          |                  |
 *          |  DVB-RCS Sat     |  <- Set carrier infos
 *          |                  |
 *           ------------------
 *
 * </pre>
 *
 */

#ifndef BLOC_DVB_RCS_SAT_H
#define BLOC_DVB_RCS_SAT_H

#include <linux/param.h>

using namespace std;

#include "bloc_dvb.h"
#include "GenericSwitch.h"
#include "SatSpot.h"


/**
 * Blocs heritate from mgl_bloc clam_singleSpot.sse
 * mgl_bloc classe defines some default handlers such as 'onEvent'
 */
class BlocDVBRcsSat: public BlocDvb
{

 private:

	/// Whether the bloc has been initialized or not
	bool initOk;

	/// The satellite spots
	SpotMap spots;

	/// The satellite delay to emulate
	int m_delay;


	/* DVB-RCS/S2 emulation */

	/// Satellite type (transparent/regenerative)
	string satellite_type;

	/// how often do we refresh the adaptive physical layer scenario (in ms)
	int dvb_scenario_refresh;


	/* Timers */

	// Internal event handlers
	/// frame timer, used to awake the block regurlarly in order to send BBFrames
	mgl_timer m_frameTimer;
	/// timer used to awake the block every second in order to retrieve
	/// the modcods
	mgl_timer scenario_timer;
	/// the frame duration
	int frameDuration;


	/* misc */

	/// The input encapsulation protocol (ATM or MPEG)
	std::string in_encap_proto;

	/// Flag set 1 to activate error generator
	int m_useErrorGenerator;



 public:

	BlocDVBRcsSat(mgl_blocmgr *blocmgr, mgl_id fatherid, const char *name);
	~BlocDVBRcsSat();

	/// Get the satellite type
	string getSatelliteType();

	/// get the bandwidth
	int getBandwidth();

	/// event handlers
	mgl_status onEvent(mgl_event * event);

 private:

	// initialization
	int onInit();
	int initMode();
	int initErrorGenerator();
	int initTimers();
	int initSwitchTable();
	int initSpots();
	int initStList();

	// event management
	mgl_status onRcvDVBFrame(unsigned char *frame, unsigned int length, long carrier_id);
	int sendSigFrames(dvb_fifo * sigFifo);
	mgl_status forwardDVBFrame(dvb_fifo * sigFifo, char *ip_buf, int i_len);
	int onSendFrames(dvb_fifo *fifo, long current_time);

	/**
	 * Get next random delay provided the two preceeding members
	 */
	inline int getNextDelay()
	{
		return this->m_delay;
	}

	/// generate some error
	void errorGenerator(NetPacket * encap_packet);

	/// update the probes
	void getProbe(NetBurst burst, dvb_fifo fifo, sat_StatBloc m_stat_fifo);

};

#endif
