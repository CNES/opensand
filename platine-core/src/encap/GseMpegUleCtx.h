/**
 * @file GseMpegUleCtx.h
 * @brief GSE/MPEG/ULE encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_MPEG_ULE_CTX_H
#define GSE_MPEG_ULE_CTX_H

#include <string>

#include <EncapCtx.h>
#include <GseCtx.h>
#include <MpegUleCtx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <MpegPacket.h>
#include <NetBurst.h>


/**
 * @class GseMpegUleCtx
 * @brief GSE/MPEG/ULE encapsulation / desencapsulation context
 */
class GseMpegUleCtx: public MpegUleCtx, public GseCtx
{
 public:

	/**
	 * Build a GSE/MPEG/ULE encapsulation / desencapsulation context
	 *
	 * @param packing_threshold  The number of QoS possible values used
	 *                           for GSE Frag ID
	 */
	GseMpegUleCtx(int qos_nbr, unsigned int packing_threshold);

	/**
	 * Destroy the GSE/MPEG/ULE encapsulation / desencapsulation context
	 */
	~GseMpegUleCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

