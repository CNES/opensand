/**
 * @file lib_dama_ctrl_st.h
 * @brief Define the CR contexts
 * @author ASP - IUSO, DTP (B. BAUDOIN)
 * @author Julien Bernard / Viveris Technologies
 *
 */

#ifndef LIB_DAMA_CTRL_ST_H
#define LIB_DAMA_CTRL_ST_H

#include <stdio.h>
#include <map>
#include "lib_dvb_rcs.h"
#include "lib_dama_utils.h"


/**
 * @class DC_RbdcRequest
 * @brief Define the RBDC request context
 *
 * Describe the context of a RBDC request
 */
class DC_RbdcRequest
{
 protected:
	double InitialRequest; ///< the received RBDC request

	int Request;           ///< integer part of the current request for this
	                       ///< frame (in time slot number)
	double Credit;         ///< decimal part of the request
	int Timeout;           ///< RBDC request duration
	int Timer;             ///< timeout for the request validity
	int MaxRbdc;           ///< Maximum allocation allowed for the RBDC

 public:

	DC_RbdcRequest(int MaxRbdc, int Timeout);
	~DC_RbdcRequest();
	int SetRequest(double Request);
	int SetMax(int max);
	int GetRequest();
	void AddCredit(double Credit);
	double GetCredit();
	void Update();
	void Trace();
	void SetTimeout(int timeout);
};


/**
 * @class DC_VbdcRequest
 * @brief Define the VBDC request context
 *
 * Describe the context of a VBDC request
 */
class DC_VbdcRequest
{
 protected:

	int Request; ///< the cumulated VBDC request
	int MinVbdc; ///< Minimum VBDC allocation
 public:

	DC_VbdcRequest(int MinVbdc);
	~DC_VbdcRequest();
	int SetRequest(int Cr);
	int GetRequest();
	void Reset();
	int Decrease(int Served);
	void Trace();
};


/**
 * @class DC_St
 * @brief Define the ST context managed by the NCC for the allocation
 *
 * Describe the context associated with a loggued st
 */
class DC_St
{
 protected:

	int CarrierSize;        ///< Carrier size in time slot number. Refers to
	                        ///< the maximum allocation for the ST
	int CraAllocation;      ///< The station RT fixed bandwidth
	int RbdcMaxAllocation;  ///< The maximum rbdc allocation for the ST
	int FcaAllocation;      ///< FCA per st
	int Allocation;         ///< Allocation during this frame
	int AllocationCycle;    ///< Allocation cycle
	T_DVB_BTP *Btp;         ///< Associated BTP entry during the current sf
	DC_RbdcRequest *RbdcCr; ///< RBDC request
	DC_VbdcRequest *VbdcCr; ///< VBDC request
	int DRASchemeID;        ///< DRA-Scheme ID

 public:

	DC_St(int CarrierCapacity, int Cra, int Fca, int MaxRbdc, int MinVbdc,
	      int Timeout, int FramePerSuperFrame, T_DVB_BTP * Btp, int dra_id);
	virtual ~DC_St();
	int SetCra(int Cra);
	int GetCra();
	int GetRbdc();
	int GetVbdc();
	int GetDRASchemeID();
	int SetRbdc(double Req);
	int SetMaxRbdc(int rbdcMax);
	int GetRbdcMax();
	int SetVbdc(int Req);
	void AddCredit(double Credit);
	double GetCredit();
//	int SetCr(T_DVB_SAC_CR_INFO *Cr);
	int SetAllocation(int Alloc, int Type);
	int GetAllocation();
	int GetMaxAllocation();
	void SetBtp(T_DVB_BTP * NewBtp);
	void SetDRASchemeID(int new_dra);
	void Update();
	void Trace();
	void SetTimeout(int timeout);
};

#endif
