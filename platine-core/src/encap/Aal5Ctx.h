/**
 * @file Aal5Ctx.h
 * @brief AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef AAL5_CTX_H
#define AAL5_CTX_H

#include <map>
#include <string>

#include <platine_conf/conf.h>
#include <EncapCtx.h>
#include <NetPacket.h>
#include <IpPacket.h>
#include <Ipv4Packet.h>
#include <Ipv6Packet.h>
#include <Aal5Packet.h>
#include <RohcPacket.h>
#include <NetBurst.h>


/**
 * @class Aal5Ctx
 * @brief Aal5 encapsulation / desencapsulation context
 */
class Aal5Ctx: public EncapCtx
{
 public:

	/**
	 * Build an AAL5 encapsulation / desencapsulation context
	 */
	Aal5Ctx();

	/**
	 * Destroy the AAL5 encapsulation / desencapsulation context
	 */
	~Aal5Ctx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

