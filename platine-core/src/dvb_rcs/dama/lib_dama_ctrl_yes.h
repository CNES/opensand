/**
 *  @file lib_dama_ctrl_yes.h
 *  @brief header file for the YES dama controller
 *  This library defines a DAMA controller that allocated every request.
 *
 *  @author ASP - IUSO, DTP (B. BAUDOIN)
 *  @version $Id: lib_dama_ctrl_yes.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $
 *  $Log: lib_dama_ctrl_yes.h,v $
 *  Revision 1.1.1.1  2006/08/02 11:50:28  dbarvaux
 *  PLATINE satellite testbed
 *
 *  Revision 1.1.1.1  2005/03/01 09:44:34  root
 *  création initiale du projet
 *
 */
#ifndef LIB_DAMA_CTRL_YES_H
#   define LIB_DAMA_CTRL_YES_H

#   include "lib_dama_ctrl.h"

class DvbRcsDamaCtrlYes:public DvbRcsDamaCtrl
{
 private:

	virtual int runDama();

 public:

	DvbRcsDamaCtrlYes();
	virtual ~ DvbRcsDamaCtrlYes();
};
#endif
