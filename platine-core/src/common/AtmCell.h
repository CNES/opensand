/**
 * @file AtmCell.h
 * @brief ATM cell
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ATM_CELL_H
#define ATM_CELL_H

#include <NetPacket.h>


/**
 * @class AtmCell
 * @brief ATM cell
 */
class AtmCell: public NetPacket
{
 public:

	/**
	 * Build an ATM cell
	 * @param data raw data from which an ATM cell can be created
	 * @param length length of raw data
	 */
	AtmCell(unsigned char *data, unsigned int length);

	/**
	 * Build an ATM cell
	 * @param data raw data from which an ATM cell can be created
	 */
	AtmCell(Data data);

	/**
	 * Build an empty ATM cell
	 */
	AtmCell();

	/**
	 * Destroy the ATM cell
	 */
	~AtmCell();

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
	 * Retrieve the VPI field from the ATM header
	 * @return the VPI field from the ATM header
	 */
	uint8_t vpi();

	/**
	 * Retrieve the VCI field from the ATM header
	 * @return the VCI field from the ATM header
	 */
	uint16_t vci();

	/**
	 * Retrieve the PT field from the ATM header
	 * @return the PT field from the ATM header
	 */
	uint8_t pt();

	/**
	 * Is this ATM cell the last one of an AAL5 packet?
	 * @return true if ATM cell is the last one, false otherwise
	 */
	bool isLastCell();

	/**
	 * Set the GFC field of the ATM header
	 * @param gfc the value to set the GFC field to
	 */
	void setGfc(uint8_t gfc);

	/**
	 * Set the VPI field of the ATM header
	 * @param vpi the value to set the VPI field to
	 */
	void setVpi(uint8_t vpi);

	/**
	 * Set the VCI field of the ATM header
	 * @param vci the value to set the VCI field to
	 */
	void setVci(uint16_t vci);

	/**
	 * Set the PT field of the ATM header
	 * @param pt the value to set the PT field to
	 */
	void setPt(uint8_t pt);

	/**
	 * Set the CLP field of the ATM header
	 * @param clp the value to set the CLP field to
	 */
	void setClp(uint8_t clp);

	/**
	 * Set whether the ATM cell is last one of the AAL5 packet
	 * @param is_last_cell true if the ATM cell is the last one, false otherwise
	 */
	void setIsLastCell(bool is_last_cell);

	/**
	 * Create an ATM cell
	 * @param gfc the value for the GFC field in the ATM header
	 * @param vpi the value for the VPI field in the ATM header
	 * @param vci the value for the VCI field in the ATM header
	 * @param pt the value for the PT field in the ATM header
	 * @param clp the value for the CLP field in the ATM header
	 * @param is_last_cell whether the packet is the last cell of an AAL5
	 *                     packet or not
	 * @param payload the payload of the ATM cell to be created
	 * @return a newly created ATM cell
	 */
	static AtmCell *create(uint8_t gfc, uint8_t vpi, uint16_t vci,
	                       uint8_t pt, uint8_t clp, bool is_last_cell,
	                       Data payload);

	/**
	 * Get the length of an ATM cell (= 53 bytes)
	 * @return the ATM cell length
	 */
	static unsigned int length();

	/**
	 * Create an ATM cell
	 *
	 * @param data  raw data from which an ATM cell can be created
	 * @return      the created ATM cell
	 */
	static NetPacket * create(Data data);

	/**
	 * Get the length of a given ATM cell
	 *
	 * @param data    raw data which contains at least one ATM cell
	 * @param offset  the offset in data where the ATM cell starts
	 * @return        the length of the ATM cell
	 */
	static unsigned int length(Data *data, unsigned int offset);
};

#endif
