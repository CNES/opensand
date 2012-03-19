/**
 * @file AtmAal5Ctx.h
 * @brief ATM/AAL5 encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ATM_AAL5_CTX_H
#define ATM_AAL5_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <Aal5Ctx.h>
#include <AtmCtx.h>
#include <NetPacket.h>
#include <NetBurst.h>


/**
 * @class AtmAal5Ctx
 * @brief ATM/AAL5 encapsulation / desencapsulation context
 */
class AtmAal5Ctx: public Aal5Ctx, public AtmCtx
{
 public:

	/**
	 * Build a ATM/AAL5 encapsulation / desencapsulation context
	 */
	AtmAal5Ctx();

	/**
	 * Destroy the ATM/AAL5 encapsulation / desencapsulation context
	 */
	~AtmAal5Ctx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

