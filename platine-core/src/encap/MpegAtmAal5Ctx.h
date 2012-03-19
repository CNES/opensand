/**
 * @file MpegAtmAal5Ctx.h
 * @brief MPEG2-TS/ATM/AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_ATM_AAL5_CTX_H
#define MPEG_ATM_AAL5_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <MpegCtx.h>
#include <AtmAal5Ctx.h>
#include <NetPacket.h>
#include <MpegPacket.h>
#include <AtmCell.h>
#include <NetBurst.h>


/**
 * @class MpegAtmAal5Ctx
 * @brief MPEG2-TS/ATM/AAL5 encapsulation / desencapsulation context
 */
class MpegAtmAal5Ctx: public AtmAal5Ctx, public MpegCtx
{
 public:

	/**
	 * Build a MPEG2-TS/ATM/AAL5 encapsulation / desencapsulation context
	 *
	 * @param packing_threshold The Packing Threshold, ie. the maximum time (in
	 *                          milliseconds) to wait before sending an
	 *                          incomplete MPEG packet
	 */
	MpegAtmAal5Ctx(unsigned long packing_threshold);

	/**
	 * Destroy the MPEG2-TS/ATM/AAL5 encapsulation / desencapsulation context
	 */
	~MpegAtmAal5Ctx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

