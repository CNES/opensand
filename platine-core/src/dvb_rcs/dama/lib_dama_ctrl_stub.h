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
