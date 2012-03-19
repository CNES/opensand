/**
 * @file ServiceClass.h
 * @brief A service class characterises the underlying application behaviour,
 *        e.g. Real-Time (RT), Non Real-Time (NRT) or Best Effort (BE)
 *        or Diffserv names:  EF, AF, BE
 * @author ASP - IUSO, DTP (P. SIMONNET-BORRY)
 */

#ifndef SERVICECLASS_H
#define SERVICECLASS_H

#include <string>
#include <vector>
#include "TrafficCategory.h"

using namespace std;

class TrafficCategory;


/**
 * @class ServiceClass
 * @brief A service class characterises the underlying application behaviour
 */
class ServiceClass
{
 public:

	ServiceClass();
	virtual ~ ServiceClass();

	unsigned int id;         ///< service class identifier
	string name;             ///< class name
	unsigned int schedPrio;  ///< priority of this class in main scheduler
	unsigned int macQueueId; ///< MAC queue to which this traffic is sent

	/// List of traffic flow categories inside this service class
	vector < TrafficCategory * >categoryList;

	/// operator < used by sort on scheduler priority
	bool operator<(const ServiceClass & svcClass) const
	{
		return (schedPrio < svcClass.schedPrio);
	}
	/// operator == used by find on class identifier
	bool operator==(const ServiceClass & svcClass) const
	{
		return (id == svcClass.id);
	}
};

#endif //SERVICECLASS_H
