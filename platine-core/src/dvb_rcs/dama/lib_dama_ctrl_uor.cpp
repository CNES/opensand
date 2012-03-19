/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under
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
 * @file lib_dama_ctrl_uor.cpp
 * @brief This library defines UoR DAMA controller
 * @author Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */


#include <string>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <stdio.h>
#include "lib_dvb_rcs.h"
#include "lib_dama_ctrl_uor.h"


#define DBG_PACKAGE PKG_DAMA_DC
#include "platine_conf/uti_debug.h"

#define DC_DBG_PREFIX "[UOR]"
#define ATM_CELL 53.0

using namespace std;



/**
 * Constructor
 */
DvbRcsDamaCtrlUoR::DvbRcsDamaCtrlUoR(): DvbRcsDamaCtrl()
{
}


/**
 * Destructor
 */
DvbRcsDamaCtrlUoR::~DvbRcsDamaCtrlUoR()
{
}


/**
 * Initializes internal data structure according to configuration file
 *
 * @return  0 if ok -1 otherwise
 */
int DvbRcsDamaCtrlUoR::init(long carrier_id,
                            int frame_duration,
                            int allocation_cycle,
                            int packet_length,
                            DraSchemeDefinitionTable *dra_scheme_def_table)
{
	dra_def_table_pos_t dra_def_pos;
	DraSchemeDefinition *dra_scheme_def;
	double minBdra = INT_MAX;
	int ret;

	// call the default DAMA controler init
	UTI_INFO("init default part of DAMA UoR...\n");
	ret = DvbRcsDamaCtrl::init(carrier_id, frame_duration, allocation_cycle,
	                           packet_length, dra_scheme_def_table);
	if(ret != 0)
	{
		UTI_ERROR("failed to init the default part of the UoR DAMA "
		          "controler\n");
		goto error;
	}

	// now, init some UoR specific parameters
	UTI_INFO("init specific part of DAMA UoR...\n");

	// retrieve the number of TS associated for each DRA
	// and the relative bandwidth in MHz
	if(this->dra_scheme_def_table != NULL)
	{
		UTI_INFO("retrieve DRA info...\n");
		dra_def_pos = this->dra_scheme_def_table->begin();
		while((dra_scheme_def = this->dra_scheme_def_table->next(dra_def_pos)) != NULL)
		{
			DRA_UoR_OPTVALUE dra_opt_value;
			double bDra;
			int draTs;

			// bDra [Mhz]
			// TODO remove ATM_CELL from here !!
			bDra = dra_scheme_def->getSymbolRate() * 1.5 * 0.001;
			draTs = (int) (dra_scheme_def->getBitRate() * frame_duration *
			               0.001 / ATM_CELL / 8 * 0.82025 * 1000);

			UTI_DEBUG("DRA_Id := %d, draTS := %d, bDra := %f\n",
			          dra_scheme_def->getId(), draTs, bDra);

			dra_opt_value.bDRA = bDra;
			dra_opt_value.draTS = draTs;
			this->draOptionScheme[dra_scheme_def->getId()] = dra_opt_value;

			if(bDra < minBdra)
			{
				minBdra = bDra;
			}
		}
		UTI_INFO("DRA info successfully retrieved\n");
	}

	UTI_INFO("retrieve total bandwidth...\n");
	this->bandwidth_total = RetrieveTotBandwidth();
	UTI_INFO("total bandwidth successfully retrieved: %lf\n",
	         this->bandwidth_total);

	// determine the number of possible carriers
	if(minBdra == 0)
	{
		UTI_ERROR("minimum bDRA should not be 0\n");
		goto error;
	}
	this->numCarrier = (int) (this->bandwidth_total / minBdra);

	UTI_INFO("DAMA UoR controler successfully initialized\n");
	return 0;

error:
	UTI_ERROR("failed to init DAMA UoR controler\n");
	return -1;
}



/***
 * Maximum bandwidth [Mhz] based on the m_total_capacity [cells*frame]
 * realtive to the best DRA situation
 * @return bandwidth [Mhz];
 */
double DvbRcsDamaCtrlUoR::RetrieveTotBandwidth()
{
	const char *FUNCNAME = DC_DBG_PREFIX "[RetrieveTotBandwidth]";
	UTI_DEBUG("%s Retrieve Bandwidth.....",FUNCNAME);

	double Btot = 0; // [Mhz]
	int maxCapacity = m_total_capacity; // [Cells x frame]

	while(maxCapacity >= 0)
	{
		DRA_UoR_OPTVALUE* dra = getMaxDraIncluded(maxCapacity);

		if(dra==NULL)
		{
			break;
		}
		else
		{
			maxCapacity = maxCapacity - dra->draTS;
			Btot = Btot + dra->bDRA;
		}
	}

	return Btot;
}

/***
 * Auxiliar function used by the RetrieveTotBandwidth method;
 * @param residue: the available bandwidth [Mhz]
 * @return the best DRA_UoR_OPTVALUE included in the residue bw
 */
DRA_UoR_OPTVALUE* DvbRcsDamaCtrlUoR::getMaxDraIncluded(int residue)
{
	DRA_UoR_OPTVALUE* dra_result = NULL;
	DRA_UoR_OPTVALUE* dra;
	dra_opt_map::iterator draIt;
	int maxTs = 0;

	for(draIt = draOptionScheme.begin(); draIt!= draOptionScheme.end();draIt++)
	{
		dra = &draIt->second;

		if((dra->draTS > maxTs) && (residue - dra->draTS >= 0))
		{
			maxTs = dra->draTS;
			dra_result = dra;
		}
	}

	return dra_result;
}


void DvbRcsDamaCtrlUoR::InitArray(int* array,int dim)
{
	for(int i = 0;i<dim; i++)
	{
		array[i] = 0;
	}
}




 void DvbRcsDamaCtrlUoR::InitArray(double* array,int dim)
{
	for(int i = 0;i<dim; i++)
	{
		array[i] = 0.0;
	}
}


/**
 * @brief Init TBTB matrix to zero
 */
bool DvbRcsDamaCtrlUoR::InitTBTP()
{
	for(int i = 0; i < numCarrier;i++)
	{
		matrixTbtp[i] = new int[numST];
		if(matrixTbtp[i] == NULL)
		{
			return false;
		}

		for(int j = 0; j < numST;j++)
		{
			matrixTbtp[i][j] = 0;
		}
	}

	return true;
}


/**
 * @brief Init the map of the number of TS to be allocated bases on type param
 *
 * @param tsAll   the remaining capacity in slot per frame to be alloccated
 * @param type    DVB_CR_TYPE_RBDC, DVB_CR_TYPE_VBDC or DVB_CR_TYPE_FCA
 */
void DvbRcsDamaCtrlUoR::InitTsAll(map_tsAllocation& tsAll,int type)
{
	DC_Context::iterator st;
	DC_St *ThisSt;
	int i = 0;
	//UTI_DEBUG("Allocation Request....");

	for(st = m_context.begin(); st != m_context.end(); st++)
	{
		ThisSt = st->second;

		switch(type)
		{
			case DVB_CR_TYPE_RBDC :
				stationID[i] = st->first;
				i++;
				tsAll[st->first] = ThisSt->GetRbdc();
				//UTI_DEBUG("ST:=%d - RbdcReq:=%d - DraReq:=%d",st->first,ThisSt->GetRbdc(),ThisSt->GetDRASchemeID());
				break;

			case DVB_CR_TYPE_VBDC:
				tsAll[st->first] = ThisSt->GetVbdc();
				break;

			case DVB_CR_TYPE_FCA:
				tsAll[st->first] = m_fca;
				break;

		};
	}
}



/**
 * Count the max number of TS to be allocated
 * @param tsAll map (station, TS) to be allocated
 * @return the max number of TS to be allocated
 */
int maxTSAll(map_tsAllocation& tsAll)
{
	int maxValue = -1;
	map_tsAllocation::iterator it;

	for(it = tsAll.begin(); it != tsAll.end(); it++)
	{
		if(tsAll[it->first] >= maxValue)
		{
			maxValue = tsAll[it->first];
		}
	}

	return maxValue;
}


/**
 * @brief Return the nunber of TS allocated to a carrier
 *
 * @param tbtp     matrix of TS allocated
 * @param carrier  index of the carrier
 * @param numST    number of logged stations
 * @return         TS allocated
 */
int sumTSofCarrier(int** tbtp,int carrier, int numST)
{
	int sumTS = 0;

	for(int j = 0; j < numST; j ++)
	{
		sumTS+=tbtp[carrier][j];
	}

	return sumTS;
}



int DvbRcsDamaCtrlUoR::getStIndex(int stId)
{
	int stIndex = 0;
	for(int i = 0;i < numST; i++)
	{
		if(stationID[i] == stId)
		{
			stIndex = i;
		}
	}

	return stIndex;
}


/**
 * Perform the allocation of a TS on for a station
 *
 * @param tsAll  reference to the number of TS must be allocated
 * @param st     station context
 * @param type   type of allocation (rbdc, vbdc or fca)
 */
void DvbRcsDamaCtrlUoR::tsAllocation(map_tsAllocation& tsAll,
                                     DC_Context::iterator st,
                                     int type)
{
	bool found = false;
	int s;
	DC_St *ThisSt;
	int d;
	DRA_UoR_OPTVALUE dra_opt;
	int draTS;

	s = getStIndex(st->first);
	ThisSt = st->second;
	d = ThisSt->GetDRASchemeID();
	dra_opt = draOptionScheme.find(d)->second;
	draTS = dra_opt.draTS;

	//check the timeline constrain
	if((timeline[s] + ((double)frame_duration/draTS)) <= ((double)(frame_duration * 1.001)))
	{
		int c = 0;

		while(c < numCarrier)
		{
			// carrier-DRA association constrain, yes try to allocate TS
			if(carrierDRA[c] == d)
			{
				int sumTS = sumTSofCarrier(matrixTbtp,c, m_context.size());

				if(sumTS < draTS)
				{
					matrixTbtp[c][s]++;
					ThisSt->SetAllocation(1,type);
					tsAll[st->first]--;
					timeline[s]+=(double)frame_duration/draTS;
					c = numCarrier;
					found = true;
				}
			}
			c++;
		}

		if(found == false)
		{
			//carrier-DRA association not found
			double draB = dra_opt.bDRA;

			// Bandwidth constrain
			if(bAll + draB <= this->bandwidth_total)
			{
				c = 0;

				while(c<numCarrier)
				{
					//if carrierDRA[c] is empty, set DRA
					if(carrierDRA[c] == 0)
					{
						carrierDRA[c] = d;
						bAll += draB;
						matrixTbtp[c][s]++;
						ThisSt->SetAllocation(1,type);
						tsAll[st->first]--;
						timeline[s]+=(double)frame_duration/draTS;
						c = numCarrier;
						found = true;
					}
					c++;
				}
			}
		}
	}

	if(found ==  false)
	{
		tsAll[st->first]= -1;
	}
}


/**
 * This function run the Dama
 * We use internal ST contexts for doing that. After DAMA has been run
 * TBTP must have been completed and context must have been reinitialized
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaCtrlUoR::runDama()
{
	map_tsAllocation tsAll;
	int retval = -1;

	UTI_DEBUG("UoR DAMA algorithm called...\n");

	this->bAll = 0.0;
	this->numST = m_nb_st;

	// create TBTP matrix
	matrixTbtp = new int*[numCarrier];
	if(matrixTbtp == NULL)
	{
		UTI_ERROR("failed to create TBTP matrix\n");
		goto error;
	}

	// fill the TBTP matrix in
	if(this->InitTBTP() != true)
	{
		UTI_ERROR("failed to fill TBTP matrix in\n");
		goto free_tbtp_matrix;
	}

	// create the carrier DRA array
	carrierDRA = new int[numCarrier];
	if(carrierDRA == NULL)
	{
		UTI_ERROR("failed to create carrier DRA array\n");
		goto empty_tbtp_matrix;
	}
	this->InitArray(carrierDRA,numCarrier);

	// create the timeline array
	timeline = new double[numST];
	if(timeline == NULL)
	{
		UTI_ERROR("failed to create timeline array\n");
		goto free_carrierDRA;
	}
	this->InitArray(timeline,numST);

	// create the station ID array
	stationID = new int[numST];
	if(stationID == NULL)
	{
		UTI_ERROR("failed to create station ID array\n");
		goto free_timeline;
	}
	this->InitArray(stationID, numST);

	this->runDamaRbdc(tsAll);
	this->runDamaVbdc(tsAll);
	this->runDamaFca(tsAll);

	PrintDynamicAssignment();

	UTI_INFO("DAMA algorithm successfully run\n");
	retval = 0;

	delete [] stationID;
free_timeline:
	delete [] timeline;
free_carrierDRA:
	delete [] carrierDRA;
empty_tbtp_matrix:
	for(int i = 0; i < numCarrier; i++)
	{
		delete [] matrixTbtp[i];
	}
free_tbtp_matrix:
	delete [] matrixTbtp;
error:
	UTI_ERROR("failed to run DAMA algorithm\n");
	return retval;
}


/**
 * Perform the RBDC allocation
 * @param tsAll is the remain capacity in slot per frame to be alloccated
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaCtrlUoR::runDamaRbdc(map_tsAllocation& tsAll)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDamaRbdc]";

	InitTsAll(tsAll,DVB_CR_TYPE_RBDC);
	DC_Context::iterator st = m_context.begin();

	if(maxTSAll(tsAll) > 0)
	{
		UTI_DEBUG("%s: Perform UoR Rbdc Allocation...\n", FUNCNAME);

		while(maxTSAll(tsAll) > 0)
		{
			if(tsAll[st->first] > 0)
			{
				tsAllocation(tsAll,st,DVB_CR_TYPE_RBDC);
			}
			st++;

			if(st == m_context.end())
			{
				st = m_context.begin();
			}
		}
	}

	return 0;
}

/**
 * Perform the VBDC allocation
 * @param tsAll is the remain capacity in slot per frame to be alloccated
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaCtrlUoR::runDamaVbdc(map_tsAllocation& tsAll)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDamaVbdc]";

	InitTsAll(tsAll,DVB_CR_TYPE_VBDC);
	DC_Context::iterator st = m_context.begin();

	if(maxTSAll(tsAll) > 0)
	{
		UTI_DEBUG("%s: Perform UoR Vbdc Allocation...\n",FUNCNAME);

		while(maxTSAll(tsAll) > 0)
		{
			if(tsAll[st->first] > 0)
			{
				tsAllocation(tsAll,st,DVB_CR_TYPE_RBDC);
			}
			st++;

			if(st == m_context.end())
			{
				st = m_context.begin();
			}
		}
	}

	return 0;
}


/**
 * Perform the FCA allocation
 * @param tsAll is the remain capacity in slot per frame to be alloccated
 * @return 0 on success, -1 otherwise
 */
int DvbRcsDamaCtrlUoR::runDamaFca(map_tsAllocation& tsAll)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[runDamaFca]";

	if(m_fca >0)
	{
		UTI_INFO("%s: Perform UoR Fca Allocation...\n", FUNCNAME);
		InitTsAll(tsAll,DVB_CR_TYPE_FCA);
		DC_Context::iterator st = m_context.begin();

		while(maxTSAll(tsAll) > -1)
		{
			if(tsAll[st->first] > -1)
			{
				tsAllocation(tsAll,st,DVB_CR_TYPE_FCA);
			}
			st++;

			if(st == m_context.end())
			{
				st = m_context.begin();
			}
		}
	}

	return 0;
}




/**
 * @brief Write the log into platine.log to debug the matrix frame composition
 */
void DvbRcsDamaCtrlUoR::PrintDynamicAssignment()
{
	const char *FUNCNAME = DC_DBG_PREFIX "[PrintDynamicAssignment]";
	UTI_DEBUG_L3("%s TBTP \n",FUNCNAME);

	for(int i = 0;i< numCarrier; i++)
	{
		UTI_DEBUG_L3("%s carrier %d --> DRA:=%d",DC_DBG_PREFIX,i+1,carrierDRA[i]);
		for(int j = 0; j < numST; j++)
		{
			UTI_DEBUG_L3("%s st%d:=%d",DC_DBG_PREFIX,stationID[j],matrixTbtp[i][j]);
		}
	}
}
