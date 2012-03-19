/**
 * @file MpegCtx.h
 * @brief MPEG2-TS encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_CTX_H
#define MPEG_CTX_H

#include <map>
#include <string>

#include <EncapCtx.h>
#include <MpegEncapCtx.h>
#include <MpegDesencapCtx.h>
#include <NetPacket.h>
#include <MpegPacket.h>
#include <NetBurst.h>

#ifndef MIN
#define MIN(a,b) (((a) >= (b)) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b) (((a) >= (b)) ? (a) : (b))
#endif


/**
 * @class MpegCtx
 * @brief MPEG2-TS encapsulation / desencapsulation context
 */
class MpegCtx: public EncapCtx
{
 private:

	/// Encapsulation contexts. Contexts are identified by an unique
	/// identifier (= PID)
	std::map < int, MpegEncapCtx * > encap_contexts;

	/// Desencapsulation contexts. Contexts are identified by an unique
	/// identifier (= PID)
	std::map < int, MpegDesencapCtx * > desencap_contexts;

	/// The packing minimal length for encapsulation. Packing minimal length is
	/// the minimal length (in bytes) needed to pack additional SNDU packets in
	/// an incomplete MPEG packet
	unsigned int packing_min_len;

	/// The packing threshold for encapsulation. Packing Threshold is the time
	/// the context can wait for additional SNDU packets to fill the incomplete
	/// MPEG packet before sending the MPEG packet with padding.
	unsigned long packing_threshold;

	/// The callback function to get the length of the SNDU
	unsigned int (*sndu_length)(Data *data, unsigned int offset);

	/// The callback funcion to create the adequate network packet
	NetPacket *(*create_sndu)(Data data);

 public:

	/**
	 * Build a MPEG2-TS encapsulation / desencapsulation context
	 *
	 * @param packing_min_len   The packing minimal length, ie. the minimal
	 *                          length (in bytes) needed to pack additional SNDU
	 *                          in an incomplete MPEG packet
	 * @param packing_threshold The Packing Threshold, ie. the maximum time (in
	 *                          milliseconds) to wait before sending an
	 *                          incomplete MPEG packet
	 * @param sndu_length       The callback to get the length of the SNDU
	 * @param create_sndu       The callback to create the adequate network
	 *                          packet for SNDU
	 */
	MpegCtx(unsigned int packing_min_len,
	        unsigned long packing_threshold,
	        unsigned int (*sndu_length)(Data *data, unsigned int offset),
	        NetPacket * (*create_sndu)(Data data));

	/**
	 * Destroy the MPEG2-TS encapsulation / desencapsulation context
	 */
	~MpegCtx();

	NetBurst *encapsulate(NetPacket *packet,
	                      int &context_id,
	                      long &time);
	NetBurst *desencapsulate(NetPacket *packet);

	std::string type();

	NetBurst *flush(int context_id);

	NetBurst *flushAll();

 private:

	/**
	 * Find the encapsulation context identified by the given PID
	 *
	 * @param pid  The PID to search for
	 * @return     The encapsulation context if successful, NULL otherwise
	 */
	MpegEncapCtx *find_encap_context(uint16_t pid);
};

#endif

