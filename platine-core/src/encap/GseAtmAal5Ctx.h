/**
 * @file GseAtmAal5Ctx.h
 * @brief GSE/ATM/AAL5 encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_ATM_AAL5_CTX_H
#define GSE_ATM_AAL5_CTX_H

#include <string>

#include <EncapCtx.h>
#include <GseCtx.h>
#include <AtmAal5Ctx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <AtmCell.h>
#include <NetBurst.h>


/**
 * @class GseAtmAal5Ctx
 * @brief GSE/ATM/AAL5 encapsulation / desencapsulation context
 */
class GseAtmAal5Ctx: public AtmAal5Ctx, public GseCtx
{
 public:

	/**
	 * Build a GSE/ATM/AAL5 encapsulation / desencapsulation context
	 *
	 * @param packing_threshold  The number of QoS possible values used
	 *                           for GSE Frag ID
	 */
	GseAtmAal5Ctx(int qos_nbr, unsigned int packing_threshold);

	/**
	 * Destroy the GSE/ATM/AAL5 encapsulation / desencapsulation context
	 */
	~GseAtmAal5Ctx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

