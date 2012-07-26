/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ortp/ortp.h"
#include "utils.h"
#include "ortp/rtpsession.h"
#include "rtpsession_priv.h"

#if defined(WIN32) || defined(_WIN32_WCE)
#include "ortp-config-win32.h"
#elif HAVE_CONFIG_H
#include "ortp-config.h" /*needed for HAVE_SYS_UIO_H */
#endif

#if (_WIN32_WINNT >= 0x0600)
#include <delayimp.h>
#undef ExternC
#include <QOS2.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#define USE_SENDMSG 1
#endif

void tc_qos_delete_flow(RtpSession *session);

#define can_connect(s)	( (s)->use_connect && !(s)->symmetric_rtp)

static bool_t try_connect(int fd, const struct sockaddr *dest, socklen_t addrlen){
	if (connect(fd,dest,addrlen)<0){
		ortp_warning("Could not connect() socket: %s",getSocketError());
		return FALSE;
	}
	return TRUE;
}

static ortp_socket_t create_and_bind(const char *addr, int port, int *sock_family, bool_t reuse_addr){
	int err;
	int optval = 1;
	ortp_socket_t sock=-1;

#ifdef ORTP_INET6
	char num[8];
	struct addrinfo hints, *res0, *res;
#else
	struct sockaddr_in saddr;
#endif
	
#ifdef ORTP_INET6
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	snprintf(num, sizeof(num), "%d",port);
	err = getaddrinfo(addr,num, &hints, &res0);
	if (err!=0) {
		ortp_warning ("Error in getaddrinfo on (addr=%s port=%i): %s", addr, port, gai_strerror(err));
		return -1;
	}
	
	for (res = res0; res; res = res->ai_next) {
		sock = socket(res->ai_family, res->ai_socktype, 0);
		if (sock==-1)
			continue;

		if (reuse_addr){
			err = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
					(SOCKET_OPTION_VALUE)&optval, sizeof (optval));
			if (err < 0)
			{
				ortp_warning ("Fail to set rtp address reusable: %s.", getSocketError());
			}
		}

		*sock_family=res->ai_family;
		err = bind (sock, res->ai_addr, res->ai_addrlen);
		if (err != 0){
			ortp_warning ("Fail to bind rtp socket to (addr=%s port=%i) : %s.", addr,port, getSocketError());
			close_socket (sock);
			sock=-1;
			continue;
		}
#ifndef __hpux
		switch (res->ai_family)
		  {
		    case AF_INET:
		      if (IN_MULTICAST(ntohl(((struct sockaddr_in *) res->ai_addr)->sin_addr.s_addr)))
			{
		          struct ip_mreq mreq;
			  mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr.s_addr;
			  mreq.imr_interface.s_addr = INADDR_ANY;
			  err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (SOCKET_OPTION_VALUE) &mreq, sizeof(mreq));
			  if (err < 0){
				ortp_warning ("Fail to join address group: %s.", getSocketError());
				close_socket (sock);
				sock=-1;
				continue;
			    }
			}
		      break;
		    case AF_INET6:
		      if (IN6_IS_ADDR_MULTICAST(&(((struct sockaddr_in6 *) res->ai_addr)->sin6_addr)))
			{
			  struct ipv6_mreq mreq;
			  mreq.ipv6mr_multiaddr = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
			  mreq.ipv6mr_interface = 0;
			  err = setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (SOCKET_OPTION_VALUE)&mreq, sizeof(mreq));
			  if (err < 0)
			    {
			      ortp_warning ("Fail to join address group: %s.", getSocketError());
			      close_socket (sock);
			      sock=-1;
			      continue;
			    }
			}
		      break;
		  }
#endif /*hpux*/
		break;
	}
	freeaddrinfo(res0);
#else
	saddr.sin_family = AF_INET;
	*sock_family=AF_INET;
	err = inet_aton (addr, &saddr.sin_addr);
	if (err < 0)
	{
		ortp_warning ("Error in socket address:%s.", getSocketError());
		return -1;
	}
	saddr.sin_port = htons (port);

	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock==-1) return -1;

	if (reuse_addr){
		err = setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
				(SOCKET_OPTION_VALUE)&optval, sizeof (optval));
		if (err < 0)
		{
			ortp_warning ("Fail to set rtp address reusable: %s.",getSocketError());
		}
	}

	err = bind (sock,
		    (struct sockaddr *) &saddr,
		    sizeof (saddr));

	if (err != 0)
	{
		ortp_warning ("Fail to bind rtp socket to port %i: %s.", port, getSocketError());
		close_socket (sock);
		return -1;
	}
#endif
	if (sock!=-1){
		set_non_blocking_socket (sock);
	}
	return sock;
}

static void set_socket_sizes(int sock, unsigned int sndbufsz, unsigned int rcvbufsz){
	int err;
	bool_t done=FALSE;
	if (sndbufsz>0){
#ifdef SO_SNDBUFFORCE
		err = setsockopt(sock, SOL_SOCKET, SO_SNDBUFFORCE, (void *)&sndbufsz, sizeof(sndbufsz)); 
		if (err == -1) {
			ortp_error("Fail to increase socket's send buffer size with SO_SNDBUFFORCE: %s.", getSocketError());
		}else done=TRUE;
#endif
		if (!done){
			err = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void *)&sndbufsz, sizeof(sndbufsz)); 
			if (err == -1) {
				ortp_error("Fail to increase socket's send buffer size with SO_SNDBUF: %s.", getSocketError());
			}
		}
	}
	done=FALSE;
	if (rcvbufsz>0){
#ifdef SO_RCVBUFFORCE
		err = setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, (void *)&rcvbufsz, sizeof(rcvbufsz)); 
		if (err == -1) {
			ortp_error("Fail to increase socket's recv buffer size with SO_RCVBUFFORCE: %s.", getSocketError());
		}
#endif
		if (!done){
			err = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbufsz, sizeof(rcvbufsz)); 
			if (err == -1) {
				ortp_error("Fail to increase socket's recv buffer size with SO_RCVBUF: %s.", getSocketError());
			}
		}
		
	}
}

static ortp_socket_t create_and_bind_random(const char *localip, int *sock_family, int *port){
	int retry;
	ortp_socket_t sock = -1;
	for (retry=0;retry<100;retry++)
	{
		int localport;
		do
		{
			localport = (rand () + 5000) & 0xfffe;
		}
		while ((localport < 5000) || (localport > 0xffff));
		/*do not set REUSEADDR in case of random allocation */
		sock = create_and_bind(localip, localport, sock_family,FALSE);
		if (sock!=-1) {
			*port=localport;
			return sock;
		}
	}
	ortp_warning("create_and_bind_random: Could not find a random port for %s !",localip);
	return -1;
}

/**
 *rtp_session_set_local_addr:
 *@session:		a rtp session freshly created.
 *@addr:		a local IP address in the xxx.xxx.xxx.xxx form.
 *@port:		a local port or -1 to let oRTP choose the port randomly
 *
 *	Specify the local addr to be use to listen for rtp packets or to send rtp packet from.
 *	In case where the rtp session is send-only, then it is not required to call this function:
 *	when calling rtp_session_set_remote_addr(), if no local address has been set, then the 
 *	default INADRR_ANY (0.0.0.0) IP address with a random port will be used. Calling 
 *	rtp_sesession_set_local_addr() is mandatory when the session is recv-only or duplex.
 *
 *	Returns: 0 on success.
**/

int
rtp_session_set_local_addr (RtpSession * session, const char * addr, int port)
{
	ortp_socket_t sock;
	int sockfamily;
	bool_t reuse_addr;

	if ((session->flags&RTP_SESSION_USING_EXT_SOCKETS) != 0){
		return 0;
	}

	if (sockisvalid(session->rtp.socket)){
		/* don't rebind, but close before*/
		rtp_session_release_sockets(session);
	}
#ifdef SO_REUSE_ADDR
	reuse_addr=TRUE;
#else
	reuse_addr=FALSE;
#endif
	/* try to bind the rtp port */
	if (port>0)
		sock=create_and_bind(addr,port,&sockfamily,reuse_addr);
	else
		sock=create_and_bind_random(addr,&sockfamily,&port);
	if (sock!=-1){
		set_socket_sizes(sock,session->rtp.snd_socket_size,session->rtp.rcv_socket_size);
		session->rtp.sockfamily=sockfamily;
		session->rtp.socket=sock;
		session->rtp.loc_port=port;
		/*try to bind rtcp port */
		sock=create_and_bind(addr,port+1,&sockfamily,reuse_addr);
		if (sock!=-1){
			session->rtcp.sockfamily=sockfamily;
			session->rtcp.socket=sock;
		}else{
			ortp_warning("Could not create and bind rtcp socket.");
		}
		
		/* set socket options (but don't change chosen states) */
		rtp_session_set_dscp( session, -1 );
		rtp_session_set_multicast_ttl( session, -1 );
		rtp_session_set_multicast_loopback( session, -1 );

		return 0;
	}
	return -1;
}


/**
 *rtp_session_set_multicast_ttl:
 *@session: a rtp session
 *@ttl: desired Multicast Time-To-Live
 *
 * Sets the TTL (Time-To-Live) for outgoing multicast packets.
 *
 * Returns: 0 on success.
 *
**/
int rtp_session_set_multicast_ttl(RtpSession *session, int ttl)
{
    int retval;
    
    // Store new TTL if one is specified
    if (ttl>0) session->multicast_ttl = ttl;
    
    // Don't do anything if socket hasn't been created yet
    if (!sockisvalid(session->rtp.socket)) return 0;

    switch (session->rtp.sockfamily) {
        case AF_INET: {
 
			retval= setsockopt(session->rtp.socket, IPPROTO_IP, IP_MULTICAST_TTL,
						 (SOCKET_OPTION_VALUE)  &session->multicast_ttl, sizeof(session->multicast_ttl));
            
			if (retval<0) break;

			retval= setsockopt(session->rtcp.socket, IPPROTO_IP, IP_MULTICAST_TTL,
					 (SOCKET_OPTION_VALUE)	   &session->multicast_ttl, sizeof(session->multicast_ttl));

 		} break;
#ifdef ORTP_INET6
        case AF_INET6: {

			retval= setsockopt(session->rtp.socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 
					 (SOCKET_OPTION_VALUE)&session->multicast_ttl, sizeof(session->multicast_ttl));
					
			if (retval<0) break;
			
			retval= setsockopt(session->rtcp.socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 
					 (SOCKET_OPTION_VALUE) &session->multicast_ttl, sizeof(session->multicast_ttl));

        } break;
#endif
        default:
            retval=-1;
    }
    
	if (retval<0)
		ortp_warning("Failed to set multicast TTL on socket.");
  

	return retval;
}


/**
 *rtp_session_get_multicast_ttl:
 *@session: a rtp session
 *
 * Returns the TTL (Time-To-Live) for outgoing multicast packets.
 *
**/
int rtp_session_get_multicast_ttl(RtpSession *session)
{
	return session->multicast_ttl;
}


/**
 *rtp_session_set_multicast_loopback:
 *@session: a rtp session
 *@ttl: desired Multicast Time-To-Live
 *
 * Sets the TTL (Time-To-Live) for outgoing multicast packets.
 *
 * Returns: 0 on success.
 *
**/
int rtp_session_set_multicast_loopback(RtpSession *session, int yesno)
{
    int retval;
    
    // Store new loopback state if one is specified
    if (yesno==0) {
    	// Don't loop back
    	session->multicast_loopback = 0;
    } else if (yesno>0) {
    	// Do loop back
    	session->multicast_loopback = 1;
    }
     
    // Don't do anything if socket hasn't been created yet
    if (!sockisvalid(session->rtp.socket)) return 0;

    switch (session->rtp.sockfamily) {
        case AF_INET: {
 
			retval= setsockopt(session->rtp.socket, IPPROTO_IP, IP_MULTICAST_LOOP,
						 (SOCKET_OPTION_VALUE)   &session->multicast_loopback, sizeof(session->multicast_loopback));
            
			if (retval<0) break;

			retval= setsockopt(session->rtcp.socket, IPPROTO_IP, IP_MULTICAST_LOOP,
						 (SOCKET_OPTION_VALUE)   &session->multicast_loopback, sizeof(session->multicast_loopback));

 		} break;
#ifdef ORTP_INET6
        case AF_INET6: {

			retval= setsockopt(session->rtp.socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 
				 (SOCKET_OPTION_VALUE)	&session->multicast_loopback, sizeof(session->multicast_loopback));
					
			if (retval<0) break;
			
			retval= setsockopt(session->rtcp.socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 
				 (SOCKET_OPTION_VALUE)	&session->multicast_loopback, sizeof(session->multicast_loopback));

        } break;
#endif
        default:
            retval=-1;
    }
    
	if (retval<0)
		ortp_warning("Failed to set multicast loopback on socket.");
  

	return retval;
}


/**
 *rtp_session_get_multicast_loopback:
 *@session: a rtp session
 *
 * Returns the multicast loopback state of rtp session (true or false).
 *
**/
int rtp_session_get_multicast_loopback(RtpSession *session)
{
	return session->multicast_loopback;
}

#if (_WIN32_WINNT >= 0x0600)

#define UNICODE

#include <ntddndis.h> 
#include <Traffic.h>

VOID MyClNotifyHandler(
    IN  HANDLE      ClRegCtx,
    IN  HANDLE      ClIfcCtx,
    IN  ULONG       Event,      // See list below
    IN  HANDLE      SubCode,
    IN  ULONG       BufSize,
    IN  PVOID       Buffer
    )
{
	ortp_message("MyClNotifyHandler handler");
}

struct _tc_qos_ifc {
	char interfaceName[256];
    HANDLE flowHandle;
    HANDLE FilterHandle;
    HANDLE ifcHandle;
};

static HANDLE QosClientHandle = NULL;
static BOOL tc_initialized = FALSE;

static void tc_qos_init()
{
    TCI_CLIENT_FUNC_LIST QoSFunctions;
	long result;

	if (tc_initialized==TRUE)
		return;
	tc_initialized = TRUE;

	QoSFunctions.ClAddFlowCompleteHandler = NULL;
    QoSFunctions.ClDeleteFlowCompleteHandler = NULL;
    QoSFunctions.ClModifyFlowCompleteHandler = NULL;
    QoSFunctions.ClNotifyHandler = (TCI_NOTIFY_HANDLER)MyClNotifyHandler;

    //Register call back functions with Traffic control interface.
    result = TcRegisterClient(CURRENT_TCI_VERSION, NULL, &QoSFunctions , &QosClientHandle); 

    if (result!= NO_ERROR)
    {
        ortp_warning("TC: no admin rights for TcRegisterClient usage");
        return;
    }
    else if (result!= NO_ERROR)
    {
        ortp_error("TC: %d reported by TcRegisterClient",result);
        return;
    }
}

void tc_qos_delete_flow(RtpSession *session)
{
	if (session->tc_qos.num_ifc<=0)
	{
		session->tc_qos.num_ifc=0;
		if (session->tc_qos.ifc!=NULL)
			ortp_free(session->tc_qos.ifc);
		session->tc_qos.ifc=NULL;
	}
	while (session->tc_qos.num_ifc>0)
	{
		ULONG retCode;
		struct _tc_qos_ifc *ifc;
		session->tc_qos.num_ifc--;
		ifc=&session->tc_qos.ifc[session->tc_qos.num_ifc];

		// Add filter to the flow. This point onwards all traffic matching the filter, will pass through the flow.
		if (ifc->FilterHandle!=NULL)
		{
			retCode = TcDeleteFilter(ifc->FilterHandle);

			if (retCode!= NO_ERROR)
			{
				ortp_error("TC: %s: %d reported by in TcDeleteFilter", ifc->interfaceName, retCode);
			}
		}
		if (ifc->flowHandle!=NULL)
		{
			retCode = TcDeleteFlow(ifc->flowHandle);
			if(retCode != NO_ERROR)
			{
				ortp_error("TC: %s: %d reported by TcDeleteFlow",ifc->interfaceName, retCode);
			}
		}
		if (ifc->ifcHandle!=NULL)
		{
			retCode = TcCloseInterface(ifc->ifcHandle);
			if (retCode !=  NO_ERROR)
			{
				ortp_error("TC: %s: %d reported by TcCloseInterface",ifc->interfaceName, retCode);
			}
		}
		ifc->FilterHandle=NULL;
		ifc->flowHandle=NULL;
		ifc->ifcHandle=NULL;

		ortp_message("TC: %s: flow and filter removed", ifc->interfaceName);
	}
	if (session->tc_qos.ifc!=NULL)
		ortp_free(session->tc_qos.ifc);
	session->tc_qos.ifc=NULL;
}

int rtp_session_set_dscp_with_tcapi(RtpSession *session, int dscp)
{
	long result;
    PTC_IFC_DESCRIPTOR pInterfaceBuffer = NULL;
    ULONG BufferSize;
	ULONG RemainingBufferSize;
	int idx;
	PTC_IFC_DESCRIPTOR pCurrentIfc;

    PTC_GEN_FLOW newFlow;

    // Create the new temp flow.
    LPQOS_OBJECT_HDR objHdr = NULL; 

	LPQOS_DS_CLASS pQOSClass;

    ULONG retCode;
	TC_GEN_FILTER GenericFilter;
    IP_PATTERN Pattern, Mask;

	char src6host[NI_MAXHOST];
	int recvport = 0;

	tc_qos_init();
	if (QosClientHandle==NULL)
		return -1; /* -> no admin rights? */

	if (session->rtp.rem_addrlen<=0)
		return -1;

	memset(src6host, 0, sizeof(src6host));

	if (((struct sockaddr_in *) &session->rtp.rem_addr)->sin_family == AF_INET)
		recvport = ntohs(((struct sockaddr_in *) &session->rtp.rem_addr)->sin_port);
	else
		recvport = ntohs(((struct sockaddr_in6 *) &session->rtp.rem_addr)->sin6_port);

	result = getnameinfo((struct sockaddr *) &session->rtp.rem_addr, session->rtp.rem_addrlen,
		src6host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

	if (result != 0) {
		ortp_error("TC: getnameinfo failure: %s", gai_strerror(result));
		return -1;
	}

	// Create a filter. All traffic matching the filter will pass through the flow
	memset(&Pattern,0,sizeof(IP_PATTERN));
	memset(&Mask,0,sizeof(IP_PATTERN));
	GenericFilter.AddressType = NDIS_PROTOCOL_ID_TCP_IP; // Protocol stack is TCP/IP
	GenericFilter.PatternSize = sizeof(IP_PATTERN);
	GenericFilter.Pattern = &Pattern; // pattern to match, defined below
	GenericFilter.Mask = &Mask;
	// Filter pattern. This is where you can tweak the flow filter parameters
	// configured to match on all outgoing UDP packets.
	Pattern.Reserved1 = 0;
	Pattern.Reserved2 = 0;
	Pattern.SrcAddr = inet_addr("0.0.0.0"); //IP_ADDRESS;
	Pattern.DstAddr = inet_addr(src6host); //IP_ADDRESS;
	Pattern.tcSrcPort = 0;
	Pattern.tcDstPort = htons(recvport); 
	Pattern.ProtocolId = IPPROTO_UDP;
	Pattern.Reserved3[0] = 0;
	Pattern.Reserved3[1] = 0;
	Pattern.Reserved3[2] = 0;
	// Patterns mask
	Mask.Reserved1 = 0;
	Mask.Reserved2 = 0;
	Mask.SrcAddr = htonl( 0 );
	Mask.DstAddr = 0xFFFFFFFF;
	Mask.tcSrcPort = 0;
	Mask.tcDstPort = 0xFFFF;
	Mask.ProtocolId = 0xFF;
	Mask.Reserved3[0] = 0;
	Mask.Reserved3[1] = 0;
	Mask.Reserved3[2] = 0;

	//Find traffic control enabled interfaces on the cmachine
    //Enumerate the available protocols
    BufferSize = 4096;
	pInterfaceBuffer = (PTC_IFC_DESCRIPTOR)ortp_malloc(BufferSize);
    result = TcEnumerateInterfaces (QosClientHandle, &BufferSize,pInterfaceBuffer);
	while (result==ERROR_INSUFFICIENT_BUFFER)
	{
		BufferSize *= 2;
		pInterfaceBuffer = (PTC_IFC_DESCRIPTOR)ortp_realloc(pInterfaceBuffer, BufferSize);
	    result = TcEnumerateInterfaces (QosClientHandle, &BufferSize,pInterfaceBuffer);
	}
    if (result != NO_ERROR) 
    {
        ortp_error("TC: %d reported by TcEnumerateInterfaces",result);
		ortp_free(pInterfaceBuffer);
        return E_FAIL;
    }

    if (BufferSize == 0)
    {
        ortp_error("TC: no traffic control interfaces with QOS Packet Scheduler installed");
		ortp_free(pInterfaceBuffer);
        return E_FAIL;
    }

	tc_qos_delete_flow(session);
	session->tc_qos.num_ifc=0;

	RemainingBufferSize = BufferSize;
    pCurrentIfc = pInterfaceBuffer;
    while (RemainingBufferSize > 0)
    {
		RemainingBufferSize -= pInterfaceBuffer->Length;
		pInterfaceBuffer = (PTC_IFC_DESCRIPTOR) ((PBYTE) pInterfaceBuffer + pInterfaceBuffer->Length);
		session->tc_qos.num_ifc++;
	}

	if (session->tc_qos.num_ifc<=0)
	{
        ortp_error("TC: no interfaces available");
		ortp_free(pCurrentIfc);
		return -1;
	}

	newFlow = ( PTC_GEN_FLOW )ortp_malloc(sizeof (TC_GEN_FLOW )+ sizeof (QOS_DS_CLASS) + sizeof(QOS_OBJECT_HDR));

	session->tc_qos.ifc = (struct _tc_qos_ifc *)ortp_malloc(sizeof(struct _tc_qos_ifc)*session->tc_qos.num_ifc);
	memset(session->tc_qos.ifc, 0, sizeof(struct _tc_qos_ifc)*session->tc_qos.num_ifc);

	RemainingBufferSize = BufferSize;
    pInterfaceBuffer = pCurrentIfc;
	idx=0;
    while (RemainingBufferSize > 0)
    {
		struct _tc_qos_ifc *ifc=&session->tc_qos.ifc[idx];
		idx++;
		WideCharToMultiByte(CP_UTF8,0,pInterfaceBuffer->pInterfaceName, -1,ifc->interfaceName,sizeof(ifc->interfaceName), 0, 0 );

		result = TcOpenInterface(pInterfaceBuffer->pInterfaceName, QosClientHandle, NULL, &ifc->ifcHandle );

		if (result!= NO_ERROR)
		{
			ortp_error("TC: %s: %d reported by TcOpenInterface",ifc->interfaceName, result);
			RemainingBufferSize -= pInterfaceBuffer->Length;
			pInterfaceBuffer = (PTC_IFC_DESCRIPTOR) ((PBYTE) pInterfaceBuffer + pInterfaceBuffer->Length);
			continue;
		}

		//Set the Flow Spec
		newFlow->ReceivingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
		newFlow->ReceivingFlowspec.Latency = QOS_NOT_SPECIFIED;
		newFlow->ReceivingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;
		newFlow->ReceivingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
		newFlow->ReceivingFlowspec.PeakBandwidth = QOS_NOT_SPECIFIED;
		newFlow->ReceivingFlowspec.ServiceType = SERVICETYPE_BESTEFFORT;
		newFlow->ReceivingFlowspec.TokenBucketSize = QOS_NOT_SPECIFIED;
		newFlow->ReceivingFlowspec.TokenRate =QOS_NOT_SPECIFIED;

		newFlow->SendingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
		newFlow->SendingFlowspec.Latency = QOS_NOT_SPECIFIED;
		newFlow->SendingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;
		newFlow->SendingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
		newFlow->SendingFlowspec.PeakBandwidth = QOS_NOT_SPECIFIED;
		newFlow->SendingFlowspec.ServiceType=SERVICETYPE_BESTEFFORT;
		newFlow->SendingFlowspec.TokenBucketSize = QOS_NOT_SPECIFIED;
		newFlow->SendingFlowspec.TokenRate = QOS_NOT_SPECIFIED;

		newFlow->TcObjectsLength =sizeof(QOS_DS_CLASS) + sizeof(QOS_OBJECT_HDR); 

		pQOSClass = (LPQOS_DS_CLASS)(&(newFlow->TcObjects[0]));

		pQOSClass->ObjectHdr.ObjectType = QOS_OBJECT_DS_CLASS;
		pQOSClass->ObjectHdr.ObjectLength = sizeof(QOS_DS_CLASS);
		pQOSClass->DSField = 0x2E;
		objHdr = (LPQOS_OBJECT_HDR)((char *)&(newFlow->TcObjects[0]) + sizeof(QOS_DS_CLASS)); 

		objHdr->ObjectType = QOS_OBJECT_END_OF_LIST;
		objHdr->ObjectLength = sizeof(QOS_OBJECT_HDR); 

		// adds a new flow on the specified interface. The flow is like a channel,
		// that determines how to shape the traffic if it tunnelled through it.
		retCode = TcAddFlow(ifc->ifcHandle, NULL, 0, newFlow, &ifc->flowHandle );
		if(retCode != NO_ERROR)
		{
			ortp_error("TC: %s: %d reported by TcAddFlow",ifc->interfaceName, retCode);
			RemainingBufferSize -= pInterfaceBuffer->Length;
			pInterfaceBuffer = (PTC_IFC_DESCRIPTOR) ((PBYTE) pInterfaceBuffer + pInterfaceBuffer->Length);
			continue;
		}

		// Add filter to the flow. This point onwards all traffic matching the filter, will pass through the flow.
		retCode = TcAddFilter(ifc->flowHandle, &GenericFilter, &ifc->FilterHandle); 

		if (retCode!= NO_ERROR)
		{
			ortp_error("TC: %s: %d reported by in TcAddFilter", ifc->interfaceName, retCode);
			RemainingBufferSize -= pInterfaceBuffer->Length;
			pInterfaceBuffer = (PTC_IFC_DESCRIPTOR) ((PBYTE) pInterfaceBuffer + pInterfaceBuffer->Length);
			continue;
		}
	    
		ortp_message("TC: %s: DSCP %2X for UDP->%s:%i", ifc->interfaceName, dscp, src6host, recvport);

        RemainingBufferSize -= pInterfaceBuffer->Length;
        pInterfaceBuffer = (PTC_IFC_DESCRIPTOR) ((PBYTE) pInterfaceBuffer + pInterfaceBuffer->Length);
    }

	ortp_free(newFlow);
	ortp_free(pCurrentIfc);
	return 0;
}
#else
void tc_qos_delete_flow(RtpSession *session)
{
}
#endif

#if 0

#include <qos.h>
#include <qossp.h>

struct all_objects {
	QOS_DESTADDR qos_dest;
	QOS_DS_CLASS qos_ds;
	QOS_OBJECT_HDR qos_hdr;
};

int rtp_session_set_dscp_BROKEN(RtpSession *session, int dscp)
{
	QOS qos;
	DWORD dwBytes;
	struct all_objects obj;
	int err;

	if (session->rtp.rem_addrlen<=0)
		return -1;

	memset(&obj, 0, sizeof(struct all_objects));

	qos.ReceivingFlowspec.ServiceType = SERVICE_BESTEFFORT | SERVICE_NO_QOS_SIGNALING;
	qos.ReceivingFlowspec.TokenRate = QOS_NOT_SPECIFIED;
	qos.ReceivingFlowspec.TokenBucketSize = QOS_NOT_SPECIFIED;
	qos.ReceivingFlowspec.PeakBandwidth = QOS_NOT_SPECIFIED;
	qos.ReceivingFlowspec.Latency	 = QOS_NOT_SPECIFIED;
	qos.ReceivingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
	qos.ReceivingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
	qos.ReceivingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;

	qos.SendingFlowspec.ServiceType = SERVICETYPE_BESTEFFORT | SERVICE_NO_QOS_SIGNALING;
	qos.SendingFlowspec.TokenRate = QOS_NOT_SPECIFIED;
	qos.SendingFlowspec.TokenBucketSize = QOS_NOT_SPECIFIED;
	qos.SendingFlowspec.PeakBandwidth = QOS_NOT_SPECIFIED;
	qos.SendingFlowspec.Latency	 = QOS_NOT_SPECIFIED;
	qos.SendingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
	qos.SendingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
	qos.SendingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;
	qos.ProviderSpecific.len = 0;
	qos.ProviderSpecific.buf = NULL;

	obj.qos_dest.ObjectHdr.ObjectType = QOS_OBJECT_DESTADDR;
	obj.qos_dest.ObjectHdr.ObjectLength = sizeof(QOS_DESTADDR);
	obj.qos_dest.SocketAddress = (SOCKADDR *)&session->rtp.rem_addr;
	obj.qos_dest.SocketAddressLength = session->rtp.rem_addrlen;

	obj.qos_ds.ObjectHdr.ObjectType = QOS_OBJECT_DS_CLASS;
	obj.qos_ds.ObjectHdr.ObjectLength = sizeof(QOS_DS_CLASS);
	obj.qos_ds.DSField = 0x2e;

    // Set the end of the list
    obj.qos_hdr.ObjectType = QOS_OBJECT_END_OF_LIST;
    obj.qos_hdr.ObjectLength = sizeof(QOS_OBJECT_HDR); 

	
	//qos.ProviderSpecific.len = sizeof(QOS_DS_CLASS)+sizeof(QOS_DESTADDR)+sizeof(QOS_OBJECT_HDR);
	//qos.ProviderSpecific.buf = (char*)&obj;
	qos.ProviderSpecific.len = sizeof(QOS_DS_CLASS);
	qos.ProviderSpecific.buf = (char*)&obj.qos_ds;

	err = WSAIoctl(session->rtp.socket, SIO_SET_QOS, &qos, sizeof(QOS)+qos.ProviderSpecific.len, NULL, 0, &dwBytes, NULL, NULL);
	if (err != 0)
	{
		ortp_error("WSAIoctl FAILED: ");
		return -1;
	} else {
		ortp_message("LIPSocket::SetTOSHigh() WSAIoctl(SIO_SET_QOS) OK");
		return 0;
	}
}
#endif

/**
 *rtp_session_set_dscp:
 *@session: a rtp session
 *@dscp: desired DSCP PHB value
 *
 * Sets the DSCP (Differentiated Services Code Point) for outgoing RTP packets.
 *
 * Returns: 0 on success.
 *
**/
int rtp_session_set_dscp(RtpSession *session, int dscp){
	int retval=0;
	int tos;
#if (_WIN32_WINNT >= 0x0600)
	OSVERSIONINFOEX ovi;
#endif

	// Store new DSCP value if one is specified
	if (dscp>=0) session->dscp = dscp;
	
	// Don't do anything if socket hasn't been created yet
	if (!sockisvalid(session->rtp.socket)) return 0;

#if (_WIN32_WINNT >= 0x0600)

	memset(&ovi, 0, sizeof(ovi));
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	GetVersionEx((LPOSVERSIONINFO) & ovi);

	//This works on vista with admin rights
	if (session->dscp==0 && ovi.dwMajorVersion > 5)
		tos=QOSTrafficTypeBestEffort;
	else if (session->dscp==0x8 && ovi.dwMajorVersion > 5)
		tos=QOSTrafficTypeBackground;
	else if (session->dscp==0x28 && ovi.dwMajorVersion > 5)
		tos=QOSTrafficTypeAudioVideo;
	else if (session->dscp==0x38 && ovi.dwMajorVersion > 5)
		tos=QOSTrafficTypeVoice;
	else {
		if (rtp_session_set_dscp_with_tcapi(session, session->dscp)==0)
			return 0;
		else{
			if (QosClientHandle!=NULL)
			{
				/* we have admin rights but remote addr is most probably not yet defined */
				/* postpone dscp activation for later */
				return -1;
			}
		}
	}

	ortp_message("check OS support for qwave.lib: %i %i %i\n",
				ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber);
	if (ovi.dwMajorVersion > 5) {

		if (FAILED(__HrLoadAllImportsForDll("qwave.dll"))) {
			ortp_warning("Failed to load qwave.dll: no QoS available\n" );
		}
		else
		{
			if (session->dscp==0)
				tos=QOSTrafficTypeBestEffort;
			else if (session->dscp<=0x8)
				tos=QOSTrafficTypeBackground;
			else if (session->dscp<=0x28)
				tos=QOSTrafficTypeAudioVideo;
			else if (session->dscp<=0x38)
				tos=QOSTrafficTypeVoice;
			else
				tos=QOSTrafficTypeExcellentEffort; /* 0x28 */

			if (session->rtp.QoSHandle==NULL) {
				QOS_VERSION version;
				BOOL QoSResult;

				version.MajorVersion = 1;
				version.MinorVersion = 0;

				QoSResult = QOSCreateHandle(&version, &session->rtp.QoSHandle);

				if (QoSResult != TRUE){
					ortp_error("QOSCreateHandle failed to create handle with error %d\n",
						GetLastError());
					retval=-1;
				}
			}
			if (session->rtp.QoSHandle!=NULL && session->rtp.rem_addrlen>0) {
				BOOL QoSResult;
				QoSResult = QOSAddSocketToFlow(
					session->rtp.QoSHandle, 
					session->rtp.socket,
					(struct sockaddr*)&session->rtp.rem_addr,
					tos, 
					QOS_NON_ADAPTIVE_FLOW, 
					&session->rtp.QoSFlowID);

				if (QoSResult != TRUE){
					ortp_error("QOSAddSocketToFlow failed to add a flow with error %d\n", 
						GetLastError());
					retval=-1;
				} else{
#if 0
					DWORD dscp_val = session->dscp;
					QoSResult = QOSSetFlow(session->rtp.QoSHandle,
										session->rtp.QoSFlowID,
										 /* -> available on windows 7/ no vista support */ 
										2, /* QOSSetOutgoingDSCPValue, */
										sizeof(dscp_val),
										&dscp_val,
										0,
										NULL);
					if (QoSResult != TRUE)
					{
						ortp_error("QOSSetFlow failed to set dscp value on flow with error %d\n", 
							GetLastError());
					}
					else
					{
						ortp_message("QOSSetFlow succeeded to set dscp value on flow\n");
					}
#endif
				}
			}
		}
	} else {
#endif
		// DSCP value is in the upper six bits of the TOS field
		tos = (session->dscp << 2) & 0xFC;
		switch (session->rtp.sockfamily) {
			case AF_INET:
			retval = setsockopt(session->rtp.socket, IPPROTO_IP, IP_TOS, (SOCKET_OPTION_VALUE)&tos, sizeof(tos));
			break;
#ifdef ORTP_INET6
		case AF_INET6:
#	ifdef IPV6_TCLASS /*seems not defined by my libc*/
			retval = setsockopt(session->rtp.socket, IPPROTO_IPV6, IPV6_TCLASS,
			 (SOCKET_OPTION_VALUE)&tos, sizeof(tos));
#	else
			/*in case that works:*/
			retval = setsockopt(session->rtp.socket, IPPROTO_IPV6, IP_TOS,
			 (SOCKET_OPTION_VALUE)&tos, sizeof(tos));
#	endif
			break;
#endif
		default:
			retval=-1;
		}
#if (_WIN32_WINNT >= 0x0600)
	}
#endif

	if (retval<0)
		ortp_warning("Failed to set DSCP value on socket.");

	return retval;
}


/**
 *rtp_session_get_dscp:
 *@session: a rtp session
 *
 * Returns the DSCP (Differentiated Services Code Point) for outgoing RTP packets.
 *
**/
int rtp_session_get_dscp(const RtpSession *session)
{
	return session->dscp;
}


/**
 *rtp_session_get_local_port:
 *@session:	a rtp session for which rtp_session_set_local_addr() or rtp_session_set_remote_addr() has been called
 *
 *	This function can be useful to retrieve the local port that was randomly choosen by 
 *	rtp_session_set_remote_addr() when rtp_session_set_local_addr() was not called.
 *
 *	Returns: the local port used to listen for rtp packets, -1 if not set.
**/

int rtp_session_get_local_port(const RtpSession *session){
	return (session->rtp.loc_port>0) ? session->rtp.loc_port : -1;
}


static char * ortp_inet_ntoa(struct sockaddr *addr, int addrlen, char *dest, int destlen){
#ifdef ORTP_INET6
	int err;
	dest[0]=0;
	err=getnameinfo(addr,addrlen,dest,destlen,NULL,0,NI_NUMERICHOST);
	if (err!=0){
		ortp_warning("getnameinfo error: %s",gai_strerror(err));
	}
#else
	char *tmp=inet_ntoa(((struct sockaddr_in*)addr)->sin_addr);
	strncpy(dest,tmp,destlen);
	dest[destlen-1]='\0';
#endif
	return dest;
}

/**
 *rtp_session_set_remote_addr:
 *@session:		a rtp session freshly created.
 *@addr:		a local IP address in the xxx.xxx.xxx.xxx form.
 *@port:		a local port.
 *
 *	Sets the remote address of the rtp session, ie the destination address where rtp packet
 *	are sent. If the session is recv-only or duplex, it also sets the origin of incoming RTP 
 *	packets. Rtp packets that don't come from addr:port are discarded.
 *
 *	Returns: 0 on success.
**/
int
rtp_session_set_remote_addr (RtpSession * session, const char * addr, int port){
	return rtp_session_set_remote_addr_full (session, addr, port, port+1);
}

/**
 *rtp_session_set_remote_addr_full:
 *@session:		a rtp session freshly created.
 *@addr:		a local IP address in the xxx.xxx.xxx.xxx form.
 *@rtp_port:		a local rtp port.
 *@rtcp_port:		a local rtcp port.
 *
 *	Sets the remote address of the rtp session, ie the destination address where rtp packet
 *	are sent. If the session is recv-only or duplex, it also sets the origin of incoming RTP 
 *	packets. Rtp packets that don't come from addr:port are discarded.
 *
 *	Returns: 0 on success.
**/

int
rtp_session_set_remote_addr_full (RtpSession * session, const char * addr, int rtp_port, int rtcp_port)
{
	int err;
#ifdef ORTP_INET6
	struct addrinfo hints, *res0, *res;
	char num[8];
#endif
	if ((session->flags&RTP_SESSION_USING_EXT_SOCKETS) != 0){
		return 0;
	}
#ifdef ORTP_INET6
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	snprintf(num, sizeof(num), "%d", rtp_port);
	err = getaddrinfo(addr, num, &hints, &res0);
	if (err) {
		ortp_warning ("Error in socket address: %s", gai_strerror(err));
		return -1;
	}
#endif
	if (session->rtp.socket == -1){
		/* the session has not its socket bound, do it */
		ortp_message ("Setting random local addresses.");
#ifdef ORTP_INET6
		/* bind to an address type that matches the destination address */
		if (res0->ai_addr->sa_family==AF_INET6)
			err = rtp_session_set_local_addr (session, "::", -1);
		else err=rtp_session_set_local_addr (session, "0.0.0.0", -1);
#else
		err = rtp_session_set_local_addr (session, "0.0.0.0", -1);
#endif
		if (err<0) return -1;
	}

#ifdef ORTP_INET6
	err=1;
	for (res = res0; res; res = res->ai_next) {
		/* set a destination address that has the same type as the local address */
		if (res->ai_family==session->rtp.sockfamily ) {
			memcpy( &session->rtp.rem_addr, res->ai_addr, res->ai_addrlen);
			session->rtp.rem_addrlen=res->ai_addrlen;
		  	err=0;
		  	break;
		}
	}
	freeaddrinfo(res0);
	if (err) {
		ortp_warning("Could not set destination for RTP socket to %s:%i.",addr,rtp_port);
		return -1;
	}
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	snprintf(num, sizeof(num), "%d", rtcp_port);
	err = getaddrinfo(addr, num, &hints, &res0);
	if (err) {
		ortp_warning ("Error: %s", gai_strerror(err));
		return err;
	}
	err=1;
	for (res = res0; res; res = res->ai_next) {
		/* set a destination address that has the same type as the local address */
		if (res->ai_family==session->rtp.sockfamily ) {
		  	err=0;
		  	memcpy( &session->rtcp.rem_addr, res->ai_addr, res->ai_addrlen);
			session->rtcp.rem_addrlen=res->ai_addrlen;
		  	break;
		}
	}
	freeaddrinfo(res0);
	if (err) {
		ortp_warning("Could not set destination for RCTP socket to %s:%i.",addr,rtcp_port);
		return -1;
	}
#else
	session->rtp.rem_addrlen=sizeof(session->rtp.rem_addr);
	session->rtp.rem_addr.sin_family = AF_INET;

	err = inet_aton (addr, &session->rtp.rem_addr.sin_addr);
	if (err < 0)
	{
		ortp_warning ("Error in socket address:%s.", getSocketError());
		return err;
	}
	session->rtp.rem_addr.sin_port = htons (rtp_port);

	memcpy (&session->rtcp.rem_addr, &session->rtp.rem_addr,
		sizeof (struct sockaddr_in));
	session->rtcp.rem_addr.sin_port = htons (rtcp_port);
	session->rtcp.rem_addrlen=sizeof(session->rtcp.rem_addr);
#endif
	if (can_connect(session)){
		if (try_connect(session->rtp.socket,(struct sockaddr*)&session->rtp.rem_addr,session->rtp.rem_addrlen))
			session->flags|=RTP_SOCKET_CONNECTED;
		if (sockisvalid(session->rtcp.socket)){
			if (try_connect(session->rtcp.socket,(struct sockaddr*)&session->rtcp.rem_addr,session->rtcp.rem_addrlen))
				session->flags|=RTCP_SOCKET_CONNECTED;
		}
	}else if (session->flags & RTP_SOCKET_CONNECTED){
		/*must dissolve association done by connect().
		See connect(2) manpage*/
		struct sockaddr sa;
		sa.sa_family=AF_UNSPEC;
		if (connect(session->rtp.socket,&sa,sizeof(sa))<0){
			ortp_error("Cannot dissolve connect() association for rtp socket: %s", getSocketError());
		}
		if (connect(session->rtcp.socket,&sa,sizeof(sa))<0){
			ortp_error("Cannot dissolve connect() association for rtcp socket: %s", getSocketError());
		}
		session->flags&=~RTP_SOCKET_CONNECTED;
		session->flags&=~RTCP_SOCKET_CONNECTED;
	}
	return 0;
}

int
rtp_session_set_remote_addr_and_port(RtpSession * session, const char * addr, int rtp_port, int rtcp_port){
	return rtp_session_set_remote_addr_full(session,addr,rtp_port,rtcp_port);
}

void rtp_session_set_sockets(RtpSession *session, int rtpfd, int rtcpfd)
{
	if (rtpfd>=0) set_non_blocking_socket(rtpfd);
	if (rtcpfd>=0) set_non_blocking_socket(rtcpfd);
	session->rtp.socket=rtpfd;
	session->rtcp.socket=rtcpfd;
	if (rtpfd>=0 || rtcpfd>=0 )
		session->flags|=(RTP_SESSION_USING_EXT_SOCKETS|RTP_SOCKET_CONNECTED|RTCP_SOCKET_CONNECTED);
	else session->flags&=~(RTP_SESSION_USING_EXT_SOCKETS|RTP_SOCKET_CONNECTED|RTCP_SOCKET_CONNECTED);
}

void rtp_session_set_transports(RtpSession *session, struct _RtpTransport *rtptr, struct _RtpTransport *rtcptr)
{
	if (session->rtp.tr!=NULL)
		ortp_transport_free(session->rtp.tr);
	if (session->rtcp.tr!=NULL)
		ortp_transport_free(session->rtcp.tr);
	session->rtp.tr = 0;
	session->rtcp.tr = 0;

	session->rtp.tr = rtptr;
	session->rtcp.tr = rtcptr;
#if 0
	if (rtptr)
		rtptr->session=session;
	if (rtcptr)
		rtcptr->session=session;
#endif

	if (rtptr || rtcptr )
		session->flags|=(RTP_SESSION_USING_TRANSPORT);
	else session->flags&=~(RTP_SESSION_USING_TRANSPORT);
}

/**
 *rtp_session_flush_sockets:
 *@session: a rtp session
 *
 * Flushes the sockets for all pending incoming packets.
 * This can be usefull if you did not listen to the stream for a while
 * and wishes to start to receive again. During the time no receive is made
 * packets get bufferised into the internal kernel socket structure.
 *
**/
void rtp_session_flush_sockets(RtpSession *session){
	unsigned char trash[4096];
#ifdef ORTP_INET6
	struct sockaddr_storage from;
#else
	struct sockaddr from;
#endif
	socklen_t fromlen=sizeof(from);
	if (rtp_session_using_transport(session, rtp))
	  {
		mblk_t *trashmp=esballoc(trash,sizeof(trash),0,NULL);
		
	    while (session->rtp.tr->t_rtp_recvfrom(session,trashmp,0,(struct sockaddr *)&from,&fromlen)>0){};

	    if (session->rtcp.tr)
	      while (session->rtcp.tr->t_rtcp_recvfrom(session,trashmp,0,(struct sockaddr *)&from,&fromlen)>0){};
		freemsg(trashmp);
	    return;
	  }

	if (sockisvalid(session->rtp.socket)){
		while (recvfrom(session->rtp.socket,(char*)trash,sizeof(trash),0,(struct sockaddr *)&from,&fromlen)>0){};
	}
	if (sockisvalid(session->rtcp.socket)){
		while (recvfrom(session->rtcp.socket,(char*)trash,sizeof(trash),0,(struct sockaddr*)&from,&fromlen)>0){};
	}
}


#ifdef USE_SENDMSG 
#define MAX_IOV 30
static int rtp_sendmsg(int sock,mblk_t *m, struct sockaddr *rem_addr, int addr_len){
	int error;
	struct msghdr msg;
	struct iovec iov[MAX_IOV];
	int iovlen;
	for(iovlen=0; iovlen<MAX_IOV && m!=NULL; m=m->b_cont,iovlen++){
		iov[iovlen].iov_base=m->b_rptr;
		iov[iovlen].iov_len=m->b_wptr-m->b_rptr;
	}
	if (iovlen==MAX_IOV){
		ortp_error("Too long msgb, didn't fit into iov, end discarded.");
	}
	msg.msg_name=(void*)rem_addr;
	msg.msg_namelen=addr_len;
	msg.msg_iov=&iov[0];
	msg.msg_iovlen=iovlen;
	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_flags=0;
	error=(int)sendmsg(sock,&msg,0);
	return error;
}
#endif	

#define IP_UDP_OVERHEAD (20+8)

static void update_sent_bytes(RtpSession*s, int nbytes){
	if (s->rtp.sent_bytes==0){
		gettimeofday(&s->rtp.send_bw_start,NULL);
	}
	s->rtp.sent_bytes+=nbytes+IP_UDP_OVERHEAD;
}

static void update_recv_bytes(RtpSession*s, int nbytes){
	if (s->rtp.recv_bytes==0){
		gettimeofday(&s->rtp.recv_bw_start,NULL);
	}
	s->rtp.recv_bytes+=nbytes+IP_UDP_OVERHEAD;
}

int
rtp_session_rtp_send (RtpSession * session, mblk_t * m)
{
	int error;
	int i;
	rtp_header_t *hdr;
	struct sockaddr *destaddr=(struct sockaddr*)&session->rtp.rem_addr;
	socklen_t destlen=session->rtp.rem_addrlen;
	ortp_socket_t sockfd=session->rtp.socket;

	hdr = (rtp_header_t *) m->b_rptr;
	/* perform host to network conversions */
	hdr->ssrc = htonl (hdr->ssrc);
	hdr->timestamp = htonl (hdr->timestamp);
	hdr->seq_number = htons (hdr->seq_number);
	for (i = 0; i < hdr->cc; i++)
		hdr->csrc[i] = htonl (hdr->csrc[i]);

	if (session->flags & RTP_SOCKET_CONNECTED) {
		destaddr=NULL;
		destlen=0;
	}

	if (rtp_session_using_transport(session, rtp)){
		error = (session->rtp.tr->t_rtp_sendto) (session,m,0,destaddr,destlen);
	}else{
#ifdef USE_SENDMSG
		error=rtp_sendmsg(sockfd,m,destaddr,destlen);
#else
		if (m->b_cont!=NULL)
			msgpullup(m,-1);
		error = sendto (sockfd, (char*)m->b_rptr, (int) (m->b_wptr - m->b_rptr),
			 0,destaddr,destlen);
#endif
	}
	if (error < 0){
		if (session->on_network_error.count>0){
			rtp_signal_table_emit3(&session->on_network_error,(long)"Error sending RTP packet",INT_TO_POINTER(getSocketErrorCode()));
		}else ortp_warning ("Error sending rtp packet: %s ; socket=%i", getSocketError(), sockfd);
		session->rtp.send_errno=getSocketErrorCode();
	}else{
		update_sent_bytes(session,error);
	}
	freemsg (m);
	return error;
}

int
rtp_session_rtcp_send (RtpSession * session, mblk_t * m)
{
	int error=0;
	ortp_socket_t sockfd=session->rtcp.socket;
	struct sockaddr *destaddr=(struct sockaddr*)&session->rtcp.rem_addr;
	socklen_t destlen=session->rtcp.rem_addrlen;
	bool_t using_connected_socket=(session->flags & RTCP_SOCKET_CONNECTED)!=0;

	if (using_connected_socket) {
		destaddr=NULL;
		destlen=0;
	}

	ortp_message("sending RTCP %i", session->rtp.loc_port+1);

	if (session->rtcp.enabled &&
		( (sockisvalid(sockfd) && (session->rtcp.rem_addrlen>0 ||using_connected_socket))
			|| rtp_session_using_transport(session, rtcp) ) ){
		if (rtp_session_using_transport(session, rtcp)){
			error = (session->rtcp.tr->t_rtcp_sendto) (session, m, 0,
			destaddr, destlen);
		}
		else{
#ifdef USE_SENDMSG
			error=rtp_sendmsg(sockfd,m,destaddr, destlen);
#else
			if (m->b_cont!=NULL){
				msgpullup(m,-1);
			}
			error = sendto (sockfd, (char*)m->b_rptr,
			(int) (m->b_wptr - m->b_rptr), 0,
			destaddr, destlen);
#endif
		}
		if (error < 0){
			char host[65];
			if (session->on_network_error.count>0){
				rtp_signal_table_emit3(&session->on_network_error,(long)"Error sending RTCP packet",INT_TO_POINTER(getSocketErrorCode()));
			}else ortp_warning ("Error sending RTCP packet: %s ; socket=%i; addr=%s", getSocketError(), session->rtcp.socket, ortp_inet_ntoa((struct sockaddr*)&session->rtcp.rem_addr,session->rtcp.rem_addrlen,host,sizeof(host)) );
		}
	}else ortp_message("Not sending RTCP report: sockfd=%i, rem_addrlen=%i, connected=%i",sockfd,session->rtcp.rem_addrlen,using_connected_socket);
	freemsg (m);
	return error;
}

int
rtp_session_rtp_recv (RtpSession * session, uint32_t user_ts)
{
	int error;
	ortp_socket_t sockfd=session->rtp.socket;
#ifdef ORTP_INET6
	struct sockaddr_storage remaddr;
#else
	struct sockaddr remaddr;
#endif
	socklen_t addrlen = sizeof (remaddr);
	mblk_t *mp;
	
	if (!sockisvalid(sockfd) && !rtp_session_using_transport(session, rtp)) return -1;  /*session has no sockets for the moment*/

	while (1)
	{
		int bufsz;
		bool_t sock_connected=!!(session->flags & RTP_SOCKET_CONNECTED);

		if (session->rtp.cached_mp==NULL)
			 session->rtp.cached_mp = msgb_allocator_alloc(&session->allocator,session->recv_buf_size);
		mp=session->rtp.cached_mp;
		bufsz=(int) (mp->b_datap->db_lim - mp->b_datap->db_base);
		if (sock_connected){
			error=(int)recv(sockfd,(char*)mp->b_wptr,bufsz,0);
		}else if (rtp_session_using_transport(session, rtp)) 
			error = (session->rtp.tr->t_rtp_recvfrom)(session, mp, 0,
				  (struct sockaddr *) &remaddr,
				  &addrlen);
		else error = (int)recvfrom(sockfd, (char*)mp->b_wptr,
				  bufsz, 0,
				  (struct sockaddr *) &remaddr,
				  &addrlen);
		if (error > 0){
			if (session->symmetric_rtp && !sock_connected){
				if (session->use_connect){
					/* store the sender rtp address to do symmetric RTP */
					memcpy(&session->rtp.rem_addr,&remaddr,addrlen);
					session->rtp.rem_addrlen=addrlen;
					if (try_connect(sockfd,(struct sockaddr*)&remaddr,addrlen))
					{
						session->flags|=RTP_SOCKET_CONNECTED;
						rtp_session_set_dscp(session, -1);
					}
				}
			}
			/* then parse the message and put on queue */
			mp->b_wptr+=error;
			rtp_session_rtp_parse (session, mp, user_ts, (struct sockaddr*)&remaddr,addrlen);
			session->rtp.cached_mp=NULL;
			/*for bandwidth measurements:*/
			update_recv_bytes(session,error);
		}
		else
		{
		 	int errnum=getSocketErrorCode();
			if (error == 0){
				/*0 can be returned by RtpTransport functions in case of EWOULDBLOCK*/
				/*we ignore it*/
				/*ortp_warning
					("rtp_recv: strange... recv() returned zero.");*/
			}
			else if (!is_would_block_error(errnum))
			{
				if (session->on_network_error.count>0){
					rtp_signal_table_emit3(&session->on_network_error,(long)"Error receiving RTP packet",INT_TO_POINTER(getSocketErrorCode()));
				}else ortp_warning("Error receiving RTP packet: %s.",getSocketError());
				session->rtp.recv_errno=errnum;
			}
			/* don't free the cached_mp, it will be reused next time */
			return -1;	/* avoids an infinite loop ! */
		}
	}
	return error;
}

static void rtp_session_notify_inc_rtcp(RtpSession *session, mblk_t *m){
	if (session->eventqs!=NULL){
		OrtpEvent *ev=ortp_event_new(ORTP_EVENT_RTCP_PACKET_RECEIVED);
		OrtpEventData *d=ortp_event_get_data(ev);
		d->packet=m;
		rtp_session_dispatch_event(session,ev);
	}
	else freemsg(m);  /* avoid memory leak */
}

int
rtp_session_rtcp_recv (RtpSession * session)
{
	int error;
#ifdef ORTP_INET6
	struct sockaddr_storage remaddr;
#else
	struct sockaddr remaddr;
#endif
	socklen_t addrlen=0;
	mblk_t *mp;

	if (!sockisvalid(session->rtcp.socket) && !rtp_session_using_transport(session, rtcp)) return -1;  /*session has no rtcp sockets for the moment*/
	

	while (1)
	{
		bool_t sock_connected=!!(session->flags & RTCP_SOCKET_CONNECTED);
		if (session->rtcp.cached_mp==NULL)
			 session->rtcp.cached_mp = allocb (RTCP_MAX_RECV_BUFSIZE, 0);
		
		mp=session->rtcp.cached_mp;
		if (sock_connected){
			error=(int)recv(session->rtcp.socket,(char*)mp->b_wptr,RTCP_MAX_RECV_BUFSIZE,0);
		}else {
			addrlen=sizeof (remaddr);

			if (rtp_session_using_transport(session, rtcp))
			  error=(session->rtcp.tr->t_rtcp_recvfrom)(session, mp, 0,
				  (struct sockaddr *) &remaddr,
				  &addrlen);
			else
			  error=(int)recvfrom (session->rtcp.socket,(char*) mp->b_wptr,
				  RTCP_MAX_RECV_BUFSIZE, 0,
				  (struct sockaddr *) &remaddr,
				  &addrlen);
		}
		if (error > 0)
		{
			mp->b_wptr += error;
			/* post an event to notify the application*/
			{
				mblk_t *mp2 = dupb(mp);
				rtp_session_notify_inc_rtcp(session,mp);
				if (mp2!=NULL)
				{
					rtp_session_rtcp_parse(session,mp2); //(this modify the content of RTCP data so we work on duplicate)
					freemsg(mp2);
				}
			}
			session->rtcp.cached_mp=NULL;
			if (session->symmetric_rtp && !sock_connected){
				/* store the sender rtp address to do symmetric RTP */
			  if (addrlen!=session->rtcp.rem_addrlen
			      || memcmp(&session->rtcp.rem_addr,&remaddr, addrlen)!=0)
			    {
                                ortp_message("Symmetric RTCP with initial RTCP/data packet! %i", session->rtp.loc_port+1);

				memcpy(&session->rtcp.rem_addr,&remaddr,addrlen);
				session->rtcp.rem_addrlen=addrlen;
				if (session->use_connect){
					if (try_connect(session->rtcp.socket,(struct sockaddr*)&remaddr,addrlen))
						session->flags|=RTCP_SOCKET_CONNECTED;
				}
			    }
			}
		}
		else
		{
			int errnum=getSocketErrorCode();

			if (error == 0)
			{
				ortp_warning
					("rtcp_recv: strange... recv() returned zero.");
			}
			else if (!is_would_block_error(errnum))
			{
				if (session->on_network_error.count>0){
					rtp_signal_table_emit3(&session->on_network_error,(long)"Error receiving RTCP packet",INT_TO_POINTER(errnum));
				}else ortp_warning("Error receiving RTCP packet: %s.",getSocketError());
			}
			/* don't free the cached_mp, it will be reused next time */
			return -1;	/* avoids an infinite loop ! */
		}
	}
	return error;
}

