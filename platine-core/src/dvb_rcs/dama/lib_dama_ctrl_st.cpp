/**
 * @file lib_dama_ctrl_st.cpp
 * @brief This bloc implements a ST and request context used by the DAMA CTRL
 *        within the NCC.
 * @author ASP - IUSO, DTP (B. BAUDOIN)
 * @author Julien Bernard / Viveris Technologies
 *
 */

#include <math.h>
#include <string>
#include <cstdlib>

using namespace std;

#include "lib_dama_ctrl_st.h"
#include "lib_dvb_rcs.h"

#define DBG_PACKAGE PKG_DAMA_DC
#include "platine_conf/uti_debug.h"
#define DC_DBG_PREFIX "[Generic]"


/**
 * RBDC REQUEST
 */

/**
 * RBDC request constructor
 * @param RbdcMaxRbdc is Max RBDC value
 * @param RbdcTimeout RBDC timeout
 * @return
 */

DC_RbdcRequest::DC_RbdcRequest(int RbdcMaxRbdc, int RbdcTimeout)
{
	InitialRequest = 0.0;
	Request = 0;
	Credit = 0.0;
	MaxRbdc = RbdcMaxRbdc;
	Timeout = RbdcTimeout;
	Timer = 0;
}

/**
 * RBDC request destructor
 * @return
 */

DC_RbdcRequest::~DC_RbdcRequest()
{

}

/**
 * Set the RBDC request value.
 * The timer, timeout credit and initial request are initialised
 * The RBDC request are overwriten
 * @param Cr is the capacity request
 * @return 0 on success, -1 otherwise
 */

int DC_RbdcRequest::SetRequest(double Cr)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[RBDCSetRequest]";

	// limit the requets to Max RBDC
	Cr = MIN(Cr, (double) MaxRbdc);

	// save the request
	if(Cr >= 0.0)
	{
		InitialRequest = Cr;
		Request = (int) Cr;
		Credit = InitialRequest - (double) Request;
		Timer = Timeout;
		UTI_DEBUG_L3("%s new RBDC request %f credit %f timer %d.\n", FUNCNAME,
                     Cr, Credit, Timer);
		return (0);
	}
	else
	{
		UTI_ERROR("%s RBDC CR invalid (xbdc=%f). Discarded.\n", FUNCNAME, Cr);
		return (-1);
	}
}

/**
 * Set the maximum RBDC value for a request
 * @param max is the maximum value in slots per frame
 * @return 0 on success, -1 otherwise
 */

int DC_RbdcRequest::SetMax(int max)
{
	if(max > 0)
	{
		MaxRbdc = max;
		return 0;
	}
	else
		return -1;
}

/**
 * Add a credit to the request credit.
 * @param AddCredit is the credit to be added
 * @return void
 */

void DC_RbdcRequest::AddCredit(double AddCredit)
{
	Credit += AddCredit;
}

/**
 * Get the current credit
 * @return request credit
 */

double DC_RbdcRequest::GetCredit()
{
	return Credit;
}

/**
 * Get the current request
 * @return request value for the frame
 */

int DC_RbdcRequest::GetRequest()
{
	return Request;
}

/**
 * Update the request
 * the timeout is taken into account to reset the request
 * the credit are managed
 * @return void
 */

void DC_RbdcRequest::Update()
{
	if(Timer > 0)
	{
		// timeout management
		Timer--;
	}
	if(Timer > 0)
	{
		Credit += InitialRequest - (double) ((int) InitialRequest);
		if(Credit >= 1.0)
		{
			Credit -= 1.0;
			Request++;
		}
		else
		{
			Request = (int) InitialRequest;
		}

		// add the decimal part to the request
		//    Request += InitialRequest - (double)((int)InitialRequest);

		// credit management
		//    Credit = Request - (int)Request;
	}
	else
	{
		Request = 0;
		InitialRequest = 0.0;
		Credit = 0.0;
	}
	Trace();
}


/**
 * @brief Trace the request
 */
void DC_RbdcRequest::Trace()
{
	const char *FUNCNAME = DC_DBG_PREFIX "[RBDCTrace]";

	UTI_DEBUG_L3("%s RBDC Request : initial %f actual %d credit %f timer %d\n",
	             FUNCNAME, InitialRequest, Request, Credit, Timer);
}


/**
 * @brief Set the rbdc timeout value
 *
 * @param timeout  new timeout value
 */
void DC_RbdcRequest::SetTimeout(int timeout)
{
	Timeout = timeout;
}



/**
 * VBDC REQUEST
 */

/**
 * VBDC request constructor
 * @param VbdcMinVbdc is the minimum VBDC value
 * @return
 */

DC_VbdcRequest::DC_VbdcRequest(int VbdcMinVbdc)
{
	Request = 0;
	MinVbdc = VbdcMinVbdc;
}

/**
 * VBDC request destructor
 * @return
 */

DC_VbdcRequest::~DC_VbdcRequest()
{

}

/**
 * Set the VBDC request value.
 * The VBDC request are cumulated
 * @param Cr is the capacity request
 * @return 0 on succes, -1 otherwise
 */

int DC_VbdcRequest::SetRequest(int Cr)
{
	if(Cr < 0)
	{
		UTI_ERROR("VBDC CR invalid (xBDC = %d), ignored\n", Cr);
		this->Request = 0;
		return -1;
	}

	this->Request += Cr;
	UTI_DEBUG_L3("new VBDC request %d\n", Cr);

	return 0;
}

/**
 * Get the current request
 * @return request value for the frame
 */

int DC_VbdcRequest::GetRequest()
{
	return (Request);
}

/**
 * Reset the request (VBDC sum = 0)
 * @return void
 */

void DC_VbdcRequest::Reset()
{
	Request = 0;
}

/**
 * Decrease the VBDC request value.
 * @param Served is the number of served slots
 * @return remaining request
 */

int DC_VbdcRequest::Decrease(int Served)
{
	if(Request >= Served)
		Request -= Served;
	else
		Request = 0;

	return (Request);
}

/**
 * Trace the request
 * @return void
 */

void DC_VbdcRequest::Trace()
{
	const char *FUNCNAME = DC_DBG_PREFIX "[VBDCTrace]";
	UTI_DEBUG_L3("%s VBDC Request : actual %d minimum %d\n",
	             FUNCNAME, Request, MinVbdc);
}


/**
 * ST
 */

/**
 * @brief Terminal context constructor
 *
 * @param Carrier             the carrier size in slot number
 * @param Cra                 the RT allocation in slot per frame
 * @param Fca                 the FCA granularity in slot number
 * @param MaxRbdc             the Max RBDC value in slot per frame
 * @param MinVbdc             the Min VBDC value in slot number
 * @param Timeout             the RBDC timeout in superframe number
 * @param FramePerSuperFrame  the number of frame per Superframe
 * @param StBtp               the BTP pointer for the ST
 * @param dra_id              TODO
 */
DC_St::DC_St(int Carrier, int Cra, int Fca, int MaxRbdc, int MinVbdc,
             int Timeout, int FramePerSuperFrame, T_DVB_BTP * StBtp, int dra_id)
{
	CarrierSize = Carrier;
	CraAllocation = Cra;
	RbdcMaxAllocation = MaxRbdc;
	FcaAllocation = Fca;
	Allocation = 0;
	Btp = StBtp;
	Btp->assignment_count = 0;
	DRASchemeID = dra_id;
	RbdcCr = new DC_RbdcRequest(MaxRbdc, Timeout);
	VbdcCr = new DC_VbdcRequest(MinVbdc);
	AllocationCycle = FramePerSuperFrame;
}


/**
 * @brief ST context destructor
 */
DC_St::~DC_St()
{
	delete RbdcCr;
	delete VbdcCr;
}


/**
 * @brief Update the CRA value
 *
 * @param Cra   the new CRA value in slot per frame
 * @return      difference between the new and the old CRA
 */
int DC_St::SetCra(int Cra)
{
	int OldCra = CraAllocation;

	// set the new CRA value
	CraAllocation = Cra;

	// return the rt bandwidth difference
	return (Cra - OldCra);
}

int DC_St::GetCra()
{
	return (CraAllocation);
}

/**
 * Manage a incoming VBDC request
 * @param Req is the new request
 * @return 0 on succes, -1 otherwise
 */

int DC_St::SetVbdc(int Req)
{
	int Result;

	Result = VbdcCr->SetRequest(Req);

	return (Result);
}

/**
 * Retrieve the ST VBDC request
 * the return value takes count of the remaining capacity for the ST, and of the allocation cycle
 * @return Request value
 */

int DC_St::GetVbdc()
{
	int Request;

	// take into account the fact that allocation is used for each frame of the superframe
	Request = (int) ceil(VbdcCr->GetRequest() / AllocationCycle);

	// limit the request to the maximum capacity available for this ST
	Request = MIN(Request, this->GetMaxAllocation());

	return (Request);
}

/**
 * Manage a incoming RBDC request
 * @param Req is the new request
 * @return 0 on success, -1 otherwise
 */

int DC_St::SetRbdc(double Req)
{
	int Result;

	// set the request
	Result = RbdcCr->SetRequest(Req);
	return (Result);
}

/**
 * Retrieve the ST RBDC request
 * the return value takes count of the remaining capacity for the ST
 * @return Request value
 */

int DC_St::GetRbdc()
{
	int Request;

	Request = MIN(RbdcCr->GetRequest(), this->GetMaxAllocation());

	return (Request);
}

/**
 * Set the maximum value for RBDC requests
 * @param rbdcMax is the maximum RBDC value
 * @return 0 on success, -1 otherwise
 */
int DC_St::SetMaxRbdc(int rbdcMax)
{
	int result;
	result =  RbdcCr->SetMax(rbdcMax);
	if(result == 0)
		RbdcMaxAllocation = rbdcMax;
	return result;
}

int DC_St::GetRbdcMax()
{
	return RbdcMaxAllocation;
}

/**
 * Add a credit to the ST RBDC request credit
 * @param Credit is the request credit to be added
 * @return void
 */

void DC_St::AddCredit(double Credit)
{
	// add the decimal part of the
	RbdcCr->AddCredit(Credit);
}

/**
 * Get the current ST RBDC credit
 * @return request credit
 */

double DC_St::GetCredit()
{
	return (RbdcCr->GetCredit());
}

/**
 * Allocation for the ST
 * The BTP is updated accordingly
 * @param Allocation is the number of slots allocated by the DAMA
 * @param Type indicates the allocation type (RBDC, VBDC or FCA)
 * @return Total number of allocated slots for the ST
 */

int DC_St::SetAllocation(int Allocation, int Type)
{
	// modify the request
	if(Type == DVB_CR_TYPE_VBDC)
	{
		VbdcCr->Decrease(Allocation * AllocationCycle);
	}

	// update the current allocation
	Btp->assignment_count += Allocation;

	// return the total allocation
	return (Btp->assignment_count);
}

/**
 * Get the total number of allocated slots for the ST
 * @return total number of allocated slots
 */

int DC_St::GetAllocation()
{
	return (Btp->assignment_count);
}

/**
 * Terminal context update
 * this must be called each superframe
 * @return void
 */

void DC_St::Update()
{
	// Rbdc update
	RbdcCr->Update();

	// reset the allocation
	// note that the CRA value is added
	Btp->assignment_count = CraAllocation;

	Allocation = 0;
}

/**
 * Get the maximum slot number that can be allocated to a ST
 * @return Maximum allocation for a ST
 */

int DC_St::GetMaxAllocation()
{
	return (CarrierSize - Btp->assignment_count);
}

/**
 * Set the BTP pointer
 * @param NewBtp points to the new Btp structure
 * @return void
 */

void DC_St::SetBtp(T_DVB_BTP * NewBtp)
{
	Btp = NewBtp;
}

/**
 * Trace the ST context
 * @return void
 */

void DC_St::Trace()
{
	const char *FUNCNAME = DC_DBG_PREFIX "[STTrace]";

	UTI_DEBUG_L3("%s --- ST Cra %d Fca %d Allocation %d ---\n", FUNCNAME,
	             CraAllocation, FcaAllocation, Allocation);
	RbdcCr->Trace();
	VbdcCr->Trace();
	UTI_DEBUG_L3("%s -------------------------------------\n", FUNCNAME);
}


/**
 * @brief Get the DRA-Scheme ID of the terminal
 *
 * @return the ID of DRA-Scheme
 */
int DC_St::GetDRASchemeID()
{
	return DRASchemeID;
}


/**
 * @brief Set the value of the DRA-Scheme ID for the terminal
 *
 * @param new_dra  the new id of the DRA-Scheme of the terminal
 */
void DC_St::SetDRASchemeID(int new_dra)
{
	DRASchemeID = new_dra;
}


/**
 * @brief Set the rbdc timeout value
 *
 * @param timeout  new timeout value
 */
void DC_St::SetTimeout(int timeout)
{
	RbdcCr->SetTimeout(timeout);
}

