/**
 * @file ServiceClass.cpp
 * @brief A service class characterises the underlying application behaviour,
 *        e.g. Real-Time (RT), Non Real-Time (NRT) or Best Effort (BE)
 *        or Diffserv names:  EF, AF, BE
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

// System includes
#include <stdio.h>
#include <stdlib.h>

// Project includes
#define DBG_PACKAGE PKG_QOS_DATA
#include "platine_margouilla/mgl_time.h"
#include "platine_conf/uti_debug.h"
#include "TrafficCategory.h"
#include "ServiceClass.h"


/**
 * constructor
 */
ServiceClass::ServiceClass()
{
	id = 0;
	name = "";
	schedPrio = 0;
	macQueueId = 0;
}

/**
 * destructor
 */
ServiceClass::~ServiceClass()
{
	categoryList.clear();
}


