/**
 * @file UleExt.h
 * @brief ULE extension
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef ULE_EXT_H
#define ULE_EXT_H

#include <stdint.h>

#include <Data.h>


typedef enum
{
	/// building/decoding is successful and SNDU should be further analyzed
	ULE_EXT_OK,
	/// building/decoding is successful, but SNDU should be discarded
	ULE_EXT_DISCARD,
	/// building/decoding failed, SNDU should be discarded
	ULE_EXT_ERROR,
} ule_ext_status;


/**
 * @class UleExt
 * @brief ULE extension
 */
class UleExt
{
 protected:

	/// The magic number that identify the extension
	uint8_t _type;

	/// Whether the extension is mandatory or not
	bool is_mandatory;

	/// The payload modified by the ULE extension
	Data _payload;

	/// The payload type as read in the ULE extension
	uint16_t _payloadType;

 public:

	/**
	 * Build an ULE extension
	 */
	UleExt();

	/**
	 * Destroy the ULE extension
	 */
	virtual ~UleExt();

	/**
	 * Get the magic number of the extension
	 *
	 * @return  the magic number of the extension
	 */
	uint8_t type();

	/**
	 * Whether the extension is mandatory or not
	 *
	 * @return  true if extension is mandatory, false otherwise
	 */
	bool isMandatory();

	/**
	 * Build the ULE extension
	 *
	 * @param ptype    The type of the next header/payload
	 * @param payload  The next header/payload
	 * @return         The result of the build:
	 *                  - ULE_EXT_OK if build is successful
	 *                  - ULE_EXT_DISCARD should not be used
	 *                  - ULE_EXT_ERROR if build failed
	 */
	virtual ule_ext_status build(uint16_t ptype, Data payload) = 0;

	/**
	 * Analyze the ULE extension
	 *
	 * @param hlen     The H-LEN field as described in the ULE RFC
	 * @param payload  The ULE payload that contains the extension
	 * @return         The result of the analysis:
	 *                  - ULE_EXT_OK if decoding is successful and SNDU should
	 *                    be further analyzed
	 *                  - ULE_EXT_DISCARD if decoding is successful, but SNDU
	 *                    should be discarded (Test SNDU extension for example)
	 *                  - ULE_EXT_ERROR if decoding failed, SNDU should be
	 *                    discarded
	 */
	virtual ule_ext_status decode(uint8_t hlen, Data payload) = 0;

	/**
	 * Get the payload modified by the ULE extension
	 *
	 * @return  the modified payload
	 */
	Data payload();

	/**
	 * Get the Type field of the ULE extension
	 *
	 * @return  the Type field of the ULE extension
	 */
	uint16_t payloadType();
};

#endif

