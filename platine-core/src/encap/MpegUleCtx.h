/**
 * @file MpegUleCtx.h
 * @brief MPEG2-TS/ULE encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_ULE_CTX_H
#define MPEG_ULE_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <UleCtx.h>
#include <MpegCtx.h>
#include <NetPacket.h>
#include <NetBurst.h>


/**
 * @class MpegUleCtx
 * @brief MPEG2-TS/ULE encapsulation / desencapsulation context
 */
class MpegUleCtx: public UleCtx, public MpegCtx
{
 public:

	/**
	 * Build a MPEG2-TS/ULE encapsulation / desencapsulation context
	 *
	 * @param packing_threshold The Packing Threshold, ie. the maximum time (in
	 *                          milliseconds) to wait before sending an
	 *                          incomplete MPEG packet
	 */
	MpegUleCtx(unsigned long packing_threshold);

	/**
	 * Destroy the MPEG2-TS/ULE encapsulation / desencapsulation context
	 */
	~MpegUleCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

