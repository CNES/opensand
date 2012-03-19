/**
 * @file EncapCtx.cpp
 * @brief Generic encapsulation / desencapsulation context
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "EncapCtx.h"

EncapCtx::EncapCtx()
{
	this->_tal_id = -1;
}

EncapCtx::~EncapCtx()
{
}

void EncapCtx::setFilter(long tal_id)
{
	this->_tal_id = tal_id;
}

long EncapCtx::talId()
{
	return this->_tal_id;
}

