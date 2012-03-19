/**
 * @file AtmAal5RohcCtx.h
 * @brief ATM/AAL5/ROHC encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ATM_AAL5_ROHC_CTX_H
#define ATM_AAL5_ROHC_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <RohcCtx.h>
#include <AtmAal5Ctx.h>
#include <NetPacket.h>
#include <NetBurst.h>


/**
 * @class AtmAal5RohcCtx
 * @brief ATM/AAL5/ROHC encapsulation / desencapsulation context
 */
class AtmAal5RohcCtx: public RohcCtx, AtmAal5Ctx
{
 public:

	/**
	 * Build a ATM/AAL5/ROHC encapsulation / desencapsulation context
	 */
	AtmAal5RohcCtx();

	/**
	 * Destroy the ATM/AAL5 encapsulation / desencapsulation context
	 */
	~AtmAal5RohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

