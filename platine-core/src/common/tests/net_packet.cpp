/*
 *
 * Platine is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2011 TAS
 *
 *
 * This file is part of the Platine testbed.
 *
 *
 * Platine is free software : you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see http://www.gnu.org/licenses/.
 *
 */

/*
 * This small program creates plenty of NetPacket objects. It should _NEVER_
 * segfault, but stop gracefully with a message saying that the program failed
 * to create a new NetPacket object. If it segfaults, the NetPacket::new()
 * operator is broken and should be fixed !
 *
 * This program is leaking memory, it could be fixed by storing all the
 * allocated pointers, but this is not a great task and being clean is not
 * the goal of this very small test program :)
 */

#include <iostream>

#include <NetPacket.h>
#include <AtmCell.h>

int main(int argc, char *argv[])
{
	NetPacket *pkt;
	unsigned int i = 0;

	while(1)
	{
		i++;

		pkt = new AtmCell();

		if(pkt == NULL)
		{
			printf("failed to create the %uth packet\n", i);
			break;
		}
	}

	return 0;
}
