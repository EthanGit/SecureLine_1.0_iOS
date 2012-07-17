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

#include <ortp/rtptransport.h>
#include "utils.h"
#include <ortp/rtpsession.h>

struct _OList *transport_tables=NULL;

RtpTransport *ortp_transport_new(const char *name) {
	OList *elem;
	for (elem=transport_tables;elem!=NULL;elem=o_list_next(elem)){
		RtpTransport *s=(RtpTransport*) elem->data;
		if (strcmp(name,s->name)==0){
			RtpTransport *tr = ortp_new0(RtpTransport,1);
			if (tr==NULL)
				return NULL;
			memcpy(tr, s, sizeof(RtpTransport));
			return tr;
		}
	}
	return NULL;
}

void ortp_transport_add(RtpTransport *transport) {
	transport_tables = o_list_append(transport_tables, transport);
	transport->t_global_init();
}

void ortp_transport_remove_all(void) {
	OList *elem;
	for (elem=transport_tables;elem!=NULL;elem=o_list_next(elem)){
		RtpTransport *s=(RtpTransport*) elem->data;
		s->t_global_shutdown();
	}
	transport_tables = o_list_free(transport_tables);
}

int ortp_transport_set_option(RtpTransport *transport, int opt, void *value){
	if (transport==NULL)
		return 0;
	if (transport->t_set_option==NULL)
		return 0;
	return transport->t_set_option(transport, opt, value);
}

bool_t ortp_transport_is_supported(const char *name) {
	OList *elem;
	for (elem=transport_tables;elem!=NULL;elem=o_list_next(elem)){
		RtpTransport *s=(RtpTransport*) elem->data;
		if (strcmp(name,s->name)==0){
			return TRUE;
		}
	}
	return FALSE;
}

void ortp_transport_free(RtpTransport *transport) {
	//transport->t_release(transport);
	if (transport!=NULL)
	{
		transport->t_destroy(transport);
		ortp_free(transport);
	}
}



static int rtp_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen){
	int slen;
	slen=(int)(m->b_wptr-m->b_rptr);
	return (int)sendto(session->rtp.socket,m->b_rptr,slen,flags,to,tolen);
}

static int rtp_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen){
	int err;
	err=(int)recvfrom(session->rtp.socket,m->b_wptr,m->b_datap->db_lim-m->b_datap->db_base,flags,from,fromlen);
	return err;
}


static int rtcp_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen){
	int slen;
	slen=(int)(m->b_wptr-m->b_rptr);
	return (int)sendto(session->rtcp.socket,m->b_rptr,slen,flags,to,tolen);
}

static int rtcp_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen){
	int err;
	err=(int)recvfrom(session->rtcp.socket,m->b_wptr,m->b_datap->db_lim-m->b_datap->db_base,flags,from,fromlen);
	return err;
}

static ortp_socket_t rtp_getsocket(const struct _RtpSession *session)
{
  return session->rtp.socket;
}

static ortp_socket_t rtcp_getsocket(const struct _RtpSession *session)
{
  return session->rtcp.socket;
}

static int rtp_global_init(void)
{
	return 0;
}

static int rtp_global_shutdown(void)
{
	return 0;
}

int rtp_transport_setup(void) {

	RtpTransport *tr = ortp_new0(RtpTransport,1);
	snprintf(tr->name, sizeof(tr->name), "RTP");
	tr->t_global_init = rtp_global_init;
	tr->t_global_shutdown = rtp_global_shutdown;

	tr->t_rtp_getsocket=rtp_getsocket;
	tr->t_rtcp_getsocket=rtcp_getsocket;
	tr->t_rtp_sendto=rtp_sendto;
	tr->t_rtp_recvfrom=rtp_recvfrom;
	tr->t_rtcp_sendto=rtcp_sendto;
	tr->t_rtcp_recvfrom=rtcp_recvfrom;

	ortp_transport_add(tr);
	return 0;
}
