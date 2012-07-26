/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "am_calls.h"

#include <ortp/ortp.h>

#ifndef DISABLE_SRTP
#include <ortp/srtp.h>
#endif

#include "mediastreamer2/msrtp.h"

#include <osip2/osip_time.h>
#include <osip2/osip_mt.h>

#include "sdptools.h"

#ifndef DISABLE_SRTP
int
am_get_security_descriptions(RtpSession * rtp,
							 sdp_message_t * sdp_answer,
							 sdp_message_t * sdp_offer,
							 sdp_message_t * sdp_local,
							 sdp_message_t * sdp_remote, char *media_type);
#endif

static void
text_on_timestamp_jump(RtpSession * s, uint32_t * ts, void *user_data)
{
	ms_warning("Remote phone is sending data with a future timestamp: %u",
			   *ts);
	rtp_session_resync(s);
}

static void text_payload_type_changed(RtpSession * session, void *data)
{
#if 0
	am_call_t *ca = (am_call_t *) data;
	int payload = rtp_session_get_recv_payload_type(session);
	RtpProfile *prof = rtp_session_get_profile(session);
	PayloadType *pt = rtp_profile_get_payload(prof, payload);

	if (pt != NULL) {
		MSFilter *dec = ms_filter_create_decoder(pt->mime_type);

		if (dec != NULL) {

			int k;

			/* search index of elements */
			for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
				if (&_antisipc.calls[k] == ca) {
					/* Not supported */
				}
			}
		} else {
			ms_warning("No text decoder found for %s", pt->mime_type);
		}
	} else {
		ms_warning("No text payload defined with number %i", payload);
	}
#endif
}

static RtpSession *am_create_duplex_rtpsession(am_call_t * ca, int locport,
											   const char *remip,
											   int remport, int jitt_comp)
{
	RtpSession *rtpr;

	rtpr = rtp_session_new(RTP_SESSION_SENDRECV);
	rtp_session_set_recv_buf_size(rtpr, UDP_MAX_SIZE * 2);
	rtp_session_set_profile(rtpr, ca->text_rtp_profile);
	rtp_session_set_rtp_socket_recv_buffer_size(rtpr,2000000);
	rtp_session_set_rtp_socket_send_buffer_size(rtpr,2000000);
	rtp_session_set_local_addr(rtpr, "0.0.0.0", locport);
	if (remport > 0)
		rtp_session_set_remote_addr(rtpr, remip, remport);
	rtp_session_set_dscp(rtpr, _antisipc.text_dscp);
	rtp_session_set_scheduling_mode(rtpr, 0);
	rtp_session_set_blocking_mode(rtpr, 0);
	rtp_session_set_jitter_compensation(rtpr, jitt_comp);
	rtp_session_enable_adaptive_jitter_compensation(rtpr, TRUE);
	rtp_session_enable_jitter_buffer(rtpr, FALSE);
	rtp_session_set_time_jump_limit(rtpr, 0);
	rtp_session_signal_connect(rtpr, "timestamp_jump",
							   (RtpCallback) text_on_timestamp_jump, 0);

	return rtpr;
}

static void text_stream_graph_reset(am_call_t * ca)
{
	if (ca == NULL)
		return;

	if (ca->text_ticker != NULL)
		ms_ticker_destroy(ca->text_ticker);

	if (ca->text_rtp_session != NULL
		&& ca->text_rtp_session->rtp.tr!=NULL
		&& strcmp(ca->text_rtp_session->rtp.tr->name, "ZRTP")!=0)
	{
		rtp_session_set_transports(ca->text_rtp_session, NULL, NULL);
	}
	if (ca->text_rtp_session != NULL)
		rtp_session_destroy(ca->text_rtp_session);
	if (ca->text_decoder != NULL)
		ms_filter_destroy(ca->text_decoder);
	if (ca->text_encoder != NULL)
		ms_filter_destroy(ca->text_encoder);
	if (ca->text_rtpsend != NULL)
		ms_filter_destroy(ca->text_rtpsend);
	if (ca->text_rtprecv != NULL)
		ms_filter_destroy(ca->text_rtprecv);
	if (ca->text_ice != NULL)
		ms_filter_destroy(ca->text_ice);

	ca->text_ticker = NULL;
	ca->text_rtp_session = NULL;
	ca->text_encoder = NULL;
	ca->text_decoder = NULL;
	ca->text_rtpsend = NULL;
	ca->text_rtprecv = NULL;
	ca->text_ice = NULL;
}

static int text_stream_start_send_recv(am_call_t * ca, PayloadType * pt)
{
	RtpSession *rtps;

	rtps = ca->text_rtp_session;
	if (rtps == NULL)
		return -1;

	if (ca->text_encoder == NULL || ca->text_decoder == NULL) {
		text_stream_graph_reset(ca);
		ms_error("am_ms2_text.c: missing t140 filter.");
		return -1;
	}

	ca->text_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ca->text_rtpsend, MS_RTP_SEND_SET_SESSION, rtps);
	ca->text_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ca->text_rtprecv, MS_RTP_RECV_SET_SESSION, rtps);

	if (ca->text_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
		ca->text_ice = ms_filter_new(MS_ICE_ID);
		if (ca->text_ice != NULL) {
			ms_filter_call_method(ca->text_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ca->text_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->text_checklist);
		}
	}

	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) text_payload_type_changed,
							   (unsigned long) ca);

	if (pt != NULL && pt->send_fmtp != NULL)
		ms_filter_call_method(ca->text_encoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->send_fmtp);
	if (pt != NULL && pt->recv_fmtp != NULL)
		ms_filter_call_method(ca->text_decoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);
	return 0;
}

static int text_stream_start_recv_only(am_call_t * ca, PayloadType * pt)
{
	RtpSession *rtps;

	rtps = ca->text_rtp_session;
	if (rtps == NULL)
		return -1;

	if (ca->text_decoder == NULL) {
		text_stream_graph_reset(ca);
		ms_error("am_ms2_text.c: missing t140 filter.");
		return -1;
	}

	ca->text_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ca->text_rtpsend, MS_RTP_SEND_SET_SESSION, rtps);
	ca->text_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ca->text_rtprecv, MS_RTP_RECV_SET_SESSION, rtps);

	if (ca->text_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
		ca->text_ice = ms_filter_new(MS_ICE_ID);
		if (ca->text_ice != NULL) {
			ms_filter_call_method(ca->text_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ca->text_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->text_checklist);
		}
	}

	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) text_payload_type_changed,
							   (unsigned long) ca);

	if (pt != NULL && pt->recv_fmtp != NULL)
		ms_filter_call_method(ca->text_decoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);

	return 0;
}

static int text_stream_start_send_only(am_call_t * ca, PayloadType * pt)
{
	RtpSession *rtps;

	rtps = ca->text_rtp_session;
	if (rtps == NULL)
		return -1;

	if (ca->text_encoder == NULL) {
		text_stream_graph_reset(ca);
		ms_error("am_ms2_text.c: missing t140 filter.");
		return -1;
	}

	ca->text_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ca->text_rtpsend, MS_RTP_SEND_SET_SESSION, rtps);
	ca->text_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ca->text_rtprecv, MS_RTP_RECV_SET_SESSION, rtps);

	if (ca->text_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
		ca->text_ice = ms_filter_new(MS_ICE_ID);
		if (ca->text_ice != NULL) {
			ms_filter_call_method(ca->text_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ca->text_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->text_checklist);
		}
	}

	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) text_payload_type_changed,
							   (unsigned long) ca);

	if (pt != NULL && pt->send_fmtp != NULL)
		ms_filter_call_method(ca->text_encoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->send_fmtp);
	return 0;
}

static int am_ms2_text_detach(am_call_t * ca)
{
	if (ca->text_encoder != NULL && ca->text_decoder != NULL) {
		ms_ticker_detach(ca->text_ticker, ca->text_encoder);
		ms_ticker_detach(ca->text_ticker, ca->text_rtprecv);

		ms_filter_unlink(ca->text_rtprecv, 0, ca->text_decoder, 0);
		ms_filter_unlink(ca->text_encoder, 0, ca->text_rtpsend, 0);
	} else if (ca->text_decoder != NULL) {
		ms_ticker_detach(ca->text_ticker, ca->text_rtprecv);

		ms_filter_unlink(ca->text_rtprecv, 0, ca->text_decoder, 0);
	} else if (ca->text_encoder != NULL) {
		ms_ticker_detach(ca->text_ticker, ca->text_encoder);

		ms_filter_unlink(ca->text_encoder, 0, ca->text_rtpsend, 0);
	}
	if (ca->text_ice != NULL)
		ms_ticker_detach(ca->text_ticker, ca->text_ice);
	return 0;
}

static int am_ms2_text_attach(am_call_t * ca)
{
	if (ca->text_encoder != NULL && ca->text_decoder != NULL) {
		ms_filter_link(ca->text_rtprecv, 0, ca->text_decoder, 0);
		ms_filter_link(ca->text_encoder, 0, ca->text_rtpsend, 0);

		ms_ticker_attach(ca->text_ticker, ca->text_encoder);
		ms_ticker_attach(ca->text_ticker, ca->text_rtprecv);
	} else if (ca->text_decoder != NULL) {
		ms_filter_link(ca->text_rtprecv, 0, ca->text_decoder, 0);

		ms_ticker_attach(ca->text_ticker, ca->text_rtprecv);
	} else if (ca->text_encoder != NULL) {
		ms_filter_link(ca->text_encoder, 0, ca->text_rtpsend, 0);

		ms_ticker_attach(ca->text_ticker, ca->text_encoder);
	}

	if (ca->text_ice != NULL)
		ms_ticker_attach(ca->text_ticker, ca->text_ice);
	return 0;
}

static int
_am_add_fmtp_parameter_encoders(am_call_t * ca, sdp_message_t * sdp_answer,
								PayloadType * pt)
{
	int pos;

	if (sdp_answer == NULL)
		return -1;

	if (ca->text_encoder == NULL)
		return -1;

	pos = 0;
	while (!osip_list_eol(&sdp_answer->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&sdp_answer->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "text")) {
			int p_number = -1;
			char payload_text[10];
			int pos3 = 0;

			/* find rtpmap number for this payloadtype */
			while (!osip_list_eol(&med->a_attributes, pos3)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&med->a_attributes,
													  pos3);
				if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
					/* search for each supported payload */
					int n;
					char codec[64];
					char subtype[64];
					char freq[64];

					if (attr->a_att_value == NULL
						|| strlen(attr->a_att_value) > 63) {
						pos3++;
						continue;	/* avoid buffer overflow! */
					}
					n = rtpmap_sscanf(attr->a_att_value, codec, subtype,
									  freq);
					if (n == 3) {
						if (0 == osip_strcasecmp(subtype, pt->mime_type)
							&& pt->clock_rate == atoi(freq)) {
							/* found payload! */
							p_number = atoi(codec);
							break;
						}

					}
				}
				pos3++;
			}

			if (p_number >= 0) {
				snprintf(payload_text, sizeof(payload_text), "%i ",
						 p_number);

				pos3 = 0;
				while (!osip_list_eol(&med->a_attributes, pos3)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&med->
														  a_attributes,
														  pos3);
					if (osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
						if (attr->a_att_value != NULL
							&& strlen(attr->a_att_value) >
							strlen(payload_text)
							&& osip_strncasecmp(attr->a_att_value,
												payload_text,
												strlen(payload_text)) ==
							0) {
							ms_filter_call_method(ca->text_encoder,
												  MS_FILTER_ADD_FMTP,
												  (void *) attr->
												  a_att_value);
						}
					} else if (osip_strcasecmp(attr->a_att_field, "ptime")
							   == 0) {
						char ptattr[256];

						memset(ptattr, 0, sizeof(ptattr));
						snprintf(ptattr, 256, "%s:%s", attr->a_att_field,
								 attr->a_att_value);
						ms_filter_call_method(ca->text_encoder,
											  MS_FILTER_ADD_ATTR,
											  (void *) ptattr);
					}
					pos3++;
				}
			}

			return p_number;
		} else {
			/* skip non text */
		}

		pos++;
	}

	return -1;
}


static int
_am_add_bandwidth_parameter_encoders(am_call_t * ca,
									 sdp_message_t * sdp_remote,
									 PayloadType * pt)
{
	int pos;
	int bd;

	if (sdp_remote == NULL)
		return -1;

	if (ca->text_encoder == NULL)
		return -1;

	bd = _antisipc.text_codec_attr.upload_bandwidth * 1000;
	ms_message("am_ms2_text.c: set default bandwidth for %s (%i).",
			   pt->mime_type, bd);

	pos = 0;
	while (!osip_list_eol(&sdp_remote->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&sdp_remote->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "text")) {
			int p_number = -1;
			int pos3 = 0;

			/* find rtpmap number for this payloadtype */
			while (!osip_list_eol(&med->a_attributes, pos3)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&med->a_attributes,
													  pos3);
				if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
					/* search for each supported payload */
					int n;
					char codec[64];
					char subtype[64];
					char freq[64];

					if (attr->a_att_value == NULL
						|| strlen(attr->a_att_value) > 63) {
						pos3++;
						continue;	/* avoid buffer overflow! */
					}
					n = rtpmap_sscanf(attr->a_att_value, codec, subtype,
									  freq);
					if (n == 3) {
						if (0 == osip_strcasecmp(subtype, pt->mime_type)
							&& pt->clock_rate == atoi(freq)) {
							/* found payload! */
							p_number = atoi(codec);
							break;
						}

					}
				}
				pos3++;
			}

			if (p_number >= 0) {
				pos3 = 0;
				while (!osip_list_eol(&med->b_bandwidths, pos3)) {
					sdp_bandwidth_t *bandwidth;

					bandwidth =
						(sdp_bandwidth_t *) osip_list_get(&med->
														  b_bandwidths,
														  pos3);
					if (osip_strcasecmp(bandwidth->b_bwtype, "AS") == 0) {
						int bitrate = atoi(bandwidth->b_bandwidth);

						if (bitrate <= 64)
							bitrate = 64;
						if (bitrate >= 1024)
							bitrate = 1024;
						bitrate = bitrate * 1000;
						if (bitrate < bd)
							bd = bitrate;
						ms_message
							("am_ms2_text.c: %s bandwidth limited to: %i.",
							 pt->mime_type, bd);
					}

					pos3++;
				}
			}

			if (bd <= 512000)
				ms_filter_call_method(ca->text_encoder,
									  MS_FILTER_SET_BITRATE, (void *) &bd);
			return p_number;
		} else {
			/* skip non text */
		}

		pos++;
	}

	return -1;
}

static int
_am_prepare_text_decoders(am_call_t * ca, sdp_message_t * sdp,
						  sdp_media_t * med, int *decoder_payload)
{
	int pos2 = 0;

	*decoder_payload = -1;
	while (!osip_list_eol(&med->m_payloads, pos2)) {
		char *p_char = (char *) osip_list_get(&med->m_payloads, pos2);
		int p_number = atoi(p_char);

		int pos3 = 0;

		while (!osip_list_eol(&med->a_attributes, pos3)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&med->a_attributes,
												  pos3);
			if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
				/* search for each supported payload */
				int n;
				int payload = 0;
				char codec[64];
				char subtype[64];
				char freq[64];

				if (attr->a_att_value == NULL
					|| strlen(attr->a_att_value) > 63) {
					pos3++;
					continue;	/* avoid buffer overflow! */
				}
				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3)
					payload = atoi(codec);
				if (n == 3 && p_number == payload) {
					/* step 1: add profile */
					if (0 == osip_strcasecmp(subtype, "t140")) {
						rtp_profile_set_payload(ca->text_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_t140));
					}

					/* step 2: create decoders: */
					if (ca->text_decoder == NULL) {
						PayloadType *pt;

						pt = rtp_profile_get_payload(ca->text_rtp_profile,
													 p_number);
						if (pt != NULL && pt->mime_type != NULL) {
							ca->text_decoder =
								ms_filter_create_decoder(pt->mime_type);
							if (ca->text_decoder != NULL) {
								/* all fine! */
								*decoder_payload = p_number;
								OSIP_TRACE(osip_trace
										   (__FILE__, __LINE__,
											OSIP_WARNING, NULL,
											"Decoder created for payload=%i %s\n",
											p_number, pt->mime_type));
							}
						}
					}
				}
			}
			pos3++;
		}
		pos2++;
	}
	return 0;
}

static PayloadType *_am_prepare_text_encoder(am_call_t * ca,
											 sdp_message_t * sdp_remote,
											 sdp_media_t * med,
											 int *encoder_payload)
{
	int pos2 = 0;

	*encoder_payload = -1;
	/* use first supported remote payload */
	while (!osip_list_eol(&med->m_payloads, pos2)) {
		char *p_char = (char *) osip_list_get(&med->m_payloads, pos2);
		int p_number = atoi(p_char);
		PayloadType *pt = NULL;
		int found_rtpmap = 0;

		int pos3 = 0;

		while (!osip_list_eol(&med->a_attributes, pos3)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&med->a_attributes,
												  pos3);
			if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
				/* search for each supported payload */
				int n;
				int payload = 0;
				char codec[64];
				char subtype[64];
				char freq[64];

				if (attr->a_att_value == NULL
					|| strlen(attr->a_att_value) > 63) {
					pos3++;
					continue;	/* avoid buffer overflow! */
				}
				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3)
					payload = atoi(codec);
				if (n == 3 && p_number == payload) {
					PayloadType *pt_index;
					char rtpmap_tmp[128];

					found_rtpmap = 1;
					memset(rtpmap_tmp, '\0', sizeof(rtpmap_tmp));
					snprintf(rtpmap_tmp, sizeof(rtpmap_tmp), "%s/%s",
							 subtype, freq);

					/* rtp_profile are added with numbers that comes from the
					   local_sdp: the current p_number could be different!! */

					/* create an encoder: pt will exist if a decoder was found
					   or if it's H263 */
					pt_index =
						rtp_profile_get_payload(ca->text_rtp_profile,
												p_number);
					pt = rtp_profile_get_payload_from_rtpmap(ca->
															 text_rtp_profile,
															 rtpmap_tmp);
					if (pt_index != pt && pt != NULL) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_WARNING, NULL,
									"Payload for %s is different in offer(%i) and answer(%i)\n",
									rtpmap_tmp, -1, p_number));
					}

					if (pt != NULL && pt->mime_type != NULL) {
						if (ca->text_local_sendrecv != _RECVONLY) {
							ca->text_encoder =
								ms_filter_create_encoder(pt->mime_type);
							if (ca->text_encoder != NULL) {
								*encoder_payload = p_number;
								return pt;
							}
						} else {
							*encoder_payload = p_number;
							return pt;
						}
					}
					break;
				}
			}
			pos3++;
		}

		if (found_rtpmap == 0) {
			/* missing rtpmap attribute in remote_sdp!!! */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
								  "Missing rtpmap attribute in remote SDP for payload=%i\n",
								  p_number));
			pt = NULL;
			if (p_number == 34) {
				pt = rtp_profile_get_payload(ca->text_rtp_profile,
											 p_number);
			}

			if (pt != NULL && pt->mime_type != NULL) {
				if (ca->text_local_sendrecv != _RECVONLY) {
					ca->text_encoder =
						ms_filter_create_encoder(pt->mime_type);
					if (ca->text_encoder != NULL) {
						*encoder_payload = p_number;
						return pt;
					}
				} else {
					*encoder_payload = p_number;
					return pt;
				}
			}
		}

		pos2++;
	}
	return NULL;
}

static PayloadType *_am_prepare_coders(am_call_t * ca,
									   sdp_message_t * sdp_answer,
									   sdp_message_t * sdp_offer,
									   sdp_message_t * local_sdp,
									   sdp_message_t * remote_sdp,
									   char *remote_ip,
									   int *decoder_payload,
									   int *encoder_payload)
{
	int pos;

	*decoder_payload = -1;
	*encoder_payload = -1;
	if (sdp_answer == NULL)
		return NULL;
	if (sdp_offer == NULL)
		return NULL;
	if (remote_ip == NULL)
		return NULL;

	pos = 0;
	while (!osip_list_eol(&local_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&local_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "text")) {
			_am_prepare_text_decoders(ca, local_sdp, med, decoder_payload);
		} else {
			/* skip non text */
		}

		pos++;
	}

	/* find encoder that we do support AND offer */
	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "text")) {
			PayloadType *pt =
				_am_prepare_text_encoder(ca, remote_sdp, med,
										 encoder_payload);
			if (pt != NULL && ca->text_encoder != NULL) {
				/* step 3: set fmtp parameters */
				_am_add_fmtp_parameter_encoders(ca, sdp_answer, pt);
				/* when payload is not supported in sdp_answer, we might
				   want to set encoder parameter from sdp_offer... */
				_am_add_bandwidth_parameter_encoders(ca, remote_sdp, pt);
			}
			return pt;
		} else {
			/* skip non text */
		}
		pos++;
	}

	return NULL;
}

int os_text_init()
{
	return 0;
}

int
os_text_start(am_call_t * ca, sdp_message_t * sdp_answer,
			  sdp_message_t * sdp_offer, sdp_message_t * sdp_local,
			  sdp_message_t * sdp_remote, int local_port, char *remote_ip,
			  int remote_port, int setup_passive)
{
	PayloadType *pt;
	int encoder_payload = -1;
	int decoder_payload = -1;

	if (ca->p_am_sessiontype[0]!='\0' && strstr(ca->p_am_sessiontype, "chat")==NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "chat disabled using P-AM-ST private header\n"));
		return AMSIP_WRONG_STATE;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "os_text_start - RTP stream cid=%i did=%i\n",
						  ca->cid, ca->did));

	if (ca->text_local_sendrecv == _INACTIVE) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "os_text_start - RTP stream is inactive cid=%i did=%i\n",
							  ca->cid, ca->did));
		return -1;
	}

	ca->text_rtp_session = NULL;

	if (ca->text_candidate.stun_candidates[0].conn_port > 0)
		local_port = ca->text_candidate.stun_candidates[0].rel_port;
	if (ca->text_candidate.relay_candidates[0].conn_port > 0)
		local_port = ca->text_candidate.relay_candidates[0].rel_port;

	if (ca->text_ticker == NULL) {
		ca->text_ticker = ms_ticker_new_withname("amsip-text");
	}

	/* open mediastreamer session */
	ca->text_rtp_profile =
		(RtpProfile *) rtp_profile_new("RtpProfileText");
	pt = _am_prepare_coders(ca, sdp_answer, sdp_offer, sdp_local,
							sdp_remote, remote_ip, &decoder_payload,
							&encoder_payload);

	ca->text_rtp_session =
		am_create_duplex_rtpsession(ca, local_port, remote_ip, remote_port,
									50);

	if (!ca->text_rtp_session) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "Could not initialize text session for %s port %d\n",
							  remote_ip, remote_port));
		if (ca->text_local_sendrecv == _SENDONLY) {
		}
		os_text_close(ca);
		return -1;
	}
#ifndef DISABLE_SRTP
	am_get_security_descriptions(ca->text_rtp_session,
								 sdp_answer, sdp_offer,
								 sdp_local, sdp_remote, "text");
#endif
	if (ca->text_rtp_session->rtp.tr==NULL
		&& _antisipc.enable_zrtp > 0) {
		/* ZRTP */
		RtpTransport *rtpt;
		RtpTransport *rtcpt;
		rtpt = ortp_transport_new("ZRTP");
		if (rtpt == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"oRTP compiled without ZRTP support\n"));
		} else {
      rtcpt = ortp_transport_new("ZRTP");
      rtp_session_set_transports(ca->text_rtp_session, rtpt, rtcpt );
      ortp_transport_set_option(rtpt, 1, &ca->call_direction);
      ortp_transport_set_option(rtcpt, 1, &ca->call_direction);
    }
	}

	/* RTP */
	if (encoder_payload >= 0)
		rtp_session_set_send_payload_type(ca->text_rtp_session,
										  encoder_payload);
	if (encoder_payload < 0 && decoder_payload >= 0)
		rtp_session_set_send_payload_type(ca->text_rtp_session,
										  decoder_payload);
	if (decoder_payload >= 0)
		rtp_session_set_recv_payload_type(ca->text_rtp_session,
										  decoder_payload);
	if (encoder_payload >= 0 && decoder_payload < 0)
		rtp_session_set_send_payload_type(ca->text_rtp_session,
										  encoder_payload);

	if (ca->text_local_sendrecv == _SENDONLY) {
		/* send a wav file!  ca->wav_file */
		text_stream_start_send_only(ca, pt);
	} else if (ca->text_local_sendrecv == _RECVONLY) {
		text_stream_start_recv_only(ca, pt);
	} else {
		text_stream_start_send_recv(ca, pt);
	}

	/* set sdes (local user) info */

	/* check if symmetric RTP is required
	   with antisip to antisip softphone, we use STUN connectivity check
	 */
	if (ca->text_checklist.cand_pairs[0].remote_candidate.conn_addr[0]=='\0') {
		if (_antisipc.do_symmetric_rtp == 1) {
			rtp_session_set_symmetric_rtp(ca->text_rtp_session, 1);
		}
	}

	ca->enable_text = 1;
	/* should start here the mediastreamer thread */

	am_ms2_text_attach(ca);

	return 0;
}

void os_text_close(am_call_t * ca)
{
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"os_text_close - RTP text stream cid=%i did=%i\n", ca->cid,
				ca->did));

	ca->enable_text = -1;
	/* stop mediastreamer thread */

	/* stop mediastreamer session */

	/* print statistics */
	if (ca != NULL && ca->text_rtp_session != NULL)
		rtp_stats_display(&ca->text_rtp_session->rtp.stats,
						  "end of session");

	if (ca != NULL) {
		am_ms2_text_detach(ca);
		text_stream_graph_reset(ca);
	}

	if (ca->text_rtp_profile != NULL) {
		rtp_profile_destroy(ca->text_rtp_profile);
		ca->text_rtp_profile = NULL;
	}
}
