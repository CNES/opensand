/**
 * @file GseRohcCtx.h
 * @brief GSE/ROHC encapsulation / desencapsulation context
 * @author Julien BERNARD <jbernard@toulouse.viveris.com>
 */

#ifndef GSE_ROHC_CTX_H
#define GSE_ROHC_CTX_H

#include <map>
#include <string>

#include <GseCtx.h>
#include <RohcCtx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <NetBurst.h>

/**
 * @class GseRohcCtx
 * @brief GSE/ROHC encapsulation / desencapsulation context
 */
class GseRohcCtx: public RohcCtx, GseCtx
{
 public:

	/**
	 * Build a GSE/ROHC encapsulation / deencapsulation context
	 */
	GseRohcCtx(int qos_nbr,
	           unsigned int packing_threshold = 0,
	           unsigned int packet_length = 0);

	/**
	 * Destroy the GSE/ROHC encapsulation / deencapsulation context
	 */
	~GseRohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);
	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

