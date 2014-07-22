/*
 * Copyright (C) 2000 Lennert Buytenhek
 * Copyright (C) 2014 CNES
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
 */


/**
 * @file bridge_utils.h
 * @brief useful functions taken from libbridge
 *
 * Some functions of the bridge library (from brctl) used to
 * create a bridge in OpenSAND
 *
 */

#include "bridge_utils.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <linux/if.h>
//#include <linux/if_tun.h>
#include <netinet/ip6.h>
#include <linux/if_bridge.h>
#include <errno.h>

/* defined in net/if.h but that conflicts with linux/if.h... */
extern unsigned int if_nametoindex (const char *__ifname);
int br_socket_fd = -1;

int br_add_interface(const char *bridge, const char *dev)
{
	struct ifreq ifr;
	int err;
	int ifindex = if_nametoindex(dev);

	if (ifindex == 0)
		return ENODEV;

	strncpy(ifr.ifr_name, bridge, IFNAMSIZ);
	unsigned long args[4] = { BRCTL_ADD_IF, ifindex, 0, 0 };

	ifr.ifr_data = (char *) args;
	err = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

	return err < 0 ? errno : 0;
}

int br_del_interface(const char *bridge, const char *dev)
{
	struct ifreq ifr;
	int err;
	int ifindex = if_nametoindex(dev);

	if (ifindex == 0)
		return ENODEV;

	strncpy(ifr.ifr_name, bridge, IFNAMSIZ);
	unsigned long args[4] = { BRCTL_DEL_IF, ifindex, 0, 0 };

	ifr.ifr_data = (char *) args;
	err = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

	return err < 0 ? errno : 0;
}

int br_add_bridge(const char *brname)
{
	int ret;

	char _br[IFNAMSIZ];
	unsigned long arg[3]
		= { BRCTL_ADD_BRIDGE, (unsigned long) _br };

	strncpy(_br, brname, IFNAMSIZ);
	ret = ioctl(br_socket_fd, SIOCSIFBR, arg);

	return ret < 0 ? errno : 0;
}

int br_del_bridge(const char *brname)
{
	int ret;

	char _br[IFNAMSIZ];
	unsigned long arg[3]
		= { BRCTL_DEL_BRIDGE, (unsigned long) _br };

	strncpy(_br, brname, IFNAMSIZ);
	ret = ioctl(br_socket_fd, SIOCSIFBR, arg);
	return  ret < 0 ? errno : 0;
}

int br_init(void)
{
	if ((br_socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
		return errno;
	return 0;
}


void br_shutdown(void)
{
	close(br_socket_fd);
	br_socket_fd = -1;
}


int set_if_flags(int fd, const char *ifname, short flags)
{
	struct ifreq ifr;
	int res = 0;

	ifr.ifr_flags = flags;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	res = ioctl(fd, SIOCSIFFLAGS, &ifr);

	return res;
}

int set_if_up(int fd, const char *ifname, short flags)
{
	return set_if_flags(fd, ifname, flags | IFF_UP);
}

int set_if_down(int fd, const char *ifname, short flags)
{
	return set_if_flags(fd, ifname, flags & ~IFF_UP);
}

