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

/*
	Copyright (C) 2011  Aymeric Moizard <amoizard@gmail.com>
*/

/** 
 * \file rtptransport.h
 * \brief The RtpTransport api
 *
 * The RtpTransport objects provide some ability to modify packets
 * before they are sent and to send packets with different formating.
**/

#ifndef RTPTRANSPORT_H
#define RTPTRANSPORT_H

#include <ortp/port.h>
#include <ortp/rtp.h>

struct _RtpSession;

typedef struct _RtpTransport
{
	char name[64];
	void *data;
	void *opts;
	int  (*t_global_init)();
	int  (*t_global_shutdown)();
	int  (*t_set_option)(struct _RtpTransport *transport, int opt, void *value);
	ortp_socket_t (*t_rtp_getsocket)(const struct _RtpSession *t);
	ortp_socket_t (*t_rtcp_getsocket)(const struct _RtpSession *t);
	int  (*t_rtp_sendto)(struct _RtpSession *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen);
	int  (*t_rtp_recvfrom)(struct _RtpSession *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
	int  (*t_rtcp_sendto)(struct _RtpSession *t, mblk_t *msg , int flags, const struct sockaddr *to, socklen_t tolen);
	int  (*t_rtcp_recvfrom)(struct _RtpSession *t, mblk_t *msg, int flags, struct sockaddr *from, socklen_t *fromlen);
	void (*t_destroy)(struct _RtpTransport *t);
}  RtpTransport;


#ifdef __cplusplus
extern "C"
{
#endif

extern 

/* public API */
RtpTransport *ortp_transport_new(const char *name);
void ortp_transport_add(RtpTransport *transport);
int ortp_transport_set_option(RtpTransport *transport, int opt, void *value);
void ortp_transport_free(RtpTransport *transport);
bool_t ortp_transport_is_supported(const char *name);

/* private API */
void ortp_transport_remove_all(void);

#ifdef __cplusplus
}
#endif

#endif
