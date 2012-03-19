/**
 * @file MpegEncapCtx.h
 * @brief MPEG encapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef MPEG_ENCAP_CTX
#define MPEG_ENCAP_CTX


#include <MpegPacket.h> /* for TS_* constants */


/**
 * @class MpegEncapCtx
 * @brief MPEG encapsulation context
 */
class MpegEncapCtx
{
 protected:

	/// Internal buffer to store the MPEG-2 TS packet under build
	Data *_frame;
	/// The PID that identifies the encapsulation context
	uint16_t _pid;
	/// The Continuity Counter (CC) used in the MPEG header
	uint8_t _cc;

	/**
	 * Initialize the MPEG packet under build (Synchronisation bit, PID and
	 * Continuity Counter, etc.)
	 */
	void initFrame();

 public:

	/*
	 * Build an sencapsulation context identified with PID pid
	 *
	 * @param pid the PID that identifies the encapsulation context
	 */
	MpegEncapCtx(uint16_t pid);

	/**
	 * Destroy the encapsulation context
	 */
	~MpegEncapCtx();

	/**
	 * Clear the encapsulation context, ie. reset the MPEG packet under build
	 */
	void reset();

	/**
	 * Get the internal buffer that stores the MPEG packet under build
	 *
	 * @return the internal buffer that stores the MPEG packet under build
	 */
	Data *frame();

	/**
	 * Add data at the end of the MPEG2-TS frame
	 *
	 * @param data    the data set that contains the data to add
	 * @param offset  the offset of the data to add in the data set
	 * @param length  the length of the data to add
	 */
	void add(Data *data, unsigned int offset, unsigned int length);

	/**
	 * Get the amount of data stored in the context (in bytes)
	 *
	 * @return the amount of data (in bytes) stored in the context
	 */
	unsigned int length();

	/**
	 * Get the amount of bytes left free at the end of the MPEG2-TS frame
	 *
	 * @return the free space in the MPEG2-TS frame
	 */
	unsigned int left();

	/**
	 * Get the SYNC byte
	 *
	 * @return the Synchronization byte
	 */
	uint8_t sync();

	/**
	 * Get the PID of the encapsulation context
	 *
	 * @return the PID of the encapsulation context
	 */
	uint16_t pid();

	/**
	 * Get the Continuity Counter (CC) of the encapsulation context
	 *
	 * @return the Continuity Counter
	 */
	uint8_t cc();

	/**
	 * Whether the PUSI bit is set or not ?
	 *
	 * @return true if the PUSI bit is set, false otherwise
	 */
	bool pusi();

	/**
	 * Set the PUSI bit
	 */
	void setPusi();

	/**
	 * Add the Payload Pointer field
	 */
	void addPP();

	/**
	 * Add padding bytes at the end of the MPEG2-TS frame
	 */
	void padding();
};

#endif
