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
/* $Id: lib_ip_udp_cx.h,v 1.1.1.1 2006/08/02 11:50:28 dbarvaux Exp $ */



#ifndef LIB_IP_UDP_CX_H
#define LIB_IP_UDP_CX_H

/****** System Includes ******/
#include <stdlib.h>
#include <string.h>

/****** Runtime Includes ******/
#include "mgl_ip_address.h"


int mgl_ipv4_addr_cmp_with_mask(mgl_ip_addr i_ip1, mgl_ip_addr i_ip2, int i_mask_len);
int mgl_ipv6_addr_cmp_with_mask(mgl_ip_addr i_ip1, mgl_ip_addr i_ip2, int i_mask_len);

#define BLOC_IP_HEADER_LEN			20      
#define BLOC_UDP_HEADER_LEN			8
#define BLOC_IP_LEN_MAX				65535
#define BLOC_IP_LOWER_MTU			1500
#define BLOC_IP_MTU					(BLOC_IP_LOWER_MTU-BLOC_IP_HEADER_LEN)

// IP Version
#define	BLOC_IP_VERSION_4			4	
#define	BLOC_IP_VERSION_6			6	

// Type Of Service flag
#define BLOC_IP_TOS_LOWDELAY		0x10
#define BLOC_IP_TOS_THROUGHPUT		0x08
#define BLOC_IP_TOS_RELIABILITY		0x04
#define BLOC_IP_TOS_ECT				0x02
#define BLOC_IP_TOS_CE				0x01

// Fragment flags
#define BLOC_IP_RF		0x04		/* reserved fragment */
#define BLOC_IP_DF		0x02		/* dont fragment */
#define BLOC_IP_MF		0x01		/* more fragments */

// Proto
#define	BLOC_IP_PROTO_IP		0		/* Dummy for IP */
#define	BLOC_IP_PROTO_ICMP		1		/* ICMP */
#define	BLOC_IP_PROTO_IGMP		2		/* IGMP */
#define	BLOC_IP_PROTO_IPIP		4		/* IP in IP */
#define	BLOC_IP_PROTO_TCP		6		/* TCP */
#define	BLOC_IP_PROTO_EGP		8		/* Exterior gateway protocol */
#define	BLOC_IP_PROTO_PUP		12		/* PUP */
#define	BLOC_IP_PROTO_UDP		17		/* UDP */
#define	BLOC_IP_PROTO_IDP		22		/* XNS IDP */
#define	BLOC_IP_PROTO_TP		29 		/* SO TP class 4 */
#define BLOC_IP_PROTO_IPV6		41		/* IPv6 */
#define BLOC_IP_PROTO_ROUTING	43		/* IPv6 routing header */
#define BLOC_IP_PROTO_FRAGMENT	44		/* IPv6 fragmentation header */
#define BLOC_IP_PROTO_RSVP		46		/* Reservation protocol */
#define	BLOC_IP_PROTO_GRE		47		/* GRE encap, RFCs 1701/1702 */
#define	BLOC_IP_PROTO_ESP		50		/* Encap. security payload */
#define	BLOC_IP_PROTO_AH		51		/* Authentication header */
#define	BLOC_IP_PROTO_MOBILE	55		/* IP Mobility, RFC 2004 */
#define BLOC_IP_PROTO_ICMPV6	58		/* ICMP for IPv6 */
#define BLOC_IP_PROTO_NONE		59		/* IPv6 no next header */
#define BLOC_IP_PROTO_DSTOPTS	60		/* IPv6 destination options */
#define	BLOC_IP_PROTO_EON		80		/* ISO CNLP */
#define BLOC_IP_PROTO_ETHERIP	97		/* Ethernet in IPv4 */
#define	BLOC_IP_PROTO_ENCAP		98		/* Encapsulation header */
#define BLOC_IP_PROTO_PIM		103		/* Protocol indep. multicast */
#define BLOC_IP_PROTO_IPCOMP	108		/* Compression header proto */
#define	BLOC_IP_PROTO_RAW		255		/* Raw IP packets */


//
// Get IP fields
//
unsigned long mgl_ipv4_header_get_ip_src(char *ip_buf);
unsigned long mgl_ipv4_header_get_ip_dst(char *ip_buf);

void mgl_ip_header_set_version(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_hlen(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_tos(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_packet_length(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_id(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_flag(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_fragment_offset(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_ttl(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_proto(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_crc(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_ip_src(char *op_buf, unsigned long i_val);
void mgl_ipv4_header_set_ip_dst(char *op_buf, unsigned long i_val);


unsigned long mgl_ip_header_get_version(char *ip_buf);
unsigned long mgl_ipv4_header_get_hlen(char *ip_buf);
unsigned long mgl_ipv4_header_get_tos(char *ip_buf);
unsigned long mgl_ipv4_header_get_packet_length(char *ip_buf);
unsigned long mgl_ipv4_header_get_id(char *ip_buf);
unsigned long mgl_ipv4_header_get_flag(char *ip_buf);
unsigned long mgl_ipv4_header_get_fragment_offset(char *ip_buf);
unsigned long mgl_ipv4_header_get_ttl(char *ip_buf);
unsigned long mgl_ipv4_header_get_proto(char *ip_buf);
unsigned long mgl_ipv4_header_get_crc(char *ip_buf);
unsigned long mgl_ipv4_header_get_ip_src(char *ip_buf);
unsigned long mgl_ipv4_header_get_ip_dst(char *ip_buf);

mgl_ip_addr mgl_ipv6_header_get_ip_dst(char *ip_buf);


void mgl_udp_header_set_src_port(char *op_buf, unsigned long i_val) ;
void mgl_udp_header_set_dst_port(char *op_buf, unsigned long i_val) ;
void mgl_udp_header_set_data_length(char *op_buf, unsigned long i_val);
void mgl_udp_header_set_crc(char *op_buf, unsigned long i_val);

unsigned long mgl_udp_header_get_src_port(char *ip_buf);
unsigned long mgl_udp_header_get_dst_port(char *ip_buf);
unsigned long mgl_udp_header_get_data_length(char *ip_buf) ;
unsigned long mgl_udp_header_get_crc(char *ip_buf);

long mgl_ipv4_udp_segment_get_nb_packet(int i_data_buf_len);

void mgl_ipv4_build_header(char *op_pkt_buf, int i_payload_len, int i_tos, int i_pkt_id, int i_flag, int i_fragment_offset, int i_ttl, int i_proto, int i_ip_src, int i_ip_dst);
void mgl_udp_build_header(char *op_udp_buf, char *ip_buf, int i_buflen,
							  int i_udp_port_src, int i_udp_port_dst);

int  mgl_ipv4_udp_segment_build_packet(char *op_pkt_buf, int i_pkt_num, char *ip_buf, int i_buflen, 
									   int i_tos, int i_ttl, int i_ip_src, int i_ip_dst,
									   int i_udp_port_src, int i_udp_port_dst);
int  mgl_ipv4_udp_segment_reassemble_data(char *op_buf, int i_buf_len, char *ip_pkt);

void mgl_ip_dump_IPv4_address(unsigned long i_ip);
void mgl_ip_dump_packet(char *ip_packet, int i_packet_len);

#endif



