/**
 * @file TrafficCategory.cpp
 * @brief A traffic flow category regroups several traffic flows served
 *        in the same way
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

// System includes
#include <stdio.h>
#include <stdlib.h>

// Project includes
#define DBG_PACKAGE PKG_QOS_DATA
#include "platine_conf/uti_debug.h"
#include "TrafficCategory.h"


/**
 * constructor
 */
TrafficCategory::TrafficCategory(): name()
{
	id = 0;
}

/**
 * Destroy the TrafficCategory object
 */
TrafficCategory::~TrafficCategory()
{
}

