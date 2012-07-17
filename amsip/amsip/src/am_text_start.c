/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include <osip2/osip_mt.h>
#include "am_calls.h"
#include <eXosip2/eXosip.h>
#include "sdptools.h"

#include "am_text_start.h"

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

int
_calls_start_text_with_id(int tid_for_offer, int did,
						  osip_message_t * answer)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].did == did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		return -1;
	}

	ca = &(_antisipc.calls[k]);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_WARNING, NULL,
				"Start text ressource cid=%i did=%i\n", ca->cid, ca->did));

	return _calls_start_text_with_sipanswer(ca, tid_for_offer, answer, 1);
}

int
_calls_start_text_with_sipanswer(am_call_t * ca, int tid_for_offer,
								 osip_message_t * answer,
								 int local_is_offer)
{
	int i;
	sdp_message_t *offer_sdp = NULL;
	sdp_message_t *answer_sdp = NULL;

	offer_sdp = eXosip_get_remote_sdp_from_tid(tid_for_offer);
	if (offer_sdp == NULL) {
		return -1;
	}
	answer_sdp = eXosip_get_sdp_info(answer);
	if (answer_sdp == NULL) {
		sdp_message_free(offer_sdp);
		return -1;
	}

	i = _calls_start_text_from_sdpmessage(ca, offer_sdp, answer_sdp,
										  local_is_offer);
	sdp_message_free(offer_sdp);
	sdp_message_free(answer_sdp);
	return i;
}

int
_calls_start_text_from_sipmessage(am_call_t * ca, osip_message_t * offer,
								  osip_message_t * answer,
								  int local_is_offer)
{
	int i;
	sdp_message_t *offer_sdp = NULL;
	sdp_message_t *answer_sdp = NULL;

	offer_sdp = eXosip_get_sdp_info(offer);
	if (offer_sdp == NULL) {
		return -1;
	}
	answer_sdp = eXosip_get_sdp_info(answer);
	if (answer_sdp == NULL) {
		sdp_message_free(offer_sdp);
		return -1;
	}

	i = _calls_start_text_from_sdpmessage(ca, offer_sdp, answer_sdp,
										  local_is_offer);
	sdp_message_free(offer_sdp);
	sdp_message_free(answer_sdp);
	return i;
}

int
_calls_start_text_from_sdpmessage(am_call_t * ca,
								  sdp_message_t * offer_sdp,
								  sdp_message_t * answer_sdp,
								  int local_is_offer)
{
	char *sdp_content_answer = NULL;
	char *sdp_content_offer = NULL;
	char offer_ip[256];
	int offer_port = 0;
	char answer_ip[256];
	int answer_port = 0;
	int text_local_sendrecv;
	int remote_sendrecv;
	int setup_passive = 0;
	char offer_setup[256];
	char answer_setup[256];

	sdp_media_t *offer_med = NULL;
	sdp_media_t *answer_med = NULL;
	int i;

	/* contains available codecs */
	i = sdp_message_to_str(answer_sdp, &sdp_content_answer);
	if (i != 0) {
		return -1;
	}
	i = sdp_message_to_str(offer_sdp, &sdp_content_offer);
	if (i != 0) {
		osip_free(sdp_content_answer);
		return -1;
	}

	memset(answer_ip, '\0', sizeof(answer_ip));
	memset(offer_ip, '\0', sizeof(offer_ip));
	answer_port = -1;
	offer_port = -1;
	_calls_get_ip_port(answer_sdp, "text", answer_ip, &answer_port);
	_calls_get_ip_port(offer_sdp, "text", offer_ip, &offer_port);

	if (answer_ip[0] == '\0' || offer_ip[0] == '\0') {
		osip_free(sdp_content_answer);
		osip_free(sdp_content_offer);
		return -1;
	}

	if (ca->enable_text > 0) {
		ca->enable_text = -1;
		os_text_close(ca);
	}

	/* port 0 means that stream is disabled */
	if (answer_port == 0 || offer_port == 0) {
		osip_free(sdp_content_answer);
		osip_free(sdp_content_offer);
		return 0;
	}

	/* search if stream is sendonly or recvonly */
	offer_med = eXosip_get_media(offer_sdp, "text");
	answer_med = eXosip_get_media(answer_sdp, "text");

	offer_setup[0] = '\0';
	answer_setup[0] = '\0';
	_sdp_analyse_attribute_setup(offer_sdp, offer_med, offer_setup);
	_sdp_analyse_attribute_setup(answer_sdp, answer_med, answer_setup);

	if (local_is_offer == 0) {
		text_local_sendrecv = _sdp_analyse_attribute(offer_sdp, offer_med);
		remote_sendrecv = _sdp_analyse_attribute(answer_sdp, answer_med);
		if (osip_strcasecmp(offer_setup, "passive") == 0
			&& osip_strcasecmp(answer_setup, "passive") != 0) {
			setup_passive = 200;
		}
	} else {
		text_local_sendrecv =
			_sdp_analyse_attribute(answer_sdp, answer_med);
		remote_sendrecv = _sdp_analyse_attribute(offer_sdp, offer_med);
		if (osip_strcasecmp(answer_setup, "passive") == 0
			&& osip_strcasecmp(offer_setup, "passive") != 0) {
			setup_passive = 200;
		}
	}

	if (text_local_sendrecv == _INACTIVE)
		remote_sendrecv = _INACTIVE;
	if (remote_sendrecv == _INACTIVE)
		text_local_sendrecv = _INACTIVE;

	if (text_local_sendrecv == _SENDRECV) {
		if (remote_sendrecv == _SENDONLY)
			text_local_sendrecv = _RECVONLY;
		else if (remote_sendrecv == _RECVONLY)
			text_local_sendrecv = _SENDONLY;
	}

	if (text_local_sendrecv == _SENDONLY
		|| text_local_sendrecv == _SENDRECV) {
		if (local_is_offer == 0) {
			if (osip_strcasecmp(answer_ip, "0.0.0.0") == 0)
				text_local_sendrecv = _RECVONLY;
		} else {
			if (osip_strcasecmp(offer_ip, "0.0.0.0") == 0)
				text_local_sendrecv = _RECVONLY;
		}
	}

	ca->text_local_sendrecv = text_local_sendrecv;
	if (local_is_offer == 0) {
		ca->text_rtp_in_direct_mode = 0;
		snprintf(ca->text_rtp_remote_addr, 256, "%s:%i", answer_ip,
				 answer_port);
		if (0 ==
			os_text_start(ca, answer_sdp, offer_sdp, /* local */ offer_sdp,
						  /* remote */ answer_sdp, offer_port,
						  answer_ip, answer_port, setup_passive)) {
			ca->enable_text = 1;	/* text is started */
		}
	} else {
		ca->text_rtp_in_direct_mode = 0;
		snprintf(ca->text_rtp_remote_addr, 256, "%s:%i", offer_ip,
				 offer_port);
		if (0 ==
			os_text_start(ca, answer_sdp, offer_sdp, /* local */
						  answer_sdp,
						  /* remote */ offer_sdp, answer_port,
						  offer_ip, offer_port, setup_passive)) {
			ca->enable_text = 1;	/* text is started */
		}
	}

	osip_free(sdp_content_answer);
	osip_free(sdp_content_offer);
	return 0;
}
