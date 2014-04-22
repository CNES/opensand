/*
 *
 * OpenSAND is an emulation testbed aiming to represent in a cost effective way a
 * satellite telecommunication system for research and engineering activities.
 *
 *
 * Copyright © 2014 TAS
 * Copyright © 2014 CNES
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

/*
 * Simple test application for bridge between TAP interface and physical interface
 *
 * The application writes/reads on the bridged TAP interface
 * and reads/writes the frames from bridged physical interface
 *
 * Launch the application with -h to learn how to use it.
 *
 * Author: Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 * Author: Julien Bernard <julien.bernard@toulouse.viveris.com>
 * Author: Remy Pienne <remy.pienne@toulouse.viveris.com>
 */

#include "bridge_utils.h"

// system includes
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>

#include <pcap.h>


/// The program version
#define VERSION   "bridged TAP interface test application, version 0.1\n"

/// The program usage
#define USAGE \
"Bridged TAP test application: write on TAP and read on bridged physical interface.\n\n\
usage: test_write_tap [-h] [-v] [-d level] flow\n\
  -v        print version information and exit\n\
  -d level  print debug information\n\
                - 0 error only\n\
                - 1 debug messages\n\
  -h        print this usage and exit\n\
  flow      flow of Ethernet frames to write on TAP (PCAP format)\n\n"

static unsigned int verbose=0;

/** DEBUG macro */
#define DEBUG(format, ...) \
	do { \
		if(verbose) \
			printf(format, ##__VA_ARGS__); \
	} while(0)

#define ERROR(format, ...) \
	do { \
		fprintf(stderr, format, ##__VA_ARGS__); \
	} while(0)

#define MIN(x, y)  (((x) < (y)) ? (x) : (y))


static int compare_packets(const unsigned char *pkt1, int pkt1_size,
                           const unsigned char *pkt2, int pkt2_size,
                           int iter)
{
	int valid = 0;
	int min_size;
	int i, j, k;
	char str1[4][7], str2[4][7];
	char sep1, sep2;

	min_size = pkt1_size > pkt2_size ? pkt2_size : pkt1_size;

	/* do not compare more than 180 bytes to avoid huge output */
	min_size = MIN(180, min_size);

	/* if packets are equal, do not print the packets */
	if(pkt1_size == pkt2_size && memcmp(pkt1, pkt2, pkt1_size) == 0)
		goto skip;

	/* packets are different */
	valid = -1;
	
	// do not dump miscellaneous network packet
	if(iter == 0)
		goto skip;

	DEBUG("------------------------------ Compare ------------------------------\n");

	if(pkt1_size != pkt2_size)
	{
		DEBUG("packets have different sizes (%d != %d), compare only the %d "
		      "first bytes\n", pkt1_size, pkt2_size, min_size);
	}

	j = 0;
	for(i = 0; i < min_size; i++)
	{
		if(pkt1[i] != pkt2[i])
		{
			sep1 = '#';
			sep2 = '#';
		}
		else
		{
			sep1 = '[';
			sep2 = ']';
		}

		sprintf(str1[j], "%c0x%.2x%c", sep1, pkt1[i], sep2);
		sprintf(str2[j], "%c0x%.2x%c", sep1, pkt2[i], sep2);

		/* make the output human readable */
		if(j >= 3 || (i + 1) >= min_size)
		{
			for(k = 0; k < 4; k++)
			{
				if(k < (j + 1))
					DEBUG("%s  ", str1[k]);
				else /* fill the line with blanks if nothing to print */
					DEBUG("        ");
			}

			DEBUG("      ");

			for(k = 0; k < (j + 1); k++)
				DEBUG("%s  ", str2[k]);

			DEBUG("\n");

			j = 0;
		}
		else
		{
			j++;
		}
	}

	DEBUG("----------------------- packets are different -----------------------\n");

skip:
	return valid;
}


static int test_read_write_on_tap(const char *src_filename,
                                   const char *target_itf,
                                   int fd)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	unsigned char *packet;
	unsigned char *result_packet;
	int nwrite, res_read;
	pcap_t *handle_offline;
	pcap_t *handle_itf;
	struct pcap_pkthdr header;
	struct pcap_pkthdr *result_header;
	handle_offline = pcap_open_offline(src_filename, errbuf);
	handle_itf = pcap_open_live(target_itf, 65000, 0, 0, errbuf);
	if(handle_offline == NULL || target_itf == NULL)
	{
		ERROR("failed to open the source pcap file/the target interface: %s\n", errbuf);
		return -1;
	}
	/* for each packet in the dump */
	int counter = 0;
	while((packet = (unsigned char *) pcap_next(handle_offline, &header)) != NULL)
	{
		unsigned char tap_packet[header.len + 4];
		static unsigned char flags[4] = { 0, 0, 8, 0 };
		int ret;
		int iter;
		// add tap header on packet befor writing it on fd
		tap_packet[0] = flags[0];
		tap_packet[1] = flags[1];
		tap_packet[2] = flags[2];
		tap_packet[3] = flags[3];
		memcpy(tap_packet + 4, packet, header.len);
		counter++;
		DEBUG("Handle packet #%d\n", counter);
		// write packet on TAP
		nwrite = write(fd, tap_packet, header.len + 4);
		if(nwrite < 0)
		{
			perror("writing to TAP interface");
			close(fd);
			return -1;
		}
		DEBUG("wrote %d bytes frame on %s\n", header.len, "TAP interface");
		iter = 0;
		do
		{
			res_read = pcap_next_ex(handle_itf, &result_header, (const u_char **) &result_packet);
			if(res_read < 0)
			{
				pcap_perror(handle_itf, "reading on physical interface");
				close(fd);
				return -1;
			}
			DEBUG("read %d bytes frame on %s\n", result_header->caplen, target_itf);
			ret = compare_packets(result_packet, result_header->len, packet, header.len, iter);
			// on the first error try another packet because we could have received a
			// miscellaneous network packet
			if(ret != 0 && iter < 2)
			{
				iter++;
			}
		}
		while(ret != 0 && iter < 2);
		if(ret < 0)
		{
			ERROR("bad packet received\n");
			return -1;
		}

	}
	DEBUG("read %d packets, success.\n", counter);
	return 0;
}

int main(int argc, char *argv[])
{
	int status = 1;
	const char *src_filename = "source.pcap";
	//TODO: get source filename and target interface from argv
	const char *target_itf = "eth0";
	const char *clone_dev_path = "/dev/net/tun";
	const char *dev_tap = "opensand_tap";
	char *br = "opensand_br";
	struct ifreq ifr_tap;

	const char *itf = "eth0";
	int err = -1;
	struct ifreq ifr_br;
	int sockfd = -1;

	int args_used;

	int fd = -1;
	char tap_name[IFNAMSIZ];

	if(getuid())
	{
		ERROR("This program must be run as root.\n");
		goto quit;
	}
	for(argc--, argv++; argc > 0; argc -= args_used, argv += args_used)
	{
		args_used = 1;

		if(!strcmp(*argv, "-v"))
		{
			// print version
			ERROR(VERSION);
			goto quit;
		}
		else if(!strcmp(*argv, "-h"))
		{
			// print help
			ERROR(USAGE);
			goto quit;
		}
		else if(!strcmp(*argv, "-d"))
		{
			// debug mode
			verbose = atoi(argv[1]);
			args_used++;
		}
		else
		{
			ERROR(USAGE);
			goto quit;
		}
	}
	memset(&ifr_br, 0, sizeof(ifr_br));
	snprintf(ifr_br.ifr_name, IFNAMSIZ, "%s", br);

	// bridge creation
	err = br_init();
	if(err)
	{
		ERROR("br_init %s\n", strerror(err));
		goto quit;
	}

	printf("Creating bridge between tap opensand and eth0.\n");
	br_del_bridge(br);
	err = br_add_bridge(br);
	if(err)
	{
		ERROR("br_add_bridge %s\n", strerror(err));
		goto del;
	}
	err = br_add_interface(br, dev_tap);
	if(err)
	{
		ERROR("br_add_interface TAP %s\n", strerror(err));
		goto del;
	}
	err = br_add_interface(br, itf);
	if(err)
	{
		ERROR("br_add_interface ETH %s\n", strerror(err));
		goto del;
	}
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1)
	{
		ERROR("Could not get socket.\n");
		goto del;
	}
	err = set_if_up(sockfd, br, ifr_br.ifr_flags);
	if(err)
	{
		ERROR("set_if_up %s\n", strerror(err));
		goto error;
	}
	// wait for bridge to be ready
	DEBUG("Wait for bridge to be ready\n");
	sleep(20);

	/* Connect to the device */
	strncpy(tap_name, dev_tap, IFNAMSIZ);
	fd = open(clone_dev_path, O_RDWR);
	if(fd < 0)
	{
		perror("opening clone device");
		goto error;
	}
	memset(&ifr_tap, 0, sizeof(ifr_tap));
	snprintf(ifr_tap.ifr_name, IFNAMSIZ, "%s", dev_tap);
	ifr_tap.ifr_flags = IFF_TAP;
	err = ioctl(fd, TUNSETIFF, (void *) &ifr_tap);
	if(err)
	{
		ERROR("connecting to interface %s", strerror(err));
		goto error;
	}

	if(test_read_write_on_tap(src_filename, target_itf, fd) == 0)
	{
		// success
		status = 0;
	}

error:
	err = set_if_down(sockfd, br, ifr_br.ifr_flags);
	if(err)
	{
		ERROR("set_if_down %s\n", strerror(err));
	}
del:
	printf("Deleting bridge between tap opensand and eth0.\n");
	err = br_del_bridge(br);
	if(err)
	{
		ERROR("br_del_bridge %s\n", strerror(err));
	}

quit:
	br_shutdown();
	return status;
}

