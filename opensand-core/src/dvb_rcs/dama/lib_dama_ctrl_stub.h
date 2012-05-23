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
 * @file lib_dama_ctrl_stub.h
 * @brief This library defines a stub DAMA controller, process nothing,
 *        do nothing.
 */

#ifndef LIB_DAMA_CTRL_STUB_H
#define LIB_DAMA_CTRL_STUB_H

#include "lib_dama_ctrl.h"


class DvbRcsDamaCtrlStub: public DvbRcsDamaCtrl
{
 private:

	virtual int runDama();

 public:

	DvbRcsDamaCtrlStub();
	virtual ~ DvbRcsDamaCtrlStub();

	virtual int init(long l_carrierId);

	virtual int hereIsLogonReq(unsigned char *ip_buf, long i_len);
	virtual int hereIsLogoff(unsigned char *ip_buf, long i_len);
	virtual int hereIsCR(unsigned char *ip_buf, long i_len);
	virtual int hereIsSACT(unsigned char *ip_buf, long i_len);
	virtual int runOnSuperFrameChange(long i_frame);

	virtual int buildTBTP(unsigned char *op_buf, long i_len);

};

#endif
