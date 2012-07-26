/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2011-2012 Aymeric MOIZARD amoizard_at_osip.org

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

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

#if defined(WIN32) || defined(_WIN32_WCE)
#include "ortp-config-win32.h"
#elif HAVE_CONFIG_H
#include "ortp-config.h"
#endif
#include "ortp/ortp.h"
#include "utils.h"
#include "ortp/stun.h"
#include "ortp/turn.h"

#include <assert.h>

#define TCP_PAD_BYTES (sizeof(uint16_t))
#ifdef _MSC_VER
#define FUNC __FUNCTION__
#else
#define FUNC __func__
#endif

#if !defined(WIN32) && !defined(_WIN32_WCE)
#include <netinet/tcp.h>
#include <netdb.h>
#ifndef closesocket
#define closesocket close
#endif
#endif

#if 0	// Enable to get a detailed packet receive debugging log
#if !defined(WIN32) && !defined(_WIN32_WCE)
#define TRACEFILE "/tmp/recv.log"
#else
#define TRACEFILE "c:\\tmp\\recv.log"
#endif
#endif

#ifdef TRACEFILE
static struct timeval last_tv;
struct timeval tv, diff;
static FILE *f;
#endif

typedef enum {
    TurnSessionInit = 0,
    TurnSessionConnected,
    TurnSessionSentAllocate,
    TurnSessionSentChanBind,
    TurnSessionStable,
    TurnSessionSentRefresh,
    TurnSessionSentBindRefresh,
    TurnSessionSentDeleteAlloc,
    TurnSessionTerminated,
    TurnSessionFailed
} TurnSessionState;

// States during which we can read/write suceesfully
#define TurnSessionCanRecv(x) ((x)->state >= TurnSessionSentChanBind && (x)->state <= TurnSessionSentBindRefresh)
#define TurnSessionCanSend(x) ((x)->state >= TurnSessionStable && (x)->state <= TurnSessionSentBindRefresh)

// Determine how soon before the expiration we refresh allocations/channels
#define TURN_ALLOCATION_REFRESH_INTERVAL	60  /* seconds */
#define TURN_CHANNEL_REFRESH_INTERVAL		60  /* seconds */

struct TurnSocketData {
    int fd;
    TurnSessionState state;
    queue_t send_q;
    int send_remain;
    mblk_t *recv_m;
    int recv_size;
    int recv_need;
    int recv_left;
    int skipping;
    int have_turn_msg;

    /* TURN auth data */
    char* auth_username;
    char* auth_password;
    char* auth_realm;
    char* auth_nonce;

    int bad_nonce_count;

    uint16_t channel;

    StunAddress4 relay_addr;
    StunAddress4 bind_addr;

    time_t alloc_expires;
    time_t channel_expires;
};

struct _RtpTurnData {
    struct TurnSocketData rtp;
    struct TurnSocketData rtcp;
};

static int turn_allocate(struct TurnSocketData *data, TurnAtrReservationToken* resToken, bool_t reservePortPair);
static int turn_bind_chan(struct TurnSocketData *data, const char* raddr, unsigned int rport, unsigned int chan);
static int turn_flush_sendq(struct TurnSocketData *data, int flags);

int
rtp_turn_set_remote_addr(RtpTurnData *data, const char *raddr, int rport)
{
    if (turn_bind_chan(&data->rtp, raddr, rport, 0x4000)
        || turn_bind_chan(&data->rtcp, raddr, rport + 1, 0x4001))
        return -1;

    return 0;
}

static int
turn_queue_refresh(struct TurnSocketData *data)
{
    char buf[STUN_MAX_MESSAGE_SIZE];

    StunAtrString username, *pUsername = NULL;
    StunAtrString password, *pPassword = NULL;
    StunAtrString nonce, *pNonce = NULL;
    StunAtrString realm, *pRealm = NULL;
  
    int len;
    mblk_t *new_blk;
  
    ortp_warning("Refreshing allocation on fd %d...", data->fd);

    if (data->auth_username != NULL) {
        snprintf(username.value, sizeof(username.value), "%s", data->auth_username);
        username.sizeValue = strlen(username.value);
        pUsername = &username;
    }
  
    if (data->auth_password != NULL) {
        snprintf(password.value, sizeof(password.value), "%s", data->auth_password);
        password.sizeValue = strlen(password.value);
        pPassword = &password;
    }
  
    if (data->auth_realm != NULL) {
        snprintf(realm.value, sizeof(realm.value), "%s", data->auth_realm);
        realm.sizeValue = strlen(realm.value);
        pRealm = &realm;
    }
  
    if (data->auth_nonce != NULL) {
        snprintf(nonce.value, sizeof(nonce.value), "%s", data->auth_nonce);
        nonce.sizeValue = strlen(nonce.value);
        pNonce = &nonce;
    }
  
    len = turnBuildRefresh(buf, sizeof(buf), pUsername, pPassword, pRealm, pNonce);
    ortp_debug("%s: About to send Refresh msg of len %i to fd %d", FUNC, len, data->fd);

    data->state = TurnSessionSentRefresh;

    new_blk = allocb(len, 0);
    if (new_blk == NULL) {
        ortp_warning("%s: PANIC: out of memory for local buffers of %d bytes", FUNC, len);
        data->state = TurnSessionFailed;
        return -1;
    }

    msgappend(new_blk, buf, len, FALSE);
    putq(&data->send_q, new_blk);
    return 1;
}

static int
turn_queue_chanbind(struct TurnSocketData *data)
{
    char buf[STUN_MAX_MESSAGE_SIZE];

    StunAtrString username, *pUsername = NULL;
    StunAtrString password, *pPassword = NULL;
    StunAtrString nonce, *pNonce = NULL;
    StunAtrString realm, *pRealm = NULL;
  
    int len;
    mblk_t *new_blk;
  
    ortp_warning("Refreshing channel binding on fd %d channel %x...", data->fd, data->channel);

    if (data->auth_username != NULL) {
        snprintf(username.value, sizeof(username.value), "%s", data->auth_username);
        username.sizeValue = strlen(username.value);
        pUsername = &username;
    }
  
    if (data->auth_password != NULL) {
        snprintf(password.value, sizeof(password.value), "%s", data->auth_password);
        password.sizeValue = strlen(password.value);
        pPassword = &password;
    }
  
    if (data->auth_realm != NULL) {
        snprintf(realm.value, sizeof(realm.value), "%s", data->auth_realm);
        realm.sizeValue = strlen(realm.value);
        pRealm = &realm;
    }
  
    if (data->auth_nonce != NULL) {
        snprintf(nonce.value, sizeof(nonce.value), "%s", data->auth_nonce);
        nonce.sizeValue = strlen(nonce.value);
        pNonce = &nonce;
    }
  
    len = turnBuildChannelBind(buf, sizeof(buf), pUsername, pPassword, 
                                    data->channel, &data->bind_addr,
                                    pRealm, pNonce);
    ortp_debug("%s: About to send ChannelBind msg of len %i to fd %d", FUNC, len, data->fd);
    data->state = TurnSessionSentBindRefresh;

    new_blk = allocb(len, 0);
    if (new_blk == NULL) {
        ortp_warning("%s: PANIC: out of memory for local buffers of %d bytes", FUNC, len);
        data->state = TurnSessionFailed;
        return -1;
    }

    msgappend(new_blk, buf, len, FALSE);
    putq(&data->send_q, new_blk);
    return 1;
}

static int
turn_session_check_for_refresh(struct TurnSocketData *data)
{
    time_t now;

    if (data == NULL)
        return -1;

#if defined(_WIN32_WCE)
	now = timeGetTime()/1000;
#else
    now = time(NULL);
#endif
    if (data->state == TurnSessionStable &&
        data->alloc_expires - now < TURN_ALLOCATION_REFRESH_INTERVAL) {
        return turn_queue_refresh(data);
    }

    if (data->state == TurnSessionStable &&
        data->channel_expires - now < TURN_CHANNEL_REFRESH_INTERVAL) {
        return turn_queue_chanbind(data);
    }

    return 0;
}

static int
turn_session_process_turn_message(struct TurnSocketData *data, char* buf, int len)
{
  int rc, err = -1;
  char* err_reason = NULL;
  StunMessage resp;

  if (!stunParseMessage(buf, len, &resp)) {
      ortp_warning("%s: stunParseMessage() failed: len = %d", FUNC, len);
      data->state = TurnSessionFailed;
      return -1;
  }

  switch (data->state) {
    case TurnSessionSentRefresh:
      if (STUN_IS_SUCCESS_RESP(resp.msgHdr.msgType)) {
        if (!resp.hasLifetimeAttributes) {
          ortp_warning("%s: got Refresh success WITHOUT lifetime!");
          resp.lifetimeAttributes.lifetime = TURN_DEFAULT_ALLOCATION_LIFETIME;
        }

        data->bad_nonce_count = 0;
        data->state = TurnSessionStable;
#if defined(_WIN32_WCE)
		data->alloc_expires = timeGetTime()/1000 + resp.lifetimeAttributes.lifetime;
#else
        data->alloc_expires = time(NULL) + resp.lifetimeAttributes.lifetime;
#endif
        ortp_warning("%s: SUCCESSFULLY refreshed Allocation on fd %d with lifetime %d", FUNC, data->fd, resp.lifetimeAttributes.lifetime);
      } else {
        if (resp.hasErrorCode==TRUE) {
            err = (100 * resp.errorCode.errorClass) + resp.errorCode.number;
            err_reason = resp.errorCode.reason;
        }

        ortp_debug("%s: got err %d in msg type %x on fd %d", FUNC, err, resp.msgHdr.msgType, data->fd);

        if (err == 438 && resp.hasNonce == TRUE && data->bad_nonce_count == 0) {
          if (data->auth_nonce != NULL)
              ortp_free(data->auth_nonce); 

          data->auth_nonce = ortp_malloc(resp.nonceName.sizeValue + 1);
          if (data->auth_nonce == NULL) {
              ortp_warning("%s: error allocating memory for auth, failing", FUNC);
              data->state = TurnSessionFailed;
              return -1;
          }

          memcpy(data->auth_nonce, resp.nonceName.value, resp.nonceName.sizeValue);
          data->auth_nonce[resp.nonceName.sizeValue] = '\0';
          data->bad_nonce_count++;

          rc = turn_queue_refresh(data);
          if (rc)
              turn_flush_sendq(data, 0);    /* XXXrkb: what flags? */
          return rc;
        }

        ortp_warning("%s: unexpected response %x to Alloc refresh on fd %d (err %d, reason %s)", FUNC, resp.msgHdr.msgType, data->fd, err, err_reason ? err_reason : "<NONE>");
        data->state = TurnSessionFailed;
        return -1;
      }
      break;

    case TurnSessionSentBindRefresh:
      if (STUN_IS_SUCCESS_RESP(resp.msgHdr.msgType)) {
        data->state = TurnSessionStable;
    
        // ChanBind implicitly adds a permission for the bound address/port,
        // which expires much sooner than the actual binding.  So we set the
        // expiration to the permissions' lifetime.
        data->bad_nonce_count = 0;
#if defined(_WIN32_WCE)
		data->alloc_expires = timeGetTime()/1000 + TURN_PERMISSION_LIFETIME;
#else
        data->channel_expires = time(NULL) + TURN_PERMISSION_LIFETIME;
#endif
        ortp_warning("%s: SUCCESSFULLY refreshed channel binding on fd %d, channel %x", FUNC, data->fd, data->channel);
      } else {
        if (resp.hasErrorCode==TRUE) {
            err = (100 * resp.errorCode.errorClass) + resp.errorCode.number;
            err_reason = resp.errorCode.reason;
        }

        ortp_debug("%s: got err %d in msg type %x on fd %d", FUNC, err, resp.msgHdr.msgType, data->fd);

        if (err == 438 && resp.hasNonce == TRUE && data->bad_nonce_count == 0) {
          if (data->auth_nonce != NULL)
              ortp_free(data->auth_nonce); 

          data->auth_nonce = ortp_malloc(resp.nonceName.sizeValue + 1);
          if (data->auth_nonce == NULL) {
              ortp_warning("%s: error allocating memory for auth, failing", FUNC);
              data->state = TurnSessionFailed;
              return -1;
          }

          memcpy(data->auth_nonce, resp.nonceName.value, resp.nonceName.sizeValue);
          data->auth_nonce[resp.nonceName.sizeValue] = '\0';
          data->bad_nonce_count++;

          rc = turn_queue_chanbind(data);
          if (rc)
              turn_flush_sendq(data, 0);    /* XXXrkb: what flags? */
          return rc;
        }

        ortp_warning("%s: unexpected response %x to chanbind refresh on fd %d, channel %x (err %d, reason %s)", FUNC, resp.msgHdr.msgType, data->fd, data->channel, err, err_reason ? err_reason : "<NONE>");
        data->state = TurnSessionFailed;
        return -1;
      }
		  break;
  default:
	  break;
  }

  return 0;
}

static mblk_t *
turn_encapsulate_rtp_packet(struct TurnSocketData *data, const char* hdr, int hdrlen, mblk_t *m)
{
    mblk_t *new_blk;
    int len, plen, alloc_len;

    len = msgdsize(m);
    plen = (len + 3) & ~3;
    alloc_len = plen + (hdr == NULL ? sizeof(TurnChannelDataHdr) : hdrlen);

    new_blk = allocb(alloc_len, 0);
    if (new_blk == NULL) {
        ortp_warning("%s: PANIC: out of memory for local buffers of %d bytes", FUNC, alloc_len);
        return NULL;
    }
    
    // Generate a header if none passed in...
    if (hdr == NULL) {
        TurnChannelDataHdr chanDataHdr;

        chanDataHdr.chanId = htons(data->channel);
        chanDataHdr.msgLength = htons(len);

        msgappend(new_blk, (char*)&chanDataHdr, sizeof(chanDataHdr), FALSE);
    } else {
        msgappend(new_blk, hdr, hdrlen, FALSE);
    }

    // coalesce msg buffers
    msgpullup(m, -1);
    msgappend(new_blk, (const char *)m->b_rptr, len, FALSE);

    return new_blk;
}

static int
turn_flush_sendq(struct TurnSocketData *data, int flags)
{
    mblk_t *mp;
    int len, slen, n;

    mp = peekq(&data->send_q);

    len = 0;
    while (mp != NULL) {
        mblk_t *tmp;

        slen = msgdsize(mp);
        msgpullup(mp, -1);

        n = (int)send(data->fd, (const char *)mp->b_rptr, slen, flags);
        if (n < 0) {
            int errnum = getSocketErrorCode();
            if (!is_would_block_error(errnum)) {
                ortp_warning("%s: socket error with %d/%d bytes sent: %s",
                         FUNC, n, slen, getSocketError());
                return -1;
            }

            n = 0;
        }

        len += n;
        mp->b_rptr += n; 

        /* Partial write, need more data to flush */
        if (n < slen) return len;

        tmp = getq(&data->send_q);
        freemsg(tmp);
        mp = peekq(&data->send_q);
    }

    return len;
}

static int
rtp_turn_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen)
{
	struct TurnSocketData *data = (struct TurnSocketData *)session->rtp.tr->data;
    TurnChannelDataHdr chanDataHdr;
    int fd, n, len, slen;
    mblk_t *mp;

    turn_session_check_for_refresh(data);

    if (!TurnSessionCanSend(data)) {
        ortp_warning("%s: discarding %d bytes of data since no TURN channel is active (state = %d)", FUNC, msgdsize(m), data->state);
        return 0;
    }

    /* todo: review and make efficient
     *
     * need to queue remainder of partial writes, but only after connected
     * may or may not want to queue whole writes that would block
     * use queue_t in t->data from str_utils.h? or just a mblk_t?
     */
    mp = peekq(&data->send_q);
    if (mp != NULL) {
        // Append the current message to the send queue, then process as much
        // of the queue as we can...
        mp = turn_encapsulate_rtp_packet(data, NULL, 0, m);
        if (mp != NULL) {
            // Pad message to 4-byte multiple
            msgappend(mp, NULL, 0, TRUE);
            putq(&data->send_q, mp);
        }

        return turn_flush_sendq(data, flags);
    }

    fd = session->rtp.tr->t_rtp_getsocket(session);

    // Save original (un-padded) length
    len = msgdsize(m);

    // Pad the message to multiple of 4 bytes
    msgappend(m, NULL, 0, TRUE);

    chanDataHdr.chanId = htons(data->channel);
    chanDataHdr.msgLength = htons(len);

    // send ChannelData header
    slen = sizeof(chanDataHdr);
    n = (int)send(fd, (const char *)&chanDataHdr, slen, flags);
    if (n < 0) {
        int errnum = getSocketErrorCode();
        if (!is_would_block_error(errnum)) {
            ortp_warning("%s: socket error with %d/%d bytes sent: %s",
                         FUNC, n, slen, getSocketError());
            return -1;
        }

        /* Buffer below */
        n = 0;
    }

    if (n < slen) {
        mp = turn_encapsulate_rtp_packet(data, (const char*)&chanDataHdr + n, slen - n, m);
        if (mp != NULL)
            putq(&data->send_q, mp);
        return 0;    // No actual data sent...
    }

    // Re-check length in case padding affected it
    len = msgdsize(m);

    // coalesce msg buffers
    msgpullup(m, -1);

    n = (int)send(fd, (const char *)m->b_rptr, len, flags);
    if (n < 0) {
        int errnum = getSocketErrorCode();
        if (!is_would_block_error(errnum)) {
            ortp_warning("%s: socket error with %d/%d bytes sent: %s",
                         FUNC, n, slen, getSocketError());
            return -1;
        }

        /* Buffer below */
        n = 0;
    }

    m->b_rptr += n;

    if (n < len) {
        mp = allocb(len - n, 0);

        if (mp == NULL) {
            ortp_warning("%s: PANIC: out of memory for local buffers of %d bytes", FUNC, len - n);
            return -1;
        }
        
        msgappend(mp, (const char *)m->b_rptr, len - n, FALSE);
        putq(&data->send_q, mp);

        ortp_debug("%s: sent partial oRTP message on channel %x, sent %d/%d", FUNC, data->channel, n, len);
    } else {
        ortp_debug("%s: sent oRTP message on channel %x, len %d", FUNC, data->channel, n);
    }

    return n;
}

static int
rtcp_turn_sendto(struct _RtpSession *session, mblk_t *m, int flags, const struct sockaddr *to, socklen_t tolen)
{
	struct TurnSocketData *data = (struct TurnSocketData *)session->rtcp.tr->data;
    TurnChannelDataHdr chanDataHdr;
    int fd, n, len, slen;
    mblk_t *mp;

    turn_session_check_for_refresh(data);

    if (!TurnSessionCanSend(data)) {
        ortp_warning("%s: discarding %d bytes of data since no TURN channel is active (state = %d)", FUNC, msgdsize(m), data->state);
        return 0;
    }

    /* todo: review and make efficient
     *
     * need to queue remainder of partial writes, but only after connected
     * may or may not want to queue whole writes that would block
     * use queue_t in t->data from str_utils.h? or just a mblk_t?
     */
    mp = peekq(&data->send_q);
    if (mp != NULL) {
        // Append the current message to the send queue, then process as much
        // of the queue as we can...
        mp = turn_encapsulate_rtp_packet(data, NULL, 0, m);
        if (mp != NULL) {
            // Pad message to 4-byte multiple
            msgappend(mp, NULL, 0, TRUE);
            putq(&data->send_q, mp);
        }

        return turn_flush_sendq(data, flags);
    }

    fd = session->rtcp.tr->t_rtcp_getsocket(session);

    // Save original (un-padded) length
    len = msgdsize(m);

    // Pad the message to multiple of 4 bytes
    msgappend(m, NULL, 0, TRUE);

    chanDataHdr.chanId = htons(data->channel);
    chanDataHdr.msgLength = htons(len);

    // send ChannelData header
    slen = sizeof(chanDataHdr);
    n = (int)send(fd, (const char *)&chanDataHdr, slen, flags);
    if (n < 0) {
        int errnum = getSocketErrorCode();
        if (!is_would_block_error(errnum)) {
            ortp_warning("%s: socket error with %d/%d bytes sent: %s",
                         FUNC, n, slen, getSocketError());
            return -1;
        }

        /* Buffer below */
        n = 0;
    }

    if (n < slen) {
        mp = turn_encapsulate_rtp_packet(data, (const char*)&chanDataHdr + n, slen - n, m);
        if (mp != NULL)
            putq(&data->send_q, mp);
        return 0;    // No actual data sent...
    }

    // Re-check length in case padding affected it
    len = msgdsize(m);

    // coalesce msg buffers
    msgpullup(m, -1);

    n = (int)send(fd, (const char *)m->b_rptr, len, flags);
    if (n < 0) {
        int errnum = getSocketErrorCode();
        if (!is_would_block_error(errnum)) {
            ortp_warning("%s: socket error with %d/%d bytes sent: %s",
                         FUNC, n, slen, getSocketError());
            return -1;
        }

        /* Buffer below */
        n = 0;
    }

    m->b_rptr += n;

    if (n < len) {
        mp = allocb(len - n, 0);

        if (mp == NULL) {
            ortp_warning("%s: PANIC: out of memory for local buffers of %d bytes", FUNC, len - n);
            return -1;
        }
        
        msgappend(mp, (const char *)m->b_rptr, len - n, FALSE);
        putq(&data->send_q, mp);

        ortp_debug("%s: sent partial oRTP message on channel %x, sent %d/%d", FUNC, data->channel, n, len);
    } else {
        ortp_debug("%s: sent oRTP message on channel %x, len %d", FUNC, data->channel, n);
    }

    return n;
}

static int
turn_extract_rtp_packet(struct TurnSocketData *data, mblk_t *m)
{
    /* unfragment data->recv_m */
    //msgpullup(data->recv_m, -1);

    // If we're not in the middle of a packet, extract the header and go
    // from there..
    if (data->recv_need == 0) {
        TurnChannelDataHdr chanDataHdr;

        if (data->recv_left < sizeof(TurnChannelDataHdr)) return 0;

        memcpy(&chanDataHdr, data->recv_m->b_rptr, sizeof(chanDataHdr));
        chanDataHdr.chanId = ntohs(chanDataHdr.chanId);
        chanDataHdr.msgLength = ntohs(chanDataHdr.msgLength);

        // If the high bit is set, this is neither a standard STUN message
        // nor a valid TURN ChannelData message; skip through the data 
        // until we find a a header that could pass for one of the above.
        //
        if (chanDataHdr.chanId & 0x8000) {
            int skiplen = 0;

            do {
                skiplen += sizeof(TurnChannelDataHdr);
                data->recv_left -= sizeof(TurnChannelDataHdr);
                data->recv_m->b_rptr += sizeof(TurnChannelDataHdr);

                if (data->recv_left < sizeof(TurnChannelDataHdr))
                    return 0;

                memcpy(&chanDataHdr, data->recv_m->b_rptr, sizeof(chanDataHdr));
                chanDataHdr.chanId = ntohs(chanDataHdr.chanId);
                chanDataHdr.msgLength = ntohs(chanDataHdr.msgLength);
            } while ((chanDataHdr.chanId & 0x8000) == 0 &&
                     ((chanDataHdr.chanId & 0x4000) == 0x4000 ||
                      (chanDataHdr.msgLength & 0x03) == 0));

            ortp_warning("%s: received unexpected message starting with high bit set, skipped %d bytes", FUNC, skiplen);
        }

        if ((chanDataHdr.chanId & 0x4000) == 0x4000) {
            chanDataHdr.msgLength += sizeof(chanDataHdr);
        } else {
            chanDataHdr.msgLength += sizeof(StunMsgHdr);
            data->have_turn_msg = 1;
        }
    
        if (!data->skipping && !data->have_turn_msg && 
            chanDataHdr.chanId != data->channel) {
            ortp_warning("%s: chn_id %x doesn't match expected %x, skipping %d bytes", FUNC, chanDataHdr.chanId, data->channel, data->recv_need);
            data->skipping = 1;
        }
    
        if (!data->skipping && chanDataHdr.msgLength > 1500) {
            ortp_warning("%s: len too big, skipping %d bytes", FUNC, chanDataHdr.msgLength);
            data->skipping = 1;
        }

        // NB: over TCP all channel data messages must be padded to
        // multiple of 4 bytes; normal STUN/TURN mesages are also a
        // multiple of 4 bytes since the header is 20 bytes and all
        // attributes are required to be padded to 4 bytes.
        data->recv_size = chanDataHdr.msgLength;
        data->recv_need = (chanDataHdr.msgLength + 3) & ~3;
    }

    // Now check to see if we can pass a complete packet up the stack
    if (data->recv_need > 0 && data->recv_left >= data->recv_need) {
        int sz = 0;

        if (!data->skipping) {
            if (data->have_turn_msg) {
                turn_session_process_turn_message(data, (char*)data->recv_m->b_rptr, data->recv_size);
                ortp_debug("%s: received TURN message on fd %d, len %d", FUNC, data->fd, data->recv_size);
            } else {
                sz = data->recv_size - sizeof(TurnChannelDataHdr);
                msgappend(m, (char*)data->recv_m->b_rptr + sizeof(TurnChannelDataHdr), sz, FALSE);
                ortp_debug("%s: received oRTP message for channel %x, len %d", FUNC, data->channel, sz);
#ifdef TRACEFILE
                if (f && data->channel == 0x4000) fprintf(f, "%s: %lu.%06ld %lu.%06ld extract %d (skipping %d, left: was %d, will be %d)\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, sz, data->skipping, data->recv_left, data->recv_left - data->recv_need);
#endif
            }
        }

        data->recv_m->b_rptr += data->recv_need;
        data->recv_left -= data->recv_need;
        data->recv_size = data->recv_need = 0;
        data->have_turn_msg = 0;
        data->skipping = 0;
        return sz;
    }

    return 0;
}

static int
rtp_turn_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    struct TurnSocketData *data = (struct TurnSocketData *)session->rtp.tr->data;


    /* todo: review and make efficient
     *
     * definitely need to finish any partial reads
     * probably want to queue whole writes that would block
     * use queue_t in t->data from str_utils.h for write queue and read queue?
     */
    int r;
    int fd = session->rtp.tr->t_rtp_getsocket(session);

#ifdef TRACEFILE
    if (!f) { f = fopen(TRACEFILE,"w"); }
    if (f && data->channel == 0x4000) { 
        gettimeofday(&tv,NULL);
        if (last_tv.tv_sec > 0) {
            diff.tv_sec = tv.tv_sec - last_tv.tv_sec;
            diff.tv_usec = tv.tv_usec - last_tv.tv_usec;
            if (diff.tv_usec < 0) {
                diff.tv_usec += 1000000;
                --diff.tv_sec;
            }
        } else {
            diff.tv_sec = diff.tv_usec = 0;
        }
        last_tv.tv_sec = tv.tv_sec;
        last_tv.tv_usec = tv.tv_usec;
    }
#endif

    /* Check if we need to refresh, and if so, send the queued requests */
    if (turn_session_check_for_refresh(data) > 0)
        turn_flush_sendq(data, 0);    /* XXXrkb: what flags? */

    // First check if we can pull any complete messages out of buffered receive data
    if (data->recv_left > 0) {
        r = turn_extract_rtp_packet(data, m);
#ifdef TRACEFILE
        if (f && data->channel == 0x4000 && r > 0) fprintf(f, "%s: %lu.%06ld %lu.%06ld no-recv-return %d\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, r);
#endif
        if (r > 0) return r;
    }

    if (!TurnSessionCanRecv(data)) {
        ortp_warning("%s: ignoring read since no TURN channel is active (state = %d)", FUNC, data->state);
        return 0;
    }
 
    // Rewind buffer back to beginning if we don't have any data in it..
    if (data->recv_left == 0)
        data->recv_m->b_rptr = data->recv_m->b_wptr = data->recv_m->b_datap->db_base;

    /* recv into data->recv_m */
    r = (int)recv(fd, (char *)data->recv_m->b_wptr, data->recv_m->b_datap->db_lim-data->recv_m->b_wptr, flags);
#ifdef TRACEFILE
    if (f && data->channel == 0x4000) fprintf(f, "%s: %lu.%06ld %lu.%06ld recv(%d) returns %d\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, data->recv_m->b_datap->db_lim-data->recv_m->b_wptr, r);
#endif
    if (r > 0)  {
        data->recv_m->b_wptr += r;
        data->recv_left += r;
    }

    r = turn_extract_rtp_packet(data, m);
#ifdef TRACEFILE
    if (f && data->channel == 0x4000) fprintf(f, "%s: %lu.%06ld %lu.%06ld return %d\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, r);
#endif
    return r;
}

static int
rtcp_turn_recvfrom(struct _RtpSession *session, mblk_t *m, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    struct TurnSocketData *data = (struct TurnSocketData *)session->rtcp.tr->data;


    /* todo: review and make efficient
     *
     * definitely need to finish any partial reads
     * probably want to queue whole writes that would block
     * use queue_t in t->data from str_utils.h for write queue and read queue?
     */
    int r;
    int fd = session->rtcp.tr->t_rtp_getsocket(session);

#ifdef TRACEFILE
    if (!f) { f = fopen(TRACEFILE,"w"); }
    if (f && data->channel == 0x4000) { 
        gettimeofday(&tv,NULL);
        if (last_tv.tv_sec > 0) {
            diff.tv_sec = tv.tv_sec - last_tv.tv_sec;
            diff.tv_usec = tv.tv_usec - last_tv.tv_usec;
            if (diff.tv_usec < 0) {
                diff.tv_usec += 1000000;
                --diff.tv_sec;
            }
        } else {
            diff.tv_sec = diff.tv_usec = 0;
        }
        last_tv.tv_sec = tv.tv_sec;
        last_tv.tv_usec = tv.tv_usec;
    }
#endif

    /* Check if we need to refresh, and if so, send the queued requests */
    if (turn_session_check_for_refresh(data) > 0)
        turn_flush_sendq(data, 0);    /* XXXrkb: what flags? */

    // First check if we can pull any complete messages out of buffered receive data
    if (data->recv_left > 0) {
        r = turn_extract_rtp_packet(data, m);
#ifdef TRACEFILE
        if (f && data->channel == 0x4000 && r > 0) fprintf(f, "%s: %lu.%06ld %lu.%06ld no-recv-return %d\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, r);
#endif
        if (r > 0) return r;
    }

    if (!TurnSessionCanRecv(data)) {
        ortp_warning("%s: ignoring read since no TURN channel is active (state = %d)", FUNC, data->state);
        return 0;
    }
 
    // Rewind buffer back to beginning if we don't have any data in it..
    if (data->recv_left == 0)
        data->recv_m->b_rptr = data->recv_m->b_wptr = data->recv_m->b_datap->db_base;

    /* recv into data->recv_m */
    r = (int)recv(fd, (char *)data->recv_m->b_wptr, data->recv_m->b_datap->db_lim-data->recv_m->b_wptr, flags);
#ifdef TRACEFILE
    if (f && data->channel == 0x4000) fprintf(f, "%s: %lu.%06ld %lu.%06ld recv(%d) returns %d\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, data->recv_m->b_datap->db_lim-data->recv_m->b_wptr, r);
#endif
    if (r > 0)  {
        data->recv_m->b_wptr += r;
        data->recv_left += r;
    }

    r = turn_extract_rtp_packet(data, m);
#ifdef TRACEFILE
    if (f && data->channel == 0x4000) fprintf(f, "%s: %lu.%06ld %lu.%06ld return %d\n", FUNC, tv.tv_sec, tv.tv_usec, diff.tv_sec, diff.tv_usec, r);
#endif
    return r;
}

static ortp_socket_t 
turn_rtp_get_socket(const struct _RtpSession *session)
{
    struct TurnSocketData *data = (struct TurnSocketData *)session->rtp.tr->data;
    return data->fd;
}

static ortp_socket_t 
turn_rtcp_get_socket(const struct _RtpSession *session)
{
    struct TurnSocketData *data = (struct TurnSocketData *)session->rtcp.tr->data;
    return data->fd;
}


static void
turn_socket_data_init(struct TurnSocketData *data)
{
    data->fd = -1;
    data->state = TurnSessionInit;
    qinit(&data->send_q);
    data->send_remain = 0;
    data->recv_m = allocb(4096, 0);
    data->recv_need = 0;
    data->recv_size = 0;
    data->recv_left = 0;
    data->skipping = 0;
    data->have_turn_msg = 0;

    data->auth_username = NULL;
    data->auth_password = NULL;
    data->auth_realm = NULL;
    data->auth_nonce = NULL;
    data->bad_nonce_count = 0;

    data->channel = 0;
    data->relay_addr.addr = 0;
    data->relay_addr.port = 0;
    data->bind_addr.addr = 0;
    data->bind_addr.port = 0;
}

static void
turn_socket_data_destroy(struct TurnSocketData *data)
{
    if (data->fd != -1) closesocket(data->fd);
    flushq(&data->send_q, FLUSHALL);
    freeb(data->recv_m);

    if (data->auth_username) ortp_free(data->auth_username);
    if (data->auth_password) ortp_free(data->auth_password);
    if (data->auth_realm) ortp_free(data->auth_realm);
    if (data->auth_nonce) ortp_free(data->auth_nonce);
}

static int
turn_connect(struct TurnSocketData *data, const char *relay_server)
{
    unsigned short port = 3478;

    // parse addr
    int err, on = 1;
    struct addrinfo ai, *res;
    char strport[8];
    memset(&ai, 0, sizeof(ai));
    ai.ai_family = AF_INET;
    ai.ai_socktype = SOCK_STREAM;
    sprintf(strport, "%d", port);
    err = getaddrinfo(relay_server, strport, &ai, &res);
    if (err) {
        ortp_warning("TURN: Error in relay_server address %s: %s", relay_server, getSocketError());
        return -1;
    }

    // create socket
    data->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (data->fd < 0) {
        ortp_warning("TURN: unable to create socket");
        goto done;
    }

    if (setsockopt(data->fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on)) != 0) {
        ortp_warning("TURN: unable to set TCP_NODELAY on socket");
    }

    // connect
    err = connect(data->fd, res->ai_addr, res->ai_addrlen);
    if (err < 0) {
        ortp_warning("TURN: connect() failed: %s", getSocketError());
        closesocket(data->fd);
        data->fd = -1;
        goto done;
    }

    // this happens later in rtp_session_set_sockets
    //set_non_blocking_socket(fd);

done:
    if (data->fd == -1) {
        data->state = TurnSessionFailed;
    } else {
        data->state = TurnSessionConnected;
    }

    freeaddrinfo(res);
    return data->fd;
}

/* ***UNFINISHED WORK*** */
/* TODO: add some APIs to enable turn control from application */

RtpTurnData *rtp_turn_data_new(const char *relay_server);
void rtp_turn_data_destroy(RtpTurnData *data);

RtpTurnData *
rtp_turn_data_new(const char *relay_server)
{
    RtpTurnData *data = ortp_new(RtpTurnData,1);
    TurnAtrReservationToken resToken;

    turn_socket_data_init(&data->rtp);
    turn_socket_data_init(&data->rtcp);
    if (turn_connect(&data->rtp, relay_server) < 0
        || turn_connect(&data->rtcp, relay_server) < 0) {
        rtp_turn_data_destroy(data);
        ortp_free(data);
        return NULL;
    }

    /* XXXrkb: store this in am_options somewhere? */
    data->rtp.auth_username = ortp_strdup("kenstir");
    data->rtp.auth_password = ortp_strdup("password");

    data->rtcp.auth_username = ortp_strdup("kenstir");
    data->rtcp.auth_password = ortp_strdup("password");

    if (turn_allocate(&data->rtp, &resToken, TRUE) < 0 
        || turn_allocate(&data->rtcp, &resToken, FALSE) < 0) {
        rtp_turn_data_destroy(data);
        ortp_free(data);
        return NULL;
    }

    return data;
}

void
rtp_turn_data_destroy(RtpTurnData *data)
{
    turn_socket_data_destroy(&data->rtp);
    turn_socket_data_destroy(&data->rtcp);
}

static void
turn_destroy(struct _RtpTransport *transport)
{
    struct TurnSocketData *data = (struct TurnSocketData *)transport->data;
    turn_socket_data_destroy(data);
	transport->data=NULL;
}

static int turn_global_init(void)
{
	return 0;
}

static int turn_global_shutdown(void)
{
	return 0;
}

int turn_transport_setup(void);

int turn_transport_setup(void)
{
	RtpTransport *tr = ortp_new0(RtpTransport,1);
	snprintf(tr->name, sizeof(tr->name), "TURN");

	tr->t_global_init = turn_global_init;
	tr->t_global_shutdown = turn_global_shutdown;
	tr->t_set_option = NULL;

	tr->t_rtp_getsocket=turn_rtp_get_socket;
	tr->t_rtcp_getsocket=turn_rtcp_get_socket;
	
	tr->t_rtp_sendto=rtp_turn_sendto;
	tr->t_rtp_recvfrom=rtp_turn_recvfrom;
	tr->t_rtcp_sendto=rtcp_turn_sendto;
	tr->t_rtcp_recvfrom=rtcp_turn_recvfrom;
	tr->t_destroy=turn_destroy;

	ortp_transport_add(tr);

    return 0;
}

int
rtp_turn_get_relay_addr(RtpTurnData *data, StunAddress4 *relay_addr)
{
    if (data && data->rtp.relay_addr.port > 0) {
        memcpy(relay_addr, &data->rtp.relay_addr, sizeof(*relay_addr));
        return 0;
    } else {
        ortp_warning("%s: no relayed rtp port", FUNC);
        return -1;
    }
}

static int
sendTurnMessage(int fd, const char* buf, unsigned int len)
{
    int n, flags = 0;
    const char* bufptr = buf;
    int buflen = len, bufsent = 0;

    do {
        n = (int)send(fd, bufptr, buflen, flags);
        if (n < 0) {
            int errnum = getSocketErrorCode();
            if (!is_would_block_error(errnum)) {
                ortp_warning("%s: socket error with %d/%d bytes sent: %s",
                             FUNC, n, buflen, getSocketError());
                return -1;
            }

            /* Buffer below */
            n = 0;
        }

        bufptr += n;
        bufsent += n;
        buflen -= n;
    } while (bufsent < len);

    return bufsent;
}

static int
readTurnMessage(int fd, char* buf, unsigned int buflen)
{
    int r;
    int flags = 0;
    int hdrlen, nextlen, numread = 0;
    char* bufptr = buf;
    uint16_t mtype, mlen;

    hdrlen = sizeof(uint32_t);
    if (buflen < hdrlen) {
        ortp_warning("readTurnMessage: buf size too small for Stun header!");
        return -1;
    }

    do {
        r = (int)recv(fd, bufptr, hdrlen - numread, flags);
        if (r < 0) {
            int errnum = getSocketErrorCode();
            if (!is_would_block_error(errnum)) {
                ortp_warning("%s: socket error with %d/%d bytes read: %s",
                             FUNC, numread, hdrlen, getSocketError());
                return -1;
            }
        } else {
            numread += r;
            bufptr += r;
            buflen -= r;
        }
    } while (numread < hdrlen);

    // XXXrkb: use ChanDataHdr here?
    memcpy(&mtype, buf, sizeof(uint16_t));
    mtype = ntohs(mtype);
    memcpy(&mlen, buf + sizeof(uint16_t), sizeof(uint16_t));
    mlen = ntohs(mlen);

    if ((mtype & 0xc000) == 0) {
        /* Standard STUN message */
        nextlen = mlen + sizeof(StunMsgHdr) - numread;
    } else if ((mtype & 0xc000) == 0x4000) {
        /* ChanData message */
        nextlen = ((mlen + sizeof(uint32_t) + 3) & ~3) - numread;
    } else {
        /* neither... need to re-synch */
        ortp_warning("readTurnMessage: unexpected mtype %x!", mtype);
        return -1;
    }

    /* Got everything we needed... must have been a 0-length ChanData msg */
    if (nextlen == 0) 
        return numread;

    if (buflen < nextlen) {
        ortp_warning("readTurnMessage: buf size too small for Stun message!");
        return -1;
    }

    do {
        r = (int)recv(fd, bufptr, nextlen, flags);
        if (r < 0) {
            int errnum = getSocketErrorCode();
            if (!is_would_block_error(errnum)) {
                ortp_warning("%s: socket error with %d/%d bytes read: %s",
                             FUNC, numread, hdrlen, getSocketError());
                return -1;
            }

            r = 0;
        }

        numread += r;
        bufptr += r;
        nextlen -= r;
    } while (nextlen > 0);

    return numread;
}

static int 
turn_allocate(struct TurnSocketData *data, TurnAtrReservationToken* resToken, bool_t reservePortPair)
{
  int slen;
  StunMessage resp;
  char buf[STUN_MAX_MESSAGE_SIZE];
  int len = STUN_MAX_MESSAGE_SIZE;

  StunAtrString username, *pUsername = NULL;
  StunAtrString password, *pPassword = NULL;
  StunAtrString realm, *pRealm = NULL;
  StunAtrString nonce, *pNonce = NULL;

  if (data == NULL) {
     ortp_warning("%s: no TURN socket data", FUNC);
     return -1;
  }

  if (data->auth_username != NULL) {
      snprintf(username.value, sizeof(username.value), "%s", data->auth_username);
      username.sizeValue = strlen(username.value);
      pUsername = &username;
  }

  if (data->auth_password != NULL) {
      snprintf(password.value, sizeof(password.value), "%s", data->auth_password);
      password.sizeValue = strlen(password.value);
      pPassword = &password;
  }

again:
  if (data->auth_realm != NULL) {
      snprintf(realm.value, sizeof(realm.value), "%s", data->auth_realm);
      realm.sizeValue = strlen(realm.value);
      pRealm = &realm;
  }

  if (data->auth_nonce != NULL) {
      snprintf(nonce.value, sizeof(nonce.value), "%s", data->auth_nonce);
      nonce.sizeValue = strlen(nonce.value);
      pNonce = &nonce;
  }

  len = turnBuildAllocate(buf, sizeof(buf), pUsername, pPassword, pRealm, pNonce, resToken, reservePortPair);
  ortp_debug("%s: About to send msg of len %i to fd %d", FUNC, len, data->fd);

  slen = sendTurnMessage(data->fd, buf, len);
  if (slen < len) {
    data->state = TurnSessionFailed;
    return -1;
  }

  data->state = TurnSessionSentAllocate;
  len = readTurnMessage(data->fd, buf, sizeof(buf));
  ortp_debug("%s: Read msg of len %i from fd %d", FUNC, len, data->fd);

  if (!stunParseMessage(buf, len, &resp)) {
      ortp_warning("%s: stunParseMessage() failed: len = %d", FUNC, len);
      data->state = TurnSessionFailed;
      return -1;
  }

  if (STUN_IS_ERR_RESP(resp.msgHdr.msgType))
  {
    int err = -1;

    if (resp.hasErrorCode==TRUE)
      err = (100 * resp.errorCode.errorClass) + resp.errorCode.number;

    /* check if we need to authenticate */
    if (err == 401 && resp.hasNonce == TRUE && resp.hasRealm == TRUE)
    {
      ortp_warning("%s: got err %d in msg type %x on fd %d", FUNC, err, resp.msgHdr.msgType, data->fd);

      /* If we have realm already, this is a bad-password failure */
      if (data->auth_realm != NULL) {
          ortp_warning("%s: got 401 after auth, failing", FUNC);
          return -1;
      }
       
      if (data->auth_nonce != NULL)
          ortp_free(data->auth_nonce); 

      data->auth_nonce = ortp_malloc(resp.nonceName.sizeValue + 1);
      if (data->auth_nonce == NULL) {
          ortp_warning("%s: error allocating memory for auth, failing", FUNC);
          data->state = TurnSessionFailed;
          return -1;
      }

      memcpy(data->auth_nonce, resp.nonceName.value, resp.nonceName.sizeValue);
      data->auth_nonce[resp.nonceName.sizeValue] = '\0';

      data->auth_realm = ortp_malloc(resp.realmName.sizeValue + 1);
      if (data->auth_realm == NULL) {
          ortp_warning("%s: error allocating memory for auth, failing", FUNC);
          data->state = TurnSessionFailed;
          return -1;
      }

      memcpy(data->auth_realm, resp.realmName.value, resp.realmName.sizeValue);
      data->auth_realm[resp.realmName.sizeValue] = '\0';

      data->bad_nonce_count = 0;
      goto again;
    }

    if (err == 438 && resp.hasNonce == TRUE && data->bad_nonce_count == 0) {
      ortp_warning("%s: got err %d in msg type %x on fd %d", FUNC, err, resp.msgHdr.msgType, data->fd);

      if (data->auth_nonce != NULL)
          ortp_free(data->auth_nonce); 

      data->auth_nonce = ortp_malloc(resp.nonceName.sizeValue + 1);
      if (data->auth_nonce == NULL) {
          ortp_warning("%s: error allocating memory for auth, failing", FUNC);
          data->state = TurnSessionFailed;
          return -1;
      }

      memcpy(data->auth_nonce, resp.nonceName.value, resp.nonceName.sizeValue);
      data->auth_nonce[resp.nonceName.sizeValue] = '\0';
      data->bad_nonce_count++;
      goto again;
    }

    ortp_warning("%s: error %d from allocate on fd %d", FUNC, err, data->fd);
    data->state = TurnSessionFailed;
    return -1;
  }
  else if (STUN_IS_SUCCESS_RESP(resp.msgHdr.msgType))
  {
    uint32_t cookie = 0x2112A442;
    uint16_t cookie16 = 0x2112A442 >> 16;

    if (!resp.hasXorRelayedAddress || 
        (reservePortPair && !resp.hasReservationToken)) {
      ortp_warning("%s: got success BUT without required attributes (relayed addr, reservation token, lifetime)",  FUNC);
      data->state = TurnSessionFailed;
      return -1;
    }

    /* If we don't get lifetime stick in default lifetime and try and go on */
    if (!resp.hasLifetimeAttributes) {
      ortp_warning("%s: got Allocate success WITHOUT lifetime!");
      resp.lifetimeAttributes.lifetime = TURN_DEFAULT_ALLOCATION_LIFETIME;
    }

    if (reservePortPair) {
        memcpy(resToken, &resp.reservationToken, sizeof(resp.reservationToken));
    }

    data->bad_nonce_count = 0;

    data->relay_addr.port = resp.xorRelayedAddress.ipv4.port^cookie16;
    data->relay_addr.addr = resp.xorRelayedAddress.ipv4.addr^cookie;

#if defined(_WIN32_WCE)
	data->alloc_expires = timeGetTime()/1000 + resp.lifetimeAttributes.lifetime;
#else
    data->alloc_expires = time(NULL) + resp.lifetimeAttributes.lifetime;
#endif
    ortp_warning("%s: got success with XorRelayedAddress, port %d, lifetime %d", FUNC, data->relay_addr.port, resp.lifetimeAttributes.lifetime);
    return 0;
  }

  ortp_warning("%s: unknown STUN type %x", FUNC, resp.msgHdr.msgType);
  data->state = TurnSessionFailed;
  return 0;
}

static int
turn_bind_chan(struct TurnSocketData *data, const char* raddr, unsigned int rport, unsigned int chan)
{
  int slen, ret;
  StunMessage resp;
  StunMessage *presp = NULL;
  char buf[STUN_MAX_MESSAGE_SIZE];
  int len = STUN_MAX_MESSAGE_SIZE;

  StunAtrString username, *pUsername = NULL;
  StunAtrString password, *pPassword = NULL;
  StunAtrString nonce, *pNonce = NULL;
  StunAtrString realm, *pRealm = NULL;

  if (data == NULL) {
     ortp_warning("%s: no TURN socket data", FUNC);
     return -1;
  }

  memset(&data->bind_addr, 0, sizeof(data->bind_addr));
  ret = stunParseServerName(raddr, &data->bind_addr);
  if (ret != TRUE || data->bind_addr.addr == 0) {
      ortp_error("turn: bad peer address for channel bind");
      data->state = TurnSessionFailed;
      return -1;
  }

  data->bind_addr.port = rport;

  if (data->auth_username != NULL) {
      snprintf(username.value, sizeof(username.value), "%s", data->auth_username);
      username.sizeValue = strlen(username.value);
      pUsername = &username;
  }

  if (data->auth_password != NULL) {
      snprintf(password.value, sizeof(password.value), "%s", data->auth_password);
      password.sizeValue = strlen(password.value);
      pPassword = &password;
  }

again:
  if (data->auth_realm != NULL) {
      snprintf(realm.value, sizeof(realm.value), "%s", data->auth_realm);
      realm.sizeValue = strlen(realm.value);
      pRealm = &realm;
  }

  if (data->auth_nonce != NULL) {
      snprintf(nonce.value, sizeof(nonce.value), "%s", data->auth_nonce);
      nonce.sizeValue = strlen(nonce.value);
      pNonce = &nonce;
  }

  len = turnBuildChannelBind(buf, sizeof(buf), pUsername, pPassword, chan,
                                  &data->bind_addr, pRealm, pNonce);
  ortp_debug("%s: About to send msg of len %i to fd %d", FUNC, len, data->fd);

  slen = sendTurnMessage(data->fd, buf, len);
  if (slen < len) {
    data->state = TurnSessionFailed;
    return -1;
  }

  data->state = TurnSessionSentChanBind;
  len = readTurnMessage(data->fd, buf, sizeof(buf));
  ortp_debug("%s: Read msg of len %i from fd %d", FUNC, len, data->fd);

  if (!stunParseMessage(buf, len, &resp)) {
      ortp_warning("%s: stunParseMessage() failed: len = %d", FUNC, len);
      data->state = TurnSessionFailed;
      return -1;
  }

  if (STUN_IS_ERR_RESP(resp.msgHdr.msgType))
  {
    /* check if we need to authenticate */
    if (resp.hasErrorCode==TRUE
      && resp.errorCode.errorClass==4 && resp.errorCode.number==1
      && resp.hasNonce == TRUE
      && resp.hasRealm == TRUE)
    {
      if (presp == NULL) {
          ortp_warning("%s: got 401, need to re-send allocate request", FUNC);
          presp = &resp;
          goto again;
      } else {
          ortp_warning("%s: got 401 after auth, failing", FUNC);
          data->state = TurnSessionFailed;
          return -1;
      }
    }

    ortp_warning("%s: error %d from chan_bind", FUNC, 100 * resp.errorCode.errorClass + resp.errorCode.number);
    data->state = TurnSessionFailed;
    return -1;
  }
  else if (STUN_IS_SUCCESS_RESP(resp.msgHdr.msgType))
  {
    data->channel = chan;
    data->state = TurnSessionStable;

    // ChanBind implicitly adds a permission for the bound address/port,
    // which expires much sooner than the actual binding.  So we set the
    // expiration to the permissions' lifetime.
#if defined(_WIN32_WCE)
	data->alloc_expires = timeGetTime()/1000 + TURN_PERMISSION_LIFETIME;
#else
    data->channel_expires = time(NULL) + TURN_PERMISSION_LIFETIME;
#endif
    return 0;
  }

  ortp_warning("%s: unknown STUN type %x", FUNC, resp.msgHdr.msgType);
  return 0;
}

int
turnBuildAllocate(char* buf, int buf_size,
            const StunAtrString *username, const StunAtrString *password,
            const StunAtrString *realm, const StunAtrString *nonce,
            const TurnAtrReservationToken *resToken, bool_t reservePortPair)
{
  StunMessage req;
  const char serverName[] = "oRTP   " STUN_VERSION; /* must pad to mult of 4 */

  memset(&req, 0, sizeof(StunMessage));

  stunBuildReqSimple( &req, username, FALSE , FALSE , 0 );
  req.msgHdr.msgType = (TURN_MEDHOD_ALLOCATE|STUN_REQUEST);

  req.hasSoftware = TRUE;
  memcpy( req.softwareName.value, serverName, sizeof(serverName));
  req.softwareName.sizeValue = sizeof(serverName);

  req.hasRequestedTransport = TRUE;
  memset(&req.requestedTransport, 0, sizeof(req.requestedTransport));
  req.requestedTransport.proto = IPPROTO_UDP;

  req.hasDontFragment = TRUE;

  /* 
   * NB: EVEN-PORT (which besides forcing an even port to be returned also
   * allocates the following port and returns us a grant to alocate it via
   * the returned reservation token) and use of the reservationt token are
   * mutually exclusive.  We can either use the reservation token from a
   * prior EVEN-PORT allocation or we can request a new allocation (with or
   * without the EVEN-PORT attribute).
   */
  if (reservePortPair) 
  {
      req.hasEvenPort = TRUE;
      req.evenPort.value = ReserveNextPort;
  }
  else if (resToken != NULL) 
  {
      req.hasReservationToken = TRUE;
      memcpy(&req.reservationToken, resToken, sizeof(*resToken));
  }

  if (username!=NULL && username->sizeValue>0
    && password!=NULL && password->sizeValue>0
    && realm!=NULL && realm->sizeValue>0
    && nonce!=NULL && nonce->sizeValue>0)
  {
    req.hasUsername = TRUE;
    memcpy( req.username.value, username->value, username->sizeValue );
    req.username.sizeValue = username->sizeValue;

    req.hasRealm = TRUE;
    memcpy( req.realmName.value, realm->value, realm->sizeValue );
    req.realmName.sizeValue = realm->sizeValue;

    req.hasNonce = TRUE;
    memcpy( req.nonceName.value, nonce->value, nonce->sizeValue );
    req.nonceName.sizeValue = nonce->sizeValue;

    req.hasMessageIntegrity = TRUE;
  }

  return stunEncodeMessage( &req, buf, buf_size, password );
}

int 
turnBuildChannelBind(char* buf, int buf_size,
             const StunAtrString *username, const StunAtrString *password,
             unsigned int chanNumber, const StunAddress4 *peerAddress, 
             const StunAtrString *realm, const StunAtrString *nonce)
{ 
  StunMessage req;
  const char serverName[] = "oRTP   " STUN_VERSION; /* must pad to mult of 4 */

  memset(&req, 0, sizeof(StunMessage));

  stunBuildReqSimple( &req,  NULL, FALSE, FALSE, 0 );
  req.msgHdr.msgType = (TURN_METHOD_CHANNELBIND|STUN_REQUEST);

  memset(&req.channelNumberAttributes, 0, sizeof(TurnAtrChannelNumber));
  req.channelNumberAttributes.channelNumber = chanNumber;
  req.hasChannelNumberAttributes = TRUE;

  req.hasSoftware = TRUE;
  memcpy( req.softwareName.value, serverName, sizeof(serverName));
  req.softwareName.sizeValue = sizeof(serverName);

  {
      uint32_t cookie = 0x2112A442;
      req.hasXorPeerAddress = TRUE;
      req.xorPeerAddress.ipv4.port = peerAddress->port^(cookie>>16);
      req.xorPeerAddress.ipv4.addr = peerAddress->addr^cookie;
  }

  if (username!=NULL && username->sizeValue>0
    && password!=NULL && password->sizeValue>0
    && realm!=NULL && realm->sizeValue>0
    && nonce!=NULL && nonce->sizeValue>0)
  {
    req.hasUsername = TRUE;
    memcpy( req.username.value, username->value, username->sizeValue );
    req.username.sizeValue = username->sizeValue;

    req.hasRealm = TRUE;
    memcpy( req.realmName.value, realm->value, realm->sizeValue );
    req.realmName.sizeValue = realm->sizeValue;

    req.hasNonce = TRUE;
    memcpy( req.nonceName.value, nonce->value, nonce->sizeValue );
    req.nonceName.sizeValue = nonce->sizeValue;

    req.hasMessageIntegrity = TRUE;
  }

  return stunEncodeMessage( &req, buf, buf_size, password );
}

int 
turnBuildRefresh(char* buf, int buf_size,
             const StunAtrString *username, const StunAtrString *password,
             const StunAtrString *realm, const StunAtrString *nonce)
{ 
  StunMessage req;
  const char serverName[] = "oRTP   " STUN_VERSION; /* must pad to mult of 4 */

  memset(&req, 0, sizeof(StunMessage));

  stunBuildReqSimple( &req,  NULL, FALSE, FALSE, 0 );
  req.msgHdr.msgType = (TURN_METHOD_REFRESH|STUN_REQUEST);

  req.hasSoftware = TRUE;
  memcpy( req.softwareName.value, serverName, sizeof(serverName));
  req.softwareName.sizeValue = sizeof(serverName);

  if (username!=NULL && username->sizeValue>0
    && password!=NULL && password->sizeValue>0
    && realm!=NULL && realm->sizeValue>0
    && nonce!=NULL && nonce->sizeValue>0)
  {
    req.hasUsername = TRUE;
    memcpy( req.username.value, username->value, username->sizeValue );
    req.username.sizeValue = username->sizeValue;

    req.hasRealm = TRUE;
    memcpy( req.realmName.value, realm->value, realm->sizeValue );
    req.realmName.sizeValue = realm->sizeValue;

    req.hasNonce = TRUE;
    memcpy( req.nonceName.value, nonce->value, nonce->sizeValue );
    req.nonceName.sizeValue = nonce->sizeValue;

    req.hasMessageIntegrity = TRUE;
  }

  return stunEncodeMessage( &req, buf, buf_size, password );
}

