/**
 * @file BBFrame.h
 * @brief BB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef BB_FRAME_H
#define BB_FRAME_H

#include <DvbFrame.h>


/**
 * @class BBFrame
 * @brief BB frame
 */
class BBFrame: public DvbFrame
{

 public:

	/**
	 * Build a BB frame
	 *
	 * @param data    raw data from which a BB frame can be created
	 * @param length  length of raw data
	 */
	BBFrame(unsigned char *data, unsigned int length);

	/**
	 * Build a BB frame
	 *
	 * @param data  raw data from which a BB frame can be created
	 */
	BBFrame(Data data);

	/**
	 * Duplicate a BB frame
	 *
	 * @param frame  the BB frame to duplicate
	 */
	BBFrame(BBFrame *frame);

	/**
	 * Build an empty BB frame
	 */
	BBFrame();

	/**
	 * Destroy the BB frame
	 */
	~BBFrame();

	// implementation of virtual functions
	uint16_t payloadLength();
	Data payload();
	bool addPacket(NetPacket *packet);
	void empty(void);
	void setEncapPacketType(int type);
	t_pkt_type getEncapPacketType();

	// BB frame specific

	/**
	 * Get the MODCOD of the BB frame
	 *
	 * @return  the MODCOD ID of the frame
	 */
	unsigned int getModcodId(void);

	/**
	 * Set the MODCOD of the BB frame
	 *
	 * @param modcod_id  the MODCOD ID of the frame
	 */
	void setModcodId(unsigned int modcod_id);
};

#endif

