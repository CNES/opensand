/*
 *
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

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
#include "DamaUtils.h"


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
