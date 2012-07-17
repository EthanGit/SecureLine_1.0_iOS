/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_EVENT_H__
#define __AM_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>
#include <osipparser2/osip_port.h>

/**
 * @file am_event.h
 * @brief amsip event API
 *
 * This file provide the API needed to get events from amsip.
 *
 */

/**
 * @defgroup amsip_event amsip event interface
 * @ingroup amsip_setup
 * @{
 */

/**
 * Log facility for amsip.
 * DEPRECATED METHOD: use am_trace instead
 *
 * @param level              Level for debug data.
 * @param chfr               Format for data.
 */
	PPL_DECLARE (void) am_log(int level, char *chfr, ...);

/**
 * Log facility for amsip.
 *
 * @param fi                 filename of source file.
 * @param li                 line of source file.
 * @param level              Level for debug data.
 * @param chfr               Format for data.
 */
	PPL_DECLARE (void) am_trace(char *fi, int li, int level, char *chfr, ...);

/**
 * Get Event.
 *
 * @param evt                Pointer to fill event.
 */
	PPL_DECLARE (int) am_event_get(eXosip_event_t * evt);

/**
 * Wait Event.
 * NOTE: the maximum allowed value is theorically 0s and 500 ms
 * NOTE2: Introduced in 4.4.2. Prefer am_even_get() for released app.
 *
 * @param evt                Pointer to fill event.
 */
	PPL_DECLARE (int) am_event_wait(eXosip_event_t * evt, int tv_s, int tv_ms);

	typedef struct am_header {
		int index;
		char value[1024];
	} am_header_t;

/**
 * Get header from message.
 *
 * @param msg                SIP message where to find header.
 * @param header             Header name to search.
 * @param value              value of header.
 */
	PPL_DECLARE (int) am_message_get_header(osip_message_t * msg,
											char *header,
											am_header_t * value);

	typedef struct am_bodyinfo {
		int index;
		int content_length;
		char content_type[1024];
		char *attachment;
	} am_bodyinfo_t;

/**
 * Get body from message.
 *
 * @param msg                SIP message where to find header.
 * @param attachemnt_index   index of attachement.
 * @param bodyinfo           Elements for bodyinfo.
 */
	PPL_DECLARE (int) am_message_get_bodyinfo(osip_message_t * msg,
											  int attachemnt_index,
											  am_bodyinfo_t * bodyinfo);

/**
 * Get body from message.
 *
 * @param bodyinfo           Elements to release.
 */
	PPL_DECLARE (int) am_message_release_bodyinfo(am_bodyinfo_t *
												  bodyinfo);

	typedef struct am_messageinfo {
		int answer_code;
		char method[128];
		char reason[1024];
		char target[1024];

		char from[1024];
		char to[1024];
		char contact[1024];
		char call_id[1024];
	} am_messageinfo_t;

/**
 * Get message information (method, target//code, reason + From/To) from message.
 *
 * @param msg                SIP message where to extract message info.
 * @param value              Elements from message info.
 */
	PPL_DECLARE (int) am_message_get_messageinfo(osip_message_t * msg,
												 am_messageinfo_t * value);

/**
 * Get message information (method, target//code, reason + From/To) from message.
 *
 * @param uri                Original uri.
 * @param header_name        Header name to add
 * @param header_value       Header value to add
 * @param dest_uri           target string for new uri
 * @param dest_size          size of target string for new uri
 */
	PPL_DECLARE (int) am_message_add_header_to_uri(char *uri,
												   const char *header_name,
												   const char
												   *header_value,
												   char *dest_uri,
												   int dest_size);

/**
 * Get audio RTP direction from SDP attribute.
 * return 0 for _SENDRECV
 * return 1 for _SENDONLY
 * return 2 for _RECVONLY
 *
 * @param msg                SIP message where to extract message info.
 */
	PPL_DECLARE (int) am_message_get_audio_rtpdirection(osip_message_t *
														msg);

/**
 * Get video RTP direction from SDP attribute.
 * return 0 for _SENDRECV
 * return 1 for _SENDONLY
 * return 2 for _RECVONLY
 *
 * @param msg                SIP message where to extract message info.
 */
	PPL_DECLARE (int) am_message_get_video_rtpdirection(osip_message_t *
														msg);


/**
 * Set body in SIP message
 *
 * @param msg                SIP message where to add info.
 * @param ctt                Content-Type.
 * @param body               Body To add.
 */
	PPL_DECLARE (int) am_message_set_body(osip_message_t * msg,
										  const char *ctt,
										  const char *body, int body_size);

/**
 * Allocate and Add an "unknown" header (not defined in oSIP).
 * @param sip The element to work on.
 * @param hname The token name.
 * @param hvalue The token value.
 */
PPL_DECLARE (int)
am_message_set_header(osip_message_t * sip, const char *hname,
							const char *hvalue);

/**
 * Release event.
 *
 * @param evt                Event structure to release.
 */
	PPL_DECLARE (void) am_event_release(eXosip_event_t * evt);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
