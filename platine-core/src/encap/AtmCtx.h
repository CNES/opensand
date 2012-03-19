/**
 * @file AtmCtx.h
 * @brief ATM encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ATM_CTX_H
#define ATM_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <NetPacket.h>
#include <AtmCell.h>
#include <Aal5Packet.h>
#include <NetBurst.h>
#include <AtmIdentifier.h>


/**
 * @class AtmCtx
 * @brief ATM encapsulation / desencapsulation context
 */
class AtmCtx: public EncapCtx
{
 private:

	/**
	 * Temporary buffers for desencapsulation contexts. Contexts are identified
	 * by an unique ATM identifier (= VPI + VCI)
	 */
	std::map < AtmIdentifier *, Data *, ltAtmIdentifier > contexts;

 public:

	/**
	 * Build an ATM encapsulation / desencapsulation context
	 */
	AtmCtx();

	/**
	 * Destroy the ATM encapsulation / desencapsulation context
	 */
	~AtmCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

