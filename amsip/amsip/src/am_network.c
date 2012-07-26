/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#if defined(WIN32) || defined(_WIN32_WCE)
#include <windowsx.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Iphlpapi.h>
#endif

#include "amsip/am_options.h"
#include "amsip/am_network.h"
#include <osipparser2/osip_port.h>

#include <eXosip2/eXosip.h>

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifndef WIN32
#ifndef closesocket
#define closesocket close
#endif
#endif

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

PPL_DECLARE (int) am_network_guess_ip(int family, char *address, int size)
{
	return eXosip_guess_localip(family, address, size);
}

PPL_DECLARE (int)
am_network_connection_tests(struct stun_test *stuntest, char *stun_server,
							int srcport)
{
	int port = 0;
	int i;
	int fd;
	StunAddress4 stunServerAddr;

	memset(stuntest, 0, sizeof(struct stun_test));
	stuntest->use_symmetric_rtp = 1;

	stunServerAddr.addr = 0;
	stunServerAddr.port = 0;
	stunParseServerName(stun_server, &stunServerAddr);
	if (stunServerAddr.addr == 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "STUN detection (%s): not a valid stun server \r\n",
							  stun_server));

		return -1;
	}

	stuntest->local_port = srcport;

	/* CHECK 1 */
	/* get firewall IP address. */
	fd = _amsip_get_stun_socket(stun_server, srcport,
								stuntest->firewall_ip, &port);
	if (fd > 0) {
		closesocket(fd);
	}

	/* !CHECK 1 */
	if (stuntest->firewall_ip != NULL && stuntest->firewall_ip[0] != '\0') {
		struct sockaddr_in addr;

		fd = (int) socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (fd > 0) {
			memset((char *) &(addr), 0, sizeof((addr)));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(stuntest->firewall_ip);
			addr.sin_port = htons(0);
			if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
				/* address is the firewall address */
			} else {
				closesocket(fd);
				stuntest->nat_type = StunTypeOpen;
				stuntest->secondary_nat_type = StunTypeOpen;
				stuntest->preserve_port = 1;
				stuntest->could_become_symmetric = 0;
				stuntest->use_stun_server = 0;
				stuntest->use_stun_mapped_ip = 0;
				stuntest->use_stun_mapped_port = 0;
				stuntest->use_setup_both = 0;
				stuntest->use_ice = 0;
				stuntest->use_turn_server = 0;
				return 0;
			}
		}
	}

	/* CHECK 2 */
	/* get NAT information  */
	i = _am_network_get_nat(stuntest, stun_server, srcport);
	if (stuntest->nat_info[0] != '\0') {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "STUN detection (%s): %s\r\n", stun_server,
							  stuntest->nat_info));
	} else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "STUN detection failure (%s)\r\n",
							  stun_server));
	}
	/* !CHECK 2 */

#if 0
	/* CHECK 3 */
	/* check if port restricted can become symmetric (iptable default?) */
	switch (stuntest->nat_type) {
	case StunTypeRestrictedNat:
	case StunTypePortRestrictedNat:
		stuntest->could_become_symmetric = 0;
		stuntest->secondary_nat_type = stuntest->nat_type;
		if (stuntest->preserve_port == 1) {
			StunAddress4 stunServerAddr2;
			StunAddress4 sChangeAddr;
			StunAddress4 sMappedAddr;
			StunAddress4 sAddr;
			int randomsrcport;

			sAddr.addr = 0;
			if (srcport != 0)
				sAddr.port = srcport + 2;
			else
				sAddr.port = 0;

			stunServerAddr2.port = stunServerAddr.port;
			stunServerAddr2.addr = stunServerAddr.addr;

			/* Test1 and get changed-address */
			i = stunTest(&stunServerAddr2, 1, &sAddr, &sMappedAddr,
						 &sChangeAddr);
			if (i >= 0 && sMappedAddr.port == sAddr.port) {
				randomsrcport = sAddr.port;
				stuntest->preserve_port = 1;
				/* Test2 (stream from IP2 of STUN server should be refused) */
				i = stunTest(&stunServerAddr2, 2, &sAddr, NULL, NULL);
				if (i < 0) {
					sAddr.port = randomsrcport;
					/* Test if new Test1 on IP2 (equivalent to test10) preserve ports. */
					stunServerAddr2.addr = sChangeAddr.addr;
					/* keep same port stunServerAddr.port = sChangeAddr.port; */

					i = stunTest(&stunServerAddr2, 1, &sAddr, &sMappedAddr,
								 NULL);
					if (i >= 0 && randomsrcport != sMappedAddr.port) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_WARNING, NULL,
									"STUN detection (%s): seems like NAT could be symmetric (srcport: %i mapped_port(ip1): %i mapped_port(ip2)=%i \r\n",
									stun_server, randomsrcport,
									randomsrcport, sMappedAddr.port));
						stuntest->could_become_symmetric = 1;
						stuntest->secondary_nat_type = StunTypeSymNat;
					}
				}
			}
		}
		break;

	case StunTypeSymNat:
	case StunTypeSymFirewall:
		stuntest->could_become_symmetric = 1;
		stuntest->secondary_nat_type = stuntest->nat_type;
		break;

	case StunTypeFailure:
	case StunTypeUnknown:
	case StunTypeOpen:
	case StunTypeConeNat:
	case StunTypeBlocked:
	default:
		stuntest->could_become_symmetric = 0;
		stuntest->secondary_nat_type = stuntest->nat_type;
		break;
	}

	/* !CHECK 3 */
#else
	stuntest->could_become_symmetric = 0;
	stuntest->secondary_nat_type = stuntest->nat_type;
	switch (stuntest->nat_type) {
	case StunTypeSymNat:
	case StunTypeSymFirewall:
		stuntest->could_become_symmetric = 1;
		stuntest->secondary_nat_type = stuntest->nat_type;
		break;
	case StunTypeRestrictedNat:
	case StunTypePortRestrictedNat:
		stuntest->could_become_symmetric = 1;
		stuntest->secondary_nat_type = StunTypeSymNat;
	default:
		stuntest->could_become_symmetric = 0;
		stuntest->secondary_nat_type = stuntest->nat_type;
		break;
	}
#endif

	switch (stuntest->nat_type) {
	case StunTypeFailure:
	case StunTypeUnknown:
		stuntest->use_stun_server = 0;
		stuntest->use_stun_mapped_ip = 0;
		stuntest->use_stun_mapped_port = 0;
		break;
	case StunTypeOpen:
		stuntest->use_stun_server = 0;
		stuntest->use_stun_mapped_ip = 0;
		stuntest->use_stun_mapped_port = 0;
		stuntest->use_setup_both = 0;
		break;
	case StunTypeConeNat:
		stuntest->use_stun_mapped_ip = 1;
		stuntest->use_stun_mapped_port = 0;
		if (stuntest->preserve_port == 0) {
			stuntest->use_stun_server = 1;
			stuntest->use_stun_mapped_port = 1;
		}
		stuntest->use_setup_both = 0;
		break;
	case StunTypeRestrictedNat:
	case StunTypePortRestrictedNat:
		stuntest->use_stun_server = 1;
		stuntest->use_stun_mapped_ip = 1;
		stuntest->use_stun_mapped_port = 1;
		if (stuntest->secondary_nat_type == StunTypeSymNat) {
			stuntest->use_setup_both = 1;
		}
		stuntest->use_setup_both = 1;
		break;
	case StunTypeSymNat:
		stuntest->use_stun_server = 1;
		stuntest->use_stun_mapped_ip = 1;
		stuntest->use_stun_mapped_port = 1;
		stuntest->use_setup_both = 1;
		break;
	case StunTypeSymFirewall:
		stuntest->use_stun_server = 1;
		stuntest->use_stun_mapped_ip = 1;
		stuntest->use_stun_mapped_port = 1;
		stuntest->use_setup_both = 1;
		break;
	case StunTypeBlocked:
		stuntest->use_stun_server = 0;
		stuntest->use_setup_both = 1;
		break;
	default:
		stuntest->use_stun_server = 0;
		stuntest->use_stun_mapped_ip = 0;
		stuntest->use_stun_mapped_port = 0;
		snprintf(stuntest->nat_info, 256, "%s", "unknown NAT type (?)");
		break;
	}

	return 0;
}

#ifdef WIN32

PPL_DECLARE (int)
am_network_add_local_candidates(struct SdpCandidate *_candidates,
								int srcport)
{
	int i;
	unsigned int pos;
	unsigned int pos_candidate;

	/* First, try to get the interface where we should listen */
	DWORD size_of_iptable = 0;
	PMIB_IPADDRTABLE ipt;
	PMIB_IFROW ifrow;

	struct SdpCandidate *candidates = _candidates;
	memset(candidates, 0, sizeof(struct SdpCandidate) * 10);
	pos_candidate = 1;

	i = eXosip_guess_localip(AF_INET, candidates[0].conn_addr,
							 sizeof(candidates[0].conn_addr));
	if (i == 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Default ethernet interface: %s\r\n",
					candidates[0].conn_addr));
		candidates[0].foundation = 1;
		candidates[0].component_id = 1;
		candidates[0].conn_port = srcport;
		candidates[0].priority = (126 << 24) | (255 << 16) | (255 << 8)
			| (256 - candidates[0].component_id);
		//candidates[0].peer_derived = 0;
		snprintf(candidates[0].cand_type,
				 sizeof(candidates[0].cand_type), "host");

		snprintf(candidates[0].transport,
				 sizeof(candidates[0].transport), "UDP");
	}

	if (GetIpAddrTable(NULL, &size_of_iptable, TRUE) ==
		ERROR_INSUFFICIENT_BUFFER) {
		ifrow = (PMIB_IFROW) _alloca(sizeof(MIB_IFROW));
		ipt = (PMIB_IPADDRTABLE) _alloca(size_of_iptable);
		if (ifrow == NULL || ipt == NULL) {
			/* not very usefull to continue */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO4, NULL,
								  "ERROR alloca failed\r\n"));
			return -1;
		}

		if (!GetIpAddrTable(ipt, &size_of_iptable, TRUE)) {
			/* look for the best public interface */

			for (pos = 0; pos < ipt->dwNumEntries; ++pos) {
				/* index is */
				struct in_addr addr;
				struct in_addr mask;

				ifrow->dwIndex = ipt->table[pos].dwIndex;
				if (GetIfEntry(ifrow) == NO_ERROR) {
					switch (ifrow->dwType) {
					case MIB_IF_TYPE_LOOPBACK:
						/*      break; */
					case MIB_IF_TYPE_ETHERNET:
					default:
						addr.s_addr = ipt->table[pos].dwAddr;
						mask.s_addr = ipt->table[pos].dwMask;
						if (addr.s_addr != 0) {
							candidates[pos_candidate].foundation =
								candidates[pos_candidate - 1].foundation +
								1;
							candidates[pos_candidate].component_id = 1;
							snprintf(candidates[pos_candidate].conn_addr,
									 sizeof(candidates[pos_candidate].
											conn_addr), "%s",
									 inet_ntoa(addr));

							candidates[pos_candidate].priority =
								(126 << 24) | (255 << 16) |
								((255 - pos_candidate) << 8)
								| (256 -
								   candidates[pos_candidate].component_id);
							candidates[pos_candidate].conn_port = srcport;
							//candidates[pos_candidate].peer_derived = 0;
							snprintf(candidates[pos_candidate].cand_type,
									 sizeof(candidates[pos_candidate].
											cand_type), "host");
							snprintf(candidates[pos_candidate].transport,
									 sizeof(candidates[pos_candidate].
											transport), "UDP");

							if (osip_strcasecmp
								(candidates[pos_candidate].conn_addr,
								 "0.0.0.0") == 0
								||
								osip_strcasecmp
								(candidates[pos_candidate].conn_addr,
								 "127.0.0.1") == 0
								||
								osip_strcasecmp(candidates
												[pos_candidate].conn_addr,
												candidates[0].conn_addr) ==
								0) {
								OSIP_TRACE(osip_trace
										   (__FILE__, __LINE__, OSIP_INFO2,
											NULL,
											"Unused Secondary ethernet interface: %s\r\n",
											candidates[pos_candidate].
											conn_addr));
								candidates[pos_candidate].foundation = 0;
								candidates[pos_candidate].component_id = 0;
								candidates[pos_candidate].conn_addr[0] =
									'\0';
								candidates[pos_candidate].conn_port = 0;
								candidates[pos_candidate].priority = 0;
								candidates[pos_candidate].transport[0] =
									'\0';
								//candidates[pos_candidate].peer_derived = 0;
								candidates[pos_candidate].cand_type[0] =
									'\0';
								candidates[pos_candidate].rel_addr[0] =
									'\0';
								candidates[pos_candidate].rel_port = 0;
							} else {
								OSIP_TRACE(osip_trace
										   (__FILE__, __LINE__, OSIP_INFO2,
											NULL,
											"Secondary ethernet interface: %s\r\n",
											candidates[pos_candidate].
											conn_addr));
								pos_candidate++;
							}
						}
						break;
					}
				}
			}
		}
	}

	if (pos_candidate == 1 && candidates[0].conn_addr[0] == '\0') {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "ERROR No network interface found\r\n"));
		return -1;
	}

	return 0;
}

#elif defined(HAVE_GETIFADDRS)

#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>

PPL_DECLARE (int)
am_network_add_local_candidates(struct SdpCandidate *_candidates,
								int srcport)
{
	int i;
	unsigned int pos_candidate;

	struct ifaddrs *ifap, *ifa;

	/* First, try to get the interface where we should listen */
	struct SdpCandidate *candidates = _candidates;
	memset(candidates, 0, sizeof(struct SdpCandidate) * 10);
	pos_candidate = 1;

	i = eXosip_guess_localip(AF_INET, candidates[0].conn_addr,
							 sizeof(candidates[0].conn_addr));
	if (i == 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Default ethernet interface: %s\r\n",
					candidates[0].conn_addr));
		candidates[0].foundation = 1;
		candidates[0].component_id = 1;
		candidates[0].conn_port = srcport;
		candidates[0].priority = (126 << 24) | (255 << 16) | ((255) << 8)
			| (256 - candidates[0].component_id);
		//candidates[0].peer_derived = 0;
		snprintf(candidates[0].cand_type,
				 sizeof(candidates[0].cand_type), "host");

		snprintf(candidates[0].transport,
				 sizeof(candidates[0].transport), "UDP");
	}

	if (getifaddrs(&ifap))
		return -1;				/* cannot fetch interfaces */


	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
			/* skip non AF_INET interface */
			continue;
		
		if ((ifa->ifa_flags & (IFF_LOOPBACK)) == (IFF_LOOPBACK)){
			ms_message("skipping loopback");
			continue;
		}
		
		if (!((ifa->ifa_flags & (IFF_UP | IFF_RUNNING | IFF_LOOPBACK)) == (IFF_RUNNING | IFF_UP))) {
			ms_message("skipping network interface flag=%x", ifa->ifa_flags);
			continue;
		}
		
		candidates[pos_candidate].foundation =
			candidates[pos_candidate - 1].foundation + 1;
		candidates[pos_candidate].component_id = 1;
		snprintf(candidates[pos_candidate].conn_addr,
				 sizeof(candidates[pos_candidate].conn_addr), "%s",
				 inet_ntoa(((struct sockaddr_in *) ifa->ifa_addr)->
						   sin_addr));

		candidates[pos_candidate].priority =
			(126 << 24) | (255 << 16) | ((255 - pos_candidate) << 8)
			| (256 - candidates[pos_candidate].component_id);
		candidates[pos_candidate].conn_port = srcport;
		//candidates[pos_candidate].peer_derived = 0;
		snprintf(candidates[pos_candidate].cand_type,
				 sizeof(candidates[pos_candidate].cand_type), "host");
		snprintf(candidates[pos_candidate].transport,
				 sizeof(candidates[pos_candidate].transport), "UDP");

		if (osip_strcasecmp(candidates[pos_candidate].conn_addr, "0.0.0.0") == 0
			|| osip_strcasecmp(candidates[pos_candidate].conn_addr, "127.0.0.1") == 0
			|| osip_strcasecmp(candidates[pos_candidate].conn_addr,
							   candidates[0].conn_addr) == 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Unused Secondary ethernet interface: %s\r\n",
						candidates[pos_candidate].conn_addr));
			candidates[pos_candidate].foundation = 0;
			candidates[pos_candidate].component_id = 0;
			candidates[pos_candidate].conn_addr[0] = '\0';
			candidates[pos_candidate].conn_port = 0;
			candidates[pos_candidate].priority = 0;
			candidates[pos_candidate].transport[0] = '\0';
			//candidates[pos_candidate].peer_derived = 0;
			candidates[pos_candidate].cand_type[0] = '\0';
			candidates[pos_candidate].rel_addr[0] = '\0';
			candidates[pos_candidate].rel_port = 0;
		} else {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Secondary ethernet interface: %s\r\n",
						candidates[pos_candidate].conn_addr));
			pos_candidate++;
		}
	}

	freeifaddrs(ifap);

	if (pos_candidate == 1 && candidates[0].conn_addr[0] == '\0') {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "ERROR No network interface found\r\n"));
		return -1;
	}

	return 0;
}

#else


#if defined(__sun__)
#include <sys/sockio.h>
#endif

#include <sys/ioctl.h>
#include <net/if.h>

PPL_DECLARE (int)
am_network_add_local_candidates(struct SdpCandidate *_candidates,
								int srcport)
{
	int i;
	unsigned int pos_candidate;

	struct ifconf netconf;

	//struct ifaddrs *ifap, *ifa;
	int sock, err, if_count = 0;
	char tmp[160];

	/* First, try to get the interface where we should listen */
	struct SdpCandidate *candidates = _candidates;
	memset(candidates, 0, sizeof(struct SdpCandidate) * 10);
	pos_candidate = 1;

	i = eXosip_guess_localip(AF_INET, candidates[0].conn_addr,
							 sizeof(candidates[0].conn_addr));
	if (i == 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"Default ethernet interface: %s\r\n",
					candidates[0].conn_addr));
		candidates[0].foundation = 1;
		candidates[0].component_id = 1;
		candidates[0].conn_port = srcport;
		candidates[0].priority = (126 << 24) | (255 << 16) | ((255) << 8)
			| (256 - candidates[0].component_id);
		//candidates[0].peer_derived = 0;
		snprintf(candidates[0].cand_type,
				 sizeof(candidates[0].cand_type), "host");
		snprintf(candidates[0].transport,
				 sizeof(candidates[0].transport), "UDP");
	}

	netconf.ifc_len = 160;
	netconf.ifc_buf = tmp;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	err = ioctl(sock, SIOCGIFCONF, &netconf);
	if (err < 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "ioctl failed! cannot detect the ip address!\n"));
		close(sock);
		return -1;
	}
	close(sock);
	if_count = netconf.ifc_len / 32;

	for (i = 0; i < if_count; i++) {
		char *atmp;

		//if (strcmp (netconf.ifc_req[i].ifr_name, "lo") != 0)
		//continue;

		atmp =
			inet_ntoa(((struct sockaddr_in *) (&netconf.
											   ifc_req[i].ifr_addr))->
					  sin_addr);

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "candidate ip: %s!\n", atmp));

		candidates[pos_candidate].foundation =
			candidates[pos_candidate - 1].foundation + 1;
		candidates[pos_candidate].component_id = 1;
		snprintf(candidates[pos_candidate].conn_addr,
				 sizeof(candidates[pos_candidate].conn_addr), "%s", atmp);
		candidates[pos_candidate].priority =
			(126 << 24) | (255 << 16) | ((255 - pos_candidate) << 8)
			| (256 - candidates[pos_candidate].component_id);
		candidates[pos_candidate].conn_port = srcport;
		//candidates[pos_candidate].peer_derived = 0;
		snprintf(candidates[pos_candidate].cand_type,
				 sizeof(candidates[pos_candidate].cand_type), "host");
		snprintf(candidates[pos_candidate].transport,
				 sizeof(candidates[pos_candidate].transport), "UDP");

		if (osip_strcasecmp(candidates[pos_candidate].conn_addr, "0.0.0.0") == 0
			|| osip_strcasecmp(candidates[pos_candidate].conn_addr, "127.0.0.1") == 0
			|| osip_strcasecmp(candidates[pos_candidate].conn_addr,
							   candidates[0].conn_addr) == 0) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Unused Secondary ethernet interface: %s\r\n",
						candidates[pos_candidate].conn_addr));
			candidates[pos_candidate].foundation = 0;
			candidates[pos_candidate].component_id = 1;
			candidates[pos_candidate].conn_addr[0] = '\0';
			candidates[pos_candidate].conn_port = 0;
			candidates[pos_candidate].priority = 0;
			candidates[pos_candidate].transport[0] = '\0';
			//candidates[pos_candidate].peer_derived = 0;
			candidates[pos_candidate].cand_type[0] = '\0';
			candidates[pos_candidate].rel_addr[0] = '\0';
			candidates[pos_candidate].rel_port = 0;
		} else {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_INFO2, NULL,
						"Secondary ethernet interface: %s\r\n",
						candidates[pos_candidate].conn_addr));
			pos_candidate++;
		}
	}

	if (pos_candidate == 1 && candidates[0].conn_addr[0] == '\0') {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "ERROR No network interface found\r\n"));
		return -1;
	}

	return 0;
}

#endif

PPL_DECLARE (int)
am_network_add_stun_candidates(struct SdpCandidate *candidates,
							   struct SdpCandidate *localcandidates,
							   char *stun_server, int srcport)
{
	char firewall_ip[256];
	int port = 5060;
	int port2 = 5061;
	int i;

	int fd_rtp;
	int fd_rtcp;
	unsigned int pos;
	unsigned int pos2;

	for (pos = 0; pos < 10 && candidates[pos].conn_addr[0] != '\0'; pos++) {
	}

	if (pos >= 10) {
		/* stun is disabled */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "candidate table for STUN is full\r\n"));
		return 0;
	}

	i = _am_network_get_stun_socketpair(stun_server, srcport, firewall_ip,
										&port, &port2, &fd_rtp, &fd_rtcp);
	if (i == 0) {
		/* stun is disabled */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "STUN is disabled - no candidate search\r\n"));
		return 0;
	} else if (i < 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "STUN detection failed - no candidate answer\r\n"));
		return -1;
	} else {
		candidates[pos].foundation = 10;
		candidates[pos].component_id = 1;
		candidates[pos].rel_port = srcport;
		eXosip_guess_localip(AF_INET, candidates[pos].rel_addr,
							 sizeof(candidates[pos].rel_addr));
		candidates[pos].conn_port = port;
		snprintf(candidates[pos].conn_addr,
				 sizeof(candidates[pos].conn_addr), "%s", firewall_ip);
		/* closesocket(fd_rtp); already done in _am_network_get_stun_socketpair */
		/* closesocket(fd_rtcp); already done in _am_network_get_stun_socketpair  */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Add candidate stun:%i -> %s:%i\r\n",
							  candidates[pos].rel_port,
							  candidates[pos].conn_addr,
							  candidates[pos].conn_port));
		candidates[pos].priority = (100 << 24) | (255 << 16) | ((255) << 8)
			| (256 - candidates[pos].component_id);
		//candidates[pos].peer_derived = 1;
		snprintf(candidates[pos].cand_type,
				 sizeof(candidates[pos].cand_type), "srflx");
		snprintf(candidates[pos].transport,
				 sizeof(candidates[pos].transport), "UDP");

		for (pos2 = 0;
			 pos2 < 10 && localcandidates[pos2].conn_addr[0] != '\0';
			 pos2++) {
			if (osip_strcasecmp
				(localcandidates[pos2].conn_addr,
				 candidates[pos].conn_addr) == 0
				&& localcandidates[pos2].conn_port ==
				candidates[pos].conn_port) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"Remove duplicate candidate stun:%i -> %s:%i\r\n",
							candidates[pos].rel_port,
							candidates[pos].conn_addr,
							candidates[pos].conn_port));
				candidates[pos].foundation = 0;
				candidates[pos].component_id = 1;
				candidates[pos].conn_addr[0] = '\0';
				candidates[pos].conn_port = 0;
				candidates[pos].priority = 0;
				candidates[pos].transport[0] = '\0';
				//candidates[pos].peer_derived = 0;
				candidates[pos].cand_type[0] = '\0';
				candidates[pos].rel_addr[0] = '\0';
				candidates[pos].rel_port = 0;
				return 0;
			}
		}
	}


	return 1;
}

PPL_DECLARE (int)
am_network_add_turn_candidates(struct SdpCandidate *candidates,
							   char *turn_server, int srcport)
{
	char firewall_ip[256];
	int port = 5060;
	int port2 = 5061;
	int i;

	int fd_rtp;
	int fd_rtcp;
	unsigned int pos;

	for (pos = 0; pos < 10 && candidates[pos].conn_addr[0] != '\0'; pos++) {
	}

	if (pos >= 10) {
		/* stun is disabled */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "candidate table for TURN is full\r\n"));
		return 0;
	}

	i = _am_network_get_stun_socketpair(turn_server, srcport, firewall_ip,
										&port, &port2, &fd_rtp, &fd_rtcp);
	if (i == 0) {
		/* stun is disabled */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "TURN is disabled - no candidate search\r\n"));
		return 0;
	} else if (i < 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "TURN detection failed - no candidate answer\r\n"));
		return -1;
	} else {
		candidates[pos].foundation = 11;
		candidates[pos].component_id = 1;
		candidates[pos].rel_port = srcport;
		eXosip_guess_localip(AF_INET, candidates[pos].rel_addr, sizeof(candidates[pos].rel_addr));	/* which address: turn server or mine? */
		candidates[pos].conn_port = port;
		snprintf(candidates[pos].conn_addr,
				 sizeof(candidates[pos].conn_addr), "%s", firewall_ip);
		/* closesocket(fd_rtp); already done in _am_network_get_stun_socketpair */
		/* closesocket(fd_rtcp); already done in _am_network_get_stun_socketpair  */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Add turn candidate:%i -> %s:%i\r\n",
							  candidates[pos].rel_port,
							  candidates[pos].conn_addr,
							  candidates[pos].conn_port));
		candidates[pos].priority = (0 << 24) | (255 << 16) | ((255) << 8)
			| (256 - candidates[pos].component_id);
		//candidates[pos].peer_derived = 2;
		snprintf(candidates[pos].cand_type,
				 sizeof(candidates[pos].cand_type), "relay");
		snprintf(candidates[pos].transport,
				 sizeof(candidates[pos].transport), "UDP");
	}

	return 1;
}


PPL_DECLARE (int)
am_network_add_relay_candidates(struct SdpCandidate *candidates,
								char *relay_server, int srcport)
{
	char firewall_ip[256];
	int port = 5060;
	int port2 = 5061;
	int i;

	int fd_rtp;
	int fd_rtcp;
	unsigned int pos;

	for (pos = 0; pos < 10 && candidates[pos].conn_addr[0] != '\0'; pos++) {
	}

	if (pos >= 10) {
		/* stun is disabled */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "candidate table for TURN is full\r\n"));
		return 0;
	}

	i = _am_network_get_turn_socketpair(relay_server, srcport, firewall_ip,
										&port, &port2, &fd_rtp, &fd_rtcp);
	if (i == 0) {
		/* stun is disabled */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "RELAY SERVER is disabled - no candidate search\r\n"));
		return 0;
	} else if (i < 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "RELAY SERVER detection failed - no candidate answer\r\n"));
		return -1;
	} else {
		candidates[pos].foundation = 12;
		candidates[pos].component_id = 1;
		candidates[pos].rel_port = srcport;
		eXosip_guess_localip(AF_INET, candidates[pos].rel_addr, sizeof(candidates[pos].rel_addr));	/* which address: turn server or mine? */
		candidates[pos].conn_port = port;
		snprintf(candidates[pos].conn_addr,
				 sizeof(candidates[pos].conn_addr), "%s", firewall_ip);
		/* closesocket(fd_rtp); already done in _am_network_get_stun_socketpair */
		/* closesocket(fd_rtcp); already done in _am_network_get_stun_socketpair  */
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Add relay candidate:%i -> %s:%i\r\n",
							  candidates[pos].rel_port,
							  candidates[pos].conn_addr,
							  candidates[pos].conn_port));
		candidates[pos].priority = (0 << 24) | (255 << 16) | ((255) << 8)
			| (256 - candidates[pos].component_id);
		//candidates[pos].peer_derived = 2;
		snprintf(candidates[pos].cand_type,
				 sizeof(candidates[pos].cand_type), "relay");
		snprintf(candidates[pos].transport,
				 sizeof(candidates[pos].transport), "UDP");
	}

	return 1;
}

PPL_DECLARE (int) am_network_masquerade(const char *ip, int port)
{
	int i;
	if (ip==NULL || ip[0]=='\0')
	{
		memset(_antisipc.stun_firewall, '\0', sizeof(_antisipc.stun_firewall));
		_antisipc.stun_port = 0;
		_antisipc.stuntest.use_stun_mapped_ip=0;
		i = 0;
		eXosip_set_option(EXOSIP_OPT_UDP_LEARN_PORT, &i);
		return 0;
	}
	_antisipc.stuntest.use_stun_mapped_ip=1;
	snprintf(_antisipc.stun_firewall, sizeof(_antisipc.stun_firewall), "%s", ip);
	eXosip_masquerade_contact(ip, port);
	i = 1;
	eXosip_set_option(EXOSIP_OPT_UDP_LEARN_PORT, &i);
	return 0;
}

PPL_DECLARE (int) am_network_start(const char *transport, int port)
{
	int secure = 0;
	int i = 0;

	if (0 == osip_strcasecmp(transport, "tls"))
		secure = 1;
	else if (0 == osip_strcasecmp(transport, "dtls-udp"))
		secure = 1;

	if (0 == osip_strcasecmp(transport, "udp")
		|| 0 == osip_strcasecmp(transport, "dtls-udp")) {

		/* stun check is made on audio port which is more sensible... */
		if (_antisipc.use_stun_server == 1)
			am_network_connection_tests(&_antisipc.stuntest,
										_antisipc.stun_server,
										_antisipc.port_range_min);

		if (_antisipc.use_stun_server == 1
			&& _antisipc.stun_server != NULL
			&& _antisipc.stun_server[0] != '\0') {

			i = _amsip_get_stun_socket(_antisipc.stun_server, port,
									   _antisipc.stun_firewall,
									   &_antisipc.stun_port);
			if (i > 0 && _antisipc.stun_firewall[0] != '\0') {
				closesocket(i);
				am_trace(__FILE__, __LINE__, -OSIP_INFO1, "successfull stun detection %s:%i",
					   _antisipc.stun_firewall, _antisipc.stun_port);
				i = eXosip_listen_addr(IPPROTO_UDP, "0.0.0.0", port,
									   AF_INET, secure);
				if (i != 0) {
					am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot open udp/dtls-udp network on port=%i",
						   port);
					return -1;
				}
				eXosip_masquerade_contact(_antisipc.stun_firewall,
										  _antisipc.stun_port);
				am_trace(__FILE__, __LINE__, -OSIP_INFO1, "udp/dtls-udp network started on port=%i", port);
				return 1;
			}
			am_trace(__FILE__, __LINE__, -OSIP_ERROR, "unsuccessfull stun detection (0x%i)", i);
			i = eXosip_listen_addr(IPPROTO_UDP, "0.0.0.0", port, AF_INET,
								   secure);
			if (i != 0) {
				am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot open udp/dtls-udp network on port=%i",
					   port);
				return -1;
			}
			am_trace(__FILE__, __LINE__, -OSIP_INFO1, "udp/dtls-udp network started on port=%i", port);
			return 1;
		}


		i = eXosip_listen_addr(IPPROTO_UDP, "0.0.0.0", port, AF_INET,
							   secure);
		if (i != 0) {
			am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot open udp/dtls-udp network on port=%i", port);
			return -1;
		}
		i = 0;
		eXosip_set_option(EXOSIP_OPT_UDP_LEARN_PORT, &i);
		am_trace(__FILE__, __LINE__, -OSIP_INFO1, "udp/dtls-udp network started on port=%i", port);
		return 1;

	} else if (0 == osip_strcasecmp(transport, "tcp")
			   || 0 == osip_strcasecmp(transport, "tls")) {
		if (_antisipc.use_stun_server == 1)
			am_network_connection_tests(&_antisipc.stuntest,
										_antisipc.stun_server,
										_antisipc.port_range_min);

		i = eXosip_listen_addr(IPPROTO_TCP, "0.0.0.0", port,
							   AF_INET, secure);
		if (i != 0) {
			am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot open tcp/tls network on port=%i", port);
			return -1;
		}
		i = 0;
		eXosip_set_option(EXOSIP_OPT_UDP_LEARN_PORT, &i);
		am_trace(__FILE__, __LINE__, -OSIP_INFO1, "tcp/tls network started on port=%i", port);
		return 1;
	}

	return -1;
}

PPL_DECLARE (char *)
am_network_get_nat_info(const char *stun_server, int srcport)
{
	static struct stun_test stuntest;	/* char nat_info[256]; */
	int i;
	memset(&stuntest, 0, sizeof(struct stun_test));

	i = _am_network_get_nat(&stuntest, stun_server, srcport);
	if (stuntest.nat_info[0] != '\0')
		return stuntest.nat_info;
	return NULL;
}

PPL_DECLARE (int) am_network_get_nat(char *stun_server, int srcport)
{
	static struct stun_test stuntest;	/* char nat_info[256]; */
	int i;
	memset(&stuntest, 0, sizeof(struct stun_test));

	i = _am_network_get_nat(&stuntest, stun_server, srcport);
	return i;
}

PPL_DECLARE (char *)
am_network_test_nat(const char *stun_server, int port, const char *proxy)
{
	static char nat_ip[256];
	static char local_ip[256];
	int s;
	int stun_port = 0;

	memset(nat_ip, 0, sizeof(nat_ip));
	s = _amsip_get_stun_socket(stun_server, port + 2, nat_ip, &stun_port);
	if (s > 0 && nat_ip[0] != '\0') {
		int err;

		memset(local_ip, 0, sizeof(local_ip));
		err = eXosip_guess_localip(AF_INET, local_ip, sizeof(local_ip));
		if (err != 0) {
			am_log(OSIP_ERROR,
				   "NAT detection failed detect local_ip (nat_ip=%s)",
				   nat_ip);
			closesocket(s);
			return nat_ip;
		}

		if (osip_strcasecmp(nat_ip, local_ip) != 0) {
			am_log(OSIP_INFO1, "NAT detected: nat_ip=%s local_ip=%s",
				   nat_ip, local_ip);
			closesocket(s);
			return nat_ip;
		}
		closesocket(s);
		am_log(OSIP_INFO1, "No NAT detected: local_ip=%s", local_ip);
		return NULL;
	}
	return NULL;
}

int
_am_network_get_nat(struct stun_test *stuntest, const char *stun_server,
					int srcport)
{
	bool_t ret;

	StunAddress4 sAddr;
	StunAddress4 stunServerAddr;
	int nic = 0;

	stuntest->nat_type = StunTypeFailure;

	stunServerAddr.addr = 0;

	sAddr.addr = 0;
	sAddr.port = 0;

	ret = stunParseServerName(stun_server, &stunServerAddr);
	if (ret != TRUE || stunServerAddr.addr == 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s is not a valid host name\n", stun_server);
		return -1;
	}

	nic = 0;
	sAddr.port = srcport;

	stuntest->nat_type =
		stunNatType(&stunServerAddr, (bool_t *) (&stuntest->preserve_port),
					(bool_t *) (&stuntest->hairpin), srcport, &sAddr);

	switch (stuntest->nat_type) {
	case StunTypeFailure:
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "NAT detection failed\n"));
		return -1;
		break;
	case StunTypeUnknown:
		snprintf(stuntest->nat_info, 256, "%s", "Unknwon NAT type");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Unknown NAT type\n"));
		break;
	case StunTypeOpen:
		snprintf(stuntest->nat_info, 256, "%s", "No Nat detected");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NO NAT detected: OPEN\n"));
		break;
	case StunTypeConeNat:
		snprintf(stuntest->nat_info, 256, "%s", "Full Cone Nat");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NAT detected: Full Cone Nat\n"));
		break;
	case StunTypeRestrictedNat:
		snprintf(stuntest->nat_info, 256, "%s", "Address Restricted Nat");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NAT detected: Address Restricted Nat\n"));
		break;
	case StunTypePortRestrictedNat:
		snprintf(stuntest->nat_info, 256, "%s", "Port Restricted Nat");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NAT detected: Port Restricted Nat\n"));
		break;
	case StunTypeSymNat:
		snprintf(stuntest->nat_info, 256, "%s", "Symmetric Nat");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NAT detected: Symmetric Nat\n"));
		break;
	case StunTypeSymFirewall:
		snprintf(stuntest->nat_info, 256, "%s", "Symmetric Firewall");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NAT detected: Symmetric Firewall\n"));
		break;
	case StunTypeBlocked:
		snprintf(stuntest->nat_info, 256, "%s",
				 "Blocked or could not reach STUN server");
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_INFO2, NULL,
					"NAT detected: Blocked or could not reach STUN server\n"));
		break;
	default:
		snprintf(stuntest->nat_info, 256, "%s", "unknown NAT type (?)");
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "NAT detected: (unknown NAT type?) %i\n",
							  stuntest->nat_type));
		break;
	}

	switch (stuntest->nat_type) {
	case StunTypeConeNat:
	case StunTypeRestrictedNat:
	case StunTypePortRestrictedNat:
	case StunTypeSymNat:
		if (stuntest->preserve_port) {
			strcat(stuntest->nat_info, " - Preserves ports");
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "NAT type - preserves ports\n"));
		} else {
			strcat(stuntest->nat_info, " - random port");
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "NAT type - random port\n"));
		}
		if (stuntest->hairpin) {
			strcat(stuntest->nat_info, " - will hairpin");
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "NAT type - will hairpin\n"));
		} else {
			strcat(stuntest->nat_info, " - no hairpin");
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "NAT type - no hairpin\n"));
		}
		break;
	case StunTypeSymFirewall:
		if (stuntest->hairpin) {
			strcat(stuntest->nat_info, " - will hairpin");
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "NAT type - will hairpin\n"));
		} else {
			strcat(stuntest->nat_info, " - no hairpin");
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "NAT type - no hairpin\n"));
		}
		break;
	case StunTypeFailure:
	case StunTypeUnknown:
	case StunTypeOpen:
	case StunTypeBlocked:
	default:
		break;
	}

	return 0;
}

int
_amsip_get_stun_socket(const char *stun_server, int srcport,
					   char *firewall, int *port)
{
	bool_t ret;

	StunAddress4 sAddr[3];
	StunAddress4 stunServerAddr;
	int nic = 0;
	int retval[3];
	int i;

	StunAddress4 mappedAddr;
	int fd;
	struct in_addr inaddr;

	stunServerAddr.addr = 0;

	for (i = 0; i < 3; i++) {
		sAddr[i].addr = 0;
		sAddr[i].port = 0;
		retval[i] = 0;
	}

	ret = stunParseServerName((char *) stun_server, &stunServerAddr);
	if (ret != TRUE || stunServerAddr.addr == 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s is not a valid host name\n", stun_server);
		return -1;
	}

	nic = 0;
	sAddr[nic].port = srcport;

	fd = stunOpenSocket(&stunServerAddr, &mappedAddr, srcport,
						&sAddr[nic]);
	if (fd > 0) {
		inaddr.s_addr = htonl(mappedAddr.addr);
		am_trace(__FILE__, __LINE__, -OSIP_INFO1, "stun/sip socket mapped to %s:%i\n", inet_ntoa(inaddr),
			   mappedAddr.port);
		osip_strncpy(firewall, inet_ntoa(inaddr), 255);
		*port = mappedAddr.port;
	}

	return fd;
}

int
_am_network_get_stun_socketpair(char *stun_server, int srcport,
								char *firewall, int *rtp_port,
								int *rtcp_port, int *_fd, int *_fd_rtcp)
{
	bool_t ret;

	StunAddress4 sAddr[3];
	StunAddress4 stunServerAddr;
	int nic = 0;
	int retval[3];
	int i;

	StunAddress4 mappedAddr_rtp;
	StunAddress4 mappedAddr_rtcp;
	struct in_addr inaddr;

	stunServerAddr.addr = 0;

	*_fd = 0;
	*_fd_rtcp = 0;
	for (i = 0; i < 3; i++) {
		sAddr[i].addr = 0;
		sAddr[i].port = 0;
		retval[i] = 0;
	}

	ret = stunParseServerName(stun_server, &stunServerAddr);
	if (ret != TRUE || stunServerAddr.addr == 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s is not a valid host name\n", stun_server);
		return -1;
	}

	nic = 0;
	sAddr[nic].port = srcport;

	ret = stunOpenSocketPair(&stunServerAddr,
							 &mappedAddr_rtp,
							 &mappedAddr_rtcp,
							 _fd, _fd_rtcp, srcport, &sAddr[nic]);

	if (ret != TRUE) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s: cannot get socketpair from stun server\n",
			   stun_server);
		return -1;
	}

	inaddr.s_addr = htonl(mappedAddr_rtp.addr);
	am_trace(__FILE__, __LINE__, -OSIP_INFO1, "stun/rtp socket mapped to %s:%i\n", inet_ntoa(inaddr),
		   mappedAddr_rtp.port);
	am_trace(__FILE__, __LINE__, -OSIP_INFO1, "stun/rtcp socket mapped to %s:%i\n", inet_ntoa(inaddr),
		   mappedAddr_rtcp.port);
	osip_strncpy(firewall, inet_ntoa(inaddr), 255);
	*rtp_port = mappedAddr_rtp.port;
	*rtcp_port = mappedAddr_rtcp.port;

	return *_fd;
}

int
_am_network_get_turn_socketpair(char *turn_server, int srcport,
								char *firewall, int *rtp_port,
								int *rtcp_port, int *_fd, int *_fd_rtcp)
{
#ifndef TEST_RELAY
	am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s: feature disabled\n", turn_server);
	*_fd = 0;
	*_fd_rtcp = 0;
	return -1;
#else
	bool_t ret;

	StunAddress4 sAddr[3];
	StunAddress4 stunServerAddr;
	int nic = 0;
	int retval[3];
	int i;

	StunAddress4 mappedAddr_rtp;
	StunAddress4 mappedAddr_rtcp;
	struct in_addr inaddr;

	stunServerAddr.addr = 0;

	*_fd = 0;
	*_fd_rtcp = 0;
	for (i = 0; i < 3; i++) {
		sAddr[i].addr = 0;
		sAddr[i].port = 0;
		retval[i] = 0;
	}

	ret = stunParseServerName(turn_server, &stunServerAddr);
	if (ret != TRUE || stunServerAddr.addr == 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s is not a valid host name\n", turn_server);
		return -1;
	}

	nic = 0;
	sAddr[nic].port = srcport;

	ret = turnAllocateSocketPair(&stunServerAddr,
								 &mappedAddr_rtp,
								 &mappedAddr_rtcp,
								 _fd, _fd_rtcp, srcport, &sAddr[nic],
								 TRUE);

	if (ret != TRUE) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "%s: cannot get socketpair from stun server\n",
			   turn_server);
		return -1;
	}

	inaddr.s_addr = htonl(mappedAddr_rtp.addr);
	am_trace(__FILE__, __LINE__, -OSIP_INFO1, "stun/rtp socket mapped to %s:%i\n", inet_ntoa(inaddr),
		   mappedAddr_rtp.port);
	am_trace(__FILE__, __LINE__, -OSIP_INFO1, "stun/rtcp socket mapped to %s:%i\n", inet_ntoa(inaddr),
		   mappedAddr_rtcp.port);
	osip_strncpy(firewall, inet_ntoa(inaddr), 255);
	*rtp_port = mappedAddr_rtp.port;
	*rtcp_port = mappedAddr_rtcp.port;

	return *_fd;
#endif
}
