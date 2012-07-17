/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/


#ifndef __AM_SOUND_COMMON_H__
#define __AM_SOUND_COMMON_H__

#include <eXosip2/eXosip.h>
#include "am_calls.h"
/**
 * @file am_sound_common.h
 * @brief amsip common sound API
 *
 * This file provide the API needed to manage sound.
 *
 */

/**
 * @defgroup amsip_common_sound amsip common sound interface
 * @ingroup amsip_internal
 * @{
 */

/* list of state for STUN connectivity check */
#define TESTING 0
#define WAITING 1
#define RECV_VALID 2
#define SEND_VALID 3
#define VALID 4
#define INVALID 5

/**
 * Send needed STUN request.
 *
 * @param ca                call context.
 * @param round             like a timestamp value.
 */
int _am_sound_send_stun_request(call_t * ca, int round);

/**
 * recv STUN message from network.
 *
 * @param session           session context.
 * @param s                 stun event.
 */
int _am_process_stun_message(struct rtp *session, stun_event * s);

/**
 * Find initial active candidate.
 *
 * @param ca                call context.
 * @param remote_ip         remote_ip
 * @param remote_port       remote_port.
 * @param local_port        local_port.
 */
int _am_stun_active_candidate(call_t * ca, char *remote_ip,
							  int *remote_port, int local_port);

typedef struct {
	unsigned char event;		/* event Types (DTMF,fax,...) */
	unsigned char volume:6;		/* LSB... in dbm0 defined only for DTMF digits */
	unsigned char r:1;			/*    ... reserved bit */
	unsigned char e:1;			/* MSB... end marker bit */
	unsigned short duration;	/* Duration of this digit, in timestamp units */
} dtmf_payload_t;

/**
 * Send a DTMF in rtp telephone-event mode.
 *
 * @param ca                call context.
 * @param dtmf_number       dtmf number.
 */
int _am_sound_common_send_dtmf(call_t * ca, int dtmf_number);

/** @} */

#endif
