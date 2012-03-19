/**
 * @file PepRequest.cpp
 * @brief Allocation or release request from a PEP component
 * @author Didier Barvaux / Viveris Technologies
 */

#include "PepRequest.h"


/**
 * @brief Build a new release request from PEP
 */
PepRequest::PepRequest(pep_request_type_t type, unsigned int st_id,
                       unsigned int cra, unsigned int rbdc, unsigned int rbdc_max)
{
	this->type = type;
	this->st_id = st_id;
	this->cra = cra;
	this->rbdc = rbdc;
	this->rbdc_max = rbdc_max;
}

/**
 * @brief Destroy the interface between NCC and PEP components
 */
PepRequest::~PepRequest()
{
	// nothing to do
}


/**
 * @brief Get the type of PEP request
 *
 * @return  the type of PEP request
 */
pep_request_type_t PepRequest::getType()
{
	return this->type;
}


/**
 * @brief Get the ST the PEP request is for
 *
 * @return  the ID of the ST the PEP request is for
 */
unsigned int PepRequest::getStId()
{
	return this->st_id;
}


/**
 * @brief Get the CRA of PEP request
 *
 * @return  the CRA of PEP request
 */
unsigned int PepRequest::getCra()
{
	return this->cra;
}


/**
 * @brief Get the RBDC of PEP request
 *
 * @return  the RBDC of PEP request
 */
unsigned int PepRequest::getRbdc()
{
	return this->rbdc;
}


/**
 * @brief Get the RBDCmax of PEP request
 *
 * @return  the RBDCmax of PEP request
 */
unsigned int PepRequest::getRbdcMax()
{
	return this->rbdc_max;
}

