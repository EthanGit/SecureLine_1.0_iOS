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

#if defined(_MSC_VER)  && (defined(WIN32) || defined(_WIN32_WCE))
#include "ortp-config-win32.h"
#elif HAVE_CONFIG_H
#include "ortp-config.h"
#endif
#include "ortp/ortp.h"

#ifdef HAVE_SRTP

#undef PACKAGE_NAME 
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME 
#undef PACKAGE_VERSION

#include "ortp/srtp.h"

#define SRTP_PAD_BYTES 64 /*?? */

static int  srtp_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen){
	srtp_t srtp=(srtp_t)session->rtp.tr->data;
	int slen;
	err_status_t err;
	/* enlarge the buffer for srtp to write its data */
	msgpullup(m,msgdsize(m)+SRTP_PAD_BYTES);
	slen=(int)(m->b_wptr-m->b_rptr);
	err=srtp_protect(srtp,m->b_rptr,&slen);
	if (err==err_status_ok){
		return (int)sendto(session->rtp.socket,m->b_rptr,slen,flags,to,tolen);
	}
	ortp_error("srtp_protect() failed");
	return -1;
}

static int srtp_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen){
	srtp_t srtp=(srtp_t)session->rtp.tr->data;
	int err;
	int slen;
	err=(int)recvfrom(session->rtp.socket,m->b_wptr,m->b_datap->db_lim-m->b_datap->db_base,flags,from,fromlen);
	if (err>0){

		/* keep NON-RTP data unencrypted */
		rtp_header_t *rtp;
		if (err>=RTP_FIXED_HEADER_SIZE)
		{
			rtp = (rtp_header_t*)m->b_wptr;
			if (rtp->version!=2)
			{
				return err;
			}
		}

		slen=err;
		if (srtp_unprotect(srtp,m->b_wptr,&slen)==err_status_ok)
		{
			//complete with difference
			if (session->rtp.recv_bytes==0){
				gettimeofday(&session->rtp.recv_bw_start,NULL);
			}
			session->rtp.recv_bytes+=err-slen;
			return slen;
		} else {
			ortp_error("srtp_unprotect() failed");
			return -1;
		}
	}
	return err;
}

static int  srtcp_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen){
	srtp_t srtp=(srtp_t)session->rtcp.tr->data;
	int slen;
	/* enlarge the buffer for srtp to write its data */
	msgpullup(m,msgdsize(m)+SRTP_PAD_BYTES);
	slen=(int)(m->b_wptr-m->b_rptr);
	if (srtp_protect_rtcp(srtp,m->b_rptr,&slen)==err_status_ok){
		return (int)sendto(session->rtcp.socket,m->b_rptr,slen,flags,to,tolen);
	}
	ortp_error("srtp_protect_rtcp() failed");
	return -1;
}

static int srtcp_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen){
	srtp_t srtp=(srtp_t)session->rtcp.tr->data;
	int err;
	int slen;
	err=(int)recvfrom(session->rtcp.socket,m->b_wptr,m->b_datap->db_lim-m->b_datap->db_base,flags,from,fromlen);
	if (err>0){
		slen=err;
		if (srtp_unprotect_rtcp(srtp,m->b_wptr,&slen)==err_status_ok)
			return slen;
		else {
			ortp_error("srtp_unprotect_rtcp() failed");
			return -1;
		}
	}
	return err;
}

static ortp_socket_t 
srtp_getsocket(const struct _RtpSession *session)
{
  return session->rtp.socket;
}

static ortp_socket_t 
srtcp_getsocket(const struct _RtpSession *session)
{
  return session->rtcp.socket;
}

static void 
srtp_destroy(struct _RtpTransport *transport)
{
	srtp_t srtp=(srtp_t)transport->data;
	if (srtp!=NULL)
		srtp_dealloc(srtp);
	transport->data=NULL;
}

static int srtp_global_init(void)
{
	return srtp_init();
}

static int srtp_global_shutdown(void)
{
	return srtp_shutdown();
}

err_status_t ortp_srtp_create(srtp_t *session, const srtp_policy_t *policy)
{
	return srtp_create(session, policy);
}

err_status_t ortp_srtp_dealloc(srtp_t session)
{
	return srtp_dealloc(session);
}

err_status_t ortp_srtp_add_stream(srtp_t session, const srtp_policy_t *policy)
{
	return srtp_add_stream(session, policy);
}

#else

int ortp_srtp_create(void *session, void *policy)
{
	return -1;
}

int ortp_srtp_dealloc(void *session)
{
	return -1;
}

int ortp_srtp_add_stream(void *session, const void *policy)
{
	return -1;
}

#endif

int srtp_transport_setup(void);

int srtp_transport_setup(void) {

#ifdef HAVE_SRTP
	RtpTransport *tr = ortp_new0(RtpTransport,1);
	snprintf(tr->name, sizeof(tr->name), "SRTP");

	tr->t_global_init = srtp_global_init;
	tr->t_global_shutdown = srtp_global_shutdown;
	tr->t_set_option = NULL;

	tr->t_rtp_getsocket=srtp_getsocket;
	tr->t_rtcp_getsocket=srtcp_getsocket;
	tr->t_rtp_sendto=srtp_sendto;
	tr->t_rtp_recvfrom=srtp_recvfrom;
	tr->t_rtcp_sendto=srtcp_sendto;
	tr->t_rtcp_recvfrom=srtcp_recvfrom;
	tr->t_destroy=srtp_destroy;

	ortp_transport_add(tr);
#endif
	return 0;
}
