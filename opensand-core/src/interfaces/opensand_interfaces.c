/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2013 TAS
 * Copyright © 2013 CNES
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
usage: opensand_interfaces [-h] [-d]\n\
  -h        print this usage and exit\n\
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
	printf("Deleting bridge");
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

int main(int argc, char *argv[])
{
	int delete = 0;
	
	struct ifreq ifr_tun;
	struct ifreq ifr_tap;
	struct ifreq ifr_br;
	int fd = -1;
	int err = -1;
	int owner;
	int group;

	const char *clone_dev_path = "/dev/net/tun";
	const char *dev_tun = "opensand_tun";
	const char *dev_tap = "opensand_tap";
	const char *dev_br = "opensand_br";

	struct passwd *pwd;

	pwd = getpwnam("opensand");
	owner = pwd->pw_uid;
	group = pwd->pw_gid;
	
	if(argc > 2)
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
		else
		{
			fprintf(stderr, "%s", USAGE);
			return 1;
		}
	}

	if(!delete)
		printf("Create TUN interface with user opensand:\n");
	else
		printf("Delete TUN interface\n");
		
	fd = open(clone_dev_path, O_RDWR);
	if(fd < 0)
	{
		return fd;
	}
	memset(&ifr_tun, 0, sizeof(ifr_tun));

	/* create TUN interface */
	snprintf(ifr_tun.ifr_name, IFNAMSIZ, "%s", dev_tun);
	
	/* Flags: IFF_TUN - TUN device (no Ethernet headers)
	 *        IFF_TAP - TAP device
	 *        IFF_NO_PI - Do not provide packet information
	 */
	ifr_tun.ifr_flags = IFF_TUN;
	err = ioctl(fd, TUNSETIFF, (void *) &ifr_tun);
	ON_ERROR_RETURN(fd, err, "TUNSETIFF: %s\n", strerror(errno));
	set_dev_ioctl(fd, owner, group, delete);
	close(fd);
	if(!delete)
		printf("Interface %s created.\n", ifr_tun.ifr_name);

	if(!delete)
		printf("Create TAP interface with user opensand:\n");
	else
		printf("Delete TAP interface\n");
	fd = open(clone_dev_path, O_RDWR);
	if(fd < 0)
	{
		return fd;
	}
	memset(&ifr_tap, 0, sizeof(ifr_tap));
	/* create TAP interface */
	snprintf(ifr_tap.ifr_name, IFNAMSIZ, "%s", dev_tap);
	ifr_tap.ifr_flags = IFF_TAP;
	err = ioctl(fd, TUNSETIFF, (void *) &ifr_tap);
	ON_ERROR_RETURN(fd, err, "TUNSETIFF: %s\n", strerror(errno));
	set_dev_ioctl(fd, owner, group, delete);

	close(fd);
	if(!delete)
		printf("Interface %s created.\n", ifr_tap.ifr_name);

	/* create bridge and add TAP in it */
	memset(&ifr_br, 0, sizeof(ifr_br));
	snprintf(ifr_br.ifr_name, IFNAMSIZ, "%s", dev_br);

	// bridge creation
	if(!delete)
		printf("Create bridge and add TAP in it:\n");
	err = br_init();
	if(err)
	{
		fprintf(stderr, "Failed to init bridge: %s\n", strerror(errno));
		return err;
	}

	// remove bridge if it already exists or if we want to delete interfaces
	del_bridge();
	if(delete)
	{
		br_shutdown();
		return 0;
	}
	
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
	err = set_if_up(fd, dev_tun, ifr_tun.ifr_flags);
	if(err)
	{
		fprintf(stderr, "Failed to set TUN interface up: %s\n",
		        strerror(errno));
		return err;
	}
	err = set_if_up(fd, dev_tap, ifr_tap.ifr_flags);
	if(err)
	{
		fprintf(stderr, "Failed to set TUN interface up: %s\n",
		        strerror(errno));
		return err;
	}
	err = set_if_up(fd, dev_br, ifr_br.ifr_flags);
	if(err)
	{
		fprintf(stderr, "Failed to set TUN interface up: %s\n",
		        strerror(errno));
		return err;
	}

	return 0;
}


