/**
 * @file lib_dama_agent.h
 * @brief This library defines DAMA agent interfaces
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Eddy Fromentin
 */

#ifndef LIB_DAMA_AGENT_H
#define LIB_DAMA_AGENT_H

#include "lib_dama_utils.h"
#include "dvb_fifo.h"
#include "NetBurst.h"
#include "AtmCell.h"
#include "Aal5Packet.h"
#include "MpegPacket.h"
#include "DvbFrame.h"


/// DAMA agent statistics context
typedef struct _DAStatContext
{
	int rbdcRequest; ///< RBDC request sent at this frame (in kbits/s)
	int vbdcRequest; ///< VBDC request sent at this frame (in cell nb)
	int craAlloc;    ///< fixed bandwith allocated in kbits/s
	int globalAlloc; ///< global bandwith allocated in kbits/s
	int unusedAlloc; ///< unused bandwith in kbits/s
} DAStatContext;

/**
 * @class DvbRcsDamaAgent
 * @brief Define methods to Manage DAMA requests and Ul scheduling in the ST,
 *        should be inherited
 *
 * This class is used as a common central point for implementing a set of DAMA
 * It can be used (instanciated) directly but it is not the recommended usage.
 * Other classes exists in the SatIP6 project that each implements a separated
 * DAMA.
 */
class DvbRcsDamaAgent
{
 protected:

	unsigned char m_groupId;  ///< Group ID of the ST that uses the DAMA agent
	unsigned short m_talId;   ///< Logon ID (see T_DVB_LOGON_RESP in
	                          ///< lib_dvb_rcs.h)
	long m_CRCarrierId;       ///< Carrier Id where CR goes
	long m_currentSuperFrame; ///< Current Superframe Number
	double m_frameDuration;   ///< frame duration in msec
	dvb_fifo *m_nrt_fifo;     ///< The NRT fifo being under authorization
	                          ///< management
	long m_next_allocated;    ///< The number of cells to be allocated for the
	                          ///< next sf
	long m_NRTMaxBandwidth;   ///< Is the NRT maximum bandwidth avail
	DAStatContext m_statContext; ///< stats context
	DU_Converter *m_converter;      ///< Used to convert from/to KB from/to ATM cells

 public:

	// Ctor & Dtor
	DvbRcsDamaAgent();
	virtual ~DvbRcsDamaAgent();

	// Initializations
	virtual int init(dvb_fifo * nrt_fifo,
	                 long max_bandwidth,
	                 long carrier_id);
	virtual int initComplete(dvb_fifo *dvb_fifos,
	                         int dvb_fifos_number,
	                         double frameDuration,
	                         int craBw,
	                         int obrPeriod) = 0;

	// Protocol frames processing
	virtual int hereIsLogonResp(unsigned char *buf, long len);
	virtual int hereIsSOF(unsigned char *buf, long len); // at each superframe
	virtual int processOnFrameTick() = 0;
	virtual int hereIsTBTP(unsigned char *buf, long len);

	// Result in CR Computation
	virtual int buildCR(dvb_fifo *dvb_fifo,
	                    int dvb_fifos_number,
	                    unsigned char *frame,
	                    long length) = 0;

	// scheduling
	virtual int globalSchedule(dvb_fifo *dvb_fifos,
	                           int dvb_fifos_number,
	                           int &remainingAlloc,
	                           int encap_packet_type,
	                           std::list<DvbFrame *> *complete_dvb_frames) = 0;

	// stat
	DAStatContext *getStatsCxt();
	void resetStatsCxt();
};

#endif
