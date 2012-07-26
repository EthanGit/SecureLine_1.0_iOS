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

#include "rtpsession_priv.h"

/* #define ENABLE_ZRTP */
#ifdef ENABLE_ZRTP
#include <zrtp.h>
static zrtp_global_t *zrtp_global;
#define ZRTP_MITM_TRIES 100

#define ZRTP_PAD_BYTES 64 /*?? */

typedef struct ZrtpData
{
	struct _RtpSession *rtp_session;
	zrtp_zid_t zid;
	zrtp_session_t  *zrtp_session;  // ZRTP Session structure
	zrtp_stream_t   *zrtp_stream;   // ZRTP stream for encryption
	int snd_secure;
	int rcv_secure;
	int mitm_snd_secure;
	int mitm_rcv_secure;
} ZrtpData_t;

static void zrtp_session_notify_zrtp_sas(RtpSession *session, zrtp_session_info_t *zrtp_session_info) {
	if (session->eventqs!=NULL){
		OrtpEvent *ev=ortp_event_new(ORTP_EVENT_ZRTP_SAS);
		OrtpEventData *d=ortp_event_get_data(ev);
		memset(&d->info.zrtp_sas, 0, sizeof(d->info.zrtp_sas));
		memcpy(d->info.zrtp_sas.zrtp_sas,zrtp_session_info->sas1.buffer,4);
		d->info.zrtp_sas.zrtp_sas_verified=zrtp_session_info->sas_is_ready;
		strncpy(d->info.zrtp_sas.hash_name, zrtp_session_info->hash_name.buffer, zrtp_session_info->hash_name.length);
		strncpy(d->info.zrtp_sas.cipher_name, zrtp_session_info->cipher_name.buffer, zrtp_session_info->cipher_name.length);
		strncpy(d->info.zrtp_sas.auth_name, zrtp_session_info->auth_name.buffer, zrtp_session_info->auth_name.length);
		strncpy(d->info.zrtp_sas.sas_name, zrtp_session_info->sas_name.buffer, zrtp_session_info->sas_name.length);
		strncpy(d->info.zrtp_sas.pk_name, zrtp_session_info->pk_name.buffer, zrtp_session_info->pk_name.length);
		rtp_session_dispatch_event(session,ev);
	}
}

static int create_zrtp_session(struct _RtpSession *session, struct _RtpTransport *transport) {
	if (transport->data==NULL) {
		ZrtpData_t *zrtpData = ortp_new0(ZrtpData_t, 1);
		zrtp_status_t s;
		zrtp_profile_t profile;

		int val1 = random();
		int val2 = random();
		int val3 = random();
		snprintf((char *)zrtpData->zid, sizeof(zrtp_zid_t), "%4.4X%4.4X%4.4X" , val1, val2, val3);
		
		zrtpData->rtp_session=session;

		/* add ZRTP_ATL_HS80 */
		zrtp_profile_defaults(&profile, zrtp_global);
		profile.auth_tag_lens[0] = ZRTP_ATL_HS80;
		profile.auth_tag_lens[1] = ZRTP_ATL_HS32;

		s = zrtp_session_init( zrtp_global,
			&profile,
			zrtpData->zid,                        
			(transport->opts)?0:1, /* 1 -> outgoing, 0 -> incoming */
			&zrtpData->zrtp_session);
		if (zrtp_status_ok != s) {
			// Check error code and debug logs  
			return -1;
		}

		// Set call-back pointer to our parent structure
		zrtp_session_set_userdata(zrtpData->zrtp_session, &zrtpData);

		// 
		// Attach Streams
		//
		s = zrtp_stream_attach(zrtpData->zrtp_session, &zrtpData->zrtp_stream);
		if (zrtp_status_ok != s) {
			// Check error code and debug logs
			zrtp_session_down(zrtpData->zrtp_session);
			zrtpData->zrtp_session=NULL;
			return -1;
		}
		zrtp_stream_set_userdata(zrtpData->zrtp_stream, zrtpData);
		transport->data = zrtpData;

		zrtp_stream_start(zrtpData->zrtp_stream, session->snd.ssrc);
	}
	return 0;
}

static int zrtp_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen){
	ZrtpData_t *zrtpData;
	zrtp_status_t s = zrtp_status_fail;
	unsigned int slen;
	int i;

	create_zrtp_session(session, session->rtp.tr);

	zrtpData = (ZrtpData_t *)session->rtp.tr->data;
	if (zrtpData==NULL)
		return -1;

	msgpullup(m,msgdsize(m)+ZRTP_PAD_BYTES);

	slen=(int)(m->b_wptr-m->b_rptr);
	s = zrtp_process_rtp(zrtpData->zrtp_stream, (char *)m->b_rptr, &slen);
	switch (s) {
    case zrtp_status_ok:
      break;
    case zrtp_status_drop:
      ortp_message("zrtp_process_rtp: zrtp_status_drop (slen=%d)", slen);
      return (int) slen;
    case zrtp_status_fail:
      ortp_message("zrtp_process_rtp: zrtp_status_fail");
      return -1;
    default:
      break;
	}

	i = (int)sendto(session->rtp.socket,m->b_rptr,slen,flags,to,tolen);

	return i;
}

static int zrtp_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen){
	ZrtpData_t *zrtpData;
	zrtp_status_t s = zrtp_status_fail;
	int err;
	unsigned int slen;

	create_zrtp_session(session, session->rtp.tr);

	err=(int)recvfrom(session->rtp.socket,m->b_wptr,m->b_datap->db_lim-m->b_datap->db_base,flags,from,fromlen);
	if (err<0) {
		return err;
	}

	zrtpData = (ZrtpData_t *)session->rtp.tr->data;
	if (zrtpData==NULL)
		return -1;

	slen=err;
	s = zrtp_process_srtp(zrtpData->zrtp_stream, (char *)m->b_wptr, &slen);

	switch (s) {
    case zrtp_status_ok:
      session->rtp.recv_bytes+=err-slen;
      return slen;
    case zrtp_status_drop:
      return 0;
    case zrtp_status_fail:
      return -1;
    default:
      return -1;
	}

	return -1;
}


static int  zrtcp_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen){
	ZrtpData_t *zrtpData;
	zrtp_status_t s = zrtp_status_fail;
	unsigned int slen;
	int i;

	create_zrtp_session(session, session->rtp.tr);

	zrtpData = (ZrtpData_t *)session->rtp.tr->data;
	if (zrtpData==NULL)
		return -1;

	msgpullup(m,msgdsize(m)+ZRTP_PAD_BYTES);

	slen=(int)(m->b_wptr-m->b_rptr);
	s = zrtp_process_rtcp(zrtpData->zrtp_stream, (char *)m->b_rptr, &slen);

	switch (s) {
    case zrtp_status_ok:
      break;
    case zrtp_status_drop:
      ortp_message("zrtp_process_rtp: zrtp_status_drop (slen=%d)", slen);
      return (int) slen;
    case zrtp_status_fail:
      ortp_message("zrtp_process_rtp: zrtp_status_fail");
      return -1;
    default:
      break;
	}

	i = (int)sendto(session->rtcp.socket,m->b_rptr,slen,flags,to,tolen);

	return i;
}

static int zrtcp_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen){
	ZrtpData_t *zrtpData;
	zrtp_status_t s = zrtp_status_fail;
	int err;
  unsigned int slen;

	create_zrtp_session(session, session->rtp.tr);

	err=(int)recvfrom(session->rtcp.socket,m->b_wptr,m->b_datap->db_lim-m->b_datap->db_base,flags,from,fromlen);
	if (err<0) {
		return err;
	}

	zrtpData = (ZrtpData_t *)session->rtp.tr->data;
	if (zrtpData==NULL)
		return -1;

 	slen=err;
	s = zrtp_process_srtcp(zrtpData->zrtp_stream, (char *)m->b_wptr, &slen);

	switch (s) {
    case zrtp_status_ok:
      return err;
    case zrtp_status_drop:
      return 0;
    case zrtp_status_fail:
      return -1;
    default:
      return -1;
	}

	return -1;
}

static ortp_socket_t 
zrtp_getsocket(const struct _RtpSession *session)
{
  return session->rtp.socket;
}

static ortp_socket_t 
zrtcp_getsocket(const struct _RtpSession *session)
{
  return session->rtcp.socket;
}

static void
zrtp_destroy(struct _RtpTransport *transport)
{
	ZrtpData_t *zrtpData = (ZrtpData_t *)transport->data;
	if (zrtpData!=NULL) {
		if (zrtpData->zrtp_stream!=NULL)
			zrtp_stream_stop(zrtpData->zrtp_stream);
		if (zrtpData->zrtp_session!=NULL)
			zrtp_session_down(zrtpData->zrtp_session);
		ortp_free(zrtpData);
	}
	transport->data=NULL;
}

static int on_send_packet(const zrtp_stream_t *stream, char *packet, unsigned int len)
{
	ZrtpData_t *zrtpData = (ZrtpData_t *)zrtp_stream_get_userdata(stream);

	struct sockaddr *destaddr=(struct sockaddr*)&zrtpData->rtp_session->rtp.rem_addr;
	socklen_t destlen=zrtpData->rtp_session->rtp.rem_addrlen;

	if (zrtpData->rtp_session->flags & RTP_SOCKET_CONNECTED) {
		destaddr=NULL;
		destlen=0;
	}

	if (len==sendto(zrtpData->rtp_session->rtp.socket,packet,len,0,destaddr,destlen))
		return zrtp_status_ok;

	return zrtp_status_fail;
}

static void on_zrtp_secure(zrtp_stream_t *stream)
{
	ortp_message("on_zrtp_secure: ");
}

static void on_zrtp_event(zrtp_stream_t *stream, zrtp_security_event_t zrtp_event)
{
    switch (zrtp_event) {
        case ZRTP_EVENT_PROTOCOL_ERROR:
        {
			ortp_message("on_zrtp_event: ZRTP_EVENT_PROTOCOL_ERROR");
			break;
		}
	
        case ZRTP_EVENT_WRONG_SIGNALING_HASH:
        {
			ortp_message("on_zrtp_event: ZRTP_EVENT_WRONG_SIGNALING_HASH");
			break;
		}
	
        case ZRTP_EVENT_WRONG_MESSAGE_HMAC:
        {
			ortp_message("on_zrtp_event: ZRTP_EVENT_WRONG_MESSAGE_HMAC");
			break;
		}
	
        case ZRTP_EVENT_MITM_WARNING:
        {
			ortp_message("on_zrtp_event: ZRTP_EVENT_MITM_WARNING");
			break;
		}
	}
}

static void on_zrtp_protocol_event(zrtp_stream_t *stream, zrtp_protocol_event_t zrtp_event)
{
    ZrtpData_t *zrtpData = (ZrtpData_t*)zrtp_stream_get_userdata(stream);
    zrtp_session_info_t zrtp_session_info;

    switch (zrtp_event) {
        case ZRTP_EVENT_IS_SECURE:
        {
			ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_IS_SECURE!");
			zrtpData->snd_secure=1;
			zrtpData->rcv_secure=1;
			zrtpData->mitm_snd_secure=1;
			zrtpData->mitm_rcv_secure=1;
            zrtp_session_get(zrtpData->zrtp_session, &zrtp_session_info);
			if (zrtp_session_info.sas_is_ready) {
				ortp_message("SAS1: %s", stream->session->sas1.buffer);
				ortp_message("SAS2: %s", stream->session->sas2.buffer);
				zrtp_session_notify_zrtp_sas(zrtpData->rtp_session, &zrtp_session_info);
				/* zrtp_verified_set(zrtp_global, &stream->session->zid, &stream->session->peer_zid, zrtp_session_info.sas_is_verified ^ 1); */
			}
			break;
		}
		case ZRTP_EVENT_IS_CLIENT_ENROLLMENT:
			{
				ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_IS_CLIENT_ENROLLMENT!");
			}
			break;

		case ZRTP_EVENT_USER_ALREADY_ENROLLED:
			{
				ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_USER_ALREADY_ENROLLED!");

				if (zrtp_status_ok == zrtp_session_get(stream->session, &zrtp_session_info)) {
					if (zrtp_session_info.sas_is_ready) {
						/* zrtp_verified_set(zrtp_global, &stream->session->zid, &stream->session->peer_zid, zrtp_session_info.sas_is_verified ^ 1); */
					}
				}
			}
			break;

		case ZRTP_EVENT_NEW_USER_ENROLLED:
			{
				ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_NEW_USER_ENROLLED!");

				if (zrtp_status_ok == zrtp_session_get(stream->session, &zrtp_session_info)) {
					if (zrtp_session_info.sas_is_ready) {
						/* zrtp_verified_set(zrtp_global, &stream->session->zid, &stream->session->peer_zid, zrtp_session_info.sas_is_verified ^ 1); */
					}
				}
			}
			break;

		case ZRTP_EVENT_USER_UNENROLLED:
			{
				ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_USER_UNENROLLED!");

				if (zrtp_status_ok == zrtp_session_get(stream->session, &zrtp_session_info)) {
					if (zrtp_session_info.sas_is_ready) {
						/* zrtp_verified_set(zrtp_global, &stream->session->zid, &stream->session->peer_zid, zrtp_session_info.sas_is_verified ^ 1); */
					}
				}
			}
			break;

		case ZRTP_EVENT_IS_PENDINGCLEAR:
			{
				ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_IS_PENDINGCLEAR!");

				zrtpData->snd_secure=0;
				zrtpData->rcv_secure=0;
				zrtpData->mitm_snd_secure=0;
				zrtpData->mitm_rcv_secure=0;
			}
			break;

		case ZRTP_EVENT_NO_ZRTP:
			{
				ortp_message("on_zrtp_protocol_event: ZRTP_EVENT_NO_ZRTP!");
			}
			break;
        
        // handle other events there

        default: 
			ortp_message("on_zrtp_protocol_event: other event:%i!",zrtp_event);
            break;
   }
}

static int zrtp_set_option(struct _RtpTransport *transport, int opt, void *value)
{
	if (opt==1) {
		transport->opts= *(int*)value;
		return 0;
	}
	if (opt==2) {
		int verified = *(int*)value;
		ZrtpData_t *zrtpData = (ZrtpData_t *)transport->data;
		if (zrtpData!=NULL && zrtpData->zrtp_stream!=NULL) {
			zrtp_session_info_t zrtp_session_info;
            zrtp_session_get(zrtpData->zrtp_session, &zrtp_session_info);
			if (zrtp_session_info.sas_is_ready) {
				zrtp_verified_set(zrtp_global, &zrtp_session_info.zid, &zrtp_session_info.peer_zid, verified);
			}
		}
		return 0;
	}
	return 0;
}

static int zrtp_global_init(void)
{
	zrtp_status_t s = zrtp_status_ok;
	zrtp_config_t zrtp_config;

	// Initialize zrtp config with default values 
	zrtp_config_defaults(&zrtp_config);

	// Make some adjustments:
	// - Set Client ID to identify ourself
	// - Set appropriate license mode
	// - We going to use  default zrtp cache implementation, so let's specify cache file path
	strcpy(zrtp_config.client_id, "AmsipSDKCId");
	zrtp_config.lic_mode = ZRTP_LICENSE_MODE_ACTIVE;
	//zrtp_zstrcpyc( ZSTR_GV(zrtp_config.def_cache_path), TEST_CACHE_PATH);

	// Define interface callback functions
	zrtp_config.cb.misc_cb.on_send_packet           = on_send_packet;
	zrtp_config.cb.event_cb.on_zrtp_secure          = on_zrtp_secure;
	zrtp_config.cb.event_cb.on_zrtp_security_event  = on_zrtp_event;
	zrtp_config.cb.event_cb.on_zrtp_protocol_event  = on_zrtp_protocol_event;

	// Everything is ready - initialize libzrtp.        
	s = zrtp_init(&zrtp_config, &zrtp_global);
	if (zrtp_status_ok != s) {
		// Check error code and debug logs
		ortp_message("zrtp: zrtp_init failed");
		return -1;
	}
	return 0;
}

static int zrtp_global_shutdown(void)
{
	//zrtp_down(zrtp_global);
	return 0;
}

#endif

int zrtp_transport_setup(void);

int zrtp_transport_setup(void) {

#ifdef ENABLE_ZRTP
	RtpTransport *tr = ortp_new0(RtpTransport,1);
	snprintf(tr->name, sizeof(tr->name), "ZRTP");

	tr->t_global_init = zrtp_global_init;
	tr->t_global_shutdown = zrtp_global_shutdown;
	tr->t_set_option = zrtp_set_option;

	tr->t_rtp_getsocket=zrtp_getsocket;
	tr->t_rtcp_getsocket=zrtcp_getsocket;
	tr->t_rtp_sendto=zrtp_sendto;
	tr->t_rtp_recvfrom=zrtp_recvfrom;
	tr->t_rtcp_sendto=zrtcp_sendto;
	tr->t_rtcp_recvfrom=zrtcp_recvfrom;
	tr->t_destroy=zrtp_destroy;

	ortp_transport_add(tr);
#endif
	return 0;
}
