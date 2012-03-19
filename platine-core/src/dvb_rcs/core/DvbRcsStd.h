/**
 * @file DvbRcsStd.h
 * @brief DVB-RCS Transmission Standard
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef DVB_RCS_STD_H
#define DVB_RCS_STD_H

#include "PhysicStd.h"
#include "DvbRcsFrame.h"
#include <GenericSwitch.h>


/**
 * @class DvbRcsStd
 * @brief DVB-RCS Transmission Standard
 */
class DvbRcsStd: public PhysicStd
{

 public:

	/**
	 * Build a DVB-RCS Transmission Standard
	 */
	DvbRcsStd();

	/**
	 * Destroy the DVB-RCS Transmission Standard
	 */
	~DvbRcsStd();

	int scheduleEncapPackets(dvb_fifo *fifo,
	                         long current_time,
	                         std::list<DvbFrame *> *complete_dvb_frames);

	int onRcvFrame(unsigned char *frame,
	               long length,
	               long type,
	               int mac_id,
	               NetBurst **burst);

	/* function for regenerative satellite */
	bool setSwitch(GenericSwitch *generic_switch);


 private:

	/**
	 * Switch which manages the different spots
	 * (for regenerative satellite only)
	 */
	GenericSwitch *generic_switch;


	/**
	 * @brief Create an incomplete DVB-RCS frame
	 *
	 * @param incomplete_dvb_frame OUT: the DVB-RCS frame that will be created
	 * return                      1 on success, 0 on error
	 */
	int createIncompleteDvbRcsFrame(DvbRcsFrame **incomplete_dvb_frame);

};

#endif
