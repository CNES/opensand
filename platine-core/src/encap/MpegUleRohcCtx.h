/**
 * @file MpegUleRohcCtx.h
 * @brief MPEG2-TS/ULE/ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_ULE_ROHC_CTX_H
#define MPEG_ULE_ROHC_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <RohcCtx.h>
#include <MpegUleCtx.h>
#include <NetPacket.h>
#include <NetBurst.h>


/**
 * @class MpegUleRohcCtx
 * @brief MPEG2-TS/ULE/ROHC encapsulation / desencapsulation context
 */
class MpegUleRohcCtx: public RohcCtx, MpegUleCtx
{
 public:

	/**
	 * Build a MPEG2-TS/ULE/ROHC encapsulation / desencapsulation context
	 *
	 * @param packing_threshold The Packing Threshold, ie. the maximum time (in
	 *                          milliseconds) to wait before sending an
	 *                          incomplete MPEG packet
	 */
	MpegUleRohcCtx(unsigned long packing_threshold);

	/**
	 * Destroy the MPEG2-TS/ULE/ROHC encapsulation / desencapsulation context
	 */
	~MpegUleRohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

