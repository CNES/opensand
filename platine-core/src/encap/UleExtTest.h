/**
 * @file UleExtTest.h
 * @brief Mandatory Test SNDU ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_EXT_TEST_H
#define ULE_EXT_TEST_H

#include <UleExt.h>


/**
 * @class UleExtTest
 * @brief Mandatory Test SNDU ULE extension
 */
class UleExtTest: public UleExt
{
 public:

	/**
	 * Build an Test SNDU ULE extension
	 */
	UleExtTest();

	/**
	 * Destroy the Test SNDU ULE extension
	 */
	~UleExtTest();

	// implementation of virtual functions
	ule_ext_status build(uint16_t ptype, Data payload);
	ule_ext_status decode(uint8_t hlen, Data payload);
};

#endif

