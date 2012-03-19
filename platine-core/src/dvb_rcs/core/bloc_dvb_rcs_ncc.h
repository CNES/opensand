/**
 * @file bloc_dvb_rcs_ncc.h
 * @brief This bloc implements a DVB-S/RCS stack for a Ncc.
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
 *   |  DVB-RCS Ncc     |
 *   |  Dama Controler  |
 *   |                  |
 *    ------------------
 *            ^
 *            | DVB Frame / BBFrame
 *            v
 *
 * </pre>
 *
 */

#ifndef BLOC_DVB_RCS_NCC_H
#define BLOC_DVB_RCS_NCC_H

#include "bloc_dvb.h"
#include "lib_dama_ctrl.h"
#include "NccPepInterface.h"


class BlocDVBRcsNcc: public BlocDvb, NccPepInterface
{

 private:

	/// is true if the bloc is correctly initialized
	bool init_ok;

	/// The DAMA controller
	DvbRcsDamaCtrl *m_pDamaCtrl;

	/// carrier ids
	long m_carrierIdDvbCtrl;
	long m_carrierIdSOF;
	long m_carrierIdData;


	/* superframes and frames */

	/// the current super frame number
	long super_frame_counter;
	/// the current frame number inside the current super frame
	int frame_counter;
	/// the number of frames per superframe
	int frames_per_superframe;

	/// frame timer, used to awake the block every frame period
	mgl_timer m_frameTimer;
	/// the frame duration
	int frameDuration;


	/* DVB-RCS/S2 emulation */

	/// ST unique mac id (configuration param)
	int macId;

	/// Output encapsulation scheme
	std::string out_encap_scheme;

	/// timer used to awake the block every second in order to retrieve
	/// the current MODCODs and DRA schemes
	mgl_timer scenario_timer;
	/// how often do we refresh the adaptive physical layer scenario (in ms)
	int dvb_scenario_refresh;

	/// the list of complete DVB-RCS/BB frames that were not sent yet
	std::list<DvbFrame *> complete_dvb_frames;


	/**** Fifo parameters ****/

	/// a fifo to keep the received packet from encap bloc
	dvb_fifo data_dvb_fifo;


	/**** NGN network / Policy Enforcement Point (PEP) ****/

	/// timer used for applying resources allocations received from PEP
	mgl_timer pep_cmd_apply_timer;

	/// Delay for allocation requests from PEP (in ms)
	int pepAllocDelay;


 public:

	/// Class constructor
	/// Use mgl_bloc default constructor
	BlocDVBRcsNcc(mgl_blocmgr * ip_blocmgr, mgl_id i_fatherid, const char *ip_name);

	~BlocDVBRcsNcc();

	/// event handlers
	mgl_status onEvent(mgl_event * event);

	/// a map of bbframes to manage different bbframes
	std::map<int,T_DVB_BBFRAME *> * m_bbframe;
	/// number of the next BBFrame
	int nb_sequencing;


	/* Methods */

	/// Get frame duration
	int getFrameDuration();

 private:

	// initialization methods
	int onInit();
	int initTimers();
	int initMode();
	int initEncap();
	int initCarrierIds();
	int initFiles();
	int initSimuParams();
	int initDraFiles();
	int initDama();
	int initFifo();

	/// DVB frame from lower layer
	int onRcvDVBFrame(unsigned char *ip_buf, int l_len);
	void onRcvLogonReq(unsigned char *ip_buf, int l_len);
	void onRcvLogoffReq(unsigned char *ip_buf, int l_len);

	// NCC functions
	void sendTBTP();
	void sendSOF();

	bool getBBFRAMEDuration(unsigned int modcod_id, float *duration);

};

#endif
