/**
 * @file GseAtmAal5RohcCtx.h
 * @brief GSE/ATM/AAL5/ROHC encapsulation / desencapsulation context
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_ATM_AAL5_ROHC_CTX_H
#define GSE_ATM_AAL5_ROHC_CTX_H

#include <string>

#include <EncapCtx.h>
#include <GseAtmAal5Ctx.h>
#include <RohcCtx.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <AtmCell.h>
#include <NetBurst.h>


/**
 * @class GseAtmAal5RohcCtx
 * @brief GSE/ATM/AAL5/ROHC encapsulation / desencapsulation context
 */
class GseAtmAal5RohcCtx: public RohcCtx, public GseAtmAal5Ctx
{
 public:

	/**
	 * Build a GSE/ATM/AAL5 encapsulation / desencapsulation context
	 *
	 * @param qos_nbr            The number of QoS possible values used
	 *                           for GSE Frag ID
	 * @param packing_threshold  The maximum time (ms) to wait before sending
	 *                           an incomplete MPEG packet
	 */
	GseAtmAal5RohcCtx(int qos_nbr, unsigned int packing_threshold);

	/**
	 * Destroy the GSE/ATM/AAL5/ROHC encapsulation / desencapsulation context
	 */
	~GseAtmAal5RohcCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);

	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();
};

#endif

