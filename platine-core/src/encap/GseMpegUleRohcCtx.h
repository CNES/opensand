/**
 * @file GseMpegUleRohcCtx.h
 * @brief GSE/MPEG/ULE/ROHC encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_MPEG_ULE_ROHC_CTX_H
#define GSE_MPEG_ULE_ROHC_CTX_H

#include <string>

#include <GseMpegUleCtx.h>
#include <RohcCtx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <MpegPacket.h>
#include <NetBurst.h>


/**
 * @class GseMpegUleRohcCtx
 * @brief GSE/MPEG/ULE/ROHC encapsulation / desencapsulation context
 */
class GseMpegUleRohcCtx: public RohcCtx, public GseMpegUleCtx
{
 public:

	/**
	 * Build a GSE/MPEG/ULE/ROHC encapsulation / desencapsulation context
	 *
	 * @param qos_nbr            The number of QoS possible values used
	 *                           for GSE Frag ID
	 * @param packing_threshold  The maximum time (ms) to wait before sending
	 *                           an incomplete MPEG packet
	 */
	GseMpegUleRohcCtx(int qos_nbr, unsigned int packing_threshold);

	/**
	 * Destroy the GSE/MPEG/ULE/ROHC encapsulation / desencapsulation context
	 */
	~GseMpegUleRohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

