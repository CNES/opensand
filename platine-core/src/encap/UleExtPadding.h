/**
 * @file UleExtPadding.h
 * @brief Optional Padding ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_EXT_PADDING_H
#define ULE_EXT_PADDING_H

#include <UleExt.h>


/**
 * @class UleExtPadding
 * @brief Optional Padding ULE extension
 */
class UleExtPadding: public UleExt
{
 public:

	/**
	 * Build a Padding ULE extension
	 */
	UleExtPadding();

	/**
	 * Destroy the Padding ULE extension
	 */
	~UleExtPadding();

	// implementation of virtual functions
	ule_ext_status build(uint16_t ptype, Data payload);
	ule_ext_status decode(uint8_t hlen, Data payload);
};

#endif

