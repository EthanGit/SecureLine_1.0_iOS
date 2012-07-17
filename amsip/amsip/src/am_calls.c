/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/


#include <osip2/osip_mt.h>
#include "am_calls.h"
#include <eXosip2/eXosip.h>
#include "sdptools.h"
#ifdef ENABLE_VIDEO
#include "am_video_start.h"
#endif
#include "am_text_start.h"
#include "am_udpftp_start.h"
#include <mediastreamer2/msfilerec.h>

#ifndef OK_RETURN_CODE
#define OK_RETURN_CODE 200
#endif

#ifdef EXOSIP4

#include "amsip-internal.h"
#endif

int _calls_start_audio_with_sipanswer(am_call_t * ca, int tid_for_offer,
									  osip_message_t * answer,
									  int local_is_offer);
int _calls_start_audio_from_sdpmessage(am_call_t * ca,
									   sdp_message_t * offer,
									   sdp_message_t * answer,
									   int local_offer);
int _calls_start_audio_from_sipmessage(am_call_t * ca,
									   osip_message_t * offer,
									   osip_message_t * answer,
									   int local_offer);

int _am_session_stop_transfer(int cid, int did);

static void am_call_init(am_call_t * ca, int idx)
{
	if (ca == NULL)
		return;
	memset(ca, 0, sizeof(am_call_t));
	_antisipc.audio_media->audio_module_sound_init(ca, idx);

	eXosip_generate_random(ca->ice_pwd, 16);
	am_hexa_generate_random(ca->ice_pwd, 23, ca->ice_pwd, "key1", "key");
	eXosip_generate_random(ca->ice_ufrag, 16);
	am_hexa_generate_random(ca->ice_ufrag,
						  5, ca->ice_ufrag, ca->ice_pwd, "key");

#ifdef ENABLE_VIDEO
	_antisipc.video_media->video_module_session_init(ca, idx);
#endif
}

void am_call_release(am_call_t * ca)
{
	if (ca == NULL)
		return;

	if (ca->enable_udpftp > 0) {
		ca->enable_udpftp = -1;
		os_udpftp_close(ca);
	}
	_am_session_stop_transfer(ca->cid, ca->did);
	if (ca->enable_text > 0) {
		ca->enable_text = -1;
		os_text_close(ca);
	}
	if (ca->enable_audio > 0) {
		ca->enable_audio = -1;
		_antisipc.audio_media->audio_module_sound_close(ca);
	}
#ifdef ENABLE_VIDEO
	if (ca->enable_video > 0) {
		ca->enable_video = -1;
		_antisipc.video_media->video_module_session_close(ca);
	}
	_antisipc.video_media->video_module_session_release(ca);
#endif

	_antisipc.audio_media->audio_module_sound_release(ca);
	if (ca->local_sdp != NULL)
		sdp_message_free(ca->local_sdp);
	ca->local_sdp = NULL;

	ca->state = NOT_USED;
}

int calls_options_outside_calls(eXosip_event_t * je)
{
	osip_message_t *options = NULL;
	int i = eXosip_options_build_answer(je->tid, 202, &options);

	if (i == 0) {
		char tmp[4096];
		char localip[128];
		char rtpmaps[2048];
		char payloads[128];

		eXosip_guess_localip(AF_INET, localip, 128);

		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 0 IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n"
				 "m=audio 0 %s", localip, localip,
				 _antisipc.audio_profile);

		memset(rtpmaps, 0, sizeof(rtpmaps));
		memset(payloads, 0, sizeof(payloads));
		for (i = 0; i < 5; i++) {
			if (ms_filter_codec_supported(_antisipc.codecs[i].name)
				&& _antisipc.codecs[i].enable > 0) {
				char tmp_payloads[64];
				char tmp_rtpmaps[256];

				memset(tmp_payloads, 0, sizeof(tmp_payloads));
				memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
				snprintf(tmp_payloads, 64, "%i",
						 _antisipc.codecs[i].payload);
				strcat(payloads, " ");
				strcat(payloads, tmp_payloads);

				snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
						 "a=rtpmap:%i %s/%i\r\n",
						 _antisipc.codecs[i].payload,
						 _antisipc.codecs[i].name,
						 _antisipc.codecs[i].freq);
				strcat(rtpmaps, tmp_rtpmaps);

				if (osip_strcasecmp(_antisipc.codecs[i].name, "ilbc") == 0) {
					if (_antisipc.codec_attr.ptime > 0
						&& _antisipc.codecs[i].mode > 0) {
						if (_antisipc.codec_attr.ptime %
							_antisipc.codecs[i].mode != 0)
							_antisipc.codecs[i].mode = 0;	/* bad ilbc mode */
					}

					if (_antisipc.codecs[i].mode <= 0) {
						_antisipc.codecs[i].mode = 20;
						if (_antisipc.codec_attr.ptime > 0) {
							if (_antisipc.codec_attr.ptime % 20 == 0)
								_antisipc.codecs[i].mode = 20;
							else
								_antisipc.codecs[i].mode = 30;
						}
					}
					snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
							 "a=fmtp:%i mode=%i\r\n",
							 _antisipc.codecs[i].payload,
							 _antisipc.codecs[i].mode);
					strcat(rtpmaps, tmp_rtpmaps);
				}

				if (osip_strcasecmp(_antisipc.codecs[i].name, "speex") ==
					0) {
					if (_antisipc.codecs[i].mode > 6)
						_antisipc.codecs[i].mode = 6;

					if (_antisipc.codecs[i].mode <= 0)
						_antisipc.codecs[i].mode = 0;

					if (_antisipc.codecs[i].vbr == 1
						&& _antisipc.codecs[i].cng == 1) {
						if (_antisipc.codecs[i].mode > 0)
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"%i,any\";vbr=on;cng=on\r\n",
									 _antisipc.codecs[i].payload,
									 _antisipc.codecs[i].mode);
						else
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"any\";vbr=on;cng=on\r\n",
									 _antisipc.codecs[i].payload);
						strcat(rtpmaps, tmp_rtpmaps);
					} else if (_antisipc.codecs[i].vbr == 1) {
						if (_antisipc.codecs[i].mode > 0)
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"%i,any\";vbr=on\r\n",
									 _antisipc.codecs[i].payload,
									 _antisipc.codecs[i].mode);
						else
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"any\";vbr=on\r\n",
									 _antisipc.codecs[i].payload);
						strcat(rtpmaps, tmp_rtpmaps);
					} else if (_antisipc.codecs[i].cng == 1) {
						if (_antisipc.codecs[i].mode > 0)
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"%i,any\";cng=on\r\n",
									 _antisipc.codecs[i].payload,
									 _antisipc.codecs[i].mode);
						else
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"any\";cng=on\r\n",
									 _antisipc.codecs[i].payload);
						strcat(rtpmaps, tmp_rtpmaps);
					} else {
						if (_antisipc.codecs[i].mode > 0)
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"%i,any\"\r\n",
									 _antisipc.codecs[i].payload,
									 _antisipc.codecs[i].mode);
						else
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i mode=\"any\"\r\n",
									 _antisipc.codecs[i].payload);
						strcat(rtpmaps, tmp_rtpmaps);
					}
				}

				if (osip_strcasecmp(_antisipc.codecs[i].name, "ip-mr_v2.5") == 0) {
					if (_antisipc.codecs[i].vbr==0xF000)
						strcat(rtpmaps, "a=ars\r\n");
				}

			}
		}

		if (payloads[0] != '\0')
			strcat(tmp, payloads);

		strcat(tmp, " 101");
		strcat(tmp, "\r\n");

		if (_antisipc.codec_attr.bandwidth > 0) {
			char tmp_b[256];

			memset(tmp_b, 0, sizeof(tmp_b));
			snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
					 _antisipc.codec_attr.bandwidth);
			strcat(tmp, tmp_b);
		}

		strcat(tmp, rtpmaps);
		strcat(tmp, "a=rtpmap:101 telephone-event/8000\r\n");
		strcat(tmp, "a=fmtp:101 0-11\r\n");
		if (_antisipc.codec_attr.ptime > 0) {
			char tmp_ptime[256];

			memset(tmp_ptime, 0, sizeof(tmp_ptime));
			snprintf(tmp_ptime, sizeof(tmp_ptime), "a=ptime:%i\r\n",
					 _antisipc.codec_attr.ptime);
			strcat(tmp, tmp_ptime);
		}

#ifdef ENABLE_VIDEO
		memset(rtpmaps, 0, sizeof(rtpmaps));
		memset(payloads, 0, sizeof(payloads));
		for (i = 0; i < 5; i++) {
			if (ms_filter_codec_supported(_antisipc.video_codecs[i].name)
				&& _antisipc.video_codecs[i].enable > 0) {
				char tmp_payloads[64];
				char tmp_rtpmaps[256];

				memset(tmp_payloads, 0, sizeof(tmp_payloads));
				memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
				snprintf(tmp_payloads, 64, "%i",
						 _antisipc.video_codecs[i].payload);
				strcat(payloads, " ");
				strcat(payloads, tmp_payloads);

				snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
						 "a=rtpmap:%i %s/%i\r\n",
						 _antisipc.video_codecs[i].payload,
						 _antisipc.video_codecs[i].name,
						 _antisipc.video_codecs[i].freq);
				strcat(rtpmaps, tmp_rtpmaps);

				if (osip_strcasecmp(_antisipc.video_codecs[i].name, "H264")
					== 0) {
						if (_antisipc.video_codecs[i].mode==0)
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i profile-level-id=42800c\r\n",
									 _antisipc.video_codecs[i].payload);
						else if (_antisipc.video_codecs[i].mode==1)
							snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
									 "a=fmtp:%i profile-level-id=42800c; packetization-mode=1\r\n",
									 _antisipc.video_codecs[i].payload);
						strcat(rtpmaps, tmp_rtpmaps);
						if (_antisipc.video_codecs[i].vbr==0xF000)
							strcat(rtpmaps, "a=ars\r\n");
				}
			}
		}

		if (payloads[0] != '\0') {
			strcat(tmp, "m=video 0 ");
			strcat(tmp, _antisipc.video_profile);
			strcat(tmp, payloads);
			strcat(tmp, "\r\n");

			if (_antisipc.video_codec_attr.download_bandwidth > 0) {
				char tmp_b[256];

				memset(tmp_b, 0, sizeof(tmp_b));
				snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
						 _antisipc.video_codec_attr.download_bandwidth);
				strcat(tmp, tmp_b);
			}
			strcat(tmp, rtpmaps);
		}
#endif


		osip_message_set_body(options, tmp, strlen(tmp));
		osip_message_set_content_type(options, "application/sdp");

		if (_antisipc.allowed_methods[0] != '\0')
			osip_message_set_allow(options, _antisipc.allowed_methods);
		else {
			osip_message_set_allow(options, "INVITE, ACK, BYE, OPTIONS, CANCEL, UPDATE");
		}
		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(options, _antisipc.supported_extensions);
		else {
			osip_message_set_supported(options, "100rel");
		}
		if (_antisipc.accepted_types[0] != '\0')
			osip_message_set_accept(options, _antisipc.accepted_types);
		else
			osip_message_set_accept(options, "application/sdp");
		if (_antisipc.allowed_events[0] != '\0')
			osip_message_set_header(options, "Allow-Events", _antisipc.allowed_events);

		i = eXosip_options_send_answer(je->tid, 202, options);
		if (i != 0) {
			return i;
		}
		return AMSIP_SUCCESS;
	}
	return i;
}

int call_new(eXosip_event_t * je, int *answered_code)
{
	sdp_message_t *remote_sdp = NULL;
	am_call_t *ca;
	int k;

	*answered_code = 0;
	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state == NOT_USED)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		*answered_code = 486;
		eXosip_call_send_answer(je->tid, 486, NULL);
		return AMSIP_TOOMUCHCALL;
	}

	ca = &(_antisipc.calls[k]);
	am_call_init(ca, k);

	ca->cid = je->cid;
	ca->did = je->did;
	ca->tid = je->tid;

	ca->call_direction = AMSIP_INCOMINGCALL;

	if (ca->did < 1 && ca->cid < 1) {
		*answered_code = 500;
		eXosip_call_send_answer(je->tid, 500, NULL);
		return AMSIP_UNDEFINED_ERROR;	/* not enough information for this event?? */
	}

	os_sound_init();

	/* negotiate payloads */
	if (je->request != NULL) {
		remote_sdp = eXosip_get_sdp_info(je->request);
	}

	if (je->request != NULL)
		_am_calls_get_remote_user_agent(ca, je->request);
	if (je->request != NULL)
		_am_calls_get_p_am_sessiontype(ca, je->request);

	if (remote_sdp == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"missing SDP in INVITE request\n"));
	}

	if (remote_sdp != NULL) {	/* TODO: else build an offer */
		sdp_connection_t *conn;
		sdp_media_t *remote_med;
		char *tmp = NULL;

		conn = eXosip_get_audio_connection(remote_sdp);
		if (conn == NULL || conn->c_addr == NULL) {
			conn = eXosip_get_connection(remote_sdp, "text");
		}
		if (conn == NULL || conn->c_addr == NULL) {
			conn = eXosip_get_connection(remote_sdp, "x-udpftp");
		}

		if (conn == NULL || conn->c_addr == NULL) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_WARNING, NULL,
						"No remote IP address found\n"));
			*answered_code = 400;
			eXosip_call_send_answer(ca->tid, 400, NULL);
			sdp_message_free(remote_sdp);
			return AMSIP_SYNTAXERROR;
		}

		remote_med = eXosip_get_audio_media(remote_sdp);
		if (remote_med == NULL || remote_med->m_port == NULL) {
			remote_med = eXosip_get_media(remote_sdp, "text");
		}
		if (remote_med == NULL || remote_med->m_port == NULL) {
			remote_med = eXosip_get_media(remote_sdp, "x-udpftp");
		}

		if (remote_med == NULL || remote_med->m_port == NULL) {
			/* no audio media proposed */
			*answered_code = 415;
			eXosip_call_send_answer(ca->tid, 415, NULL);
			sdp_message_free(remote_sdp);
			return AMSIP_NOCOMMONCODEC;
		}

		tmp = NULL;
		{
			int pos = 0;

			while (!osip_list_eol(&remote_med->a_attributes, pos)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&remote_med->
													  a_attributes, pos);
				if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
					/* search for each supported payload */
					int n;
					char codec[64];
					char subtype[64];
					char freq[64];

					if (attr->a_att_value == NULL
						|| strlen(attr->a_att_value) > 63) {
						pos++;
						continue;	/* avoid buffer overflow! */
					}
					n = rtpmap_sscanf(attr->a_att_value, codec, subtype,
									  freq);

					/* TODO: we should check the SDP offer instead... */
					if (n == 3) {
						int r = _am_codec_get_definition(subtype);

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = attr->a_att_value;
							break;
						}
					}
					if (n == 3) {
						int r = _am_text_codec_get_definition(subtype);

						if (r >= 0 && _antisipc.text_codecs[r].enable > 0) {
							tmp = attr->a_att_value;
							break;
						}
					}
					if (n == 3) {
						int r = _am_udpftp_codec_get_definition(subtype);

						if (r >= 0
							&& _antisipc.udpftp_codecs[r].enable > 0) {
							tmp = attr->a_att_value;
							break;
						}
					}
				}
				pos++;
			}

			if (tmp == NULL) {
				pos = 0;
				while (!osip_list_eol(&remote_med->m_payloads, pos)) {
					char *pld =
						(char *) osip_list_get(&remote_med->m_payloads,
											   pos);

					if (osip_strcasecmp(pld, "0") == 0) {
						int r = _am_codec_get_definition("PCMU");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "8") == 0) {
						int r = _am_codec_get_definition("PCMA");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "18") == 0) {
						int r = _am_codec_get_definition("G729");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "3") == 0) {
						int r = _am_codec_get_definition("GSM");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "4") == 0) {
						int r = _am_codec_get_definition("g723");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "9") == 0) {
						int r = _am_codec_get_definition("g722");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}

					pos++;
				}
			}
		}
		if (tmp == NULL) {
			*answered_code = 415;
			eXosip_call_send_answer(ca->tid, 415, NULL);
			sdp_message_free(remote_sdp);
			return AMSIP_NOCOMMONCODEC;
		}

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "connection accepted: prefered payload=%s -> %s:%s\n",
							  tmp, conn->c_addr, remote_med->m_port));
	}


	ca->state = je->type;
	_am_calls_get_remote_candidate(ca, je->request, "audio");
	_am_calls_get_remote_candidate(ca, je->request, "video");
	_am_calls_get_remote_candidate(ca, je->request, "text");
	_am_calls_get_remote_candidate(ca, je->request, "x-udpftp");

	if (remote_sdp != NULL) {
		/* controlling state */
		ca->audio_checklist.rem_controlling = 1;
#ifdef ENABLE_VIDEO
		ca->video_checklist.rem_controlling = 1;
#endif
		ca->text_checklist.rem_controlling = 1;
		ca->udpftp_checklist.rem_controlling = 1;
	}

	sdp_message_free(remote_sdp);
	return AMSIP_SUCCESS;
}

int
_calls_get_ip_port(sdp_message_t * sdp, const char *media, char *ip,
				   int *port)
{
	sdp_connection_t *conn;
	sdp_media_t *med;

	ip[0] = '\0';
	*port = 0;

	if (sdp == NULL)
		return AMSIP_BADPARAMETER;

	conn = eXosip_get_connection(sdp, media);
	if (conn != NULL && conn->c_addr != NULL) {
		snprintf(ip, 50, "%s", conn->c_addr);
	}
	med = eXosip_get_media(sdp, media);
	if (med != NULL && med->m_port != NULL) {
		*port = atoi(med->m_port);
	}
	return AMSIP_SUCCESS;
}

int call_ack(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS)
		return AMSIP_NOTFOUND;

	ca = &(_antisipc.calls[k]);

	if (je->ack != NULL && je->response != NULL) {
		sdp_message_t *remote_sdp;

		remote_sdp = eXosip_get_sdp_info(je->ack);
		if (remote_sdp != NULL) {
			_calls_start_audio_from_sipmessage(ca, je->response, je->ack,
											   0);
#ifdef ENABLE_VIDEO
			_calls_start_video_from_sipmessage(ca, je->response, je->ack,
											   0);
#endif
			_calls_start_text_from_sipmessage(ca, je->response, je->ack,
											  0);
			_calls_start_udpftp_from_sipmessage(ca, je->response, je->ack,
												0);
			sdp_message_free(remote_sdp);
		}
	}

	ca->state = je->type;

#ifdef ENABLE_VIDEO
	if (ca->enable_video > 0 && _antisipc.automatic_rfc5168>0) {
		osip_message_t *request;
		int res;

		res = eXosip_call_build_request(ca->did, "INFO", &request);
		if (res == 0) {
			osip_message_set_content_type(request, "application/media_control+xml");
			osip_message_set_body(request,
				"<?xml version=\"1.0\" encoding=\"utf-8\" ?> <media_control> <vc_primitive> <to_encoder> <picture_fast_update/> </to_encoder> </vc_primitive> </media_control>"
				, strlen("<?xml version=\"1.0\" encoding=\"utf-8\" ?> <media_control> <vc_primitive> <to_encoder> <picture_fast_update/> </to_encoder> </vc_primitive> </media_control>"));

			res = eXosip_call_send_request(ca->did, request);
		}
	}
#endif

	return AMSIP_SUCCESS;
}

int call_proceeding(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state == NOT_USED)
				break;
		}
		if (k == MAX_NUMBER_OF_CALLS)
			return AMSIP_TOOMUCHCALL;

		ca = &(_antisipc.calls[k]);
		am_call_init(ca, k);

		ca->cid = je->cid;
		ca->did = je->did;

		if (ca->did < 1 || ca->cid < 1) {
			return AMSIP_UNDEFINED_ERROR;	/* not enough information for this event?? */
		}
	}

	ca = &(_antisipc.calls[k]);
	ca->did = je->did;

	_am_calls_get_conf_name(ca, je->request);

	ca->state = je->type;
	return AMSIP_SUCCESS;

}

int call_ringing(eXosip_event_t * je)
{
	sdp_message_t *remote_sdp;
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state == NOT_USED)
				break;
		}
		if (k == MAX_NUMBER_OF_CALLS)
			return AMSIP_TOOMUCHCALL;
		ca = &(_antisipc.calls[k]);
		am_call_release(ca);
		am_call_init(ca, k);

		ca->cid = je->cid;
		ca->did = je->did;

		if (ca->did < 1 || ca->cid < 1) {
			am_call_release(ca);
			return AMSIP_UNDEFINED_ERROR;	/* not enough information for this event?? */
		}
	}

	ca = &(_antisipc.calls[k]);

	_am_calls_get_conf_name(ca, je->request);

	ca->cid = je->cid;
	ca->did = je->did;
	ca->tid = je->tid;

	{
		osip_header_t *rseq;

		osip_message_header_get_byname(je->response, "RSeq", 0, &rseq);
		if (rseq != NULL && rseq->hvalue != NULL) {
			/* try sending a PRACK */
			osip_message_t *prack = NULL;
			int i;

			i = eXosip_call_build_prack(ca->tid, &prack);
			if (i != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"Failed to build PRACK request\n"));
			} else {
				eXosip_call_send_prack(ca->tid, prack);
			}

		}
	}


	ca->state = je->type;

	_am_calls_get_remote_user_agent(ca, je->response);
	_am_calls_get_remote_candidate(ca, je->response, "audio");
	_am_calls_get_remote_candidate(ca, je->response, "video");
	_am_calls_get_remote_candidate(ca, je->response, "text");
	_am_calls_get_remote_candidate(ca, je->response, "x-udpftp");


	/* solve negotiation issue with icelandair */
	if (je->response != NULL) {
		remote_sdp = eXosip_get_sdp_info(je->response);
	}

	if (remote_sdp == NULL) {
		return AMSIP_SUCCESS;
	}

	{
		sdp_media_t *remote_med;
		char *tmp = NULL;

		remote_med = eXosip_get_audio_media(remote_sdp);
		if (remote_med == NULL || remote_med->m_port == NULL) {
			remote_med = eXosip_get_media(remote_sdp, "text");
		}
		if (remote_med == NULL || remote_med->m_port == NULL) {
			remote_med = eXosip_get_media(remote_sdp, "x-udpftp");
		}

		if (remote_med == NULL || remote_med->m_port == NULL) {
			/* no audio media proposed */
			sdp_message_free(remote_sdp);
			return AMSIP_NOCOMMONCODEC;
		}

		tmp = NULL;
		{
			int pos = 0;

			while (!osip_list_eol(&remote_med->a_attributes, pos)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&remote_med->
													  a_attributes, pos);
				if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
					/* search for each supported payload */
					int n;
					char codec[64];
					char subtype[64];
					char freq[64];

					if (attr->a_att_value == NULL
						|| strlen(attr->a_att_value) > 63) {
						pos++;
						continue;	/* avoid buffer overflow! */
					}
					n = rtpmap_sscanf(attr->a_att_value, codec, subtype,
									  freq);

					/* TODO: we should check the SDP offer instead... */
					if (n == 3) {
						int r = _am_codec_get_definition(subtype);

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = attr->a_att_value;
							break;
						}
					}
					if (n == 3) {
						int r = _am_text_codec_get_definition(subtype);

						if (r >= 0 && _antisipc.text_codecs[r].enable > 0) {
							tmp = attr->a_att_value;
							break;
						}
					}
					if (n == 3) {
						int r = _am_udpftp_codec_get_definition(subtype);

						if (r >= 0
							&& _antisipc.udpftp_codecs[r].enable > 0) {
							tmp = attr->a_att_value;
							break;
						}
					}
				}
				pos++;
			}

			if (tmp == NULL) {
				pos = 0;
				while (!osip_list_eol(&remote_med->m_payloads, pos)) {
					char *pld =
						(char *) osip_list_get(&remote_med->m_payloads,
											   pos);

					if (osip_strcasecmp(pld, "0") == 0) {
						int r = _am_codec_get_definition("PCMU");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "8") == 0) {
						int r = _am_codec_get_definition("PCMA");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "18") == 0) {
						int r = _am_codec_get_definition("G729");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "3") == 0) {
						int r = _am_codec_get_definition("GSM");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "4") == 0) {
						int r = _am_codec_get_definition("g723");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}
					if (osip_strcasecmp(pld, "9") == 0) {
						int r = _am_codec_get_definition("g722");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							tmp = _antisipc.codecs[r].name;
							break;
						}
					}

					pos++;
				}
			}
		}
		if (tmp == NULL) {
			sdp_message_free(remote_sdp);
			return AMSIP_NOCOMMONCODEC;
		}

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio connection accepted: prefered payload=%s -> %s\n",
							  tmp, remote_med->m_port));

		sdp_message_free(remote_sdp);
	}



	_calls_start_audio_from_sipmessage(ca, je->request, je->response, 0);
#ifdef ENABLE_VIDEO
	_calls_start_video_from_sipmessage(ca, je->request, je->response, 0);
#endif
	_calls_start_text_from_sipmessage(ca, je->request, je->response, 0);
	_calls_start_udpftp_from_sipmessage(ca, je->request, je->response, 0);

	return AMSIP_SUCCESS;
}

int call_answered(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;
	osip_message_t *ack_with_sdp = NULL;
	osip_message_t *ack = NULL;
	int i;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state == NOT_USED)
				break;
		}
		if (k == MAX_NUMBER_OF_CALLS)
			return AMSIP_TOOMUCHCALL;
		ca = &(_antisipc.calls[k]);
		am_call_init(ca, k);
		ca->cid = je->cid;
		ca->did = je->did;

		if (ca->did < 1 && ca->cid < 1) {
			return AMSIP_UNDEFINED_ERROR;	/* not enough information for this event?? */
		}
	}

	ca = &(_antisipc.calls[k]);
	ca->did = je->did;

	_am_calls_get_conf_name(ca, je->request);

	i = eXosip_call_build_ack(ca->did, &ack);
	if (i != 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Cannot build ACK for call!\n"));
	} else {
		sdp_message_t *local_sdp = NULL;
		sdp_message_t *remote_sdp = NULL;

		if (je->request != NULL && je->response != NULL) {
			local_sdp = eXosip_get_sdp_info(je->request);
			remote_sdp = eXosip_get_sdp_info(je->response);
		}
		if (local_sdp == NULL && remote_sdp != NULL) {
			/* sdp in ACK */
			i = sdp_complete_message(ca, remote_sdp, ack);
			if (i != 0) {
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"Cannot complete ACK with sdp body?!\n"));
			} else {
				i = osip_message_clone(ack, &ack_with_sdp);

				if (ca->local_sdp != NULL)
					sdp_message_free(ca->local_sdp);
				ca->local_sdp = eXosip_get_sdp_info(ack);
				if (ca->local_sdp == NULL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_WARNING, NULL,
								"Failed to set local_sdp\n"));
				}
			}
		}
		sdp_message_free(local_sdp);
		sdp_message_free(remote_sdp);
		i = eXosip_call_send_ack(ca->did, ack);
	}

	_am_calls_get_remote_user_agent(ca, je->response);
	_am_calls_get_remote_candidate(ca, je->response, "audio");
	_am_calls_get_remote_candidate(ca, je->response, "video");
	_am_calls_get_remote_candidate(ca, je->response, "text");
	_am_calls_get_remote_candidate(ca, je->response, "x-udpftp");
	if (ack_with_sdp != NULL) {
		_calls_start_audio_from_sipmessage(ca, je->response, ack_with_sdp,
										   1);
#ifdef ENABLE_VIDEO
		_calls_start_video_from_sipmessage(ca, je->response, ack_with_sdp,
										   1);
#endif
		_calls_start_text_from_sipmessage(ca, je->response, ack_with_sdp,
										  0);
		_calls_start_udpftp_from_sipmessage(ca, je->response, ack_with_sdp,
											0);
		osip_message_free(ack_with_sdp);
	} else {
		_calls_start_audio_from_sipmessage(ca, je->request, je->response,
										   0);
#ifdef ENABLE_VIDEO
		_calls_start_video_from_sipmessage(ca, je->request, je->response,
										   0);
#endif
		_calls_start_text_from_sipmessage(ca, je->request, je->response,
										  0);
		_calls_start_udpftp_from_sipmessage(ca, je->request, je->response,
											0);
	}

	ca->call_established = 1;
	ca->state = je->type;

#ifdef ENABLE_VIDEO
	if (ca->enable_video > 0 && _antisipc.automatic_rfc5168>0) {
		osip_message_t *request;
		int res;

		res = eXosip_call_build_request(ca->did, "INFO", &request);
		if (res == 0) {
			osip_message_set_content_type(request, "application/media_control+xml");
			osip_message_set_body(request,
				"<?xml version=\"1.0\" encoding=\"utf-8\" ?> <media_control> <vc_primitive> <to_encoder> <picture_fast_update/> </to_encoder> </vc_primitive> </media_control>"
				, strlen("<?xml version=\"1.0\" encoding=\"utf-8\" ?> <media_control> <vc_primitive> <to_encoder> <picture_fast_update/> </to_encoder> </vc_primitive> </media_control>"));

			res = eXosip_call_send_request(ca->did, request);
		}
	}
#endif
	return i;
}

int call_redirected(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);
	ca->did = je->did;

	if (ca->call_established == 1)
		return AMSIP_SUCCESS;	/* WHEN AN ERROR IS RECEIVED, NO CHANGE SHOULD BE DONE TO THE EXISTING CALL */

	_am_calls_get_conf_name(ca, je->request);

	return AMSIP_SUCCESS;		/* don't release call as we are redirected */
}

int call_requestfailure(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}

	if ((je->response != NULL && je->response->status_code == 407)
		|| (je->response != NULL && je->response->status_code == 401)
		|| (je->response != NULL && je->response->status_code == 422)) {
		return AMSIP_SUCCESS;
	}

	if (k == MAX_NUMBER_OF_CALLS) {
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);
	ca->did = je->did;

	if (ca->call_established == 1 && je->response->status_code != 408
		&& je->response->status_code != 481)
		return AMSIP_SUCCESS;	/* WHEN AN ERROR IS RECEIVED, NO CHANGE SHOULD BE DONE TO THE EXISTING CALL */

	_am_calls_get_conf_name(ca, je->request);

	am_call_release(ca);
	return AMSIP_SUCCESS;
}

int call_serverfailure(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);
	ca->did = je->did;

	if (ca->call_established == 1)
		return AMSIP_SUCCESS;	/* WHEN AN ERROR IS RECEIVED, NO CHANGE SHOULD BE DONE TO THE EXISTING CALL */

	_am_calls_get_conf_name(ca, je->request);

	am_call_release(ca);
	return AMSIP_SUCCESS;
}

int call_globalfailure(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);
	ca->did = je->did;

	if (ca->call_established == 1)
		return AMSIP_SUCCESS;	/* WHEN AN ERROR IS RECEIVED, NO CHANGE SHOULD BE DONE TO THE EXISTING CALL */

	_am_calls_get_conf_name(ca, je->request);

	am_call_release(ca);
	return AMSIP_SUCCESS;
}

int call_closed(eXosip_event_t * je)
{
	am_call_t *ca;
	int k;

	while (1) {

		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == je->cid)
				break;
		}
		if (k == MAX_NUMBER_OF_CALLS) {
			return AMSIP_NOTFOUND;
		}

		ca = &(_antisipc.calls[k]);

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Deleting amsip call ressource cid=%i did=%i\n",
					ca->cid, ca->did));

		am_call_release(ca);
	}

	return AMSIP_SUCCESS;
}

int call_stop_sound(int cid, int did)
{
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == cid
			&& _antisipc.calls[k].did == did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].cid == cid
				&& _antisipc.calls[k].did == 0)
				break;
		}
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);
	ca->did = did;

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_WARNING, NULL,
				"Deleting amsip call ressource cid=%i did=%i\n", ca->cid,
				ca->did));

	if (ca->enable_udpftp > 0) {
		ca->enable_udpftp = -1;
		os_udpftp_close(ca);
	}

	if (ca->enable_text > 0) {
		ca->enable_text = -1;
		os_text_close(ca);
	}

	if (ca->enable_audio > 0) {
		ca->enable_audio = -1;
		_antisipc.audio_media->audio_module_sound_close(ca);
	}
#ifdef ENABLE_VIDEO
	if (ca->enable_video > 0) {
		ca->enable_video = -1;
		_antisipc.video_media->video_module_session_close(ca);
	}
#endif
	return AMSIP_SUCCESS;
}

int call_modified(eXosip_event_t * je, int *answer_code)
{
	am_call_t *ca;
	int k;
	osip_message_t *answer_with_sdp = NULL;
	osip_message_t *answer = NULL;
	sdp_message_t *remote_sdp = NULL;
	int i;

	*answer_code = 0;
	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		*answer_code = 500;
		eXosip_call_send_answer(je->tid, 500, NULL);
		return AMSIP_NOTFOUND;
	}
	ca = &(_antisipc.calls[k]);
	ca->tid = je->tid;

	ca->conf_name[0] = '\0';

	ca->state = je->type;

	i = eXosip_call_build_answer(ca->tid, OK_RETURN_CODE, &answer);
	if (i != 0) {
		*answer_code = 400;
		eXosip_call_send_answer(ca->tid, 400, NULL);
		return i;
	}

	if (je->request != NULL)
		remote_sdp = eXosip_get_sdp_info(je->request);
	if (remote_sdp == NULL) {
		sdp_message_t *local_sdp = NULL;
		char *tmp = NULL;

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"No remote SDP body found for call\n"));
		/* TODO: sdp in 200 & ACK */
		local_sdp = eXosip_get_previous_local_sdp(ca->did);
		if (local_sdp == NULL && ca->local_sdp == NULL) {
			*answer_code = 400;
			osip_message_free(answer);
			eXosip_call_send_answer(ca->tid, 400, NULL);
			return AMSIP_UNDEFINED_ERROR;
		}
		if (local_sdp == NULL && ca->local_sdp != NULL) {
			_sdp_hold_call(ca->local_sdp); /* interoperate with polycom */
			_sdp_off_hold_call(ca->local_sdp);
			i = sdp_message_to_str(ca->local_sdp, &tmp);
			if (i != 0) {
				*answer_code = 400;
				osip_message_free(answer);
				eXosip_call_send_answer(ca->tid, 400, NULL);
				return i;
			}

		} else if (local_sdp != NULL) {
			_sdp_hold_call(local_sdp); /* interoperate with polycom */
			_sdp_off_hold_call(local_sdp);
			i = sdp_message_to_str(local_sdp, &tmp);
			if (i != 0) {
				*answer_code = 400;
				osip_message_free(answer);
				eXosip_call_send_answer(ca->tid, 400, NULL);
				return i;
			}
			sdp_message_free(local_sdp);
		}

		osip_message_set_body(answer, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(answer, "application/sdp");
		i = eXosip_call_send_answer(ca->tid, OK_RETURN_CODE, answer);
		if (i != 0) {
			*answer_code = 400;
			eXosip_call_send_answer(ca->tid, 400, NULL);
			return i;
		}
	} else {
		i = sdp_complete_message(ca, remote_sdp, answer);
		if (i != 0) {
			*answer_code = 415;
			sdp_message_free(remote_sdp);
			osip_message_free(answer);
			eXosip_call_send_answer(ca->tid, 415, NULL);
			return i;
		} else {
			osip_message_clone(answer, &answer_with_sdp);
			i = eXosip_call_send_answer(ca->tid, OK_RETURN_CODE, answer);
		}
		sdp_message_free(remote_sdp);
	}

	if (je->request != NULL)
		_am_calls_get_p_am_sessiontype(ca, je->request);

	if (answer_with_sdp != NULL) {
		_am_calls_get_remote_user_agent(ca, je->request);
		_am_calls_get_remote_candidate(ca, je->request, "audio");
		_am_calls_get_remote_candidate(ca, je->request, "video");
		_am_calls_get_remote_candidate(ca, je->request, "text");
		_am_calls_get_remote_candidate(ca, je->request, "x-udpftp");
		_calls_start_audio_from_sipmessage(ca, je->request,
										   answer_with_sdp, 1);
#ifdef ENABLE_VIDEO
		_calls_start_video_from_sipmessage(ca, je->request,
										   answer_with_sdp, 1);
#endif
		_calls_start_text_from_sipmessage(ca, je->request, answer_with_sdp,
										  1);
		_calls_start_udpftp_from_sipmessage(ca, je->request,
											answer_with_sdp, 1);
		osip_message_free(answer_with_sdp);
	}

	*answer_code = OK_RETURN_CODE;
	return i;
}

int call_update(eXosip_event_t * je, int *answer_code)
{
	am_call_t *ca;
	int k;
	osip_message_t *answer_with_sdp = NULL;
	osip_message_t *answer = NULL;
	int i;

	*answer_code = 0;
	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].cid == je->cid
			&& _antisipc.calls[k].did == je->did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS) {
		*answer_code = 500;
		eXosip_call_send_answer(je->tid, 500, NULL);
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);
	ca->tid = je->tid;

	ca->state = je->type;

	i = eXosip_call_build_answer(ca->tid, OK_RETURN_CODE, &answer);
	if (i != 0) {
		*answer_code = 400;
		eXosip_call_send_answer(ca->tid, 400, NULL);
		return i;
	} else {
		sdp_message_t *remote_sdp = NULL;

		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(answer, _antisipc.supported_extensions);

		if (je->request != NULL)
			remote_sdp = eXosip_get_sdp_info(je->request);
		if (remote_sdp == NULL) {
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_WARNING, NULL,
						"No remote SDP body found for call\n"));
			i = eXosip_call_send_answer(ca->tid, OK_RETURN_CODE, answer);
		} else {
			i = sdp_complete_message(ca, remote_sdp, answer);
			if (i != 0) {
				*answer_code = 415;
				sdp_message_free(remote_sdp);
				osip_message_free(answer);
				eXosip_call_send_answer(ca->tid, 415, NULL);
				return i;
			} else {
				osip_message_clone(answer, &answer_with_sdp);
				i = eXosip_call_send_answer(ca->tid, OK_RETURN_CODE,
											answer);
			}
			sdp_message_free(remote_sdp);

		}
	}

	if (answer_with_sdp != NULL) {
		_am_calls_get_remote_user_agent(ca, je->request);
		_am_calls_get_remote_candidate(ca, je->request, "audio");
		_am_calls_get_remote_candidate(ca, je->request, "video");
		_am_calls_get_remote_candidate(ca, je->request, "text");
		_am_calls_get_remote_candidate(ca, je->request, "x-udpftp");
		_calls_start_audio_from_sipmessage(ca, je->request,
										   answer_with_sdp, 1);
#ifdef ENABLE_VIDEO
		_calls_start_video_from_sipmessage(ca, je->request,
										   answer_with_sdp, 1);
#endif
		_calls_start_text_from_sipmessage(ca, je->request, answer_with_sdp,
										  1);
		_calls_start_udpftp_from_sipmessage(ca, je->request,
											answer_with_sdp, 1);
		osip_message_free(answer_with_sdp);
	}

	*answer_code = OK_RETURN_CODE;


#ifdef ENABLE_VIDEO
	if (ca->enable_video > 0 && _antisipc.automatic_rfc5168) {
		osip_message_t *request;
		int res;

		res = eXosip_call_build_request(ca->did, "INFO", &request);
		if (res == 0) {
			osip_message_set_content_type(request, "application/media_control+xml");
			osip_message_set_body(request,
				"<?xml version=\"1.0\" encoding=\"utf-8\" ?> <media_control> <vc_primitive> <to_encoder> <picture_fast_update/> </to_encoder> </vc_primitive> </media_control>"
				, strlen("<?xml version=\"1.0\" encoding=\"utf-8\" ?> <media_control> <vc_primitive> <to_encoder> <picture_fast_update/> </to_encoder> </vc_primitive> </media_control>"));

			res = eXosip_call_send_request(ca->did, request);
		}
	}
#endif
	return i;
}

int
_calls_start_audio_with_id(int tid_for_offer, int did,
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
		return AMSIP_NOTFOUND;
	}

	ca = &(_antisipc.calls[k]);

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_WARNING, NULL,
				"Start audio ressource cid=%i did=%i\n", ca->cid,
				ca->did));

	return _calls_start_audio_with_sipanswer(ca, tid_for_offer, answer, 1);
}

int
_calls_start_audio_with_sipanswer(am_call_t * ca, int tid_for_offer,
								  osip_message_t * answer,
								  int local_is_offer)
{
	int i;
	sdp_message_t *offer_sdp = NULL;
	sdp_message_t *answer_sdp = NULL;

	offer_sdp = eXosip_get_remote_sdp_from_tid(tid_for_offer);
	if (offer_sdp == NULL) {
		return AMSIP_BADPARAMETER;
	}
	answer_sdp = eXosip_get_sdp_info(answer);
	if (answer_sdp == NULL) {
		sdp_message_free(offer_sdp);
		return AMSIP_BADPARAMETER;
	}

	i = _calls_start_audio_from_sdpmessage(ca, offer_sdp, answer_sdp,
										   local_is_offer);
	sdp_message_free(offer_sdp);
	sdp_message_free(answer_sdp);
	return i;
}

int
_calls_start_audio_from_sipmessage(am_call_t * ca, osip_message_t * offer,
								   osip_message_t * answer,
								   int local_is_offer)
{
	int i;
	sdp_message_t *offer_sdp = NULL;
	sdp_message_t *answer_sdp = NULL;

	offer_sdp = eXosip_get_sdp_info(offer);
	if (offer_sdp == NULL) {
		return AMSIP_BADPARAMETER;
	}
	answer_sdp = eXosip_get_sdp_info(answer);
	if (answer_sdp == NULL) {
		sdp_message_free(offer_sdp);
		return AMSIP_BADPARAMETER;
	}

	i = _calls_start_audio_from_sdpmessage(ca, offer_sdp, answer_sdp,
										   local_is_offer);
	sdp_message_free(offer_sdp);
	sdp_message_free(answer_sdp);
	return i;
}

int
_calls_start_audio_from_sdpmessage(am_call_t * ca,
								   sdp_message_t * offer_sdp,
								   sdp_message_t * answer_sdp,
								   int local_is_offer)
{
	char offer_ip[256];
	int offer_port = 0;
	char answer_ip[256];
	int answer_port = 0;
	int local_sendrecv;
	int remote_sendrecv;
	int setup_passive = 0;
	char offer_setup[256];
	char answer_setup[256];
	int i;

	sdp_media_t *offer_med = NULL;
	sdp_media_t *answer_med = NULL;

	memset(answer_ip, '\0', sizeof(answer_ip));
	memset(offer_ip, '\0', sizeof(offer_ip));
	answer_port = -1;
	offer_port = -1;
	_calls_get_ip_port(answer_sdp, "audio", answer_ip, &answer_port);
	_calls_get_ip_port(offer_sdp, "audio", offer_ip, &offer_port);

	if (answer_ip[0] == '\0' || offer_ip[0] == '\0') {
		return AMSIP_SUCCESS;	/* disabled with the old way */
	}

	if (ca->enable_audio > 0) {
		ca->enable_audio = -1;
		_antisipc.audio_media->audio_module_sound_close(ca);
	}

	/* port 0 means that stream is disabled */
	if (answer_port == 0 || offer_port == 0) {
		return AMSIP_SUCCESS;
	}

	/* search if stream is sendonly or recvonly */
	offer_med = eXosip_get_audio_media(offer_sdp);
	answer_med = eXosip_get_audio_media(answer_sdp);

	offer_setup[0] = '\0';
	answer_setup[0] = '\0';
	_sdp_analyse_attribute_setup(offer_sdp, offer_med, offer_setup);
	_sdp_analyse_attribute_setup(answer_sdp, answer_med, answer_setup);

	if (local_is_offer == 0) {
		local_sendrecv = _sdp_analyse_attribute(offer_sdp, offer_med);
		remote_sendrecv = _sdp_analyse_attribute(answer_sdp, answer_med);
		if (osip_strcasecmp(offer_setup, "passive") == 0
			&& osip_strcasecmp(answer_setup, "passive") != 0) {
			setup_passive = 200;
		}
	} else {
		local_sendrecv = _sdp_analyse_attribute(answer_sdp, answer_med);
		remote_sendrecv = _sdp_analyse_attribute(offer_sdp, offer_med);
		if (osip_strcasecmp(answer_setup, "passive") == 0
			&& osip_strcasecmp(offer_setup, "passive") != 0) {
			setup_passive = 200;
		}
	}
	if (local_sendrecv == _INACTIVE)
		remote_sendrecv = _INACTIVE;
	if (remote_sendrecv == _INACTIVE)
		local_sendrecv = _INACTIVE;

	if (local_sendrecv == _SENDRECV) {
		if (remote_sendrecv == _SENDONLY)
			local_sendrecv = _RECVONLY;
		else if (remote_sendrecv == _RECVONLY)
			local_sendrecv = _SENDONLY;
	}

	if (local_sendrecv == _SENDONLY || local_sendrecv == _SENDRECV) {
		if (local_is_offer == 0) {
			if (osip_strcasecmp(answer_ip, "0.0.0.0") == 0)
				local_sendrecv = _RECVONLY;
		} else {
			if (osip_strcasecmp(offer_ip, "0.0.0.0") == 0)
				local_sendrecv = _RECVONLY;
		}
	}

	ca->local_sendrecv = local_sendrecv;
	if (local_is_offer == 0) {
		ca->audio_rtp_in_direct_mode = 0;
		snprintf(ca->audio_rtp_remote_addr, 256, "%s:%i", answer_ip,
				 answer_port);
		i = _antisipc.audio_media->audio_module_sound_start(ca, answer_sdp,
															offer_sdp,
															offer_sdp,
															answer_sdp,
															offer_port,
															answer_ip,
															answer_port,
															setup_passive);
		if (i == 0) {
			ca->enable_audio = 1;	/* audio is started */
		}
	} else {
		ca->audio_rtp_in_direct_mode = 0;
		snprintf(ca->audio_rtp_remote_addr, 256, "%s:%i", offer_ip,
				 offer_port);
		i = _antisipc.audio_media->audio_module_sound_start(ca, answer_sdp,
															offer_sdp,
															answer_sdp,
															offer_sdp,
															answer_port,
															offer_ip,
															offer_port,
															setup_passive);
		if (i == 0) {
			ca->enable_audio = 1;	/* audio is started */
		}
	}

	return i;
}

am_call_t *_am_calls_find_audio_connection(int tid, int did)
{
	struct SdpCandidate *relay_candidates;
	struct SdpCandidate *stun_candidates;
	struct SdpCandidate *local_candidates;
	int free_audio_port;

#ifdef ENABLE_VIDEO
	int free_video_port;
#endif
	int free_text_port;
	int free_udpftp_port;
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED
			&& _antisipc.calls[k].tid == tid
			&& _antisipc.calls[k].did == did)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS && did > 0) {
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state != NOT_USED
				&& _antisipc.calls[k].did == did)
				break;
		}
	}

	if (k == MAX_NUMBER_OF_CALLS)
		return NULL;

	if (_antisipc.port_range_min == 0)
		_antisipc.port_range_min = 10500;

	ca = &(_antisipc.calls[k]);

	if (tid == -2)
		return ca;

	/* check if already done */
	if (ca->audio_candidate.local_candidates[0].foundation <= 0) {
		/* Find First non-used socket */
		free_audio_port =
			eXosip_find_free_port(_antisipc.port_range_min + (k * 2),
								  IPPROTO_UDP);
		if (free_audio_port <= 1024)
			free_audio_port = _antisipc.port_range_min + (k * 2);	/* ERROR: No port seems available on this host?? */
	}
#ifdef ENABLE_VIDEO
	if (ca->video_candidate.local_candidates[0].foundation <= 0) {
		/* Find First non-used socket */
		free_video_port =
			eXosip_find_free_port(_antisipc.port_range_min + (k * 2) +
								  MAX_NUMBER_OF_CALLS * 2, IPPROTO_UDP);
		if (free_video_port <= 1024)
			free_video_port = _antisipc.port_range_min + (k * 2) + MAX_NUMBER_OF_CALLS * 2;	/* ERROR: No port seems available on this host?? */
	}
#endif
	if (ca->text_candidate.local_candidates[0].foundation <= 0) {
		/* Find First non-used socket */
		free_text_port =
			eXosip_find_free_port(_antisipc.port_range_min + (k * 2) +
								  MAX_NUMBER_OF_CALLS * 3, IPPROTO_UDP);
		if (free_text_port <= 1024)
			free_text_port = _antisipc.port_range_min + (k * 2) + MAX_NUMBER_OF_CALLS * 3;	/* ERROR: No port seems available on this host?? */
	}

	if (ca->udpftp_candidate.local_candidates[0].foundation <= 0) {
		/* Find First non-used socket */
		free_udpftp_port =
			eXosip_find_free_port(_antisipc.port_range_min + (k * 2) +
								  MAX_NUMBER_OF_CALLS * 4, IPPROTO_UDP);
		if (free_udpftp_port <= 1024)
			free_udpftp_port = _antisipc.port_range_min + (k * 2) + MAX_NUMBER_OF_CALLS * 4;	/* ERROR: No port seems available on this host?? */
	}

	/* check if already done */
	if (ca->audio_candidate.local_candidates[0].foundation <= 0) {
		stun_candidates =
			&_antisipc.calls[k].audio_candidate.stun_candidates[0];
		local_candidates =
			&_antisipc.calls[k].audio_candidate.local_candidates[0];
		relay_candidates =
			&_antisipc.calls[k].audio_candidate.relay_candidates[0];
		am_network_add_local_candidates(local_candidates, free_audio_port);

		/* build new connection */
		if (_antisipc.use_turn_server != 0
			&& _antisipc.stun_server[0] != '\0') {
			int i;

			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_audio_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_audio_port;
					}
				}
			}

			if (stun_candidates[0].rel_port == 0) {
				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_audio_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_audio_port;
				}
			}
		} else if (_antisipc.stuntest.use_stun_server != 0
				   && _antisipc.stun_server[0] != '\0') {
			if (stun_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_audio_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_audio_port;
				}
			}
		} else {
			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					int i;

					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_audio_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_audio_port;
					}
				}
			}

			stun_candidates[0].rel_port = free_audio_port;
		}
	}
#ifdef ENABLE_VIDEO
	if (ca->video_candidate.local_candidates[0].foundation <= 0) {
		stun_candidates =
			&_antisipc.calls[k].video_candidate.stun_candidates[0];
		local_candidates =
			&_antisipc.calls[k].video_candidate.local_candidates[0];
		relay_candidates =
			&_antisipc.calls[k].video_candidate.relay_candidates[0];
		am_network_add_local_candidates(local_candidates, free_video_port);

		/* build new connection */
		if (_antisipc.use_turn_server != 0
			&& _antisipc.stun_server[0] != '\0') {
			int i;

			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_video_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_video_port;
					}
				}
			}

			if (stun_candidates[0].rel_port == 0) {
				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_video_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_video_port;
				}
			}
		} else if (_antisipc.stuntest.use_stun_server != 0
				   && _antisipc.stun_server[0] != '\0') {
			if (stun_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_video_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_video_port;
				}
			}
		} else {
			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					int i;

					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_video_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_video_port;
					}
				}
			}

			stun_candidates[0].rel_port = free_video_port;
		}
	}
#endif
	if (ca->text_candidate.local_candidates[0].foundation <= 0) {
		stun_candidates =
			&_antisipc.calls[k].text_candidate.stun_candidates[0];
		local_candidates =
			&_antisipc.calls[k].text_candidate.local_candidates[0];
		relay_candidates =
			&_antisipc.calls[k].text_candidate.relay_candidates[0];
		am_network_add_local_candidates(local_candidates, free_text_port);

		/* build new connection */
		if (_antisipc.use_turn_server != 0
			&& _antisipc.stun_server[0] != '\0') {
			int i;

			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_text_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_text_port;
					}
				}
			}

			if (stun_candidates[0].rel_port == 0) {
				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_text_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_text_port;
				}
			}
		} else if (_antisipc.stuntest.use_stun_server != 0
				   && _antisipc.stun_server[0] != '\0') {
			if (stun_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_text_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_text_port;
				}
			}
		} else {
			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					int i;

					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_text_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_text_port;
					}
				}
			}

			stun_candidates[0].rel_port = free_text_port;
		}
	}

	if (ca->udpftp_candidate.local_candidates[0].foundation <= 0) {
		stun_candidates =
			&_antisipc.calls[k].udpftp_candidate.stun_candidates[0];
		local_candidates =
			&_antisipc.calls[k].udpftp_candidate.local_candidates[0];
		relay_candidates =
			&_antisipc.calls[k].udpftp_candidate.relay_candidates[0];
		am_network_add_local_candidates(local_candidates,
										free_udpftp_port);

		/* build new connection */
		if (_antisipc.use_turn_server != 0
			&& _antisipc.stun_server[0] != '\0') {
			int i;

			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_udpftp_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_udpftp_port;
					}
				}
			}

			if (stun_candidates[0].rel_port == 0) {
				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_udpftp_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_udpftp_port;
				}
			}
		} else if (_antisipc.stuntest.use_stun_server != 0
				   && _antisipc.stun_server[0] != '\0') {
			if (stun_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_stun_candidates(stun_candidates,
												   local_candidates,
												   _antisipc.stun_server,
												   free_udpftp_port);
				if (i <= 0) {
					stun_candidates[0].rel_port = free_udpftp_port;
				}
			}
		} else {
			if (_antisipc.use_relay_server != 0) {
				if (relay_candidates[0].rel_port == 0) {
					int i;

					i = am_network_add_relay_candidates(relay_candidates,
														_antisipc.
														relay_server,
														free_udpftp_port);
					if (i <= 0) {
						relay_candidates[0].rel_port = free_udpftp_port;
					}
				}
			}

			stun_candidates[0].rel_port = free_udpftp_port;
		}
	}

	return ca;
}

am_call_t *_am_calls_new_audio_connection(void)
{
	struct SdpCandidate *relay_candidates;
	struct SdpCandidate *stun_candidates;
	struct SdpCandidate *local_candidates;
	int free_audio_port;

#ifdef ENABLE_VIDEO
	int free_video_port;
#endif
	int free_text_port;
	int free_udpftp_port;
	am_call_t *ca;
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state == NOT_USED)
			break;
	}
	if (k == MAX_NUMBER_OF_CALLS)
		return NULL;

	if (_antisipc.port_range_min == 0)
		_antisipc.port_range_min = 10500;

	ca = &(_antisipc.calls[k]);
	am_call_init(ca, k);
	_antisipc.calls[k].state = -1;

	/* check if already done */
	/* Find First non-used socket */
	free_audio_port =
		eXosip_find_free_port(_antisipc.port_range_min + (k * 2),
							  IPPROTO_UDP);
	if (free_audio_port <= 1024)
		free_audio_port = _antisipc.port_range_min + (k * 2);	/* ERROR: No port seems available on this host?? */

#ifdef ENABLE_VIDEO
	/* Find First non-used socket */
	free_video_port =
		eXosip_find_free_port(_antisipc.port_range_min + (k * 2) +
							  MAX_NUMBER_OF_CALLS * 2, IPPROTO_UDP);
	if (free_video_port <= 1024)
		free_video_port = _antisipc.port_range_min + (k * 2) + MAX_NUMBER_OF_CALLS * 2;	/* ERROR: No port seems available on this host?? */
#endif
	/* Find First non-used socket */
	free_text_port =
		eXosip_find_free_port(_antisipc.port_range_min + (k * 2) +
							  MAX_NUMBER_OF_CALLS * 3, IPPROTO_UDP);
	if (free_text_port <= 1024)
		free_text_port = _antisipc.port_range_min + (k * 2) + MAX_NUMBER_OF_CALLS * 3;	/* ERROR: No port seems available on this host?? */
	free_udpftp_port =
		eXosip_find_free_port(_antisipc.port_range_min + (k * 2) +
							  MAX_NUMBER_OF_CALLS * 4, IPPROTO_UDP);
	if (free_udpftp_port <= 1024)
		free_udpftp_port = _antisipc.port_range_min + (k * 2) + MAX_NUMBER_OF_CALLS * 4;	/* ERROR: No port seems available on this host?? */

	stun_candidates =
		&_antisipc.calls[k].audio_candidate.stun_candidates[0];
	local_candidates =
		&_antisipc.calls[k].audio_candidate.local_candidates[0];
	relay_candidates =
		&_antisipc.calls[k].audio_candidate.relay_candidates[0];
	am_network_add_local_candidates(local_candidates, free_audio_port);

	/* build new connection */
	if (_antisipc.use_turn_server != 0 && _antisipc.stun_server[0] != '\0') {
		int i;

		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_audio_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_audio_port;
				}
			}
		}

		if (stun_candidates[0].rel_port == 0) {
			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_audio_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_audio_port;
			}
		}
	} else if (_antisipc.stuntest.use_stun_server != 0
			   && _antisipc.stun_server[0] != '\0') {
		if (stun_candidates[0].rel_port == 0) {
			int i;

			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_audio_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_audio_port;
			}
		}
	} else {
		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_audio_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_audio_port;
				}
			}
		}

		stun_candidates[0].rel_port = free_audio_port;
	}


#ifdef ENABLE_VIDEO
	stun_candidates =
		&_antisipc.calls[k].video_candidate.stun_candidates[0];
	local_candidates =
		&_antisipc.calls[k].video_candidate.local_candidates[0];
	relay_candidates =
		&_antisipc.calls[k].video_candidate.relay_candidates[0];
	am_network_add_local_candidates(local_candidates, free_video_port);

	/* build new connection */
	if (_antisipc.use_turn_server != 0 && _antisipc.stun_server[0] != '\0') {
		int i;

		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_video_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_video_port;
				}
			}
		}

		if (stun_candidates[0].rel_port == 0) {
			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_video_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_video_port;
			}
		}
	} else if (_antisipc.stuntest.use_stun_server != 0
			   && _antisipc.stun_server[0] != '\0') {
		if (stun_candidates[0].rel_port == 0) {
			int i;

			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_video_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_video_port;
			}
		}
	} else {
		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_video_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_video_port;
				}
			}
		}

		stun_candidates[0].rel_port = free_video_port;
	}
#endif

	stun_candidates =
		&_antisipc.calls[k].text_candidate.stun_candidates[0];
	local_candidates =
		&_antisipc.calls[k].text_candidate.local_candidates[0];
	relay_candidates =
		&_antisipc.calls[k].text_candidate.relay_candidates[0];
	am_network_add_local_candidates(local_candidates, free_text_port);

	/* build new connection */
	if (_antisipc.use_turn_server != 0 && _antisipc.stun_server[0] != '\0') {
		int i;

		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_text_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_text_port;
				}
			}
		}

		if (stun_candidates[0].rel_port == 0) {
			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_text_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_text_port;
			}
		}
	} else if (_antisipc.stuntest.use_stun_server != 0
			   && _antisipc.stun_server[0] != '\0') {
		if (stun_candidates[0].rel_port == 0) {
			int i;

			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_text_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_text_port;
			}
		}
	} else {
		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_text_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_text_port;
				}
			}
		}

		stun_candidates[0].rel_port = free_text_port;
	}

	stun_candidates =
		&_antisipc.calls[k].udpftp_candidate.stun_candidates[0];
	local_candidates =
		&_antisipc.calls[k].udpftp_candidate.local_candidates[0];
	relay_candidates =
		&_antisipc.calls[k].udpftp_candidate.relay_candidates[0];
	am_network_add_local_candidates(local_candidates, free_udpftp_port);

	/* build new connection */
	if (_antisipc.use_turn_server != 0 && _antisipc.stun_server[0] != '\0') {
		int i;

		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_udpftp_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_udpftp_port;
				}
			}
		}

		if (stun_candidates[0].rel_port == 0) {
			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_udpftp_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_udpftp_port;
			}
		}
	} else if (_antisipc.stuntest.use_stun_server != 0
			   && _antisipc.stun_server[0] != '\0') {
		if (stun_candidates[0].rel_port == 0) {
			int i;

			i = am_network_add_stun_candidates(stun_candidates,
											   local_candidates,
											   _antisipc.stun_server,
											   free_udpftp_port);
			if (i <= 0) {
				stun_candidates[0].rel_port = free_udpftp_port;
			}
		}
	} else {
		if (_antisipc.use_relay_server != 0) {
			if (relay_candidates[0].rel_port == 0) {
				int i;

				i = am_network_add_relay_candidates(relay_candidates,
													_antisipc.relay_server,
													free_udpftp_port);
				if (i <= 0) {
					relay_candidates[0].rel_port = free_udpftp_port;
				}
			}
		}

		stun_candidates[0].rel_port = free_udpftp_port;
	}

	return ca;
}

int
_am_calls_get_remote_user_agent(am_call_t * ca, osip_message_t * message)
{
	osip_header_t *header = NULL;
	int i;

	if (message == NULL)
		return AMSIP_BADPARAMETER;

	i = osip_message_get_user_agent(message, 0, &header);
	if (i < 0 || header == NULL || header->hvalue == NULL) {
		ca->remote_useragent[0] = '\0';
	} else
		snprintf(ca->remote_useragent, sizeof(ca->remote_useragent), "%s",
				 header->hvalue);
	return AMSIP_SUCCESS;
}

int
_am_calls_get_p_am_sessiontype(am_call_t * ca, osip_message_t * message)
{
	osip_header_t *header = NULL;
	int i;

	if (message == NULL)
		return AMSIP_BADPARAMETER;
	i = osip_message_header_get_byname(message, "P-AM-ST", 0, &header);
	if (i >= 0 && header != NULL && header->hvalue != NULL) {
		snprintf(ca->p_am_sessiontype, sizeof(ca->p_am_sessiontype), "%s",
				 header->hvalue);
	}
	return AMSIP_SUCCESS;
}

int
_am_calls_get_remote_candidate(am_call_t * ca, osip_message_t * message,
							   char *media)
{
	int pos;
	int pos2;
	int pos_remote_candidate;
	sdp_message_t *remote_sdp;
	struct SdpCandidate rem_cand_table[10];
	struct SdpCandidate *rem_cand;
	struct SdpCandidate *loc_cand;
	struct SdpCandidate *stun_candidates;
	struct IceCheckList *ice_checklist;
	struct CandidatePair *ordered_candidates;
	struct am_candidates *tmp = NULL;
	int found_media = 0;

	/* extract candidate from the SIP message */
	if (message == NULL)
		return AMSIP_BADPARAMETER;
	rem_cand = rem_cand_table;

	if (osip_strcasecmp(media, "audio") == 0)
		tmp = &ca->audio_candidate;
#ifdef ENABLE_VIDEO
	else if (osip_strcasecmp(media, "video") == 0)
		tmp = &ca->video_candidate;
#endif
	else if (osip_strcasecmp(media, "text") == 0)
		tmp = &ca->text_candidate;
	else if (osip_strcasecmp(media, "x-udpftp") == 0)
		tmp = &ca->udpftp_candidate;
	else
		return AMSIP_BADPARAMETER;

	loc_cand = tmp->local_candidates;
	stun_candidates = tmp->stun_candidates;

	if (osip_strcasecmp(media, "audio") == 0)
		ice_checklist = &ca->audio_checklist;
#ifdef ENABLE_VIDEO
	else if (osip_strcasecmp(media, "video") == 0)
		ice_checklist = &ca->video_checklist;
#endif
	else if (osip_strcasecmp(media, "text") == 0)
		ice_checklist = &ca->text_checklist;
	else if (osip_strcasecmp(media, "x-udpftp") == 0)
		ice_checklist = &ca->udpftp_checklist;

	ordered_candidates = ice_checklist->cand_pairs;

	if (loc_cand[0].foundation <= 0) {
		ca = _am_calls_find_audio_connection(ca->tid, ca->did);
	}

	remote_sdp = eXosip_get_sdp_info(message);

	if (remote_sdp == NULL)
		return AMSIP_SYNTAXERROR;

	memset(rem_cand, 0, sizeof(struct SdpCandidate) * 10);
	memset(ordered_candidates, 0, sizeof(struct CandidatePair) * 10);

	pos_remote_candidate = 0;
	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, media)) {
			found_media = 1;
			pos2 = 0;
			while (!osip_list_eol(&med->a_attributes, pos2)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&med->a_attributes,
													  pos2);

				if (attr->a_att_field != NULL &&
					osip_strcasecmp(attr->a_att_field, "candidate") == 0) {
					if (osip_strcasestr(attr->a_att_value, "raddr") == NULL)
						sscanf(attr->a_att_value,
							   "%i %i %s %i %s %i typ %s",
							   &rem_cand[pos_remote_candidate].foundation,
							   &rem_cand[pos_remote_candidate].
							   component_id,
							   rem_cand[pos_remote_candidate].transport,
							   &rem_cand[pos_remote_candidate].priority,
							   rem_cand[pos_remote_candidate].conn_addr,
							   &rem_cand[pos_remote_candidate].conn_port,
							   rem_cand[pos_remote_candidate].cand_type);
					else
						sscanf(attr->a_att_value,
							   "%i %i %s %i %s %i typ %s raddr %s rport %i",
							   &rem_cand[pos_remote_candidate].foundation,
							   &rem_cand[pos_remote_candidate].
							   component_id,
							   rem_cand[pos_remote_candidate].transport,
							   &rem_cand[pos_remote_candidate].priority,
							   rem_cand[pos_remote_candidate].conn_addr,
							   &rem_cand[pos_remote_candidate].conn_port,
							   rem_cand[pos_remote_candidate].cand_type,
							   rem_cand[pos_remote_candidate].rel_addr,
							   &rem_cand[pos_remote_candidate].rel_port);

					if (rem_cand[pos_remote_candidate].component_id == 1) {	/* RTP is component 1 */
					} else if (rem_cand[pos_remote_candidate].component_id == 2) {	/* RTP is component 2 */
					}
					pos_remote_candidate++;
				}

				pos2++;
			}
			break;
		}
		pos++;
	}

	if (found_media == 0) {
		sdp_message_free(remote_sdp);
		return AMSIP_SUCCESS;
	}
#if 0
	if (rem_cand[0].conn_addr[0] == '\0') {
		/* no candidate... try private ip address if differente from connection address */
		char ip_conn[256];
		int port;
		int i;

		memset(ip_conn, 0, sizeof(ip_conn));
		i = _calls_get_ip_port(remote_sdp, media, ip_conn, &port);
		if (i == 0 && ip_conn[0] != '\0') {
			if (remote_sdp->o_addr != NULL
				&& osip_strcasecmp(remote_sdp->o_addr, ip_conn) != 0
				&& isdigit(remote_sdp->o_addr[0])) {
				/* try this one */
				rem_cand[pos_remote_candidate].foundation = 1;
				snprintf(rem_cand[pos_remote_candidate].candidate_id,
						 sizeof(rem_cand
								[pos_remote_candidate].candidate_id),
						 "antisipprivateip");
				snprintf(rem_cand[pos_remote_candidate].password,
						 sizeof(rem_cand[pos_remote_candidate].password),
						 "pas%s",
						 rem_cand[pos_remote_candidate].candidate_id);
				snprintf(rem_cand[pos_remote_candidate].ipaddr,
						 sizeof(rem_cand[pos_remote_candidate].ipaddr),
						 "%s", remote_sdp->o_addr);
				rem_cand[pos_remote_candidate].rel_port = port;
			}
		}
	}
#endif

	{
		int pos_candidate_pair = 0;

		for (pos = 0; pos < 10 && loc_cand[pos].conn_addr != '\0' &&
			 loc_cand[pos].foundation > 0; pos++) {
			if (loc_cand[pos].component_id == 1) {
				for (pos2 = 0;
					 pos2 < 10 && rem_cand[pos2].conn_addr[0] != '\0'
					 && rem_cand[pos2].foundation > 0; pos2++) {
					if (rem_cand[pos2].component_id == 1) {
						/* next candidate found! copy it */
						/* controller agent */
						uint64_t G = loc_cand[pos].priority;
						/* controlled agent */
						uint64_t D = rem_cand[pos2].priority;
						memcpy(&ordered_candidates
							   [pos_candidate_pair].local_candidate,
							   &loc_cand[pos], sizeof(loc_cand[pos]));
						memcpy(&ordered_candidates
							   [pos_candidate_pair].remote_candidate,
							   &rem_cand[pos2], sizeof(rem_cand[pos2]));
						ordered_candidates[pos_candidate_pair].
							pair_priority =
							(MIN(G, D)) << 32 | (MAX(G, D)) << 1 | (G >
																	D ? 1 :
																	0);
						pos_candidate_pair++;
					}
				}
			}
		}

		for (pos = 0; pos < 10 && stun_candidates[pos].conn_addr != '\0' &&
			 stun_candidates[pos].foundation > 0; pos++) {
			if (stun_candidates[pos].component_id == 1) {
				for (pos2 = 0;
					 pos2 < 10 && rem_cand[pos2].conn_addr[0] != '\0'
					 && rem_cand[pos2].foundation > 0; pos2++) {
					if (rem_cand[pos2].component_id == 1) {
						/* next candidate found! copy it */
						/* controller agent */
						uint64_t G = stun_candidates[pos].priority;
						/* controlled agent */
						uint64_t D = rem_cand[pos2].priority;
						/* next candidate found! copy it */
						memcpy(&ordered_candidates
							   [pos_candidate_pair].local_candidate,
							   &stun_candidates[pos],
							   sizeof(stun_candidates[pos]));
						memcpy(&ordered_candidates
							   [pos_candidate_pair].remote_candidate,
							   &rem_cand[pos2], sizeof(rem_cand[pos2]));
						ordered_candidates[pos_candidate_pair].
							pair_priority =
							(MIN(G, D)) << 32 | (MAX(G, D)) << 1 | (G >
																	D ? 1 :
																	0);
						pos_candidate_pair++;
					}
				}
			}
		}
	}

	{
		struct CandidatePair tmp_candidates[10];
		long long max_priority;
		int max_index;
		memset(&tmp_candidates, 0, sizeof(struct CandidatePair) * 10);

		/* reordering: copy from highest to lowest */

		pos2 = 0;
		for (pos2 = 0; pos2 < 10; pos2++) {
			max_priority = -1;
			max_index = -1;
			pos = 0;
			for (pos = 0;
				 pos < 10
				 && ordered_candidates[pos].local_candidate.foundation > 0;
				 pos++) {
				if (max_priority < ordered_candidates[pos].pair_priority) {
					max_priority = ordered_candidates[pos].pair_priority;
					max_index = pos;
				}
			}
			if (max_index > -1) {
				memcpy(&tmp_candidates[pos2],
					   &ordered_candidates[max_index],
					   sizeof(struct CandidatePair));
				ordered_candidates[max_index].pair_priority = -1;	/* used */
			} else
				break;
		}

		memcpy(ordered_candidates,
			   tmp_candidates, sizeof(struct CandidatePair) * 10);
	}

	/* take default ice-ufrag & ice-pwd attributes */
	pos2 = 0;
	while (!osip_list_eol(&remote_sdp->a_attributes, pos2)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&remote_sdp->a_attributes,
											  pos2);
		if (attr->a_att_field != NULL
			&& osip_strcasecmp(attr->a_att_field, "ice-ufrag") == 0
			&& attr->a_att_value != NULL) {
			/* fill the 1st entry only */
			snprintf(ice_checklist->rem_ice_ufrag,
					 sizeof(ice_checklist->rem_ice_ufrag),
					 "%s", attr->a_att_value);
		}
		if (attr->a_att_field != NULL
			&& osip_strcasecmp(attr->a_att_field, "ice-pwd") == 0
			&& attr->a_att_value != NULL) {
			/* fill the 1st entry only */
			snprintf(ice_checklist->rem_ice_pwd,
					 sizeof(ice_checklist->rem_ice_pwd),
					 "%s", attr->a_att_value);
		}
		pos2++;
	}

	/* take media-level ice-ufrag & ice-pwd attributes */
	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, media)) {
			pos2 = 0;
			while (!osip_list_eol(&med->a_attributes, pos2)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&med->a_attributes,
													  pos2);

				if (attr->a_att_field != NULL
					&& osip_strcasecmp(attr->a_att_field, "ice-ufrag") == 0
					&& attr->a_att_value != NULL) {
					snprintf(ice_checklist->rem_ice_ufrag,
							 sizeof(ice_checklist->rem_ice_ufrag),
							 "%s", attr->a_att_value);
				} else if (attr->a_att_field != NULL
						   && osip_strcasecmp(attr->a_att_field,
											  "ice-pwd") == 0
						   && attr->a_att_value != NULL) {
					snprintf(ice_checklist->rem_ice_pwd,
							 sizeof(ice_checklist->rem_ice_pwd), "%s",
							 attr->a_att_value);
				}

				pos2++;
			}
			break;
		}
		pos++;
	}

	snprintf(ice_checklist->loc_ice_ufrag,
			 sizeof(ice_checklist->loc_ice_ufrag), "%s", ca->ice_ufrag);
	snprintf(ice_checklist->loc_ice_pwd,
			 sizeof(ice_checklist->loc_ice_pwd), "%s", ca->ice_pwd);

	for (pos = 0;
		 pos < 10
		 && ordered_candidates[pos].local_candidate.foundation > 0;
		 pos++) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"ordered candidate pair (pprio=%lli) (prio=%i) %s:%i -> (prio=%i) %s:%i\n",
					ordered_candidates[pos].pair_priority,
					ordered_candidates[pos].local_candidate.priority,
					ordered_candidates[pos].local_candidate.conn_addr,
					ordered_candidates[pos].local_candidate.conn_port,
					ordered_candidates[pos].remote_candidate.priority,
					ordered_candidates[pos].remote_candidate.conn_addr,
					ordered_candidates[pos].remote_candidate.conn_port));
	}
	sdp_message_free(remote_sdp);
	return AMSIP_SUCCESS;
}


int _am_calls_get_conf_name(am_call_t * ca, osip_message_t * message)
{
	osip_contact_t *co;
	osip_generic_param_t *isfocus;
	osip_uri_param_t *grid;
	int i;

	if (message == NULL)
		return AMSIP_BADPARAMETER;

	i = osip_message_get_contact(message, 0, &co);
	if (i < 0 || co == NULL || co->url == NULL)
		return AMSIP_SUCCESS;

	i = osip_contact_param_get_byname(co, "isfocus", &isfocus);
	if (i != 0) {
		ca->conf_name[0] = '\0';
		return AMSIP_SUCCESS;
	}

	i = osip_uri_uparam_get_byname(co->url, "grid", &grid);
	if (i == 0 && grid != NULL && grid->gvalue != NULL)
		snprintf(ca->conf_name, sizeof(ca->conf_name), "%s", grid->gvalue);
	else if (co->url->username != NULL)
		snprintf(ca->conf_name, sizeof(ca->conf_name), "%s", co->url->username);
	else {
		ca->conf_name[0] = '\0';
		return AMSIP_SYNTAXERROR;
	}

	return AMSIP_SUCCESS;
}

int
_am_calls_add_audio_for_conference(am_call_t * orig, char *pcm_data,
								   int pcm_size)
{
	int k;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state == NOT_USED
			|| orig->conf_name[0] == '\0'
			|| _antisipc.calls[k].conf_name[0] == '\0'
			|| osip_strcasecmp(orig->conf_name,
							   _antisipc.calls[k].conf_name) != 0) {
			/* skip */
			continue;
		}
		if (orig->cid != _antisipc.calls[k].cid) {
#if 0
			OSIP_TRACE(osip_trace
					   (__FILE__, __LINE__, OSIP_ERROR, NULL,
						"Found call for this conference\n",
						orig->conf_name));
#endif
			if (_antisipc.calls[k].enable_audio > 0) {
			}
		}
	}
	return AMSIP_SUCCESS;
}
