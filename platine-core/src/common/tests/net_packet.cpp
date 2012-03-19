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

