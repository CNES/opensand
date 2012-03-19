/**
 * @file DvbFrame.h
 * @brief DVB frame
 * @author Didier Barvaux / Viveris Technologies
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#ifndef DVB_FRAME_H
#define DVB_FRAME_H

#include <NetPacket.h>
#include "lib_dvb_rcs.h"


/**
 * @class DvbFrame
 * @brief DVB frame
 */
class DvbFrame: public NetPacket
{

 protected:

	/** The maximum size (in bytes) of the DVB frame */
	unsigned int max_size;

	/** The number of encapsulation packets added to the DVB frame */
	unsigned int num_packets;

	/** The carrier Id */
	long carrier_id;

	/** The type of packets carried by the frame */
	t_pkt_type _packet_type;

 public:

	/**
	 * Build a DVB frame
	 *
	 * @param data    raw data from which a DVB frame can be created
	 * @param length  length of raw data
	 */
	DvbFrame(unsigned char *data, unsigned int length);

	/**
	 * Build a DVB frame
	 *
	 * @param data  raw data from which a DVB frame can be created
	 */
	DvbFrame(Data data);

	/**
	 * Duplicate a DVB frame
	 *
	 * @param frame  the DVB frame to duplicate
	 */
	DvbFrame(DvbFrame *frame);

	/**
	 * Build an empty DVB frame
	 */
	DvbFrame();

	/**
	 * Destroy the DVB frame
	 */
	virtual ~DvbFrame();


	// implementation of virtual functions
	bool isValid();
	uint16_t totalLength();
	int qos();
	void setQos(int qos);
	unsigned long macId();
	void setMacId(unsigned long macId);
	long talId();
	void setTalId(long talId);


	// functions dedicated to DVB frames

	/**
	 * Get the maximum size of the DVB frame
	 *
	 * @return  the size (in bytes) of the DVB frame
	 */
	unsigned int getMaxSize(void);

	/**
	 * Set the maximum size of the DVB frame
	 *
	 * @param size  the size (in bytes) of the DVB frame
	 */
	void setMaxSize(unsigned int size);

	/**
	 * Set the carrier ID of the DVB frame
	 *
	 * @param carrier_id  the carrier ID the frame will be sent on
	 */
	void setCarrierId(long carrier_id);

	/**
	 * How many free bytes are available in the DVB frame ?
	 *
	 * @return  the size (in bytes) of the free space in the DVB frame
	 */
	unsigned int getFreeSpace(void);

	/**
	 * Add an encapsulation packet to the DVB frame
	 *
	 * @param packet  the encapsulation packet to add to the DVB frame
	 * @return        true if the packet was added to the DVB frame,
	 *                false if an error occurred
	 */
	virtual bool addPacket(NetPacket *packet);

	/**
	 * Get the number of encapsulation packets stored in the DVB frame
	 */
	unsigned int getNumPackets(void);

	/**
	 * Empty the DVB frame
	 */
	virtual void empty(void) = 0;


	/**
	 * Set the type of encapsulation packets stored in the frame
	 *
	 * @param type  the type of encapsulation packets
	 *              (PKT_TYPE_{ATM, MPEG, GSE})
	 */
	virtual void setEncapPacketType(int type) = 0;

	/**
	 * Set the type of encapsulation packets stored in the frame
	 *
	 * @return the type of encapsulation packets
	 *         (PKT_TYPE_{ATM, MPEG, GSE})
	 */
	virtual t_pkt_type getEncapPacketType() = 0;
};

#endif

