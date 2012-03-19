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
 * Simple test application for MPEG2-TS/ULE encapsulation scheme
 *
 * The application takes a flow of IPv4 packets as input, encapsulates
 * the IPv4 packets within MPEG2-TS/ULE frames and then desencapsulates them
 * to get the IPv4 packets back.
 *
 * The application outputs MPEG2-TS/ULE frames and IPv4 packets into PCAP files
 * given as arguments. The PCAP files can be compared with references.
 *
 * Launch the application with -h to learn how to use it.
 *
 * Author: Didier Barvaux <didier.barvaux@toulouse.viveris.com>
 */

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <net/ethernet.h>
#include <syslog.h>

// include for the PCAP library
#include <pcap.h>

// Platine includes
#include <NetPacket.h>
#include <Ipv4Packet.h>
#include <MpegPacket.h>
#include <EncapCtx.h>
#include <MpegUleCtx.h>
#include <UleExtTest.h>
#include <UleExtPadding.h>

// debug level:
//   0   -> error messages only
//   1   -> debug messages
//   >=2 -> dump all MPEG frames
#define DEBUG 0

#if DEBUG >= 2
#include <iostream>
#endif

/// The length of the Linux Cooked Sockets header
#define LINUX_COOKED_HDR_LEN  16

/// The program version
#define VERSION   "MPEG2-TS/ULE test application, version 0.1\n"

/// The program usage
#define USAGE \
"MPEG2-TS/ULE test application: test the MPEG2-TS/ULE encapsulation with a flow of IP packets\n\n\
usage: test [-h] [-v] -o1 output_file -o2 output_file flow\n\
  -v               print version information and exit\n\
  -h               print this usage and exit\n\
  -o1 output_file  save the generated MPEG packets in output_file (PCAP format)\n\
  -o2 output_file  save the generated IP packets in output_file (PCAP format)\n\
  flow            flow of Ethernet frames to compress (PCAP format)\n\n"


int test_encap_and_desencap(char *src_filename,
                            char *o1_filename,
                            char *o2_filename);

int main(int argc, char *argv[])
{
	int status = 1;
	char *src_filename = NULL;
	char *o1_filename = NULL;
	char *o2_filename = NULL;
	int args_used;

	if(argc <= 1)
	{
		printf(USAGE);
		goto quit;
	}

	for(argc--, argv++; argc > 0; argc -= args_used, argv += args_used)
	{
		args_used = 1;
	
		if(!strcmp(*argv, "-v"))
		{
			// print version
			printf(VERSION);
			goto quit;
		}
		else if(!strcmp(*argv, "-h"))
		{
			// print help
			printf(USAGE);
			goto quit;
		}
		else if(!strcmp(*argv, "-o1"))
		{
			// get the name of the file to store MPEG packets
			o1_filename = argv[1];
			args_used++;
		}
		else if(!strcmp(*argv, "-o2"))
		{
			// get the name of the file to store the IP packets
			o2_filename = argv[1];
			args_used++;
		}
		else if(src_filename == NULL)
		{
			// get the name of the file that contains the packets to
			// encapsulate/desencapsulate
			src_filename = argv[0];
		}
		else
		{
			// do not accept more than one filename without option name
			printf(USAGE);
			goto quit;
		}
	}

	// the filenames are mandatory
	if(src_filename == NULL || o1_filename == NULL || o2_filename == NULL)
	{
		printf(USAGE);
		goto quit;
	}

	status = test_encap_and_desencap(src_filename, o1_filename, o2_filename);

quit:
	closelog();
	return status;
}

int test_encap_and_desencap(char *src_filename,
                            char *o1_filename,
                            char *o2_filename)
{
	int failure = 1;

	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *handle;
	pcap_dumper_t *dumper1;
	pcap_dumper_t *dumper2;
	int link_layer_type;
	int link_len;

	struct pcap_pkthdr header;
	unsigned char *packet;

	MpegUleCtx *context_tmp;
	EncapCtx *context;
	UleCtx *context_ule;

	NetPacket *ip_packet;
	NetBurst *mpeg_packets;
	NetBurst *ip_packets;
	NetBurst::iterator it;
	NetBurst::iterator it2;
	UleExt *ext;

	int context_id;
	long time = 0;

	int counter;
	int counter2;

	static unsigned char output_header[65600];
	unsigned int header_init = 0;
	static unsigned char output_packet[65600];
	struct ether_header *eth_header;

	// open the source dump file
	handle = pcap_open_offline(src_filename, errbuf);
	if(handle == NULL)
	{
		printf("failed to open the source pcap file: %s\n", errbuf);
		goto error;
	}


	// link layer in the source dump must be Ethernet or emulated
	link_layer_type = pcap_datalink(handle);
	if(link_layer_type != DLT_EN10MB &&
	   link_layer_type != DLT_LINUX_SLL &&
	   link_layer_type != DLT_RAW)
	{
		printf("link layer type %d not supported in source dump (supported = "
		       "%d, %d, %d)\n", link_layer_type, DLT_EN10MB, DLT_LINUX_SLL,
		       DLT_RAW);
		goto close_input;
	}

	if(link_layer_type == DLT_EN10MB)
		link_len = ETHER_HDR_LEN;
	else if(link_layer_type == DLT_LINUX_SLL)
		link_len = LINUX_COOKED_HDR_LEN;
	else // DLT_RAW
	link_len = 0;


	// open the network dump file for MPEG storage
	if(o1_filename != NULL)
	{
		dumper1 = pcap_dump_open(handle, o1_filename);
		if(dumper1 == NULL)
		{
			printf("failed to open dump file 1: %s\n", errbuf);
			goto close_input;
		}
	}


	// open the network dump file for IP storage
	if(o2_filename != NULL)
	{
		dumper2 = pcap_dump_open(handle, o2_filename);
		if(dumper2 == NULL)
		{
			printf("failed to open dump file 2: %s\n", errbuf);
			goto close_output1;
		}
	}


	// create encap/desencap context
	context_tmp = new MpegUleCtx(10000 /* packing threshold */ );
	if(context_tmp == NULL)
	{
		printf("failed to create encap/desencap context\n");
		goto close_output2;
	}
	// cast the context to a generic context
	context = dynamic_cast<MpegCtx *>(context_tmp);
	if(context == NULL)
	{
		printf("failed to cast the created encap/desencap context to EncapCtx\n");
		goto close_output2;
	}
	// cast the context to an ULE context
	context_ule = dynamic_cast<UleCtx *>(context_tmp);
	if(context_ule == NULL)
	{
		printf("failed to cast the created encap/desencap context to UleCtx\n");
		goto close_output2;
	}

	// create Test SNDU ULE extension
	ext = new UleExtTest();
	if(ext == NULL)
	{
		printf("failed to create Test SNDU ULE extension\n");
		goto clean_context;
	}
	if(!context_ule->addExt(ext, false))
	{
		printf("failed to add Test SNDU ULE extension\n");
		delete ext;
		goto clean_context;
	}

	// create Padding ULE extension
	ext = new UleExtPadding();
	if(ext == NULL)
	{
		printf("failed to create Padding ULE extension\n");
		goto clean_context;
	}
	if(!context_ule->addExt(ext, true))
	{
		printf("failed to add Padding ULE extension\n");
		delete ext;
		goto clean_context;
	}


	// for each packet in source dump
	counter = 0;
	while((packet = (unsigned char *) pcap_next(handle, &header)) != NULL)
	{
		counter++;

		if(!header_init)
		{
			memcpy(output_header, packet, link_len);
			header_init = 1;
		}

		// create IPv4 packet from source dump
		ip_packet = new Ipv4Packet(packet + link_len, header.len - link_len);
		if(ip_packet == NULL)
		{
			printf("[packet #%d] failed to create IPv4 packet\n", counter);
			continue;
		}

		// check IP packet validity
		if(!ip_packet->isValid())
		{
			int i;

			printf("[packet #%d] IP packet is not valid\n", counter);

			for(i = 0; i < header.len - link_len; i++)
			{
				if(i % 16 == 0) printf("\n");
				else if(i % 8 == 0) printf("\t");
				printf("0x%02x ", packet[i]);
			}
			printf("\n");

			continue;
		}

#if DEBUG >= 1
		printf("[packet #%d] %s packet is %d-byte long\n", counter,
		       ip_packet->name().c_str(), ip_packet->totalLength());
#endif

		// MPEG2-TS/ULE encapsulation
		mpeg_packets = context->encapsulate(ip_packet, context_id, time);
		if(mpeg_packets == NULL)
		{
			printf("[packet #%d] MPEG2-TS/ULE encapsulation failed\n", counter);
			continue;
		}

#if DEBUG >= 1
		printf("[packet #%d] 1 %s packet => %d %s frames\n", counter,
		       ip_packet->name().c_str(), mpeg_packets->length(), mpeg_packets->name().c_str());
#endif

		// MPEG2-TS/ULE desencapsulation
		counter2 = 0;
		for(it = mpeg_packets->begin(); it != mpeg_packets->end(); it++)
		{
#if DEBUG >= 2
			Data data;
			Data::iterator it_data;
#endif

			counter2++;

			// output MPEG packets in first dump
			header.len = link_len + (*it)->totalLength();
			header.caplen = header.len;
			memcpy(output_packet, output_header, link_len);
			memcpy(output_packet + link_len, (*it)->data().c_str(), (*it)->totalLength());
			if(link_len == ETHER_HDR_LEN) /* Ethernet only */
			{
				eth_header = (struct ether_header *) output_packet;
				eth_header->ether_type = htons(0x162d); /* unused Ethernet ID ? */
			}
			else if(link_len == LINUX_COOKED_HDR_LEN) /* Linux Cooked Sockets only */
			{
				output_packet[LINUX_COOKED_HDR_LEN - 2] = 0x16;
				output_packet[LINUX_COOKED_HDR_LEN - 1] = 0x2d;
			}
			pcap_dump((u_char *) dumper1, &header, output_packet);

			// check MPEG frame validity
			if(!(*it)->isValid())
			{
				printf("[packet #%d / frame #%d] MPEG frame is not valid\n", counter, counter2);
				continue;
			}

#if DEBUG >= 2
			data = (*it)->data();
			std::cout << std::endl << "MPEG frame:" << std::endl;
			for(it_data = data.begin(); it_data != data.end(); it_data++)
				std::cout << std::hex << std::setw(2) << std::setfill('0')
				          << static_cast<int>(*it_data) << " ";
			std::cout << std::endl;
#endif

			// MPEG2-TS/ULE desencapsulation
			ip_packets = context->desencapsulate(*it);
			if(ip_packets == NULL)
			{
				printf("[packet #%d / frame #%d] MPEG2-TS/ULE desencapsulation failed\n", counter, counter2);
				continue;
			}

#if DEBUG >= 1
			printf("[packet #%d / frame #%d] 1 %s frame => %d %s packets\n", counter, counter2,
			       (*it)->name().c_str(), ip_packets->length(), ip_packets->name().c_str());
#endif

			// output IP packets in second dump
			for(it2 = ip_packets->begin(); it2 != ip_packets->end(); it2++)
			{
				header.len = link_len + (*it2)->totalLength();
				header.caplen = header.len;
				memcpy(output_packet, output_header, link_len);
				memcpy(output_packet + link_len, (*it2)->data().c_str(), (*it2)->totalLength());
				pcap_dump((u_char *) dumper2, &header, output_packet);
			}

			delete ip_packets;
		}

		delete mpeg_packets;
		delete ip_packet;
#if DEBUG >= 1
		printf("\n");
#endif
	}


	// flush MPEG2-TS contexts
	mpeg_packets = context->flushAll();
	if(mpeg_packets == NULL)
	{
		printf("[flush] MPEG2-TS/ULE flush failed\n");
		goto clean_context;
	}

#if DEBUG >= 1
	printf("[flush] => %d %s frames\n", mpeg_packets->length(),
	       mpeg_packets->name().c_str());
#endif

	// MPEG2-TS/ULE desencapsulation
	counter2 = 0;
	for(it = mpeg_packets->begin(); it != mpeg_packets->end(); it++)
	{
		counter2++;

		// output MPEG packets in first dump
		header.len = link_len + (*it)->totalLength();
		header.caplen = header.len;
		memcpy(output_packet, output_header, link_len);
		memcpy(output_packet + link_len, (*it)->data().c_str(), (*it)->totalLength());
		if(link_len == ETHER_HDR_LEN) /* Ethernet only */
		{
			eth_header = (struct ether_header *) output_packet;
			eth_header->ether_type = htons(0x162d); /* unused Ethernet ID ? */
		}
		else if(link_len == LINUX_COOKED_HDR_LEN) /* Linux Cooked Sockets only */
		{
			output_packet[LINUX_COOKED_HDR_LEN - 2] = 0x16;
			output_packet[LINUX_COOKED_HDR_LEN - 1] = 0x2d;
		}
		pcap_dump((u_char *) dumper1, &header, output_packet);

		// check MPEG frame validity
		if(!(*it)->isValid())
		{
			printf("[flush / frame #%d] MPEG frame is not valid\n", counter2);
			continue;
		}

		// MPEG2-TS/ULE desencapsulation
		ip_packets = context->desencapsulate(*it);
		if(ip_packets == NULL)
		{
			printf("[flush / frame #%d] MPEG2-TS/ULE desencapsulation failed\n", counter2);
			continue;
		}

#if DEBUG >= 1
		printf("[flush / frame #%d] 1 %s frame => %d %s packets\n", counter2,
		       (*it)->name().c_str(), ip_packets->length(), ip_packets->name().c_str());
#endif

		// output IP packets in second dump
		for(it2 = ip_packets->begin(); it2 != ip_packets->end(); it2++)
		{
			header.len = link_len + (*it2)->totalLength();
			header.caplen = header.len;
			memcpy(output_packet, output_header, link_len);
			memcpy(output_packet + link_len, (*it2)->data().c_str(), (*it2)->totalLength());
			pcap_dump((u_char *) dumper2, &header, output_packet);
		}

		delete ip_packets;
	}

	delete mpeg_packets;

	// every is fine
	failure = 0;

	// clean memory
clean_context:
	delete context;
close_output2:
	pcap_dump_close(dumper2);
close_output1:
	pcap_dump_close(dumper1);
close_input:
	pcap_close(handle);
error:
	return failure;
}
