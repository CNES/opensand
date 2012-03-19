/**
 * @file EncapCtx.h
 * @brief Generic encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ENCAP_CTX_H
#define ENCAP_CTX_H

#include <NetPacket.h>
#include <NetBurst.h>


/**
 * @class EncapCtx
 * @brief Generic encapsulation / desencapsulation context
 */
class EncapCtx
{

 private:

	/// The TAL ID to use a filter when desencapsulating packets
	long _tal_id;

 public:

	/**
	 * Build a generic encapsulation / desencapsulation context
	 */
	EncapCtx();

	/**
	 * Destroy the generic encapsulation / desencapsulation context
	 */
	virtual ~EncapCtx();

	/**
	* @brief Tell the context to filter packets against the TAL ID of the ST
	*        when desencapsulating packets
	*
	* This function is called by the encapsulation layer upon reception of
	* the link layer message in order to tell the context to filter frames that
	* are not sent to itself.
	*
	* @param tal_id   The TAL ID to use as filter
	*/
	void setFilter(long tal_id);

	/**
	* Get the TAL ID to use as a filter when desencapsulating packets
	*
	* @return  The TAL ID to use as filter
	*/
	long talId();

	/**
	 * Encapsulate a packet into one or several packets. The function returns a
	 * context ID and an expiration time. It's the caller charge to arm a timer
	 * to manage context expiration. It's also the caller charge to delete the
	 * returned NetBurst after use.
	 *
	 * @param packet      the packet to encapsulate
	 * @param context_id  the context ID in which the IP packet was encapsulated
	 * @param time        the time before the context identified by context_id
	 *                    expires
	 * @return            a list of packets
	 */
	virtual NetBurst * encapsulate(NetPacket *packet,
	                               int &context_id,
	                               long &time) = 0;

	/**
	 * Desencapsulate a packet into one or several packets. It's the caller
	 * charge to delete the returned NetBurst after use.
	 *
	 * @param packet  the encapsulation packet to desencapsulate
	 * @return        a list of packets
	 */
	virtual NetBurst * desencapsulate(NetPacket *packet) = 0;

	/**
	 * Get the type of encapsulation / desencapsulation context (ATM, MPEG, etc.)
	 *
	 * @return the type of encapsulation / desencapsulation context
	 */
	virtual std::string type() = 0;

	/**
	 * Flush the encapsulation context identified by context_id (after a context
	 * expiration for example). It's the caller charge to delete the returned
	 * NetBurst after use.
	 *
	 * @param context_id  the context to flush
	 * @return            a list of encapsulation packets
	 */
	virtual NetBurst * flush(int context_id) = 0;

	/**
	 * Flush all the encapsulation contexts. It's the caller charge to delete
	 * the returned NetBurst after use.
	 *
	 * @return  a list of encapsulation packets
	 */
	virtual NetBurst * flushAll() = 0;
};

#endif
