/*
 * @file lib_dama_ctrl.h
 * @brief This library defines DAMA controller interfaces.
 * @author satip6 (Eddy Fromentin)
 */

#ifndef LIB_DAMA_CTRL_H
#define LIB_DAMA_CTRL_H

#include <stdio.h>
#include <math.h>

#include <map>
#include <vector>

#include "lib_dvb_rcs.h"
#include "lib_dama_utils.h"
#include "lib_dama_ctrl_st.h"
#include "PepRequest.h"
#include "DraSchemeDefinitionTable.h"

#include "platine_conf/conf.h"


/**
 * Describe a station identificator
 */
typedef unsigned short DC_stId;

/**
 * The mapping between a station id and its context
 */
typedef std::map < DC_stId, DC_St * > DC_Context;


/**
 * @class DvbRcsDamaCtrl
 * @brief Define methods to process DAMA request in the NCC, should be inherited
 *
 * In fact this class is more an utility class for DVB-RCS stack on the NCC
 * agent (just looking at various build_*() methods should suffice to see it.
 *
 * This class is also used as a common central point for implementing a set of
 * DAMA
 *
 * It could _not_ be used (instanciated) directly.
 * Other classes exists in the SatIP6 project that each implements a separated
 * DAMA.
 */
class DvbRcsDamaCtrl
{

 protected:

	int frame_duration;       ///< frame duration
	int m_carrier_capacity;   ///< Carrier capacity
	long m_total_capacity;    ///< global capacity available for allocation
	long m_total_allocated;   ///< Nb of encap packets to be allocated for _all_
	                          ///< ST (RT + NRT flows)
	long m_total_cra;         ///< Nb of encap packets taken for RT flows for
	                          ///< _all_ ST
	DC_Context m_context;     ///< Information about loggued stations
	int m_nb_st;              ///< The number of loggued on station
	int m_fca;                ///< FCA is allocated up to this value
	long m_currentSuperFrame; ///< Current Superframe Number
	long m_allocation_cycle;  ///< allocation cycle (frame per superframe)
	long m_carrierId;         ///< Carried Id where control frames should go
	DU_Converter *Converter;  ///< Used to convert from/to KB to encap packets
	int m_rbdc_timeout;       ///< RBDC request duration validity
	int m_min_vbdc;           ///< Minimum VBDC allocation
	int m_max_rbdc;           ///< Maximum RBDC allocation
	int m_rbdc_start_ptr;     ///< Index of the last treated ST in the RBDC
	                          ///< allocation loop
	int m_vbdc_start_ptr;     ///< Index of the last treated ST in the VBDC
	                          ///< allocation loop
	int m_fca_start_ptr;      ///< Index of the last treated ST in the FCA
	                          ///< allocation loop
	int m_cra_decrease;       ///< Flag indicating if the RBDC request has to
	                          ///< be decrease of the CRA value

	// The tbtp must be built in the same time authorizations are
	// computed (for efficiency reason). The best place to do that is in
	// runDama().
	// For that purpose we have to manage a buffer of an unknown size.
	// Hence below, pointer is defined to buff associated with the TBTP being
	// computed, allocation is done with malloc (in init) free(in destructor)
	// and realloc (optionnally in runDama)
	// The same thing happens to sact when it is computed locally
	static const int m_buff_alloc = 256; ///< granularity of allocation
	unsigned char *m_tbtp;               ///< Internal TBTP pointer
	unsigned char *m_sact;               ///< Internal SACT pointer
	long m_tbtp_size;                    ///< TBTP buffer size
	long m_sact_size;                    ///< SACT buffer size

	/// DRA-Scheme table
	DraSchemeDefinitionTable *dra_scheme_def_table;


 public:

	DvbRcsDamaCtrl();
	virtual ~DvbRcsDamaCtrl();

	// Initialisation
	virtual int init(long carrier_id, int frame_duration,
	                 int allocation_cycle, int packet_length,
	                 DraSchemeDefinitionTable *dra_def_table);

	// Process DVB frames
	virtual int hereIsLogonReq(unsigned char *ip_buf, long i_len, int dra_id);
	virtual int hereIsLogoff(unsigned char *ip_buf, long i_len);
	virtual int hereIsCR(unsigned char *ip_buf, long i_len, int dra_id);
	virtual int hereIsSACT(unsigned char *ip_buf, long i_len);
	virtual int runOnSuperFrameChange(long i_frame);

	// Gives results
	virtual int buildTBTP(unsigned char *op_buf, long i_len);

	/* update the ST resources allocations according to given PEP request */
	virtual bool applyPepCommand(PepRequest *request);

	// Get the carrier id
	long getCarrierId();


 private:

	// Associated utility to reallocate internal buffers
	int bufferCheck(unsigned char **buffer, long *buffer_size, long wanted_size);

	// Other Utilities
	virtual int cleanTBTP();
	virtual T_DVB_BTP *appendBtpEntry(DC_stId st_id);
	virtual int removeBtpEntry(DC_stId st_id);

	/**
	 * This function run the Dama, it allocates exactly what have been asked
	 * We use internal SACT, TBTP and context for doing that.
	 * After DAMA computation, TBTP is completed and context is reinitialized
	 *
 	 * @return 0 on success, -1 otherwise
	 */
	virtual int runDama() = 0;


};

#endif
