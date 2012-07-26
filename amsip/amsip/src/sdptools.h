/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/


#ifndef __SDPTOOLS_H__
#define __SDPTOOLS_H__

#include <eXosip2/eXosip.h>
#include "am_calls.h"

/**
 * @file sdptools.h
 * @brief amsip sdp API
 *
 * This file provide the API needed to manage sdp.
 *
 */

/**
 * @defgroup amsip_sdptools amsip sdp interface
 * @ingroup amsip_internal
 * @{
 */

/**
 * Complete message with SDP.
 *
 * @param ca                 call context.
 * @param remote_sdp         sdp offer of negotiation.
 * @param ack_or_183         SIP message where to add sdp answer
 */
int sdp_complete_message(am_call_t * ca, sdp_message_t * remote_sdp,
						 osip_message_t * ack_or_183);

/**
 * Complete message with SDP.
 *
 * @param ca                 call context.
 * @param answer             SIP message where to add sdp answer.
 */
int sdp_complete_200ok(am_call_t * ca, osip_message_t * answer);

/**
 * Modify SDP to put audio stream on hold (sendonly).
 *
 * @param local_sdp          SDP message to modify.
 */
int _sdp_hold_call(sdp_message_t * local_sdp);

/**
 * Modify SDP to put audio stream off hold (sendrecv).
 *
 * @param local_sdp          SDP message to modify.
 */
int _sdp_off_hold_call(sdp_message_t * local_sdp);

/**
 * Modify SDP to put audio streams in inactive mode.
 *
 * @param local_sdp          SDP message to modify.
 */
int _sdp_inactive_call(sdp_message_t * sdp);

/**
 * Get information on audio stream (sendonly, recvonly, sendrecv, inactive)
 *
 * @param sdp          SDP message.
 * @param med          audio media.
 */
int _sdp_analyse_attribute(sdp_message_t * sdp, sdp_media_t * med);

/**
 * Get setup attribute on audio stream (active, both, passive)
 *
 * @param sdp          SDP message.
 * @param med          audio media.
 * @param setup        string to receive attribute value.
 */
int _sdp_analyse_attribute_setup(sdp_message_t * sdp, sdp_media_t * med,
								 char *setup);

/**
 * Modify SDP prefered media and parameter to increase/decrease compression.
 *
 * ALPHA method: use with care!
 *
 * @param local_sdp          SDP message.
 * @param preferred_codec    New prefered codec.
 * @param compress_more      0 or 1.
 */
int _sdp_switch_to_codec(sdp_message_t * local_sdp,
						 const char *preferred_codec, int compress_more);


/**
 * Modify SDP and add video media proposal.
 *
 * ALPHA method!
 *
 * @param ca                 call context
 * @param local_sdp          SDP message.
 */
int _sdp_add_video(am_call_t * ca, sdp_message_t * local_sdp);

/**
 * Modify SDP and modify video media proposal.
 *
 * Limitation: Only the video bandwidth parameter can be modified!
 * ALPHA method!
 *
 * @param ca                 call context
 * @param local_sdp          SDP message.
 */
int _sdp_modify_video(am_call_t * ca, sdp_message_t * local_sdp);

/**
 * Modify SDP and disable video media proposal. (0 in port number)
 *
 * @param ca                 call context
 * @param local_sdp          SDP message.
 */
int _sdp_disable_video(am_call_t * ca, sdp_message_t * local_sdp);

/**
 * Modify SDP and add text media proposal.
 *
 * ALPHA method!
 *
 * @param ca                 call context
 * @param local_sdp          SDP message.
 */
int _sdp_add_text(am_call_t * ca, sdp_message_t * local_sdp);

/**
 * Modify SDP and modify text media proposal.
 *
 * Limitation: Only the text bandwidth parameter can be modified!
 * ALPHA method!
 *
 * @param ca                 call context
 * @param local_sdp          SDP message.
 */
int _sdp_modify_text(am_call_t * ca, sdp_message_t * local_sdp);

int rtpmap_sscanf(char *rtpmap_value, char *payload, char *subtype,
				  char *freq);


/**
 * Increase SDP version ID (in o=x x <VERSIONID> ...).
 *
 * @param local_sdp          SDP message.
 */
int _sdp_increase_versionid(sdp_message_t * local_sdp);

/** @} */

#endif
