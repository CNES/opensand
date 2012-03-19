/**
 * @file GseIdentifier.cpp
 * @brief GSE identifier (unique index given by the association of
 *        the Tal Id and Mac Id and QoS of the packets)
 * @author Julien BERNARD <julien.bernard@toulouse.viveris.com>
 */

#include "GseIdentifier.h"


GseIdentifier::GseIdentifier(long tal_id, unsigned long mac_id, int qos)
{
	this->_tal_id = tal_id;
	this->_mac_id = mac_id;
	this->_qos = qos;
}

GseIdentifier::~GseIdentifier()
{
}

long GseIdentifier::talId()
{
	return this->_tal_id;
}

unsigned long GseIdentifier::macId()
{
	return this->_mac_id;
}

int GseIdentifier::qos()
{
	return this->_qos;
}

