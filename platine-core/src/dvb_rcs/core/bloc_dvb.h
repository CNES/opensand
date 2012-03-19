/*
 *
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
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
 * @file bloc.h
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author SatIP6
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *            ^
 *            | encap burst
 *            v
 *    ------------------
 *   |                  |
 *   |       DVB        |
 *   |       Dama       |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / BBFrame
 *            v
 *
 * </pre>
 *
 */

#ifndef BLOC_DVB_H
#define BLOC_DVB_H

#include "PhysicStd.h"
#include "platine_margouilla/mgl_bloc.h"
#include "NccPepInterface.h"

#define MODCOD_DRA_PATH "/etc/platine/modcod_dra/"

class BlocDvb: public mgl_bloc
{

 protected:

	/// emission standard (DVB-RCS or DVB-S2)
	PhysicStd *emissionStd;
	/// reception standard (DVB-RCS or DVB-S2)
	PhysicStd *receptionStd;


 public:

	/// Class constructor
	/// Use mgl_bloc default constructor
	BlocDvb(mgl_blocmgr * ip_blocmgr, mgl_id i_fatherid, const char *ip_name);

	~BlocDvb();

	/// event handlers
	virtual mgl_status onEvent(mgl_event * event) = 0;


	/* Methods */

 protected:

	// initialization method
	virtual int onInit() = 0;

	// Send a Netburst to the encap layer
	int SendNewMsgToUpperLayer(NetBurst *burst);

	// Common functions for satellite and NCC
	int initModcodFiles();

	// Send DVB bursts to sat carrier block
	int sendBursts(std::list<DvbFrame *> *complete_frames, long carrier_id);

	// Send a DVB frame to the sat carrier block
	bool sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id);
	bool sendDvbFrame(DvbFrame *frame, long carrier_id);



};

#endif
