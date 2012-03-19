/**
 * @file GseSwitch.h
 * @brief GSE switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef GSE_SWITCH__H
#define GSE_SWITCH__H

#include <GenericSwitch.h>
#include <NetPacket.h>
#include <GsePacket.h>
#include <platine_conf/conf.h>


/**
 * @class GseSwitch
 * @brief GSE switch for Satellite Emulator (SE)
 */
class GseSwitch: public GenericSwitch
{
 protected:

	std::map < uint8_t, long > frag_id_table;

 public:

	/**
	 * Build a GSE switch
	 */
	GseSwitch();

	/**
	 * Destroy the GSE switch
	 */
	~GseSwitch();

	long find(NetPacket *packet);
};

#endif
