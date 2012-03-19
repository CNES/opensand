/**
 * @file GenericSwitch.cpp
 * @brief Generic switch for Satellite Emulator (SE)
 * @author Didier Barvaux <didier.barvaux@b2i-toulouse.com>
 * @author Julien Bernard <julien.bernard@toulouse.viveris.com>
 */

#include "GenericSwitch.h"

GenericSwitch::GenericSwitch()
{
}

GenericSwitch::~GenericSwitch()
{
}

bool GenericSwitch::add(long tal_id, long spot_id)
{
	bool success = false;
	std::map <long, long >::iterator it =  this->switch_table.find(tal_id);

	if(it == this->switch_table.end())
	{
		// switch entry does not exist yet
		std::pair < std::map < long, long >::iterator, bool > infos;
		infos = this->switch_table.insert(std::make_pair(tal_id, spot_id));

		if(!infos.second)
			goto quit;
	}

	success = true;

quit:
	return success;
}

