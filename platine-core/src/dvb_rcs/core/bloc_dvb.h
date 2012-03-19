/**
 * @file bloc.h
 * @brief This bloc implements a DVB-S2/RCS stack.
 * @author SatIP6
 * @author Didier Barvaux / Viveris Technologies
 * @author Emmanuelle Pechereau <epechereau@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 *
 *
 * <pre>
 *
 *            ^
 *            | encap burst
 *            v
 *    ------------------
 *   |                  |
 *   |       DVB        |
 *   |       Dama       |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / BBFrame
 *            v
 *
 * </pre>
 *
 */

#ifndef BLOC_DVB_H
#define BLOC_DVB_H

#include "PhysicStd.h"
#include "platine_margouilla/mgl_bloc.h"
#include "NccPepInterface.h"

#define MODCOD_DRA_PATH "/etc/platine/modcod_dra/"

class BlocDvb: public mgl_bloc
{

 protected:

	/// emission standard (DVB-RCS or DVB-S2)
	PhysicStd *emissionStd;
	/// reception standard (DVB-RCS or DVB-S2)
	PhysicStd *receptionStd;


 public:

	/// Class constructor
	/// Use mgl_bloc default constructor
	BlocDvb(mgl_blocmgr * ip_blocmgr, mgl_id i_fatherid, const char *ip_name);

	~BlocDvb();

	/// event handlers
	virtual mgl_status onEvent(mgl_event * event) = 0;


	/* Methods */

 protected:

	// initialization method
	virtual int onInit() = 0;

	// Send a Netburst to the encap layer
	int SendNewMsgToUpperLayer(NetBurst *burst);

	// Common functions for satellite and NCC
	int initModcodFiles();

	// Send DVB bursts to sat carrier block
	int sendBursts(std::list<DvbFrame *> *complete_frames, long carrier_id);

	// Send a DVB frame to the sat carrier block
	bool sendDvbFrame(T_DVB_HDR *dvb_frame, long carrier_id);
	bool sendDvbFrame(DvbFrame *frame, long carrier_id);



};

#endif
