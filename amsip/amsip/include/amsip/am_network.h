/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_NETWORK_H__
#define __AM_NETWORK_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>
	
#include <ortp/stun_udp.h>
#include <ortp/stun.h>
	
#include <mediastreamer2/ice.h>

/**
 * @file am_network.h
 * @brief amsip network API
 *
 * This file provide the API needed to control network.
 *
 * <ul>
 * <li>start network layer (UDP/TCP)</li>
 * <li>Use Stun to determine NAT</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_network amsip network interface
 * @ingroup amsip_setup
 * @{
 */

	struct auto_setup {
		char sip_identity[256];
		char sip_password[256];
		char sip_realm[256];
		char sip_proxy[256];
		int sip_proxy_port;
		char firewall_ip[256];
		int firewall_port;
		int firewall_type;
		int sip_test;
	};

	struct stun_test {
		int local_port;

		char nat_info[256];
		char firewall_ip[256];
#ifdef WIN32
		enum NatType nat_type;
#else
		NatType nat_type;
#endif
		int preserve_port;
#ifdef WIN32
		enum NatType secondary_nat_type;
#else
		NatType secondary_nat_type;
#endif
		int secondary_preserve_port;
		int hairpin;
		int could_become_symmetric;	/* become symmetric when initial packet comes from the outside */

		int use_ice;
		int use_stun_server;
		int use_turn_server;

		int use_stun_mapped_ip;
		int use_stun_mapped_port;
		int use_setup_both;
		int use_symmetric_rtp;

		int remote_user_is_public;
		int remote_user_support_ice;

		struct SdpCandidate local_candidate[10];
		struct SdpCandidate stun_candidate[10];
		struct SdpCandidate turn_candidate[10];
	};

/**
 * Find default address of interface to reach the defined gateway.
 *
 * @param family    AF_INET or AF_INET6
 * @param address   a string containing the local IP address.
 * @param size      The size of the string
 */
	PPL_DECLARE (int) am_network_guess_ip(int family, char *address,
										  int size);

/**
 * Configure amsip to do masquerading on SIP Contact address.
 * at run time.
 *
 * PLEASE DO NOT USE UNLESS YOU REALLY UNDERSTAND THIS OPTION!
 * THIS OPTIONS CANNOT BE USED IF YOU CONFIGURED THE STACK
 * WITH STUN or ICE SUPPORT.
 *
 * @param IP            IP to appear in Contact.
 * @param port          port to appear in Contact.
 */
	PPL_DECLARE (int) am_network_masquerade(const char *ip, int port);

/**
 * Configure amsip to listen on a specific transport layer.
 *
 * @param transport     Transport Protocol to use ("UDP", "TCP", "TLS")
 * @param port          Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_start(const char *transport, int port);

/**
 * Test NAT type.
 *
 * @param stuntest      element to receive all stun test.
 * @param stun_server   Stun server to use for NAT traversal solution.
 * @param srcport       Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_connection_tests(struct stun_test
												  *stuntest,
												  char *stun_server,
												  int srcport);
/**
 * Add local candidates.
 *
 * @param _candidates      Table for 10 local candidates.
 * @param srcport          Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_add_local_candidates(struct SdpCandidate
													  *_candidates,
													  int srcport);
/**
 * Add Stun candidate.
 *
 * @param candidates       candidates to receive all stun candidates.
 * @param stun_server      Stun server to use for NAT traversal solution.
 * @param srcport          Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_add_stun_candidates(struct SdpCandidate
													 *candidates, struct SdpCandidate
													 *localcandidates,
													 char *stun_server,
													 int srcport);
/**
 * Add TURN candidate.
 *
 * @param candidates       candidates to receive all turn candidates.
 * @param turn_server      TURN server to use for NAT traversal solution.
 * @param srcport          Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_add_turn_candidates(struct SdpCandidate
													 *candidates,
													 char *turn_server,
													 int srcport);
/**
 * Add RELAY candidate.
 *
 * @param candidates       candidates to receive all turn candidates.
 * @param relay_server     STUN server to use for NAT traversal solution.
 * @param srcport          Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_add_relay_candidates(struct SdpCandidate
													  *candidates,
													  char *relay_server,
													  int srcport);
/**
 * Test NAT type.
 *
 * @param stun_server   Stun server to use for NAT traversal solution.
 * @param port          Local port to use for socket. (0 for random port)
 * @param proxy         SIP Proxy Address (used to determine which interface to test)
 */
	PPL_DECLARE (char *) am_network_test_nat(const char *stun_server,
											 int port, const char *proxy);
/**
 * Return text information on NAT type.
 *
 * @param stun_server   Stun server to use for NAT traversal solution.
 * @param srcport       Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (char *) am_network_get_nat_info(const char *stun_server,
												 int srcport);
/**
 * Return binary information on NAT type.
 *
 * @param stun_server   Stun server to use for NAT traversal solution.
 * @param srcport       Local port to use for socket. (0 for random port)
 */
	PPL_DECLARE (int) am_network_get_nat(char *stun_server, int srcport);

	int _am_network_get_nat(struct stun_test *stuntest,
							const char *stun_server, int srcport);

	int _am_network_get_stun_socketpair(char *stun_server, int srcport,
										char *firewall, int *rtp_port,
										int *rtcp_port, int *_fd,
										int *_fd_rtcp);
	int _am_network_get_turn_socketpair(char *turn_server, int srcport,
										char *firewall, int *rtp_port,
										int *rtcp_port, int *_fd,
										int *_fd_rtcp);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
