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
 *  @file lib_dama_ctrl_yes.h
 *  @brief header file for the YES dama controller
 *  This library defines a DAMA controller that allocated every request.
 *
 *  @author ASP - IUSO, DTP (B. BAUDOIN)
 *  @version $Id: lib_dama_ctrl_yes.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $
 *  $Log: lib_dama_ctrl_yes.h,v $
 *  Revision 1.1.1.1  2006/08/02 11:50:28  dbarvaux
 *  OPENSAND satellite testbed
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
