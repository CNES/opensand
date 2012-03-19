/**
 * @file RohcPacket.h
 * @brief ROHC packet
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ROHC_PACKET_H
#define ROHC_PACKET_H

#include <NetPacket.h>


/**
 * @class RohcPacket
 * @brief ROHC packet
 */
class RohcPacket: public NetPacket
{
 protected:

	/// The Quality of Service for the packet
	int _qos;
	/// The MAC identifier of the communication channel used by the packet
	unsigned long _macId;
	/// The identifier for the ST which emited this packet
	long _talId;

 public:

	/**
	 * Build a ROHC packet
	 *
	 * @param data    raw data from which a ROHC packet can be created
	 * @param length  length of raw data
	 */
	RohcPacket(unsigned char *data, unsigned int length);

	/**
	 * Build a ROHC packet
	 *
	 * @param data  raw data from which a ROHC packet can be created
	 */
	RohcPacket(Data data);

	/**
	 * Build an empty ROHC packet
	 */
	RohcPacket();

	/**
	 * Destroy the ROHC packet
	 */
	~RohcPacket();

	// implementation of virtual functions
	bool isValid();
	uint16_t totalLength();
	uint16_t payloadLength();
	Data payload();
	int qos();
	void setQos(int qos);
	unsigned long macId();
	void setMacId(unsigned long macId);
	long talId();
	void setTalId(long talId);

	/**
	 * Create a ROHC packet
	 *
	 * @param data  raw data from which a ROHC packet can be created
	 * @return      the created ROHC packet
	 */
	static NetPacket *create(Data data);

	void setType(uint16_t type);
};

#endif

