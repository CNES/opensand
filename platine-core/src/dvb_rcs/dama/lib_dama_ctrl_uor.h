/**
 * @file lib_dama_ctrl_uor.h
 * @brief This library defines the UoR DAMA controller.
 *
 * @author Erasmo Di Santo - University of Rome
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef LIB_DAMA_CTRL_UOR_H
#define LIB_DAMA_CTRL_UOR_H

#include "lib_dama_ctrl.h"
#include "lib_dama_utils.h"


typedef struct
{
	double bDRA;  /**< bandwidth of each DRA */
	int draTS;    /**< TS for each DRA */
} DRA_UoR_OPTVALUE;


// map of DRA_UoR_OPTVALUE, the key param is the draId
typedef std::map<int, DRA_UoR_OPTVALUE> dra_opt_map;

typedef std::map<int, int> map_tsAllocation;


/**
 * @class DvbRcsDamaCtrlUoR
 * @brief This library defines the UoR DAMA controller.
 */
class DvbRcsDamaCtrlUoR: public DvbRcsDamaCtrl
{
 private:

	int numCarrier;
	int numST;
	double bAll;
	double bandwidth_total;

	int* stationID;
	int* carrierDRA;
	int** matrixTbtp;
	double* timeline;

	dra_opt_map draOptionScheme;

	/*********************************/

	int runDama();  ///< the core of the class, please read comments in it
	int runDamaRbdc(map_tsAllocation&); ///< RBDC allocation
	int runDamaVbdc(map_tsAllocation&); ///< VBDC allocation
	int runDamaFca(map_tsAllocation&);  ///< FCA allocation

	bool InitTBTP();
	void InitArray(int*, int);
	void InitArray(double*, int);
	void InitTsAll(map_tsAllocation&,int);
	int getStIndex(int stId);
	void PrintDynamicAssignment();

	void tsAllocation(map_tsAllocation& ,DC_Context::iterator,int );
	double RetrieveTotBandwidth();
	DRA_UoR_OPTVALUE* getMaxDraIncluded(int residue);

 public:

	DvbRcsDamaCtrlUoR();
	~DvbRcsDamaCtrlUoR();

	int init(long carrier_id, int frame_duration,
	         int allocation_cycle, int packet_length,
	         DraSchemeDefinitionTable *dra_scheme_def_table);

};

#endif

