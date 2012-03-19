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
 * @file GseCtx.h
 * @brief GSE encapsulation / desencapsulation context
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef GSE_CTX_H
#define GSE_CTX_H

#include <EncapCtx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <AtmCell.h>
#include <MpegPacket.h>
#include <NetBurst.h>
#include <GseEncapCtx.h>
#include <GseIdentifier.h>

#include <vector>
#include <map>

extern "C"
{
	#include <gse/constants.h>
	#include <gse/status.h>
	#include <gse/virtual_fragment.h>
	#include <gse/encap.h>
	#include <gse/deencap.h>
}


/**
 * @class GseCtx
 * @brief GSE encapsulation / desencapsulation context
 */
class GseCtx: public EncapCtx
{
 private:

	/// The GSE encapsulation context
	gse_encap_t *encap;
	/// The GSE deencapsulation context
	gse_deencap_t *deencap;
	/// Vector of GSE virtual fragments
	std::vector<gse_vfrag_t *> vfrag_pkt_vec;
	/// Temporary buffers for encapsulation contexts. Contexts are identified
	/// by an unique identifier
	/// Used only for ATM or MPEG packet encapsulation
	std::map <GseIdentifier *, GseEncapCtx *, ltGseIdentifier> contexts;
	/// The packet length for MPEG or ATM (de)encapsulation.
	unsigned int packet_length;
	/// The packing threshold for encapsulation. Packing Threshold is the time
	/// the context can wait for additional SNDU packets to fill the incomplete
	/// GSE packet before sending the GSE packet with padding.
	unsigned long packing_threshold;

 public:

	/**
	 * Build a GSE encapsulation / deencapsulation context
	 */
	GseCtx(int qos_nbr,
	       unsigned int packing_threshold = 0,
	       unsigned int packet_length = 0);

	/**
	 * Destroy the GSE encapsulation / deencapsulation context
	 */
	~GseCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);
	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif
