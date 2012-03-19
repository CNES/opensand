/**
 * @file AtmIdentifier.cpp
 * @brief ATM identifier (unique index given by the association of both
 *        the VPI and VCI fields of the ATM header)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#include "AtmIdentifier.h"


AtmIdentifier::AtmIdentifier(uint8_t vpi, uint16_t vci)
{
	this->setVpi(vpi);
	this->setVci(vci);
}

AtmIdentifier::~AtmIdentifier()
{
}

uint8_t AtmIdentifier::getVpi()
{
	return this->vpi;
}

void AtmIdentifier::setVpi(uint8_t vpi)
{
	this->vpi = vpi;
}

uint16_t AtmIdentifier::getVci()
{
	return this->vci;
}

void AtmIdentifier::setVci(uint16_t vci)
{
	this->vci = vci;
}

