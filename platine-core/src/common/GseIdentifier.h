/**
 * @file GseIdentifier.h
 * @brief GSE identifier (unique index given by the association of
 *        the Tal Id and Mac Id and QoS of the packets)
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#ifndef GSE_IDENT_H
#define GSE_IDENT_H

#include <stdint.h>


/**
 * @class GseIdentifier
 * @brief GSE identifier (unique index given by the association of
 *        the Tal Id and Mac Id and QoS of the packets)
 */
class GseIdentifier
{
 private:

	/// The Tal Id value
	long _tal_id;
	/// the Mac Id value
	unsigned long _mac_id;
	/// the QoS value
	int _qos;

 public:

	/**
	 * Build an GSE identifier
	 *
	 * @param tal_id the Tal Id
	 * @param mac_id the Mac Id
	 * @param qos    the Qos
	 */
	GseIdentifier(long tal_id, unsigned long mac_id, int qos);

	/**
	 * Destroy the GSE identifier
	 */
	~GseIdentifier();

	/**
	 * Get the Tal Id
	 *
	 * @return the Tal Id
	 */
	long talId();

	/**
	 * Get the Mac Id
	 *
	 * @return the Mac Id
	 */
	unsigned long macId();

	/**
	 * Get the QoS
	 *
	 * @return the QoS
	 */
	int qos();
};

/**
 * @brief Operator to compare two GSE identifiers
 */
struct ltGseIdentifier
{
	/**
	 * Operator to test if one GSE identifier is lesser than another
	 * GSE identifier
	 *
	 * @param ai1 the first GSE identifier
	 * @param ai2 the second GSE identifier
	 * @return true if first GSE identifier is lesser than the second,
	 *         false otherwise
	 */
	bool operator() (GseIdentifier * ai1, GseIdentifier * ai2) const
	{
		if(ai1->talId() == ai2->talId())
			return (ai1->macId() < ai2->macId());
		else
			return (ai1->talId() < ai2->talId());
	}
};

#endif
