/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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
 * @file BlockEncap.h
 * @brief Generic Encapsulation Bloc
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BLOCK_ENCAP_H
#define BLOCK_ENCAP_H


#include "NetPacket.h"
#include "NetBurst.h"
#include "StackPlugin.h"
#include "EncapPlugin.h"
#include "OpenSandCore.h"
#include "LanAdaptationPlugin.h"
#include "OpenSandFrames.h"



#include <opensand_output/Output.h>
#include <opensand_rt/Rt.h>
#include <opensand_conf/conf.h>

/**
 * @class BlockEncap
 * @brief Generic Encapsulation Bloc
 */
class BlockEncap: public Block
{
 private:

	/// Expiration timers for encapsulation contexts
	std::map<event_id_t, int> timers;

	/// it is the MAC layer group id received through msg_link_up
	group_id_t group_id;

	/// it is the MAC layer MAC id received through msg_link_up
	tal_id_t tal_id;

	/// State of the satellite link
	link_state_t state;
	
	/// the MAC ID of the ST (as specified in configuration)
	int mac_id;

	/// the satellite type (regenerative o transparent)
	sat_type_t satellite_type;

	/// the emission contexts list from lower to upper context
	std::vector<EncapPlugin::EncapContext *> emission_ctx;

	/// the reception contexts list from upper to lower context
	std::vector<EncapPlugin::EncapContext *> reception_ctx;

	/// the reception contexts list from upper to lower context for SCPC mode
	std::vector<EncapPlugin::EncapContext *> reception_ctx_scpc;




 public:

	/**
	 * Build an encapsulation block
	 *
	 * @param name  The name of the block
	 * @param name  The mac id of the terminal
	 */
	BlockEncap(const string &name, tal_id_t mac_id);

	/**
	 * Destroy the encapsulation block
	 */
	~BlockEncap();

 protected:

	// Log and debug
	OutputLog *log_rcv_from_up;
	OutputLog *log_rcv_from_down;
	OutputLog *log_send_down;

	// Init output (log, debug, probes, stats)
	bool initOutput();

	// event handlers
	bool onDownwardEvent(const RtEvent *const event);
	bool onUpwardEvent(const RtEvent *const event);

	// initialization method
	bool onInit();

 private:

	/**
	 * Handle the timer event
	 *
	 * @param timer_id  The id of the timer to handle
	 * @return          Whether the timer event was successfully handled or not
	 */
	bool onTimer(event_id_t timer_id);

	/**
	 * Handle a burst received from the upper-layer block
	 *
	 * @param burst  The burst received from the upper-layer block
	 * @return        Whether the IP packet was successful handled or not
	 */
	bool onRcvBurstFromUp(NetBurst *burst);

	/**
	 * Handle a burst of encapsulation packets received from the lower-layer
	 * block
	 *
	 * @param burst  The burst received from the lower-layer block
	 * @return       Whether the burst was successful handled or not
	 */
	bool onRcvBurstFromDown(NetBurst *burst);

	/**
	 * @brief Checks if SCPC mode is activated and configured
	 *        (Available FIFOs and Carriers for SCPC)
	 *
	 * @return       Whether there are SCPC FIFOs and SCPC Carriers available or not
	 */
	bool checkIfScpc();

	/**
	 * 
	 * Get the Encapsulation context of the Up/Return or the Down/Forward link
	 *
	 * @param scheme_list   The name of encapsulation scheme list
	 * @param l_plugin      The LAN adaptation plugin
	 * @ctx                 The encapsulation context for return/up or forward/down links
	 * @link_type           The type of link: "return/up" or "forward/down"
	 * @scpc_scheme			Whether SCPC is used for the return link or not
	 * @return              Whether the Encapsulation context has been
	 *                      correctly obtained or not
	 */
	
	bool getEncapContext(const char *scheme_list,
	                     LanAdaptationPlugin *l_plugin,
	                     vector <EncapPlugin::EncapContext *> &ctx,
	                     const char *link_type, 
	                     bool scpc_scheme);
};


#endif
