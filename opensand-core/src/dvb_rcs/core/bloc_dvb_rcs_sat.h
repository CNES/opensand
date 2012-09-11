/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 * Copyright © 2011 CNES
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
#include "SatSpot.h"

// environment plane
#include "opensand_env_plane/EnvPlane.h"

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


	/* Timers */

	// Internal event handlers
	/// frame timer, used to awake the block regurlarly in order to send BBFrames
	mgl_timer m_frameTimer;
	/// timer used to awake the block every second in order to retrieve
	/// the modcods
	mgl_timer scenario_timer;

	/* misc */
	/// Flag set 1 to activate error generator
	int m_useErrorGenerator;



 public:

	BlocDVBRcsSat(mgl_blocmgr *blocmgr, mgl_id fatherid, const char *name,
	              std::map<std::string, EncapPlugin *> &encap_plug);
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
