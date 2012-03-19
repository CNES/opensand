/**
 * @file DvbRcsFrame.h
 * @brief DVB-RCS frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef DVB_RCS_FRAME_H
#define DVB_RCS_FRAME_H

#include <DvbFrame.h>


/**
 * @class DvbRcsFrame
 * @brief DVB-RCS frame
 */
class DvbRcsFrame: public DvbFrame
{
 public:

	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data    raw data from which a DVB-RCS frame can be created
	 * @param length  length of raw data
	 */
	DvbRcsFrame(unsigned char *data, unsigned int length);

	/**
	 * Build a DVB-RCS frame
	 *
	 * @param data  raw data from which a DVB-RCS frame can be created
	 */
	DvbRcsFrame(Data data);

	/**
	 * Duplicate a DVB-RCS frame
	 *
	 * @param frame  the DVB-RCS frame to duplicate
	 */
	DvbRcsFrame(DvbRcsFrame *frame);

	/**
	 * Build an empty DVB-RCS frame
	 */
	DvbRcsFrame();

	/**
	 * Destroy the DVB-RCS frame
	 */
	~DvbRcsFrame();

	// implementation of virtual functions
	uint16_t payloadLength();
	Data payload();
	bool addPacket(NetPacket *packet);
	void empty(void);
	void setEncapPacketType(int type);
	t_pkt_type getEncapPacketType();

};

#endif

