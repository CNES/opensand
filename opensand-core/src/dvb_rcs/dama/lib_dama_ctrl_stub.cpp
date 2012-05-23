/*
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
 * @file lib_dama_ctrl_stub.cpp 
 * @brief This library defines a stub DAMA controller.
 * @author Viveris Technologies
 */

#include "lib_dama_ctrl_stub.h"

#define DBG_PACKAGE PKG_DAMA_DC
#include "opensand_conf/uti_debug.h"
#define DC_DBG_PREFIX "[stub]"

int DvbRcsDamaCtrlStub::runDama()
{
	return 0;
}

DvbRcsDamaCtrlStub::DvbRcsDamaCtrlStub()
{
}

DvbRcsDamaCtrlStub::~DvbRcsDamaCtrlStub()
{
}

int DvbRcsDamaCtrlStub::init(long l_carrierId)
{
	const char *FUNCNAME = DC_DBG_PREFIX "[init]";
	UTI_INFO("%s This is a pure void controller.\n", FUNCNAME);
	return 0;
}

/**
 * Process a logon Request Frame as an information from Dvb layer and fill an internal context
 * @param ip_buf points to the buffer containing logon
 * @param i_len is the length of the buffer
 * @return 0 (success)
 */
int DvbRcsDamaCtrlStub::hereIsLogonReq(unsigned char *ip_buf, long i_len)
{
	return 0;
}

/**
 * Process a logoff Frame, must update the context
 * @param ip_buf points to the buffer containing logoff
 * @param i_len is trhe length of the buffer
 * @return 0 (success)
 */
int DvbRcsDamaCtrlStub::hereIsLogoff(unsigned char *ip_buf, long i_len)
{
	return 0;
}

/**
 * When receiving a CR, fill the internal SACT table and internal TBTP table
 * Must maintain Invariant 1 true
 * @param ip_buf the pointer to CR buff to be copied
 * @param i_len the lenght of the buffer
 * @return 0 (succes)
 */
int DvbRcsDamaCtrlStub::hereIsCR(unsigned char *ip_buf, long i_len)
{
	return 0;
}

/**
 * When receiving a SACT, memcpy into the internal SACT table and build TBTP
 * @param ip_buf the pointer to SACT buff to be copied
 * @param i_len the lenght of the buffer
 * @return 0 (succes)
 */
int DvbRcsDamaCtrlStub::hereIsSACT(unsigned char *ip_buf, long i_len)
{
	return 0;
}

/**
 * Things to do when a SOF is detected process dama() and reset SACT, at issue Invariant 1
 * @param i_frame is the super frame number
 * @return 0 (succes)
 */
int DvbRcsDamaCtrlStub::runOnSuperFrameChange(long i_frame)
{
	return 0;
}

/**
 * Copy the internal TBTP structure into the zone pointed by buf. Then clean TBTP.
 * @param op_buf is the buffer where the TBTP must be copied
 * @param i_len is the lenght of the buffer
 * @return -1 (failure)
 */
int DvbRcsDamaCtrlStub::buildTBTP(unsigned char *op_buf, long i_len)
{
	return -1;
}
