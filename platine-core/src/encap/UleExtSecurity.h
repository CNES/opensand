/**
 * @file UleExtPadding.h
 * @brief Optional Padding ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_EXT_SECURITY_H
#define ULE_EXT_SECURITY_H

#include <UleExt.h>


/**
 * @class UleExtPadding
 * @brief Optional Padding ULE extension
 */
class UleExtSecurity: public UleExt
{
 public:

	/**
	 * Build a Security ULE extension
	 */
	UleExtSecurity();

	/**
	 * Destroy the Security ULE extension
	 */
	~UleExtSecurity();

	// implementation of virtual functions
	ule_ext_status build(uint16_t ptype, Data payload);
	ule_ext_status decode(uint8_t hlen, Data payload);
	};

#endif

