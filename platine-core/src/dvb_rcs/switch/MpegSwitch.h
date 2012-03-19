/**
 * @file MpegSwitch.h
 * @brief MPEG switch for Satellite Emulator (SE)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef MPEG_SWITCH__H
#define MPEG_SWITCH__H

#include <GenericSwitch.h>
#include <NetPacket.h>
#include <MpegPacket.h>
#include <platine_conf/conf.h>


/**
 * @class MpegSwitch
 * @brief MPEG switch for Satellite Emulator (SE)
 */
class MpegSwitch: public GenericSwitch
{
 public:

	/**
	 * Build a MPEG switch
	 */
	MpegSwitch();

	/**
	 * Destroy the MPEG switch
	 */
	~MpegSwitch();

	long find(NetPacket *packet);
};

#endif
