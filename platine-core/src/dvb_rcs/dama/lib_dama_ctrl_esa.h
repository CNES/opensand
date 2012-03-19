/**
 * @file lib_dama_ctrl_esa.h
 * @brief This library defines the esa DAMA controller.
 *
 * @author ASP - IUSO, DTP (B. BAUDOIN)
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef LIB_DAMA_CTRL_ESA_H
#define LIB_DAMA_CTRL_ESA_H

#include "lib_dama_ctrl.h"
#include "lib_dama_utils.h"


/**
 *  @class DvbRcsDamaCtrlEsa
 *  @brief This library defines the esa DAMA controller.
 */
class DvbRcsDamaCtrlEsa: public DvbRcsDamaCtrl
{

 public:

	DvbRcsDamaCtrlEsa();
	virtual ~ DvbRcsDamaCtrlEsa();


 private:
	/// the core of the class
	int runDama();

	///RBDC allocation
	int runDamaRbdc(int);
	/// VBDC allocation
	int runDamaVbdc(int);
	/// FCA allocation
	int runDamaFca(int);

	/// in charge of the round robin management
	DC_St *RoundRobin(int *);

};

#endif
