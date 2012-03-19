/**
 * @file RohcCtx.h
 * @brief ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ROHC_CTX_H
#define ROHC_CTX_H

#include <EncapCtx.h>
#include <NetPacket.h>
#include <RohcPacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <NetBurst.h>

extern "C"
{
	#include <rohc.h>
	#include <rohc_comp.h>
	#include <rohc_decomp.h>
}

#define MAX_ROHC_SIZE      (5 * 1024)

/**
 * @class RohcCtx
 * @brief ROHC encapsulation / desencapsulation context
 */
class RohcCtx: public EncapCtx
{
 private:

	/// The ROHC compressor
	struct rohc_comp *comp;
	/// The ROHC decompressor
	struct rohc_decomp *decomp;

 public:

	/**
	 * Build a ROHC encapsulation / desencapsulation context
	 */
	RohcCtx();

	/**
	 * Destroy the ROHC encapsulation / desencapsulation context
	 */
	~RohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);
	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

