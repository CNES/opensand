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
 * @file bloc_encap.h
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BLOC_ENCAP_H
#define BLOC_ENCAP_H

// margouilla includes
#include "platine_margouilla/mgl_bloc.h"

// message includes
#include "msg_dvb_rcs.h"
#include "platine_margouilla/msg_ip.h"

#include "platine_conf/conf.h"

#include "EncapCtx.h"
#include "AtmAal5Ctx.h"
#include "MpegUleCtx.h"
#include "MpegAtmAal5Ctx.h"
#include "GseAtmAal5Ctx.h"
#include "GseMpegUleCtx.h"
#include "NetPacket.h"
#include "NetBurst.h"
#include "UleExtTest.h"
#include "UleExtPadding.h"
#if 0
#include "AtmAal5RohcCtx.h"
#include "MpegUleRohcCtx.h"
#include "MpegAtmAal5RohcCtx.h"
#include "GseRohcCtx.h"
#include "GseAtmAal5RohcCtx.h"
#include "GseMpegUleRohcCtx.h"
#endif
#if ULE_SECURITY
	#include "UleExtSecurity.h"
#endif


/**
 * @class BlocEncap
 * @brief Generic Encapsulation Bloc
 */
class BlocEncap: public mgl_bloc
{
 private:

	/// Reception / desencapsulation context
	EncapCtx *receptionCxt;

	/// Emission / encapsulation context
	EncapCtx *emissionCxt;

	/// Expiration timers for encapsulation contexts
	std::map < mgl_timer, int > timers;

	/// Whether the bloc has been initialized or not
	bool initOk;

	/// State of the satellite link
	enum
	{
		link_down,
		link_up
	} _state;

	/// it is the MAC layer group id received through msg_link_up
	long _group_id;

	/// it is the MAC layer MAC id received through msg_link_up
	long _tal_id;

    /// the component name (GW or ST)
    string name_;

 public:

	/**
	 * Build an encapsulation bloc
	 *
	 * @param blocmgr The bloc manager
	 * @param fatherid The father of the bloc
	 * @param name The name of the bloc
     * @param host_name The name og the host
	 */
	BlocEncap(mgl_blocmgr *blocmgr, mgl_id fatherid, const char *name,
	          string host_name);

	/**
	 * Destroy the encapsulation bloc
	 */
	~BlocEncap();

	/**
	 * Handle the events
	 *
	 * @param event The event to handle
	 * @return Whether the event was successfully handled or not
	 */
	mgl_status onEvent(mgl_event *event);

 private:

	/**
	 * Initialize the encapsulation block
	 *
	 * @return  Whether the init was successful or not
	 */
	mgl_status onInit();

	/**
	 * Handle the timer event
	 *
	 * @param timer  The Margouilla timer to handle
	 * @return       Whether the timer event was successfully handled or not
	 */
	mgl_status onTimer(mgl_timer timer);

	/**
	 * Handle an IP packet received from the upper-layer block
	 *
	 * @param packet  The IP packet received from the upper-layer block
	 * @return        Whether the IP packet was successful handled or not
	 */
	mgl_status onRcvIpFromUp(NetPacket *packet);

	/**
	 * Handle a burst of encapsulation packets received from the lower-layer
	 * block
	 *
	 * @param burst  The burst received from the lower-layer block
	 * @return       Whether the burst was successful handled or not
	 */
	mgl_status onRcvBurstFromDown(NetBurst *burst);
};

#endif
