/**
 * @file MpegAtmAal5RohcCtx.h
 * @brief MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_ATM_AAL5_ROHC_CTX_H
#define MPEG_ATM_AAL5_ROHC_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <RohcCtx.h>
#include <MpegAtmAal5Ctx.h>
#include <NetPacket.h>
#include <MpegPacket.h>
#include <NetBurst.h>


/**
 * @class MpegAtmAal5RohcCtx
 * @brief MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
 */
class MpegAtmAal5RohcCtx: public RohcCtx, MpegAtmAal5Ctx
{
 public:

	/**
	 * Build a MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
	 *
	 * @param packing_threshold The Packing Threshold, ie. the maximum time (in
	 *                          milliseconds) to wait before sending an
	 *                          incomplete MPEG packet
	 */
	MpegAtmAal5RohcCtx(unsigned long packing_threshold);

	/**
	 * Destroy the MPEG2-TS/ATM/AAL5/ROHC encapsulation / desencapsulation context
	 */
	~MpegAtmAal5RohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

