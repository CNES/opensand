/**
 * @file bloc_sat_carrier.h
 * @brief This bloc implements a satellite carrier emulation
 * @author AQL (ame)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef BLOC_SAT_CARRIER_H
#define BLOC_SAT_CARRIER_H

#include "platine_margouilla/mgl_bloc.h"
#include "sat_carrier_channel_set.h"


/**
 * @class BlocSatCarrier
 * @brief This bloc implements a satellite carrier emulation
 */
class BlocSatCarrier: public mgl_bloc
{
 public:

	/// Use mgl_bloc default constructor
	BlocSatCarrier(mgl_blocmgr *blocmgr, mgl_id fatherid, const char *name);

	~BlocSatCarrier();

	// Event handlers
	mgl_status onEvent(mgl_event *event);

 protected:

	/// List of channels
	sat_carrier_channel_set m_channelSet;

 private:

	/// Whether the bloc has been initialized or not
	bool initOk;
	// Internal event handlers
	int onInit();
	void onReceivePktFromCarrier(unsigned int i_channel,
	                             unsigned char *ip_buf,
	                             unsigned int i_len);
};

#endif
