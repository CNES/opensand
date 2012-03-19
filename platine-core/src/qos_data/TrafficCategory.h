/**
 * @file TrafficCategory.h
 * @brief A traffic flow category regroups several traffic flows served
 *        in the same way
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 */

#ifndef TRAFFICCATEGORY_H
#define TRAFFICCATEGORY_H

#include <string>
#include "ServiceClass.h"


/**
 * @class TrafficCategory
 * @brief A traffic flow category regroups several traffic flows served in the same way
 */
class TrafficCategory
{
 public:

	TrafficCategory();
	~TrafficCategory();

	/// Traffic category identifier
	unsigned short id;

	/// Traffic category name
	std::string name;

	/// The Service Class associated with the Traffic Category
	ServiceClass *svcClass;
};

#endif // TRAFFICCATEGORY_H

