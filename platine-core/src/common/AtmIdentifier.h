/**
 * @file AtmIdentifier.h
 * @brief ATM identifier (unique index given by the association of both
 *        the VPI and VCI fields of the ATM header)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ATM_IDENT_H
#define ATM_IDENT_H

#include <stdint.h>


/**
 * @class AtmIdentifier
 * @brief ATM identifier (unique index given by the association of both
 *        the VPI and VCI fields of the ATM header)
 */
class AtmIdentifier
{
 private:

	/// The VPI field in the ATM header
	uint8_t vpi;
	/// the VCI field in the ATM header
	uint16_t vci;

 public:

	/**
	 * Build an ATM identifier
	 *
	 * @param vpi the VPI field in the ATM header
	 * @param vci the VCI field in the ATM header
	 */
	AtmIdentifier(uint8_t vpi, uint16_t vci);

	/**
	 * Destroy the ATM identifier
	 */
	~AtmIdentifier();

	/**
	 * Get the current VPI value
	 *
	 * @return the current VPI value
	 */
	uint8_t getVpi();

	/**
	 * Set the VPI value
	 *
	 * @param vpi the new VPI value
	 */
	void setVpi(uint8_t vpi);

	/**
	 * Get the current VCI value
	 *
	 * @return the current VCI value
	 */
	uint16_t getVci();

	/**
	 * Set the VCI value
	 *
	 * @param vci the new VCI value
	 */
	void setVci(uint16_t vci);
};


/**
 * @brief Operator to compare two ATM identifiers
 */
struct ltAtmIdentifier
{
	/**
	 * Operator to test if one ATM identifier is lesser than another
	 * ATM identifier
	 *
	 * @param ai1 the first ATM identifier
	 * @param ai2 the second ATM identifier
	 * @return true if first ATM identifier is lesser than the second,
	 *         false otherwise
	 */
	bool operator() (AtmIdentifier * ai1, AtmIdentifier * ai2) const
	{
		if(ai1->getVpi() == ai2->getVpi())
			return (ai1->getVci() < ai2->getVci());
		else
			return (ai1->getVpi() < ai2->getVpi());
	}
};

#endif
