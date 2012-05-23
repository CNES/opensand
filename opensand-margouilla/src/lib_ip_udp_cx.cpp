/**********************************************************************
**
** Margouilla Common Blocs
**
**
** Copyright (C) 2002-2003 CQ-Software.  All rights reserved.
**
**
** This file is distributed under the terms of the GNU Library 
** General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.
**
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.margouilla.com
**********************************************************************/
/* $Id: lib_ip_udp_cx.cpp,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */

/****** System Includes ******/
#include <stdio.h>
#include <stdlib.h>

/****** Application Includes ******/
#include "lib_ip_udp_cx.h"


/*
IPv4

1 |  ver |  hlen  |
2 |<     tos     >|
3 |<    total     |
4 |     length   >|
5 |<      id      |
6 |              >|
7 |< flag | frag. |
8 |    offset    >|
9 |<     ttl     >|
10|<     proto   >|
11|<      crc     |
12|              >|
13|<              |
14|      src      |
15|               |
16|              >|
17|<              |
18|      dst      |
19|               |
20|              >|


IPv6

1 |  ver | priorit|
2 |<     flow     |
3 |     label     |
4 |              >|
5 |<    payload   |
6 |     length   >|
7 |<   next hdr  >|
8 |<   hop limit >|
9 |<              |
10|      src      |
         ...
24|              >|
25|<              |
26|      dst      |
         ...
40|              >|


UDP

1 |<   src port   |
2 |              >|
3 |<   dst port   |
4 |              >|
5 |<    length    |
6 |              >|
7 |<      crc     |
8 |              >|


TCP

1 |<   src port   |
2 |              >|
3 |<   dst port   |
4 |              >|
5 |<    seq num   |
6 |               |
7 |               |
8 |              >|
9 |<    ack num   |
10|               |
11|               |
12|              >|
13| offset| flags |
14|               |
15|<   windows    |
16|              >|
17|<      crc     |
18|              >|
19|<urgent pointer|
20|              >|
xx|<    options   |
xx|    +padding  >|

*/

long g_lib_ip_udp_cx_trace=0;



unsigned long mgl_ip_header_get_version(char *ip_buf) 
{
	unsigned long l_val = ((unsigned long)(ip_buf[0] & 0xf0))>>4;
	return l_val; 
}
void mgl_ip_header_set_version(char *op_buf, unsigned long i_val) 
{
	char l_byte = op_buf[0];
	op_buf[0] = (l_byte&0x0f) | (char)((i_val<<4)& 0xf0);
}


unsigned long mgl_ipv4_header_get_hlen(char *ip_buf) 
{
	unsigned long l_val = ((unsigned long)(ip_buf[0] & 0x0f));
	return l_val; 
}
void mgl_ipv4_header_set_hlen(char *op_buf, unsigned long i_val) 
{
	char l_byte = op_buf[0];
	op_buf[0] = (l_byte&0xf0) | (char)((i_val)& 0x0f);
}


unsigned long mgl_ipv4_header_get_tos(char *ip_buf) 
{
	unsigned long l_val = ((unsigned long)(ip_buf[1] & 0xff));
	return l_val; 
}
void mgl_ipv4_header_set_tos(char *op_buf, unsigned long i_val) 
{
	op_buf[1] = (char)((i_val    )& 0xff);
}


unsigned long mgl_ipv4_header_get_packet_length(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[2] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[3] & 0xff)));
	return l_val; 
}
void mgl_ipv4_header_set_packet_length(char *op_buf, unsigned long i_val) 
{
	op_buf[2] = (char)((i_val>>8)& 0xff);
	op_buf[3] = (char)((i_val   )& 0xff);
}


unsigned long mgl_ipv4_header_get_id(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[4] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[5] & 0xff)));
	return l_val; 
}
void mgl_ipv4_header_set_id(char *op_buf, unsigned long i_val) 
{
	op_buf[4] = (char)((i_val>>8)& 0xff);
	op_buf[5] = (char)((i_val   )& 0xff);
}


unsigned long mgl_ipv4_header_get_flag(char *ip_buf) 
{
	unsigned long l_val = ((unsigned long)(ip_buf[6] & 0xe0)>>5);
	return l_val; 
}
void mgl_ipv4_header_set_flag(char *op_buf, unsigned long i_val) 
{
	char l_byte = op_buf[6];
	op_buf[6] = (l_byte&0x1f) | (char)((i_val<<5)& 0xe0);
}


unsigned long mgl_ipv4_header_get_fragment_offset(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[6] & 0x1f))<<8)
	           	         |(((unsigned long)(ip_buf[7] & 0xff)));
	return l_val; 
}
void mgl_ipv4_header_set_fragment_offset(char *op_buf, unsigned long i_val) 
{
	char l_byte = op_buf[6];
	op_buf[6] = (l_byte&0xe0) | (char)((i_val>>8)& 0x1f);
	op_buf[7] = (char)((i_val   )& 0xff);
}

unsigned long mgl_ipv4_header_get_ttl(char *ip_buf) 
{
	unsigned long l_val = ((unsigned long)(ip_buf[8] & 0xff));
	return l_val; 
}
void mgl_ipv4_header_set_ttl(char *op_buf, unsigned long i_val) 
{
	op_buf[8] = (char)((i_val   )& 0xff);
}


unsigned long mgl_ipv4_header_get_proto(char *ip_buf) 
{
	unsigned long l_val = ((unsigned long)(ip_buf[9] & 0xff));
	return l_val; 
}
void mgl_ipv4_header_set_proto(char *op_buf, unsigned long i_val) 
{
	op_buf[9] = (char)((i_val   )& 0xff);
}


unsigned long mgl_ipv4_header_get_crc(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[10] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[11] & 0xff)));
	return l_val; 
}
void mgl_ipv4_header_set_crc(char *op_buf, unsigned long i_val) 
{
	op_buf[10] = (char)((i_val>>8)& 0xff);
	op_buf[11] = (char)((i_val   )& 0xff);
}


unsigned long mgl_ipv4_header_get_ip_src(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[12] & 0xff))<<24)
	           	         |(((unsigned long)(ip_buf[13] & 0xff))<<16)
	           	         |(((unsigned long)(ip_buf[14] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[15] & 0xff)));
	return l_val; 
}
void mgl_ipv4_header_set_ip_src(char *op_buf, unsigned long i_val) 
{
	op_buf[12] = (char)((i_val>>24)& 0xff);
	op_buf[13] = (char)((i_val>>16)& 0xff);
	op_buf[14] = (char)((i_val>>8)& 0xff);
	op_buf[15] = (char)((i_val   )& 0xff);
}


unsigned long mgl_ipv4_header_get_ip_dst(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[16] & 0xff))<<24)
	           	         |(((unsigned long)(ip_buf[17] & 0xff))<<16)
	           	         |(((unsigned long)(ip_buf[18] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[19] & 0xff)));
	return l_val; 
}
void mgl_ipv4_header_set_ip_dst(char *op_buf, unsigned long i_val) 
{
	op_buf[16] = (char)((i_val>>24)& 0xff);
	op_buf[17] = (char)((i_val>>16)& 0xff);
	op_buf[18] = (char)((i_val>>8)& 0xff);
	op_buf[19] = (char)((i_val   )& 0xff);
}


mgl_ip_addr mgl_ipv6_header_get_ip_dst(char *ip_buf) 
{
	mgl_ip_addr l_ip_addr;

	l_ip_addr.setIPV6FromBuf(&(ip_buf[25]));
	return l_ip_addr; 
}


unsigned long mgl_udp_header_get_src_port(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[0] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[1] & 0xff)));
	return l_val; 
}
void mgl_udp_header_set_src_port(char *op_buf, unsigned long i_val) 
{
	op_buf[0] = (char)((i_val>>8)& 0xff);
	op_buf[1] = (char)((i_val   )& 0xff);
}

unsigned long mgl_udp_header_get_dst_port(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[2] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[3] & 0xff)));
	return l_val; 
}
void mgl_udp_header_set_dst_port(char *op_buf, unsigned long i_val) 
{
	op_buf[2] = (char)((i_val>>8)& 0xff);
	op_buf[3] = (char)((i_val   )& 0xff);
}

unsigned long mgl_udp_header_get_data_length(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[4] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[5] & 0xff)));
	return l_val; 
}
void mgl_udp_header_set_data_length(char *op_buf, unsigned long i_val) 
{
	op_buf[4] = (char)((i_val>>8)& 0xff);
	op_buf[5] = (char)((i_val   )& 0xff);
}

unsigned long mgl_udp_header_get_crc(char *ip_buf) 
{
	unsigned long l_val = (((unsigned long)(ip_buf[6] & 0xff))<<8)
	           	         |(((unsigned long)(ip_buf[7] & 0xff)));
	return l_val; 
}
void mgl_udp_header_set_crc(char *op_buf, unsigned long i_val) 
{
	op_buf[6] = (char)((i_val>>8)& 0xff);
	op_buf[7] = (char)((i_val   )& 0xff);
}




// Data length max=65000
// IP/UDP MTU=1450
long mgl_ipv4_udp_segment_get_nb_packet(int i_data_buf_len)
{
	int l_len;
	int l_nb;

	if (i_data_buf_len>BLOC_IP_LEN_MAX) {
		// Pb: Data too huge
		return 0;
	}

	// If IP Header+UDP header+Data <= Lower layer MTU
	// Then the data fit in only one IP packet
	l_len= BLOC_IP_HEADER_LEN+BLOC_UDP_HEADER_LEN+i_data_buf_len;
	if (l_len<=BLOC_IP_LOWER_MTU) {
		return 1;
	}

	// If IP Header+UDP header+Data > Lower layer MTU
	// Then we build a first IP packet that is encapsulated in some other IP packets
	l_nb = (l_len/BLOC_IP_MTU)+1;
	return l_nb;
}

void mgl_ipv4_build_header(char *op_pkt_buf, int i_payload_len, int i_tos, int i_pkt_id, int i_flag, int i_fragment_offset, int i_ttl, int i_proto, int i_ip_src, int i_ip_dst)
{
	int l_crc;
	mgl_ip_header_set_version(op_pkt_buf, BLOC_IP_VERSION_4);
	mgl_ipv4_header_set_hlen(op_pkt_buf, BLOC_UDP_HEADER_LEN);
	mgl_ipv4_header_set_tos(op_pkt_buf, i_tos);
	mgl_ipv4_header_set_packet_length(op_pkt_buf, BLOC_IP_HEADER_LEN+i_payload_len);
	mgl_ipv4_header_set_id(op_pkt_buf, i_pkt_id);
	mgl_ipv4_header_set_flag(op_pkt_buf, i_flag);
	mgl_ipv4_header_set_fragment_offset(op_pkt_buf, i_fragment_offset);
	mgl_ipv4_header_set_ttl(op_pkt_buf, i_ttl);
	mgl_ipv4_header_set_proto(op_pkt_buf, i_proto);
	mgl_ipv4_header_set_ip_src(op_pkt_buf, i_ip_src);
	mgl_ipv4_header_set_ip_dst(op_pkt_buf, i_ip_dst);

	l_crc=0;
	mgl_ipv4_header_set_crc(op_pkt_buf, l_crc);
}

void mgl_udp_build_header(char *op_udp_buf, char *ip_buf, int i_buflen,
							  int i_udp_port_src, int i_udp_port_dst)
{
	mgl_udp_header_set_src_port(op_udp_buf, i_udp_port_src);
	mgl_udp_header_set_dst_port(op_udp_buf, i_udp_port_dst);
	mgl_udp_header_set_data_length(op_udp_buf, i_buflen);
	// Calculate UDP crc
	mgl_udp_header_set_crc(op_udp_buf, 0);

}

//return: -1, 0: Ko
// >0: IP packet length
int mgl_ipv4_udp_segment_build_packet(char *op_pkt_buf, int i_pkt_num, char *ip_buf, int i_buflen, 
									   int i_tos, int i_ttl, int i_ip_src, int i_ip_dst,
									   int i_udp_port_src, int i_udp_port_dst)

{
	int l_nb;
	static int s_cpt=0;

	// Get number of IP packets needed to carry the data
	l_nb = mgl_ipv4_udp_segment_get_nb_packet(i_buflen);

	if (l_nb<=0) {
		// Pb
		return -1;
	}
	if (l_nb==1) {
		// Nice, data are carried by only one single IP/UDP packet
		// copy datas
		memcpy(op_pkt_buf+BLOC_IP_HEADER_LEN+BLOC_UDP_HEADER_LEN, ip_buf, i_buflen);
		// Build headers
		mgl_udp_build_header(op_pkt_buf+BLOC_IP_HEADER_LEN, ip_buf, i_buflen,
			i_udp_port_src, i_udp_port_dst);
		mgl_ipv4_build_header(op_pkt_buf, i_buflen,
			i_tos, s_cpt++, 0, 0, i_ttl, BLOC_IP_PROTO_UDP, i_ip_src, i_ip_dst);
		return (i_buflen+BLOC_IP_HEADER_LEN+BLOC_UDP_HEADER_LEN);
	}
	if (l_nb>1) {
		// We need to build an IP in IP packet
		return 0;
	}


	return 0;
}


int mgl_ipv4_udp_segment_reassemble_data(char *op_buf, int i_buf_len, char *ip_pkt)
{
	int l_len;

	if (mgl_ip_header_get_version(ip_pkt)==BLOC_IP_VERSION_4) {
		if (mgl_ipv4_header_get_proto(ip_pkt)==BLOC_IP_PROTO_UDP) {
			l_len = mgl_udp_header_get_data_length(ip_pkt+BLOC_IP_HEADER_LEN);
			if (l_len<0) {
				// Invalid size
				return 0;
			}
			if (l_len>i_buf_len) {
				// Buffer too small
				return 0;
			}
			memcpy(op_buf, ip_pkt+BLOC_IP_HEADER_LEN+BLOC_UDP_HEADER_LEN, l_len); 
			return l_len;
		}
		if (mgl_ipv4_header_get_proto(ip_pkt)==BLOC_IP_PROTO_IPIP) {
			// Part of an IP in IP packet
			return 0;
		}
	}

	return 0;

}





void mgl_ip_dump_IPv4_address(unsigned long i_ip)
{
	printf("%lx.%lx.%lx.%lx", ((i_ip>>24)&0xff), ((i_ip>>16)&0xff), ((i_ip>>8)&0xff), ((i_ip)&0xff));
}

void mgl_ip_dump_packet(char *ip_packet, int i_packet_len)
{
	// Fields
	switch (mgl_ip_header_get_version(ip_packet)) {
	case BLOC_IP_VERSION_4:
		printf("IPv4 packet.\n");
		// Dump IPv4 fields
		printf("IPv4:Header Length =%lu.\n", mgl_ipv4_header_get_hlen(ip_packet));
		printf("IPv4:TOS           =%lu.\n", mgl_ipv4_header_get_tos(ip_packet));
		printf("IPv4:Packet Length =%lu.\n", mgl_ipv4_header_get_packet_length(ip_packet));
		printf("IPv4:Id            =%lu.\n", mgl_ipv4_header_get_id(ip_packet));
		printf("IPv4:Flags         =%lu.\n", mgl_ipv4_header_get_flag(ip_packet));
		printf("IPv4:Frag. Offset  =%lu.\n", mgl_ipv4_header_get_fragment_offset(ip_packet));
		printf("IPv4:TTL           =%lu.\n", mgl_ipv4_header_get_ttl(ip_packet));
		printf("IPv4:Proto         =%lu.\n", mgl_ipv4_header_get_proto(ip_packet));
		printf("IPv4:CRC           =%lu.\n", mgl_ipv4_header_get_crc(ip_packet));
		printf("IPv4:IP Src        ="); mgl_ip_dump_IPv4_address(mgl_ipv4_header_get_ip_src(ip_packet));
		printf("\nIPv4:IP Dst        ="); mgl_ip_dump_IPv4_address(mgl_ipv4_header_get_ip_dst(ip_packet));
		printf("\n");

		switch (mgl_ipv4_header_get_proto(ip_packet)) {
		case BLOC_IP_PROTO_UDP:
			printf("Proto UDP.\n");
			printf("UDP:Port Src    =%lu.\n", mgl_udp_header_get_src_port(ip_packet+BLOC_IP_HEADER_LEN));
			printf("UDP:Port Dst    =%lu.\n", mgl_udp_header_get_dst_port(ip_packet+BLOC_IP_HEADER_LEN));
			printf("UDP:Data Length =%lu.\n", mgl_udp_header_get_data_length(ip_packet+BLOC_IP_HEADER_LEN));
			printf("UDP:CRC         =%lu.\n", mgl_udp_header_get_crc(ip_packet+BLOC_IP_HEADER_LEN));
			break;
		case BLOC_IP_PROTO_TCP:
			printf("Proto TCP.\n");
			break;
		default:
			printf("Proto unknown.\n");
			break;
		}
		if (mgl_ipv4_header_get_proto(ip_packet)==BLOC_IP_PROTO_IPIP) {
			// Part of an IP in IP packet
		}
		break;
	case BLOC_IP_VERSION_6:
		printf("IPv6 packet.\n");
		break;

	default:
		printf("Unknown IP packet.\n");
		break;
	}

	// Raw dump


}


int mgl_ipv4_addr_cmp_with_mask(mgl_ip_addr i_ip1, mgl_ip_addr i_ip2, int i_mask_len)
{
	unsigned long l_mask = (1<<(i_mask_len-1))-1;
	unsigned long l_ip1=i_ip1.getV4()&l_mask;
	unsigned long l_ip2=i_ip2.getV4()&l_mask;
	if (i_mask_len==0) { return 0; }
	if (l_ip1==l_ip2) { return 0; }
	if (l_ip1>l_ip2) {
		return 1;
	} else {
		return -1;
	}
}



/*   
IP1: xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx
IP2: xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx.xx
Mask len in bits
     <---------------------------->
	 |  word    |     word  | mask   |
*/
int mgl_ipv6_addr_cmp_with_mask(mgl_ip_addr i_ip1, mgl_ip_addr i_ip2, int i_mask_len)
{
	int l_nbseg=((i_mask_len-1)/32)+1;
	unsigned long l_mask = (1<<((i_mask_len%32)-1))-1;
	unsigned long l_ip1;
	unsigned long l_ip2;
	int l_cpt;

	if (i_mask_len==0) { return 0; }
	for (l_cpt=0; l_cpt<l_nbseg; l_cpt++) {
		if (l_cpt==(l_nbseg-1)) {
			l_ip1=i_ip1.getV6W(l_cpt)&l_mask;
			l_ip2=i_ip2.getV6W(l_cpt)&l_mask;
		} else {
			l_ip1=i_ip1.getV6W(l_cpt);
			l_ip2=i_ip2.getV6W(l_cpt);
		}
		if (l_ip1>l_ip2) {
			return 1;
		} else {
			return -1;
		}
	}
	return 0;
}
