/*-
 * Copyright (c) 2014 SRI International
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LIBEXEC_TCPDUMP_HELPER_H_
#define _LIBEXEC_TCPDUMP_HELPER_H_

#define	TCPDUMP_HELPER_OP_INIT		0
#define	TCPDUMP_HELPER_OP_PRINT_PACKET	1
#define TCPDUMP_HELPER_OP_HAS_PRINTER	2

/* Netdissect dissectors */
#define TCPDUMP_HELPER_OP_EAP_PRINT		1000
#define TCPDUMP_HELPER_OP_ARP_PRINT		1001
#define TCPDUMP_HELPER_OP_TIPC_PRINT		1002
#define TCPDUMP_HELPER_OP_MSNLB_PRINT		1003
#define TCPDUMP_HELPER_OP_ICMP6_PRINT		1004
#define TCPDUMP_HELPER_OP_ISAKMP_PRINT		1005
#define TCPDUMP_HELPER_OP_ISAKMP_RFC3948_PRINT	1006
#define TCPDUMP_HELPER_OP_IP_PRINT		1007
#define TCPDUMP_HELPER_OP_IP_PRINT_INNER	1008
#define TCPDUMP_HELPER_OP_RRCP_PRINT		1009

/* Non-netdissect dissectors */
#define TCPDUMP_HELPER_OP_TCP_PRINT		1100

#endif /* _LIBEXEC_TCPDUMP_HELPER_H_ */
