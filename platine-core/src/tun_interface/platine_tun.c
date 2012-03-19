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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <pwd.h>

int main()
{
	struct ifreq ifr;
	int fd, err;
    int owner;
    int group;

    struct passwd *pwd;

    printf("create interface tun for platine\n");

    pwd = getpwnam("platine");
    owner = pwd->pw_uid;
    group = pwd->pw_gid;

	fd = open("/dev/net/tun", O_RDWR);
	if(fd < 0)
		return fd;

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */

	/* create TUN interface */
	snprintf(ifr.ifr_name, IFNAMSIZ, "platine");
	ifr.ifr_flags = IFF_TUN;

	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	if(err < 0)
	{
        perror("TUNSETIFF");
		close(fd);
		return err;
	}

	err = ioctl(fd, TUNSETPERSIST, 1);
	if(err < 0)
	{
        perror("TUNSETPERSIST");
		close(fd);
		return err;
	}

    err = ioctl(fd, TUNSETOWNER, owner);
	if(err < 0)
	{
        perror("TUNSETOWNER");
		close(fd);
		return err;
	}

    err = ioctl(fd, TUNSETOWNER, owner);
	if(err < 0)
	{
        perror("TUNSETGROUP");
		close(fd);
		return err;
	}

    printf("interface %s created\n", ifr.ifr_name);

	return 0;
}

