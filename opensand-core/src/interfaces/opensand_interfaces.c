/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2017 TAS
 * Copyright © 2017 CNES
 *
 *
 * This file is part of the OpenSAND testbed.
 *
 *
 * OpenSAND is free software : you can redistribute it and/or modify it under the
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
#include <errno.h>

#include "bridge_utils.h"

//TODO soon we should be able to create tun and tap with libnl so remove
//     all the interfaces folder and handle tun, tap and bridge in libnl

#define ON_ERROR_RETURN(fd, err, str_err, args...)\
{\
	if(err < 0)\
	{\
		fprintf(stderr, str_err, ##args);\
		close(fd);\
		exit(1); \
	}\
}

/// The program usage
#define USAGE \
"Create/Delete TUN, TAP and bridge interfaces for OpenSAND\n\n\
usage: opensand_interfaces [-h] [-d] [-l] [-n]\n\
  -h        print this usage and exit\n\
  -l        add link layer interfaces (bridge and tap)\n\
  -n        add network layer interface (tun)\n\
            if none of -l or -n is specified both will be done\n\
  -d        delete the interfaces instead of creating them\n\n"


extern int br_socket_fd;

static int set_dev_ioctl(int fd, int owner, int group, int delete) 
{
	int err;
	if(delete)
	{
		err = ioctl(fd, TUNSETPERSIST, 0);
		ON_ERROR_RETURN(fd, err, "TUNSETPERSIST: %s\n", strerror(errno));
		return err;
	}
	err = ioctl(fd, TUNSETPERSIST, 1);
	ON_ERROR_RETURN(fd, err, "TUNSETPERSIST: %s\n", strerror(errno));
	err = ioctl(fd, TUNSETOWNER, owner);
	ON_ERROR_RETURN(fd, err, "TUNSETOWNER: %s\n", strerror(errno));
	err = ioctl(fd, TUNSETGROUP, group);
	ON_ERROR_RETURN(fd, err, "TUNSETGROUP: %s\n", strerror(errno));
	return err;
}


static void del_bridge()
{
	const char *br = "opensand_br";
	struct ifreq ifr_br;
	int sockfd = -1;
	int err = -1;
	int init = 0;

	memset(&ifr_br, 0, sizeof(ifr_br));
	snprintf(ifr_br.ifr_name, IFNAMSIZ, "%s", br);

	if(br_socket_fd == -1)
	{
		init = 1;
		err = br_init();
		if(err)
		{
			fprintf(stderr, "Failed to init bridge: %s\n", strerror(errno));
			return;
		}
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1)
	{
		fprintf(stderr, "Could not get socket for bridge\n");
	}
	
	err = set_if_down(sockfd, br, ifr_br.ifr_flags);
	if(err)
	{
		fprintf(stderr, "Failed to set bridge down: %s\n", strerror(errno));
	}
	printf("Deleting bridge\n");
	err = br_del_bridge(br);
	if(err)
	{
		fprintf(stderr, "Failed to delete bridge: %s\n", strerror(errno));
	}

	close(sockfd);
	if(init)
	{
		br_shutdown();
	}
}


int tun_tap(int tun, int delete)
{
	struct ifreq ifr;
	int fd = -1;
	int err = -1;
	int owner;
	int group;
	
	const char *clone_dev_path = "/dev/net/tun";
	const char *dev = (tun ? "opensand_tun" : "opensand_tap");

	struct passwd *pwd;

	pwd = getpwnam("opensand");
	owner = pwd->pw_uid;
	group = pwd->pw_gid;

	if(!delete)
		printf("Create %s interface with user opensand:\n",
		       (tun ? "TUN": "TAP"));
	else
		printf("Delete %s interface\n", (tun ? "TUN": "TAP"));
		
	fd = open(clone_dev_path, O_RDWR);
	if(fd < 0)
	{
		return 1;
	}
	memset(&ifr, 0, sizeof(ifr));

	/* create TUN/TAP interface */
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", dev);
	
	/* Flags: IFF_TUN - TUN device (no Ethernet headers)
	 *        IFF_TAP - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */
	ifr.ifr_flags = (tun ? IFF_TUN : IFF_TAP);
	err = ioctl(fd, TUNSETIFF, (void *) &ifr);
	ON_ERROR_RETURN(fd, err, "TUNSETIFF: %s\n", strerror(errno));
	set_dev_ioctl(fd, owner, group, delete);
	close(fd);
	if(!delete)
		printf("Interface %s created.\n", ifr.ifr_name);
	return 0;
}


int bridge(int delete)
{
	struct ifreq ifr_br;
	int fd = -1;
	int err = -1;
	
	const char *dev_br = "opensand_br";
	const char *dev_tap = "opensand_tap";


	/* create bridge and add TAP in it */
	memset(&ifr_br, 0, sizeof(ifr_br));
	snprintf(ifr_br.ifr_name, IFNAMSIZ, "%s", dev_br);

	err = br_init();
	if(err)
	{
		fprintf(stderr, "Failed to init bridge: %s\n", strerror(errno));
		return err;
	}

	// remove bridge if it already exists or if we want to delete interfaces
	if(delete)
	{
		del_bridge();
		br_shutdown();
		return 0;
	}

	// bridge creation
	if(!delete)
		printf("Create bridge and add TAP in it:\n");
	
	err = br_add_bridge(dev_br);
	if(err)
	{
		fprintf(stderr, "Failed to add bridge: %s\n", strerror(errno));
		br_shutdown();
		return err;
	}
	err = br_add_interface(dev_br, dev_tap);
	if(err)
	{
		fprintf(stderr, "Failed to add TAP interface in bridge: %s\n", strerror(err));
		br_shutdown();
		return err;
	}
	br_shutdown();

	// set interfaces up
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		return fd;
	}

	return 0;
}



int main(int argc, char *argv[])
{
	int delete = 0;
	
	int err = 0;
	
	int link = 0;
	int net = 0;

	if(argc > 4)
	{
		fprintf(stderr, "%s", USAGE);
		return 1;
	}

	for(argc--, argv++; argc > 0; argc -= 1, argv += 1)
	{
		if(!strcmp(*argv, "-h"))
		{
			// print help
			printf("%s", USAGE);
			return 1;
		}
		else if(!strcmp(*argv, "-d"))
		{
			delete = 1;
		}
		else if(!strcmp(*argv, "-l"))
		{
			link = 1;
		}
		else if(!strcmp(*argv, "-n"))
		{
			net = 1;
		}
		else
		{
			fprintf(stderr, "%s", USAGE);
			return 1;
		}
	}
	if(link == 0 && net == 0)
	{
		// if nothing specified do both
		link = 1;
		net = 1;
	}
	
	if(link)
	{
		err |= tun_tap(0, delete);
		err |= bridge(delete);
	}
	if(net)
	{
		err = tun_tap(1, delete);
	}
	return err;
}



