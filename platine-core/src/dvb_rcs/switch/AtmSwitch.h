/**
 * @file AtmSwitch.h
 * @brief ATM switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef ATM_SWITCH__H
#define ATM_SWITCH__H

#include <GenericSwitch.h>
#include <AtmIdentifier.h>
#include <NetPacket.h>
#include <AtmCell.h>
#include <platine_conf/conf.h>


/**
 * @class AtmSwitch
 * @brief ATM switch for Satellite Emulator (SE)
 */
class AtmSwitch: public GenericSwitch
{
 public:

	/**
	 * Build an ATM switch
	 */
	AtmSwitch();

	/**
	 * Destroy the ATM switch
	 */
	~AtmSwitch();

	long find(NetPacket *packet);
};

#endif
