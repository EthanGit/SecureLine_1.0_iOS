/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/


#if !defined(WIN32) && !defined(_WIN32_WCE)
#include <sys/socket.h>
#endif
#include <osip2/osip_mt.h>
#include "sdptools.h"
#include "am_calls.h"
#include "amsip/am_network.h"

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

#define SDP_BUF_SIZE 4096
#define RTPMAPS_BUF_SIZE 2048
#define PAYLOADS_BUF_SIZE 128

static int strcat_fmtp_option(int buf_size, char *buf, sdp_media_t * remote_med,
							  int payload);
static int strcat_ptime_option(int buf_size, char *buf, sdp_media_t * remote_med);
static char *_am_strcat(int buf_size, char *buf, char *data);

static int _sdp_remove_audio_codec(sdp_message_t *message);
static int _sdp_keep_audio_codec(sdp_media_t *audio_med, char *name, int freq_rate);

static char *_am_strcat(int buf_size, char *buf, char *data)
{
	int len_size=(int)strlen(buf);
	int data_size=(int)strlen(data);
	if (buf_size-len_size<=data_size)
		return NULL;
	return strcat(buf, data);
}

int
rtpmap_sscanf(char *rtpmap_value, char *payload, char *subtype, char *freq)
{
	char *p1;
	char *p2;

	p1 = strchr(rtpmap_value, ' ');
	if (p1 == NULL || p1 - rtpmap_value > 5)
		return 0;
	p2 = strchr(p1, '/');
	if (p2 == NULL || p2 - p1 > 63)
		return 0;
	if (strlen(p2 + 1) > 63)
		return 0;
	osip_strncpy(payload, rtpmap_value, p1 - rtpmap_value);
	osip_strncpy(subtype, p1 + 1, p2 - p1 - 1);
	osip_strncpy(freq, p2 + 1, strlen(p2));
	return 3;
}

static bool_t __fmtp_get_value(const char *fmtp, const char *param_name, char *result, size_t result_len){
	const char *pos=osip_strcasestr(fmtp,param_name);
	memset(result, '\0', result_len);
	if (pos){
		const char *equal=strchr(pos,'=');
		if (equal){
			int copied;
			const char *end=strchr(equal+1,';');
			if (end==NULL) end=fmtp+strlen(fmtp); /*assuming this is the last param */
			copied=MIN((int)(result_len-1),(int)(end-(equal+1)));
			strncpy(result,equal+1,copied);
			result[copied]='\0';
			return TRUE;
		}
	}
	return FALSE;
}

static int
strcat_fmtp_option_ipmrv25(int buf_size, char *buf, sdp_media_t * remote_med, int payload)
{
	int r;

	if (remote_med == NULL)
		return AMSIP_BADPARAMETER;

	r = _am_codec_get_definition("ip-mr_v2.5");
	if (r>=0 && (_antisipc.codecs[r].vbr==0xF000))
		_am_strcat(buf_size, buf, "a=ars\r\n");
	return AMSIP_SUCCESS;
}

#ifdef ENABLE_VIDEO

static int
strcat_fmtp_option_h264(int buf_size, char *buf, sdp_media_t * remote_med, int payload)
{
	int pos2 = 0;
	char tmp_codec[10];
	char packetization_mode[2];
	char profile_level_id[7];

	memset(packetization_mode, 0, sizeof(packetization_mode));
	memset(profile_level_id, 0, sizeof(profile_level_id));

	if (remote_med == NULL)
		return AMSIP_BADPARAMETER;

	snprintf(tmp_codec, 10, "%i ", payload);

	while (!osip_list_eol(&remote_med->a_attributes, pos2)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&remote_med->a_attributes,
											  pos2);
		if (attr->a_att_field != NULL && attr->a_att_value != NULL
			&& osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
			if (osip_strncasecmp
				(attr->a_att_value, tmp_codec, strlen(tmp_codec))
				== 0) {
					char *tmp;
					tmp = osip_strcasestr(attr->a_att_value, "packetization-mode");
					if (tmp!=NULL)
					{
						__fmtp_get_value(attr->a_att_value, "packetization-mode", packetization_mode, sizeof(packetization_mode));
					}
					tmp = osip_strcasestr(attr->a_att_value, "profile-level-id");
					if (tmp!=NULL)
					{
						__fmtp_get_value(attr->a_att_value, "profile-level-id", profile_level_id, sizeof(profile_level_id));
					}
			}
		}
		pos2++;
	}

	_am_strcat(buf_size, buf, "a=fmtp:");
	_am_strcat(buf_size, buf, tmp_codec);

	_am_strcat(buf_size, buf, "profile-level-id=");
	if (profile_level_id[0]!='\0')
		_am_strcat(buf_size, buf, profile_level_id);
	else
		_am_strcat(buf_size, buf, "42800c"); //I would prefer: "42e014"); 14 -> level 2.1

	if (packetization_mode[0]=='1')
		_am_strcat(buf_size, buf, "; packetization-mode=1");
	else
		_am_strcat(buf_size, buf, "; packetization-mode=0");

	_am_strcat(buf_size, buf, "\r\n");
	
	{
		int r = _am_video_codec_get_definition("H264");
		if (r>=0 && _antisipc.video_codecs[r].vbr==0xF000)
			_am_strcat(buf_size, buf, "a=ars\r\n");
	}

	return AMSIP_SUCCESS;
}

#endif

static int
strcat_fmtp_option(int buf_size, char *buf, sdp_media_t * remote_med, int payload)
{
	int pos2 = 0;
	char tmp_codec[10];

	if (remote_med == NULL)
		return AMSIP_BADPARAMETER;

	snprintf(tmp_codec, 10, "%i ", payload);

	while (!osip_list_eol(&remote_med->a_attributes, pos2)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&remote_med->a_attributes,
											  pos2);
		if (attr->a_att_field != NULL && attr->a_att_value != NULL
			&& osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
			if (osip_strncasecmp
				(attr->a_att_value, tmp_codec, strlen(tmp_codec))
				== 0) {
				_am_strcat(buf_size, buf, "a=");
				_am_strcat(buf_size, buf, attr->a_att_field);
				_am_strcat(buf_size, buf, ":");
				_am_strcat(buf_size, buf, attr->a_att_value);
				_am_strcat(buf_size, buf, "\r\n");
			}
		}
		pos2++;
	}
	return AMSIP_SUCCESS;
}

static int amr_option_are_supported(am_codec_info_t *codec,
									sdp_media_t * remote_med,
									int payload)
{
	int pos2 = 0;
	char tmp_codec[10];
	char buf[128];
	int octet_align=0;
	int interleaving=0;
	//int crc=0;
	//int robust_sorting=0;
	//int channels=0;
	
	if (remote_med == NULL)
		return AMSIP_BADPARAMETER;

	snprintf(tmp_codec, 10, "%i ", payload);

	/* accept only if same:
	octet-align mode
	interleaving mode
	crc=0
	robust-sorting=0
	channels=1
	*/

	while (!osip_list_eol(&remote_med->a_attributes, pos2)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&remote_med->a_attributes,
											  pos2);
		if (attr->a_att_field != NULL && attr->a_att_value != NULL
			&& osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
			if (osip_strncasecmp
				(attr->a_att_value, tmp_codec, strlen(tmp_codec))
				== 0) {
					__fmtp_get_value(attr->a_att_value, "octet-align", buf, sizeof(buf));
					if (buf[0]!='\0'){
						octet_align=atoi(buf);
					}
					__fmtp_get_value(attr->a_att_value, "interleaving", buf, sizeof(buf));
					if (buf[0]!='\0'){
						interleaving = atoi(buf);
					}
					__fmtp_get_value(attr->a_att_value, "robust-sorting", buf, sizeof(buf));
					if (buf[0]!='\0'){
						if (atoi(buf)!=0)
							return -1;
					}
					__fmtp_get_value(attr->a_att_value, "crc", buf, sizeof(buf));
					if (buf[0]!='\0'){
						if (atoi(buf)!=0)
							return -1;
					}
					__fmtp_get_value(attr->a_att_value, "channels", buf, sizeof(buf));
					if (buf[0]!='\0'){
						if (atoi(buf)!=1)
							return -1;
					}
			}
		}
		pos2++;
	}

	/* 0x01 -> no support for receving octet-align */
	/* 0x02 -> no support for receving BandEfficient */
	/* 0x04 -> no support for receving interleaving */
	/* 0x10 -> send BandEfficient */
	/* 0x20 -> send interleaving */

	if ((codec->mode&0x07) == 0x00)
		return 0; /* all parameter supported */
	if ((codec->mode&0x01) == 0x01 && octet_align==1)
		return -1; /* SDP is octet-align but not supported */
	if ((codec->mode&0x02) == 0x02 && octet_align==0)
		return -1; /* SDP is BandEfficient but not supported */
	if ((codec->mode&0x04) == 0x04 && interleaving==1)
		return -1; /* SDP is interleaving but not supported */

	return 0;
}

static int strcat_ptime_option(int buf_size, char *buf, sdp_media_t * remote_med)
{
	int pos2 = 0;

	if (remote_med == NULL)
		return AMSIP_BADPARAMETER;

	while (!osip_list_eol(&remote_med->a_attributes, pos2)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&remote_med->a_attributes,
											  pos2);
		if (attr->a_att_field != NULL && attr->a_att_value != NULL
			&& osip_strcasecmp(attr->a_att_field, "ptime") == 0) {
			_am_strcat(buf_size, buf, "a=");
			_am_strcat(buf_size, buf, attr->a_att_field);
			_am_strcat(buf_size, buf, ":");
			_am_strcat(buf_size, buf, attr->a_att_value);
			_am_strcat(buf_size, buf, "\r\n");
			return AMSIP_SUCCESS;
		}
		pos2++;
	}
	return AMSIP_UNDEFINED_ERROR;
}

#if 0
static int strcat_bandwidth_attr(int buf_size, char *buf, sdp_media_t * remote_med)
{
	int pos2 = 0;

	if (remote_med == NULL)
		return AMSIP_BADPARAMETER;

	while (!osip_list_eol(&remote_med->b_bandwidths, pos2)) {
		sdp_bandwidth_t *bandwidth;

		bandwidth =
			(sdp_bandwidth_t *) osip_list_get(&remote_med->b_bandwidths,
											  pos2);
		if (bandwidth->b_bwtype != NULL && bandwidth->b_bandwidth != NULL
			&& osip_strcasecmp(bandwidth->b_bwtype, "AS") == 0) {
			_am_strcat(buf_size, buf, "b=");
			_am_strcat(buf_size, buf, bandwidth->b_bwtype);
			_am_strcat(buf_size, buf, ":");
			if (_antisipc.video_codec_attr.download_bandwidth <= 0
				|| atoi(bandwidth->b_bandwidth) <=
				_antisipc.video_codec_attr.download_bandwidth)
				_am_strcat(buf_size, buf, bandwidth->b_bandwidth);
			else {
				char tmp_b[256];

				memset(tmp_b, 0, sizeof(tmp_b));
				snprintf(tmp_b, sizeof(tmp_b), "%i",
						 _antisipc.video_codec_attr.download_bandwidth);
				_am_strcat(buf_size, buf, tmp_b);
			}
			_am_strcat(buf_size, buf, "\r\n");
			return AMSIP_SUCCESS;
		}
		pos2++;
	}
	return AMSIP_UNDEFINED_ERROR;
}
#endif

static int strcat_global_attr(int buf_size, char *buf, sdp_message_t * sdp)
{
	int pos;

	if (sdp == NULL)
		return AMSIP_BADPARAMETER;

	/* test global attributes */
	pos = 0;
	while (!osip_list_eol(&sdp->a_attributes, pos)) {
		sdp_attribute_t *at;

		at = (sdp_attribute_t *) osip_list_get(&sdp->a_attributes, pos);
		if (at->a_att_field != NULL
			&& 0 == strcmp(at->a_att_field, "sendonly")) {
			_am_strcat(buf_size, buf, "a=recvonly\r\n");
			return AMSIP_SUCCESS;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "recvonly")) {
			_am_strcat(buf_size, buf, "a=sendonly\r\n");
			return AMSIP_SUCCESS;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "sendrecv")) {
			_am_strcat(buf_size, buf, "a=sendrecv\r\n");
			return AMSIP_SUCCESS;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "inactive")) {
			_am_strcat(buf_size, buf, "a=inactive\r\n");
			return AMSIP_SUCCESS;
		}
		pos++;
	}

	return AMSIP_SUCCESS;
}

static int strcat_media_attr(int buf_size, char *buf, sdp_media_t * med)
{
	int found_crypto = 0;
	int pos;

	if (med == NULL)
		return AMSIP_BADPARAMETER;

	/* test media attributes */
	pos = 0;
	while (!osip_list_eol(&med->a_attributes, pos)) {
		sdp_attribute_t *at;

		at = (sdp_attribute_t *) osip_list_get(&med->a_attributes, pos);
		if (at->a_att_field != NULL
			&& 0 == strcmp(at->a_att_field, "sendonly")) {
			_am_strcat(buf_size, buf, "a=recvonly\r\n");
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "recvonly")) {
			_am_strcat(buf_size, buf, "a=sendonly\r\n");
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "sendrecv")) {
			_am_strcat(buf_size, buf, "a=sendrecv\r\n");
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "inactive")) {
			_am_strcat(buf_size, buf, "a=inactive\r\n");
		} else if (at->a_att_field != NULL && at->a_att_value != NULL
				   && 0 == strcmp(at->a_att_field, "crypto")
				   && found_crypto == 0) {
			char *end_tag = strstr(at->a_att_value, " ");
			if (end_tag==NULL || end_tag-at->a_att_value>9)
			{
				/* skip non compliant */
			}
			else if (osip_strcasestr(at->a_att_value, "AES_CM_128_HMAC_SHA1_80") != NULL
				|| osip_strcasestr(at->a_att_value, "AES_CM_128_HMAC_SHA1_32") != NULL
				|| osip_strcasestr(at->a_att_value, "AES_CM_128_NULL_AUTH") != NULL
				|| osip_strcasestr(at->a_att_value, "NULL_CIPHER_HMAC_SHA1_80") != NULL
				//|| osip_strcasestr(at->a_att_value, "AES_CM_192_HMAC_SHA1_80") != NULL
				//|| osip_strcasestr(at->a_att_value, "AES_CM_192_HMAC_SHA1_32") != NULL
				|| osip_strcasestr(at->a_att_value, "AES_CM_256_HMAC_SHA1_80") != NULL
				|| osip_strcasestr(at->a_att_value, "AES_CM_256_HMAC_SHA1_32") != NULL
				) {
				found_crypto = 1;

				if (0 == osip_strcasecmp(med->m_proto, "RTP/SAVP")
					|| _antisipc.optionnal_encryption != 0) {
						char b64_text[41];
						char key[31];
						char tag[10];
						_am_strcat(buf_size, buf, "a=crypto:");
						memset(tag, 0, sizeof(tag));
						osip_strncpy(tag, at->a_att_value, end_tag-at->a_att_value);
						_am_strcat(buf_size, buf, tag);
						if (osip_strcasestr(at->a_att_value, "AES_CM_128_HMAC_SHA1_80") != NULL)
							_am_strcat(buf_size, buf," AES_CM_128_HMAC_SHA1_80 inline:");
						else if (osip_strcasestr(at->a_att_value, "AES_CM_128_HMAC_SHA1_32") != NULL)
							_am_strcat(buf_size, buf," AES_CM_128_HMAC_SHA1_32 inline:");
						else if (osip_strcasestr(at->a_att_value, "AES_CM_128_NULL_AUTH") != NULL)
							_am_strcat(buf_size, buf," AES_CM_128_NULL_AUTH inline:");
						else if (osip_strcasestr(at->a_att_value, "NULL_CIPHER_HMAC_SHA1_80") != NULL)
							_am_strcat(buf_size, buf," NULL_CIPHER_HMAC_SHA1_80 inline:");
						//else if (osip_strcasestr(at->a_att_value, "AES_CM_192_HMAC_SHA1_80") != NULL)
						//	_am_strcat(buf_size, buf," AES_CM_192_HMAC_SHA1_80 inline:");
						//else if (osip_strcasestr(at->a_att_value, "AES_CM_192_HMAC_SHA1_32") != NULL)
						//	_am_strcat(buf_size, buf," AES_CM_192_HMAC_SHA1_32 inline:");
						else if (osip_strcasestr(at->a_att_value, "AES_CM_256_HMAC_SHA1_80") != NULL)
							_am_strcat(buf_size, buf," AES_CM_256_HMAC_SHA1_80 inline:");
						else if (osip_strcasestr(at->a_att_value, "AES_CM_256_HMAC_SHA1_32") != NULL)
							_am_strcat(buf_size, buf," AES_CM_256_HMAC_SHA1_32 inline:");
						memset(b64_text, 0, sizeof(b64_text));
						memset(key, 0, sizeof(key));
						eXosip_generate_random(key, 16);
						am_hexa_generate_random(key, 31, key, "key", "crypto");
						am_base64_encode(b64_text, key, 30);

						_am_strcat(buf_size, buf, b64_text);
						/* _am_strcat(buf_size, buf, "|2^20"); */
						end_tag = osip_strcasestr(at->a_att_value, "wsh=");
						if (end_tag!=NULL)
						{
							snprintf(key, sizeof(key), " wsh=%i", atoi(end_tag+4));
							_am_strcat(buf_size, buf, key);
						}

						_am_strcat(buf_size, buf, "\r\n");
				}
				if (_antisipc.optionnal_encryption != 0)
					_am_strcat(buf_size, buf, "a=encryption:optional\r\n");
			}
		}


		pos++;
	}


	return AMSIP_SUCCESS;
}

int
sdp_complete_message(am_call_t * ca, sdp_message_t * remote_sdp,
					 osip_message_t * msg)
{
	sdp_media_t *remote_med;
	char buf[SDP_BUF_SIZE];
	int pos;

	char localip[128];
	char setup[64];
	int _port;

#ifdef ENABLE_VIDEO
	int _video_port;
#endif
	int _text_port;
	int _udpftp_port;
	int offer_port = -1;

	char audio_profile[32];
	char video_profile[32];
	char text_profile[32];
	char udpftp_profile[32];

	strncpy(audio_profile, _antisipc.audio_profile, sizeof(audio_profile));
	strncpy(video_profile, _antisipc.video_profile, sizeof(video_profile));
	strncpy(text_profile, _antisipc.text_profile, sizeof(text_profile));
	strncpy(udpftp_profile, _antisipc.udpftp_profile,
			sizeof(udpftp_profile));

	if (remote_sdp == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"No remote SDP body found for call\n"));
		return AMSIP_BADPARAMETER;
	}
	if (msg == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"No message to complete\n"));
		return AMSIP_BADPARAMETER;
	}

	strncpy(setup, "passive", sizeof(setup));
	_port = ca->audio_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_port = ca->audio_candidate.stun_candidates[0].conn_port;
	if (_port < 1024)
		_port = ca->audio_candidate.stun_candidates[0].rel_port;

#ifdef ENABLE_VIDEO
	_video_port = ca->video_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_video_port = ca->video_candidate.stun_candidates[0].conn_port;
	if (_video_port < 1024)
		_video_port = ca->video_candidate.stun_candidates[0].rel_port;
#endif

	_text_port = ca->text_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_text_port = ca->text_candidate.stun_candidates[0].conn_port;
	if (_text_port < 1024)
		_text_port = ca->text_candidate.stun_candidates[0].rel_port;

	_udpftp_port = ca->udpftp_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_udpftp_port = ca->udpftp_candidate.stun_candidates[0].conn_port;
	if (_udpftp_port < 1024)
		_udpftp_port = ca->udpftp_candidate.stun_candidates[0].rel_port;

	if (_antisipc.stuntest.use_setup_both != 0)
		strncpy(setup, "both", sizeof(setup));

	memset(localip, '\0', sizeof(localip));

	/* find out if "0.0.0.0" is used. */
	_calls_get_ip_port(remote_sdp, "audio", localip, &offer_port);
	if (offer_port == 0) {
		snprintf(localip, sizeof(localip), "0.0.0.0");
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"audio stream is disabled (port=0)\n"));
	} else
		eXosip_guess_localip(AF_INET, localip, 128);

	if (_antisipc.stun_firewall[0] != '\0'
		&& _antisipc.stuntest.use_stun_mapped_ip != 0)
		snprintf(buf, SDP_BUF_SIZE,
				 "v=0\r\n" "o=amsip 0 %i IN IP4 %s\r\n" "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip,
				 _antisipc.stun_firewall);
	else
		snprintf(buf, SDP_BUF_SIZE,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip, localip);

	strcat_global_attr(SDP_BUF_SIZE, buf, remote_sdp);

	if (_antisipc.use_turn_server != 0
		&& strstr(ca->remote_useragent,
				  "Audiocodes-Sip-Gateway-Mediant") != NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Gateway-Mediant bug: remove ICE information\n"));
	} else if (_antisipc.use_turn_server != 0) {
		if (ca->ice_pwd[0] != '\0' && ca->ice_ufrag[0] != '\0') {
			_am_strcat(SDP_BUF_SIZE, buf, "a=ice-pwd:");
			_am_strcat(SDP_BUF_SIZE, buf, ca->ice_pwd);
			_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

			_am_strcat(SDP_BUF_SIZE, buf, "a=ice-ufrag:");
			_am_strcat(SDP_BUF_SIZE, buf, ca->ice_ufrag);
			_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
		}
	}

	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		char payloads[PAYLOADS_BUF_SIZE];
		char payloads_rtpmap[RTPMAPS_BUF_SIZE];

		memset(payloads, '\0', sizeof(payloads));
		memset(payloads_rtpmap, '\0', sizeof(payloads_rtpmap));

		remote_med =
			(sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(remote_med->m_media, "audio")
			&& (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
				|| 0 == osip_strcasecmp(remote_med->m_proto,
										_antisipc.audio_profile))) {
			int pos_payload = 0;
			int add_speex_option = 0;
			int add_ilbc_option = 0;
			int add_amr_option = 0;
			int add_amrwb_option = 0;
			int add_ipmrv25_option = 0;
			int add_telephoneevents_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}

						if (strstr(attr->a_att_value, "729a/")!=NULL
							||strstr(attr->a_att_value, "729A/")!=NULL) {
							pos2++;
							continue; /* bypass linksys wrong "g729a" information */
						}

						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype, "PCMU") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " PCMU");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "PCMA") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " PCMA");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "iLBC") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " iLBC");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									if (_antisipc.codec_attr.ptime > 0
										&& _antisipc.codecs[r].mode > 0) {
										if (_antisipc.codec_attr.ptime %
											_antisipc.codecs[r].mode != 0)
											_antisipc.codecs[r].mode = 0;	/* bad ilbc mode */
									}

									if (_antisipc.codecs[r].mode <= 0) {
										_antisipc.codecs[r].mode = 30;
										if (_antisipc.codec_attr.ptime > 0) {
											if (_antisipc.codec_attr.
												ptime % 20 == 0)
												_antisipc.codecs[r].mode =
													20;
											else
												_antisipc.codecs[r].mode =
													30;
										}
									}

									add_ilbc_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "speex") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0
									&& _antisipc.codecs[r].freq ==
									atoi(freq)) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " speex");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									if (_antisipc.codecs[r].mode > 6)
										_antisipc.codecs[r].mode = 6;

									if (_antisipc.codecs[r].mode <= 0)
										_antisipc.codecs[r].mode = 0;

									add_speex_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g729") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g729");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "amr") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {

										if (amr_option_are_supported(&_antisipc.codecs[r],
											remote_med,
											atoi(codec))==0)
										{
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " AMR");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

											add_amr_option = atoi(codec);
										}
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "amr-wb") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
										if (amr_option_are_supported(&_antisipc.codecs[r],
											remote_med,
											atoi(codec))==0)
										{
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " AMR-WB");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

											add_amrwb_option = atoi(codec);
										}
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g723") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g723");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g7221") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g7221");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-40") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-40");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-32") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-32");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-24") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-24");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-16") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-16");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "GSM") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " GSM");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "telephone-event")
									   == 0) {
								find_rtpmap = 1;
								_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
								_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
									   " telephone-event");
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

								add_telephoneevents_option = atoi(codec);
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "ip-mr_v2.5") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {

										_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
										_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " ip-mr_v2.5");
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

										add_ipmrv25_option = atoi(codec);
								}
							} else if (n == 3) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " ");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, subtype);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							}
						}
					}
					pos2++;
				}

				if (find_rtpmap == 0) {
					/* check if payload is supported but rtpmap is missing 0, 8 */
					if (osip_strcasecmp(payload_number, "0") == 0) {
						int r = _am_codec_get_definition("PCMU");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "0 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:0 PCMU/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "8") == 0) {
						int r = _am_codec_get_definition("PCMA");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "8 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:8 PCMA/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "18") == 0) {
						int r = _am_codec_get_definition("g729");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "18 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:18 G729/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "4") == 0) {
						int r = _am_codec_get_definition("g723");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "4 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:4 G723/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "3") == 0) {
						int r = _am_codec_get_definition("GSM");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "3 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:3 GSM/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "9") == 0) {
						int r = _am_codec_get_definition("G722");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "9 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:3 G722/8000\r\n");
						}
					}
				}

				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
				return -1;		/* refuse anyway */
			} else {
				char port[10];

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _port);
				_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				if (_antisipc.codec_attr.bandwidth > 0) {
					char tmp_b[256];

					memset(tmp_b, 0, sizeof(tmp_b));
					snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
							 _antisipc.codec_attr.bandwidth);
					_am_strcat(SDP_BUF_SIZE, buf, tmp_b);
				}

				if (_antisipc.use_turn_server != 0
					&& strstr(ca->remote_useragent,
							  "Audiocodes-Sip-Gateway-Mediant") != NULL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_WARNING, NULL,
								"Gateway-Mediant bug: remove ICE information\n"));
				} else if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates */

					/* add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->audio_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->audio_candidate.
								 local_candidates[_pos].foundation,
								 ca->audio_candidate.
								 local_candidates[_pos].component_id,
								 ca->audio_candidate.
								 local_candidates[_pos].transport,
								 ca->audio_candidate.
								 local_candidates[_pos].priority,
								 ca->audio_candidate.
								 local_candidates[_pos].conn_addr,
								 ca->audio_candidate.
								 local_candidates[_pos].conn_port,
								 ca->audio_candidate.
								 local_candidates[_pos].cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->audio_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->audio_candidate.stun_candidates[_pos].
								 foundation,
								 ca->audio_candidate.stun_candidates[_pos].
								 component_id,
								 ca->audio_candidate.stun_candidates[_pos].
								 transport,
								 ca->audio_candidate.stun_candidates[_pos].
								 priority,
								 ca->audio_candidate.stun_candidates[_pos].
								 conn_addr,
								 ca->audio_candidate.stun_candidates[_pos].
								 conn_port,
								 ca->audio_candidate.stun_candidates[_pos].
								 cand_type,
								 ca->audio_candidate.stun_candidates[_pos].
								 rel_addr,
								 ca->audio_candidate.stun_candidates[_pos].
								 rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				if (add_ilbc_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_ilbc_option);
				}
				if (add_speex_option > 0) {
					/* copy all speex options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_speex_option);
				}
				if (add_amr_option > 0) {
					/* copy all amr options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_amr_option);
				}
				if (add_amrwb_option > 0) {
					/* copy all amr options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_amrwb_option);
				}
				if (add_telephoneevents_option > 0) {
					/* copy all telephone-events options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_telephoneevents_option);
				}
				if (add_ipmrv25_option > 0) {
					/* copy all ipmrv25 options */
					strcat_fmtp_option_ipmrv25(SDP_BUF_SIZE, buf, remote_med,
									   add_ipmrv25_option);
				}

				strcat_ptime_option(SDP_BUF_SIZE, buf, remote_med);

			}
		}
#ifdef ENABLE_VIDEO
		else if (0 == osip_strcasecmp(remote_med->m_media, "video")
				 && (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
					 || 0 == osip_strcasecmp(remote_med->m_proto,
											 _antisipc.video_profile))) {
			int pos_payload = 0;
			int add_h2631998_option = 0;
			int add_h263_option = 0;
			int add_jpeg_option = 0;
			int add_theora_option = 0;
			int add_mp4v_option = 0;
			int add_h264_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}
						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype,
												   "H263-1998") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " H263-1998");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_h2631998_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "H263") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " H263");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_h263_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "JPEG") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " JPEG");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_jpeg_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "H264") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " H264");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_h264_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "theora") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " theora");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_theora_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "MP4V-ES") ==
									   0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " MP4V-ES");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_mp4v_option = atoi(codec);
								}
							} else if (n == 3) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " ");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, subtype);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							}
						}
					}
					pos2++;
				}

				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				//we should add a payload indicated in offer?
				_am_strcat(SDP_BUF_SIZE, buf, "115");
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
			} else {
				char port[10];
				int bandwidth;

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _video_port);
				if (ca->reject_video>0)
					_am_strcat(SDP_BUF_SIZE, buf, "0");
				else
					_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				/* bandwidth = strcat_bandwidth_attr(SDP_BUF_SIZE, buf, remote_med); */
				bandwidth=-1; /* we don't use the lowest setting??) */
				if (bandwidth < 0
					&& _antisipc.video_codec_attr.download_bandwidth > 0) {
					char tmp_b[256];

					memset(tmp_b, 0, sizeof(tmp_b));
					snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
							 _antisipc.video_codec_attr.
							 download_bandwidth);
					_am_strcat(SDP_BUF_SIZE, buf, tmp_b);
				}

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				if (add_h2631998_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_h2631998_option);
				}
				if (add_h263_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_h263_option);
				}
				if (add_jpeg_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_jpeg_option);
				}
				if (add_h264_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option_h264(SDP_BUF_SIZE, buf, remote_med, add_h264_option);
				}
				if (add_theora_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_theora_option);
				}
				if (add_mp4v_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_mp4v_option);
				}

				if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates */

					/* add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->video_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->video_candidate.
								 local_candidates[_pos].foundation,
								 ca->video_candidate.
								 local_candidates[_pos].component_id,
								 ca->video_candidate.
								 local_candidates[_pos].transport,
								 ca->video_candidate.
								 local_candidates[_pos].priority,
								 ca->video_candidate.
								 local_candidates[_pos].conn_addr,
								 ca->video_candidate.
								 local_candidates[_pos].conn_port,
								 ca->video_candidate.
								 local_candidates[_pos].cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->video_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->video_candidate.stun_candidates[_pos].
								 foundation,
								 ca->video_candidate.stun_candidates[_pos].
								 component_id,
								 ca->video_candidate.stun_candidates[_pos].
								 transport,
								 ca->video_candidate.stun_candidates[_pos].
								 priority,
								 ca->video_candidate.stun_candidates[_pos].
								 conn_addr,
								 ca->video_candidate.stun_candidates[_pos].
								 conn_port,
								 ca->video_candidate.stun_candidates[_pos].
								 cand_type,
								 ca->video_candidate.stun_candidates[_pos].
								 rel_addr,
								 ca->video_candidate.stun_candidates[_pos].
								 rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}
			}
		}
#endif
		else if (0 == osip_strcasecmp(remote_med->m_media, "text")
				 && (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
					 || 0 == osip_strcasecmp(remote_med->m_proto,
											 _antisipc.text_profile))) {
			int pos_payload = 0;
			int add_t140_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}
						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype, "t140") == 0) {
								int r =
									_am_text_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.text_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " t140");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_t140_option = atoi(codec);
								}
							}
						}
					}
					pos2++;
				}

				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
				return AMSIP_NOCOMMONCODEC;	/* refuse anyway */
			} else {
				char port[10];

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _text_port);
				_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				if (add_t140_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_t140_option);
				}

				if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates */

					/* add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->text_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->text_candidate.local_candidates[_pos].
								 foundation,
								 ca->text_candidate.local_candidates[_pos].
								 component_id,
								 ca->text_candidate.local_candidates[_pos].
								 transport,
								 ca->text_candidate.local_candidates[_pos].
								 priority,
								 ca->text_candidate.local_candidates[_pos].
								 conn_addr,
								 ca->text_candidate.local_candidates[_pos].
								 conn_port,
								 ca->text_candidate.local_candidates[_pos].
								 cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->text_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->text_candidate.stun_candidates[_pos].
								 foundation,
								 ca->text_candidate.stun_candidates[_pos].
								 component_id,
								 ca->text_candidate.stun_candidates[_pos].
								 transport,
								 ca->text_candidate.stun_candidates[_pos].
								 priority,
								 ca->text_candidate.stun_candidates[_pos].
								 conn_addr,
								 ca->text_candidate.stun_candidates[_pos].
								 conn_port,
								 ca->text_candidate.stun_candidates[_pos].
								 cand_type,
								 ca->text_candidate.stun_candidates[_pos].
								 rel_addr,
								 ca->text_candidate.stun_candidates[_pos].
								 rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}
			}
//xx
		} else if (0 == osip_strcasecmp(remote_med->m_media, "x-udpftp")
				   && (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
					   || 0 == osip_strcasecmp(remote_med->m_proto,
											   _antisipc.udpftp_profile)))
		{
			int pos_payload = 0;
			int add_udpftp_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}
						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype,
												   "x-udpftp") == 0) {
								int r =
									_am_udpftp_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.udpftp_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " x-udpftp");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_udpftp_option = atoi(codec);
								}
							}
						}
					}
					pos2++;
				}

				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
				return AMSIP_NOCOMMONCODEC;	/* refuse anyway */
			} else {
				char port[10];

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _udpftp_port);
				_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				if (add_udpftp_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_udpftp_option);
				}

				if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates */

					/* add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->udpftp_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->text_candidate.local_candidates[_pos].
								 foundation,
								 ca->text_candidate.local_candidates[_pos].
								 component_id,
								 ca->text_candidate.local_candidates[_pos].
								 transport,
								 ca->text_candidate.local_candidates[_pos].
								 priority,
								 ca->text_candidate.local_candidates[_pos].
								 conn_addr,
								 ca->text_candidate.local_candidates[_pos].
								 conn_port,
								 ca->text_candidate.local_candidates[_pos].
								 cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->udpftp_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->udpftp_candidate.
								 stun_candidates[_pos].foundation,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].component_id,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].transport,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].priority,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].conn_addr,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].conn_port,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].cand_type,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].rel_addr,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}
			}
		} else {
			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);

			/* must copy at list one payload anyway! */
			if (!osip_list_eol(&remote_med->m_payloads, 0)) {
				char *fmt;

				fmt = (char *) osip_list_get(&remote_med->m_payloads, 0);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, fmt);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
			} else {
				_am_strcat(SDP_BUF_SIZE, buf, " 0\r\n");
			}
		}
		pos++;
	}

  
  {
    char *body;
    sdp_message_t *new_sdp=NULL;
    int i;
    sdp_message_init(&new_sdp);
    if (new_sdp==NULL) {
      osip_message_set_body(msg, buf, strlen(buf));
      osip_message_set_content_type(msg, "application/sdp");
      
      ca->versionid++;
      return AMSIP_SUCCESS;
    }
    i = sdp_message_parse(new_sdp, buf);
    if (i<0) {
      sdp_message_free(new_sdp);
      
      osip_message_set_body(msg, buf, strlen(buf));
      osip_message_set_content_type(msg, "application/sdp");
      
      ca->versionid++;
      return AMSIP_SUCCESS;
    }
    _sdp_remove_audio_codec(new_sdp);
    i = sdp_message_to_str(new_sdp, &body);
    if (i<0) {
      sdp_message_free(new_sdp);
      
      osip_message_set_body(msg, buf, strlen(buf));
      osip_message_set_content_type(msg, "application/sdp");
      
      ca->versionid++;
      return AMSIP_SUCCESS;
    }
    
    osip_message_set_body(msg, body, strlen(body));
    osip_message_set_content_type(msg, "application/sdp");
  }
  
  ca->versionid++;
	return AMSIP_SUCCESS;
}

int sdp_complete_200ok(am_call_t * ca, osip_message_t * answer)
{
	sdp_message_t *remote_sdp;
	sdp_media_t *remote_med;
	char buf[SDP_BUF_SIZE];
	int pos;

	char localip[128];
	char setup[64];
	int _port;

#ifdef ENABLE_VIDEO
	int _video_port;
#endif
	int _text_port;
	int _udpftp_port;
	int offer_port = -1;

	char audio_profile[32];
	char video_profile[32];
	char text_profile[32];
	char udpftp_profile[32];

	strncpy(audio_profile, _antisipc.audio_profile, sizeof(audio_profile));
	strncpy(video_profile, _antisipc.video_profile, sizeof(video_profile));
	strncpy(text_profile, _antisipc.text_profile, sizeof(text_profile));
	strncpy(udpftp_profile, _antisipc.udpftp_profile,
			sizeof(udpftp_profile));

	if (ca == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"No call information\n"));
		return AMSIP_BADPARAMETER;
	}

	remote_sdp = eXosip_get_remote_sdp(ca->did);
	if (remote_sdp == NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"No remote SDP body found for call\n"));
		return AMSIP_WRONG_STATE;	/* no existing body? */
	}

	strncpy(setup, "passive", sizeof(setup));
	_port = ca->audio_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_port = ca->audio_candidate.stun_candidates[0].conn_port;
	if (_port < 1024)
		_port = ca->audio_candidate.stun_candidates[0].rel_port;

#ifdef ENABLE_VIDEO
	_video_port = ca->video_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_video_port = ca->video_candidate.stun_candidates[0].conn_port;
	if (_video_port < 1024)
		_video_port = ca->video_candidate.stun_candidates[0].rel_port;
#endif

	_text_port = ca->text_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_text_port = ca->text_candidate.stun_candidates[0].conn_port;
	if (_text_port < 1024)
		_text_port = ca->text_candidate.stun_candidates[0].rel_port;

	_udpftp_port = ca->udpftp_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_udpftp_port = ca->udpftp_candidate.stun_candidates[0].conn_port;
	if (_udpftp_port < 1024)
		_udpftp_port = ca->udpftp_candidate.stun_candidates[0].rel_port;

	if (_antisipc.stuntest.use_setup_both != 0)
		strncpy(setup, "both", sizeof(setup));

	memset(localip, '\0', sizeof(localip));

	/* find out if "0.0.0.0" is used. */
	_calls_get_ip_port(remote_sdp, "audio", localip, &offer_port);
	if (offer_port == 0 && eXosip_get_media(remote_sdp, "audio") != NULL
		&& osip_list_size(&remote_sdp->m_medias) == 1) {
		snprintf(localip, sizeof(localip), "0.0.0.0");
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"audio stream is disabled (port=0)\n"));
	} else
		eXosip_guess_localip(AF_INET, localip, 128);

	if (_antisipc.stun_firewall[0] != '\0'
		&& _antisipc.stuntest.use_stun_mapped_ip != 0)
		snprintf(buf, SDP_BUF_SIZE,
				 "v=0\r\n" "o=amsip 0 %i IN IP4 %s\r\n" "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip,
				 _antisipc.stun_firewall);
	else
		snprintf(buf, SDP_BUF_SIZE,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip, localip);

	strcat_global_attr(SDP_BUF_SIZE, buf, remote_sdp);

	if (_antisipc.use_turn_server != 0
		&& strstr(ca->remote_useragent,
				  "Audiocodes-Sip-Gateway-Mediant") != NULL) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Gateway-Mediant bug: remove ICE information\n"));
	} else if (_antisipc.use_turn_server != 0) {
		if (ca->ice_pwd[0] != '\0' && ca->ice_ufrag[0] != '\0') {
			_am_strcat(SDP_BUF_SIZE, buf, "a=ice-pwd:");
			_am_strcat(SDP_BUF_SIZE, buf, ca->ice_pwd);
			_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

			_am_strcat(SDP_BUF_SIZE, buf, "a=ice-ufrag:");
			_am_strcat(SDP_BUF_SIZE, buf, ca->ice_ufrag);
			_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
		}
	}

	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		char payloads[PAYLOADS_BUF_SIZE];
		char payloads_rtpmap[RTPMAPS_BUF_SIZE];

		memset(payloads, '\0', sizeof(payloads));
		memset(payloads_rtpmap, '\0', sizeof(payloads_rtpmap));

		remote_med =
			(sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(remote_med->m_media, "audio")
			&& (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
				|| 0 == osip_strcasecmp(remote_med->m_proto,
										_antisipc.audio_profile))) {
			int pos_payload = 0;
			int add_speex_option = 0;
			int add_ilbc_option = 0;
			int add_amr_option = 0;
			int add_amrwb_option = 0;
			int add_ipmrv25_option = 0;
			int add_telephoneevents_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				pos2 = 0;
				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}

						if (strstr(attr->a_att_value, "729a/")!=NULL
							||strstr(attr->a_att_value, "729A/")!=NULL) {
							pos2++;
							continue; /* bypass linksys wrong "g729a" information */
						}

						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype, "PCMU") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " PCMU");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "PCMA") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " PCMA");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "iLBC") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " iLBC");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									if (_antisipc.codec_attr.ptime > 0
										&& _antisipc.codecs[r].mode > 0) {
										if (_antisipc.codec_attr.ptime %
											_antisipc.codecs[r].mode != 0)
											_antisipc.codecs[r].mode = 0;	/* bad ilbc mode */
									}

									if (_antisipc.codecs[r].mode <= 0) {
										_antisipc.codecs[r].mode = 30;
										if (_antisipc.codec_attr.ptime > 0) {
											if (_antisipc.codec_attr.
												ptime % 20 == 0)
												_antisipc.codecs[r].mode =
													20;
											else
												_antisipc.codecs[r].mode =
													30;
										}
									}

									add_ilbc_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "speex") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0
									&& _antisipc.codecs[r].freq ==
									atoi(freq)) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " speex");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									if (_antisipc.codecs[r].mode > 6)
										_antisipc.codecs[r].mode = 6;

									if (_antisipc.codecs[r].mode <= 0)
										_antisipc.codecs[r].mode = 0;

									add_speex_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g729") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g729");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "amr") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {

										if (amr_option_are_supported(&_antisipc.codecs[r],
											remote_med,
											atoi(codec))==0)
										{
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " AMR");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

											add_amr_option = atoi(codec);
										}
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "amr-wb") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {

										if (amr_option_are_supported(&_antisipc.codecs[r],
											remote_med,
											atoi(codec))==0)
										{
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
											_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " AMR-WB");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
											_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

											add_amrwb_option = atoi(codec);
										}
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g723") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g723");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g7221") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g7221");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-40") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-40");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-32") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-32");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-24") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-24");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "g726-16") ==
									   0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " g726-16");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "GSM") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " GSM");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "telephone-event")
									   == 0) {
								find_rtpmap = 1;
								_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
								_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
									   " telephone-event");
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
								_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

								add_telephoneevents_option = atoi(codec);
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "ip-mr_v2.5") == 0) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {

										_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
										_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " ip-mr_v2.5");
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
										_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

										add_ipmrv25_option = atoi(codec);
								}
							} else if (n == 3) {
								int r = _am_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.codecs[r].enable > 0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " ");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, subtype);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");
								}
							}
						}
					}
					pos2++;
				}
				if (find_rtpmap == 0) {
					/* check if payload is supported but rtpmap is missing 0, 8 */
					if (osip_strcasecmp(payload_number, "0") == 0) {
						int r = _am_codec_get_definition("PCMU");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "0 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:0 PCMU/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "8") == 0) {
						int r = _am_codec_get_definition("PCMA");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "8 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:8 PCMA/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "18") == 0) {
						int r = _am_codec_get_definition("g729");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "18 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:18 G729/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "4") == 0) {
						int r = _am_codec_get_definition("g723");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "4 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:4 G723/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "3") == 0) {
						int r = _am_codec_get_definition("GSM");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "3 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:3 GSM/8000\r\n");
						}
					}
					if (osip_strcasecmp(payload_number, "9") == 0) {
						int r = _am_codec_get_definition("G722");

						if (r >= 0 && _antisipc.codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "9 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:3 G722/8000\r\n");
						}
					}
				}

				pos_payload++;
			}
			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
				sdp_message_free(remote_sdp);
				return AMSIP_NOCOMMONCODEC;	/* refuse anyway */
			} else {
				char port[10];

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _port);
				_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				if (_antisipc.use_turn_server != 0
					&& strstr(ca->remote_useragent,
							  "Audiocodes-Sip-Gateway-Mediant") != NULL) {
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__, OSIP_WARNING, NULL,
								"Gateway-Mediant bug: remove ICE information\n"));
				} else if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates
					   add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->audio_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->audio_candidate.
								 local_candidates[_pos].foundation,
								 ca->audio_candidate.
								 local_candidates[_pos].component_id,
								 ca->audio_candidate.
								 local_candidates[_pos].transport,
								 ca->audio_candidate.
								 local_candidates[_pos].priority,
								 ca->audio_candidate.
								 local_candidates[_pos].conn_addr,
								 ca->audio_candidate.
								 local_candidates[_pos].conn_port,
								 ca->audio_candidate.
								 local_candidates[_pos].cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->audio_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->audio_candidate.stun_candidates[_pos].
								 foundation,
								 ca->audio_candidate.stun_candidates[_pos].
								 component_id,
								 ca->audio_candidate.stun_candidates[_pos].
								 transport,
								 ca->audio_candidate.stun_candidates[_pos].
								 priority,
								 ca->audio_candidate.stun_candidates[_pos].
								 conn_addr,
								 ca->audio_candidate.stun_candidates[_pos].
								 conn_port,
								 ca->audio_candidate.stun_candidates[_pos].
								 cand_type,
								 ca->audio_candidate.stun_candidates[_pos].
								 rel_addr,
								 ca->audio_candidate.stun_candidates[_pos].
								 rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}

				if (_antisipc.codec_attr.bandwidth > 0) {
					char tmp_b[256];

					memset(tmp_b, 0, sizeof(tmp_b));
					snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
							 _antisipc.codec_attr.bandwidth);
					_am_strcat(SDP_BUF_SIZE, buf, tmp_b);
				}

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				if (add_ilbc_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_ilbc_option);
				}
				if (add_speex_option > 0) {
					/* copy all speex options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_speex_option);
				}
				if (add_amr_option > 0) {
					/* copy all amr options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_amr_option);
				}
				if (add_amrwb_option > 0) {
					/* copy all amr options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_amrwb_option);
				}
				if (add_telephoneevents_option > 0) {
					/* copy all telephone-events options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_telephoneevents_option);
				}
				if (add_ipmrv25_option > 0) {
					/* copy all ipmrv25 options */
					strcat_fmtp_option_ipmrv25(SDP_BUF_SIZE, buf, remote_med,
									   add_ipmrv25_option);
				}
				
				strcat_ptime_option(SDP_BUF_SIZE, buf, remote_med);
			}
		}
#ifdef ENABLE_VIDEO
		else if (0 == osip_strcasecmp(remote_med->m_media, "video")
				 && (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
					 || 0 == osip_strcasecmp(remote_med->m_proto,
											 _antisipc.video_profile))) {
			int pos_payload = 0;
			int add_h2631998_option = 0;
			int add_h263_option = 0;
			int add_jpeg_option = 0;
			int add_theora_option = 0;
			int add_mp4v_option = 0;
			int add_h264_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}
						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype,
												   "H263-1998") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " H263-1998");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_h2631998_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "H263") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " H263");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_h263_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "JPEG") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " JPEG");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_jpeg_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "H264") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " H264");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_h264_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "theora") == 0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " theora");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_theora_option = atoi(codec);
								}
							} else if (n == 3
									   && osip_strcasecmp(subtype,
														  "MP4V-ES") ==
									   0) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " MP4V-ES");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_mp4v_option = atoi(codec);
								}
							} else if (n == 3) {
								int r =
									_am_video_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.video_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " ");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, subtype);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_mp4v_option = atoi(codec);
								}
							}
						}
					}
					pos2++;
				}

#if 0
				if (find_rtpmap == 0) {
					/* check if payload is supported but rtpmap is missing 0, 8 */
					if (osip_strcasecmp(payload_number, "34") == 0) {
						int r =
							_am_video_codec_get_definition("H263-1998");

						if (r >= 0 && _antisipc.video_codecs[r].enable > 0) {
							_am_strcat(PAYLOADS_BUF_SIZE, payloads, "34 ");
							_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap,
								   "a=rtpmap:34 H263-1998/90000\r\n");
							add_h2631998_option = 34;
						}
					}
				}
#endif
				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				//we should add a payload indicated in offer?
				_am_strcat(SDP_BUF_SIZE, buf, "115");
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
			} else {
				char port[10];
				int bandwidth;

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _video_port);
				if (ca->reject_video>0)
					_am_strcat(SDP_BUF_SIZE, buf, "0");
				else
					_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				/* bandwidth = strcat_bandwidth_attr(SDP_BUF_SIZE, buf, remote_med); */
				bandwidth=-1; /* we don't use the lowest setting??) */
				if (bandwidth < 0
					&& _antisipc.video_codec_attr.download_bandwidth > 0) {
					char tmp_b[256];

					memset(tmp_b, 0, sizeof(tmp_b));
					snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
							 _antisipc.video_codec_attr.
							 download_bandwidth);
					_am_strcat(SDP_BUF_SIZE, buf, tmp_b);
				}

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				if (add_h2631998_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_h2631998_option);
				}
				if (add_h263_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_h263_option);
				}
				if (add_jpeg_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med,
									   add_jpeg_option);
				}
				if (add_h264_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option_h264(SDP_BUF_SIZE, buf, remote_med, add_h264_option);
				}
				if (add_theora_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_theora_option);
				}
				if (add_mp4v_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_mp4v_option);
				}

				if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates
					   add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->video_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->video_candidate.
								 local_candidates[_pos].foundation,
								 ca->video_candidate.
								 local_candidates[_pos].component_id,
								 ca->video_candidate.
								 local_candidates[_pos].transport,
								 ca->video_candidate.
								 local_candidates[_pos].priority,
								 ca->video_candidate.
								 local_candidates[_pos].conn_addr,
								 ca->video_candidate.
								 local_candidates[_pos].conn_port,
								 ca->video_candidate.
								 local_candidates[_pos].cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->video_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->video_candidate.stun_candidates[_pos].
								 foundation,
								 ca->video_candidate.stun_candidates[_pos].
								 component_id,
								 ca->video_candidate.stun_candidates[_pos].
								 transport,
								 ca->video_candidate.stun_candidates[_pos].
								 priority,
								 ca->video_candidate.stun_candidates[_pos].
								 conn_addr,
								 ca->video_candidate.stun_candidates[_pos].
								 conn_port,
								 ca->video_candidate.stun_candidates[_pos].
								 cand_type,
								 ca->video_candidate.stun_candidates[_pos].
								 rel_addr,
								 ca->video_candidate.stun_candidates[_pos].
								 rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}
			}
		}
#endif
		else if (0 == osip_strcasecmp(remote_med->m_media, "text")
				 && (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
					 || 0 == osip_strcasecmp(remote_med->m_proto,
											 _antisipc.text_profile))) {
			int pos_payload = 0;
			int add_t140_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}
						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype, "t140") == 0) {
								int r =
									_am_text_codec_get_definition(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.text_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " t140");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_t140_option = atoi(codec);
								}
							}
						}
					}
					pos2++;
				}

				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
				sdp_message_free(remote_sdp);
				return AMSIP_NOCOMMONCODEC;	/* refuse anyway */
			} else {
				char port[10];

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _text_port);
				_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				if (add_t140_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_t140_option);
				}

				if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates
					   add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->text_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->text_candidate.local_candidates[_pos].
								 foundation,
								 ca->text_candidate.local_candidates[_pos].
								 component_id,
								 ca->text_candidate.local_candidates[_pos].
								 transport,
								 ca->text_candidate.local_candidates[_pos].
								 priority,
								 ca->text_candidate.local_candidates[_pos].
								 conn_addr,
								 ca->text_candidate.local_candidates[_pos].
								 conn_port,
								 ca->text_candidate.local_candidates[_pos].
								 cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->text_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->text_candidate.stun_candidates[_pos].
								 foundation,
								 ca->text_candidate.stun_candidates[_pos].
								 component_id,
								 ca->text_candidate.stun_candidates[_pos].
								 transport,
								 ca->text_candidate.stun_candidates[_pos].
								 priority,
								 ca->text_candidate.stun_candidates[_pos].
								 conn_addr,
								 ca->text_candidate.stun_candidates[_pos].
								 conn_port,
								 ca->text_candidate.stun_candidates[_pos].
								 cand_type,
								 ca->text_candidate.stun_candidates[_pos].
								 rel_addr,
								 ca->text_candidate.stun_candidates[_pos].
								 rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}
			}
//xx
		} else if (0 == osip_strcasecmp(remote_med->m_media, "x-udpftp")
				   && (0 == osip_strcasecmp(remote_med->m_proto, "RTP/AVP")
					   || 0 == osip_strcasecmp(remote_med->m_proto,
											   _antisipc.udpftp_profile)))
		{
			int pos_payload = 0;
			int add_udpftp_option = 0;

			while (!osip_list_eol(&remote_med->m_payloads, pos_payload)) {
				char *payload_number;
				int pos2 = 0;
				int find_rtpmap = 0;

				payload_number =
					(char *) osip_list_get(&remote_med->m_payloads,
										   pos_payload);

				while (find_rtpmap==0 && !osip_list_eol(&remote_med->a_attributes, pos2)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&remote_med->
														  a_attributes,
														  pos2);
					if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
						/* search for each supported payload */
						int n;
						char codec[64];
						char subtype[64];
						char freq[64];

						if (attr->a_att_value == NULL
							|| strlen(attr->a_att_value) > 63) {
							pos2++;
							continue;	/* avoid buffer overflow! */
						}
						n = rtpmap_sscanf(attr->a_att_value, codec,
										  subtype, freq);
						if (n == 3
							&& osip_strcasecmp(payload_number,
											   codec) == 0) {
							if (n == 3
								&& osip_strcasecmp(subtype,
												   "x-udpftp") == 0) {
								int r =
									_am_udpftp_codec_get_definition
									(subtype);

								find_rtpmap = 1;
								if (r >= 0
									&& _antisipc.udpftp_codecs[r].enable >
									0) {
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, codec);
									_am_strcat(PAYLOADS_BUF_SIZE, payloads, " ");

									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "a=rtpmap:");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, codec);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, " x-udpftp");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "/");
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, freq);
									_am_strcat(RTPMAPS_BUF_SIZE, payloads_rtpmap, "\r\n");

									add_udpftp_option = atoi(codec);
								}
							}
						}
					}
					pos2++;
				}

				pos_payload++;
			}

			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			if (payloads[0] == '\0') {
				_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " \r\n");
				sdp_message_free(remote_sdp);
				return AMSIP_NOCOMMONCODEC;	/* refuse anyway */
			} else {
				char port[10];

				_am_strcat(SDP_BUF_SIZE, buf, " ");
				snprintf(port, 10, "%i", _udpftp_port);
				_am_strcat(SDP_BUF_SIZE, buf, port);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				payloads[strlen(payloads) - 1] = '\0';
				_am_strcat(SDP_BUF_SIZE, buf, payloads);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");

				strcat_media_attr(SDP_BUF_SIZE, buf, remote_med);

				_am_strcat(SDP_BUF_SIZE, buf, payloads_rtpmap);
				if (_antisipc.enable_sdpsetupparameter>0) {
					_am_strcat(SDP_BUF_SIZE, buf, "a=setup:");
					_am_strcat(SDP_BUF_SIZE, buf, setup);
					_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
				}

				if (add_udpftp_option > 0) {
					/* copy all ilbc options */
					strcat_fmtp_option(SDP_BUF_SIZE, buf, remote_med, add_udpftp_option);
				}

				if (_antisipc.use_turn_server != 0) {
					char conn[256];
					int _pos;

					/* add all stun and local ip candidates
					   add all local ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->udpftp_candidate.local_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
								 ca->udpftp_candidate.
								 local_candidates[_pos].foundation,
								 ca->udpftp_candidate.
								 local_candidates[_pos].component_id,
								 ca->udpftp_candidate.
								 local_candidates[_pos].transport,
								 ca->udpftp_candidate.
								 local_candidates[_pos].priority,
								 ca->udpftp_candidate.
								 local_candidates[_pos].conn_addr,
								 ca->udpftp_candidate.
								 local_candidates[_pos].conn_port,
								 ca->udpftp_candidate.
								 local_candidates[_pos].cand_type);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

					/* add all stun ip candidates */
					for (_pos = 0; _pos < 10; _pos++) {
						if (ca->udpftp_candidate.stun_candidates[_pos].
							conn_addr[0] == '\0')
							break;

						memset(conn, '\0', sizeof(conn));
						snprintf(conn, 256,
								 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
								 ca->udpftp_candidate.
								 stun_candidates[_pos].foundation,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].component_id,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].transport,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].priority,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].conn_addr,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].conn_port,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].cand_type,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].rel_addr,
								 ca->udpftp_candidate.
								 stun_candidates[_pos].rel_port);
						_am_strcat(SDP_BUF_SIZE, buf, conn);
					}

				}
			}
		} else {
			_am_strcat(SDP_BUF_SIZE, buf, "m=");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_media);
			_am_strcat(SDP_BUF_SIZE, buf, " 0 ");
			_am_strcat(SDP_BUF_SIZE, buf, remote_med->m_proto);

			/* must copy at list one payload anyway! */
			if (!osip_list_eol(&remote_med->m_payloads, 0)) {
				char *fmt;

				fmt = (char *) osip_list_get(&remote_med->m_payloads, 0);
				_am_strcat(SDP_BUF_SIZE, buf, " ");
				_am_strcat(SDP_BUF_SIZE, buf, fmt);
				_am_strcat(SDP_BUF_SIZE, buf, "\r\n");
			} else {
				_am_strcat(SDP_BUF_SIZE, buf, " 0\r\n");
			}
		}
		pos++;
	}

  {
    char *body;
    sdp_message_t *new_sdp=NULL;
    int i;
    sdp_message_init(&new_sdp);
    if (new_sdp==NULL) {
      osip_message_set_body(answer, buf, strlen(buf));
      osip_message_set_content_type(answer, "application/sdp");
      sdp_message_free(remote_sdp);
      
      ca->versionid++;
      return AMSIP_SUCCESS;
    }
    i = sdp_message_parse(new_sdp, buf);
    if (i<0) {
      sdp_message_free(new_sdp);
      
      osip_message_set_body(answer, buf, strlen(buf));
      osip_message_set_content_type(answer, "application/sdp");
      sdp_message_free(remote_sdp);
      
      ca->versionid++;
      return AMSIP_SUCCESS;
    }
    _sdp_remove_audio_codec(new_sdp);
    i = sdp_message_to_str(new_sdp, &body);
    if (i<0) {
      sdp_message_free(new_sdp);
      
      osip_message_set_body(answer, buf, strlen(buf));
      osip_message_set_content_type(answer, "application/sdp");
      sdp_message_free(remote_sdp);
      
      ca->versionid++;
      return AMSIP_SUCCESS;
    }
    
    osip_message_set_body(answer, body, strlen(body));
    osip_message_set_content_type(answer, "application/sdp");
    sdp_message_free(remote_sdp);
  }
  
  ca->versionid++;
	return AMSIP_SUCCESS;
}

static int _sdp_keep_audio_codec(sdp_media_t *audio_med, char *name, int freq_rate) {
  int p_number=-1;
  int pos;
  
  pos=0;
  while (!osip_list_eol(&audio_med->a_attributes, pos)) {
		sdp_attribute_t *attr;
    int n;
    char codec[64];
    char subtype[64];
    char freq[64];
    
		attr = (sdp_attribute_t *) osip_list_get(&audio_med->a_attributes, pos);

    if (attr->a_att_value == NULL || strlen(attr->a_att_value) > 63) {
      pos++;
      continue;		/* avoid buffer overflow! */
    }
    if (osip_strcasecmp(attr->a_att_field, "rtpmap")!=0) {
      pos++;
      continue;		/* avoid buffer overflow! */
    }
    n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
    if (n == 3 && osip_strcasecmp(subtype, name) == 0 && atoi(freq)==freq_rate) {
      p_number=atoi(codec);
      break;
    }
    pos++;
  }
  
  if (p_number<0)
    return -1;

  pos=0;
  while (!osip_list_eol(&audio_med->a_attributes, pos)) {
		sdp_attribute_t *attr;
    char codec[64];
    
		attr = (sdp_attribute_t *) osip_list_get(&audio_med->a_attributes, pos);
    
    if (attr->a_att_value == NULL || strlen(attr->a_att_value) > 63) {
      pos++;
      continue;		/* avoid buffer overflow! */
    }
    if (osip_strcasecmp(attr->a_att_field, "fmtp")!=0
        && osip_strcasecmp(attr->a_att_field, "rtpmap")!=0) {
      pos++;
      continue;		/* avoid buffer overflow! */
    }
    if (atoi(attr->a_att_value)!=p_number && atoi(codec)!=101) {
      osip_list_remove(&audio_med->a_attributes, pos);
      sdp_attribute_free(attr);
      continue;
    }
    pos++;
  }
  
  pos = 0;
  while (!osip_list_eol(&audio_med->m_payloads, pos)) {
    char *payload_number;
    
    payload_number = (char *) osip_list_get(&audio_med->m_payloads, pos);
    if (payload_number != NULL && atoi(payload_number)!=p_number && atoi(payload_number)!=101) {
      osip_list_remove(&audio_med->m_payloads, pos);
      osip_free(payload_number);
      continue;
    }
    pos++;
  }
  
  return 0;
}

static int _sdp_remove_audio_codec(sdp_message_t * local_sdp) {
	sdp_media_t *audio_med;
  int i;

  if (_antisipc.codec_attr.bandwidth<=0) {
    return 0; /* no change */
  }
  if (_antisipc.codec_attr.bandwidth>40) {
    return 0;
  }
  
	audio_med = eXosip_get_audio_media(local_sdp);
  if (audio_med==NULL) {
    return 0; /* error */
  }

  if (osip_list_size(&audio_med->m_payloads)<=1) {
    return 0;
  }
  
  
  /* keep only best codec // TODO: keep the list of best codecs */
  i = _sdp_keep_audio_codec(audio_med, "AMR-WB", 16000);
  if (i==0) {
    return 0;
  }
  i = _sdp_keep_audio_codec(audio_med, "AMR", 8000);
  if (i==0) {
    return 0;
  }
  i = _sdp_keep_audio_codec(audio_med, "g729", 8000);
  if (i==0) {
    return 0;
  }
  if (_antisipc.codec_attr.bandwidth>30) {
    i = _sdp_keep_audio_codec(audio_med, "speex", 8000);
    if (i==0) {
      return 0;
    }
  }
  i = _sdp_keep_audio_codec(audio_med, "g723", 8000);
  if (i==0) {
    return 0;
  }
  i = _sdp_keep_audio_codec(audio_med, "iLBC", 8000);
  if (i==0) {
    return 0;
  }
  i = _sdp_keep_audio_codec(audio_med, "GSM", 8000);
  if (i==0) {
    return 0;
  }
  if (_antisipc.codec_attr.bandwidth<30) {
    i = _sdp_keep_audio_codec(audio_med, "speex", 8000);
    if (i==0) {
      return 0;
    }
  }
  i = _sdp_keep_audio_codec(audio_med, "speex", 16000);
  if (i==0) {
    return 0;
  }
  
  return 0;
}

int _sdp_off_hold_call(sdp_message_t * sdp)
{
	int pos;
	int pos_media = -1;
	char *rcvsnd;

	pos = 0;
	rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	while (rcvsnd != NULL) {
		if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "sendonly")
							   || 0 == strcmp(rcvsnd, "inactive")
							   || 0 == strcmp(rcvsnd, "recvonly"))) {
			sprintf(rcvsnd, "sendrecv");
		}
		pos++;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	}

	pos_media = 0;
	while (!sdp_message_endof_media(sdp, pos_media)) {
		pos = 0;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
		while (rcvsnd != NULL) {
			if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "sendonly")
								   || 0 == strcmp(rcvsnd, "inactive")
								   || 0 == strcmp(rcvsnd, "recvonly"))) {
				sprintf(rcvsnd, "sendrecv");
			}
			pos++;
			rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
		}
		pos_media++;
	}

	return AMSIP_SUCCESS;
}

int _sdp_hold_call(sdp_message_t * sdp)
{
	int pos;
	int pos_media = -1;
	char *rcvsnd;
	int recv_send = -1;

	pos = 0;
	rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	while (rcvsnd != NULL) {
		if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "sendonly")) {
			recv_send = 0;
		} else if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "recvonly")
									  || 0 == strcmp(rcvsnd, "inactive")
									  || 0 == strcmp(rcvsnd, "sendrecv"))) {
			recv_send = 0;
			sprintf(rcvsnd, "sendonly");
		}
		pos++;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	}

	pos_media = 0;
	while (!sdp_message_endof_media(sdp, pos_media)) {
		sdp_media_t *sdp_media;

		pos = 0;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
		sdp_media =
			(sdp_media_t *) osip_list_get(&sdp->m_medias, pos_media);

		if (0 != osip_strcasecmp(sdp_media->m_media, "audio")) {
			int recv_send_video = -1;

			/* we need to add a global attribute with a field set to "sendonly" */
			while (rcvsnd != NULL) {
				if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "inactive")) {
					recv_send_video = 0;
				} else if (rcvsnd != NULL
						   && (0 == strcmp(rcvsnd, "recvonly")
							   || 0 == strcmp(rcvsnd, "sendrecv")
							   || 0 == strcmp(rcvsnd, "sendonly"))) {
					recv_send_video = 0;
					sprintf(rcvsnd, "inactive");
				}
				pos++;
				rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
			}

			if (recv_send_video == -1)
				sdp_message_a_attribute_add(sdp, pos_media,
											osip_strdup("inactive"), NULL);
		} else {
			while (rcvsnd != NULL) {
				if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "sendonly")) {
					recv_send = 0;
				} else if (rcvsnd != NULL
						   && (0 == strcmp(rcvsnd, "recvonly")
							   || 0 == strcmp(rcvsnd, "sendrecv")
							   || 0 == strcmp(rcvsnd, "inactive"))) {
					recv_send = 0;
					sprintf(rcvsnd, "sendonly");
				}
				pos++;
				rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
			}
		}

		pos_media++;
	}

	if (recv_send == -1) {
		/* we need to add a global attribute with a field set to "sendonly" */
		sdp_message_a_attribute_add(sdp, -1, osip_strdup("sendonly"),
									NULL);
	}

	return AMSIP_SUCCESS;
}

int _sdp_inactive_call(sdp_message_t * sdp)
{
	int pos;
	int pos_media = -1;
	char *rcvsnd;
	int recv_send = -1;

	pos = 0;
	rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	while (rcvsnd != NULL) {
		if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "inactive")) {
			recv_send = 0;
		} else if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "recvonly")
									  || 0 == strcmp(rcvsnd, "sendonly")
									  || 0 == strcmp(rcvsnd, "sendrecv"))) {
			recv_send = 0;
			sprintf(rcvsnd, "inactive");
		}
		pos++;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
	}

	if (recv_send == -1) {
		/* we need to add a global attribute with a field set to "sendonly" */
		sdp_message_a_attribute_add(sdp, -1, osip_strdup("inactive"),
									NULL);
	}

	pos_media = 0;
	while (!sdp_message_endof_media(sdp, pos_media)) {
		pos = 0;
		recv_send = -1;
		rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
		while (rcvsnd != NULL) {
			if (rcvsnd != NULL && 0 == strcmp(rcvsnd, "inactive")) {
				recv_send = 0;
			} else if (rcvsnd != NULL && (0 == strcmp(rcvsnd, "recvonly")
										  || 0 == strcmp(rcvsnd,
														 "sendrecv")
										  || 0 == strcmp(rcvsnd,
														 "sendonly"))) {
				recv_send = 0;
				sprintf(rcvsnd, "inactive");
			}
			pos++;
			rcvsnd = sdp_message_a_att_field_get(sdp, pos_media, pos);
		}
		if (recv_send == -1) {
			/* we need to add a global attribute with a field set to "sendonly" */
			sdp_message_a_attribute_add(sdp, pos_media,
										osip_strdup("inactive"), NULL);
		}
		pos_media++;
	}

	return AMSIP_SUCCESS;
}


int
_sdp_analyse_attribute_setup(sdp_message_t * sdp, sdp_media_t * med,
							 char *setup)
{
	int pos;

	if (sdp == NULL)
		return AMSIP_BADPARAMETER;
	if (med == NULL)
		return AMSIP_BADPARAMETER;

	/* test media attributes */
	pos = 0;
	while (!osip_list_eol(&med->a_attributes, pos)) {
		sdp_attribute_t *at;

		at = (sdp_attribute_t *) osip_list_get(&med->a_attributes, pos);
		if (at->a_att_field != NULL
			&& 0 == strcmp(at->a_att_field, "setup")) {
			snprintf(setup, 64, "%s", at->a_att_value);
			return AMSIP_SUCCESS;
		}
		pos++;
	}

	/* test global attributes */
	pos = 0;
	while (!osip_list_eol(&sdp->a_attributes, pos)) {
		sdp_attribute_t *at;

		at = (sdp_attribute_t *) osip_list_get(&sdp->a_attributes, pos);
		if (at->a_att_field != NULL
			&& 0 == strcmp(at->a_att_field, "setup")) {
			snprintf(setup, 64, "%s", at->a_att_value);
			return AMSIP_SUCCESS;
		}
		pos++;
	}

	return AMSIP_NOTFOUND;
}

int _sdp_analyse_attribute(sdp_message_t * sdp, sdp_media_t * med)
{
	int pos;

	if (sdp == NULL)
		return AMSIP_BADPARAMETER;
	if (med == NULL)
		return AMSIP_BADPARAMETER;

	/* test media attributes */
	pos = 0;
	while (!osip_list_eol(&med->a_attributes, pos)) {
		sdp_attribute_t *at;

		at = (sdp_attribute_t *) osip_list_get(&med->a_attributes, pos);
		if (at->a_att_field != NULL
			&& 0 == strcmp(at->a_att_field, "sendonly")) {
			return _SENDONLY;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "recvonly")) {
			return _RECVONLY;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "sendrecv")) {
			return _SENDRECV;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "inactive")) {
			return _INACTIVE;
		}
		pos++;
	}

	/* test global attributes */
	pos = 0;
	while (!osip_list_eol(&sdp->a_attributes, pos)) {
		sdp_attribute_t *at;

		at = (sdp_attribute_t *) osip_list_get(&sdp->a_attributes, pos);
		if (at->a_att_field != NULL
			&& 0 == strcmp(at->a_att_field, "sendonly")) {
			return _SENDONLY;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "recvonly")) {
			return _RECVONLY;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "sendrecv")) {
			return _SENDRECV;
		} else if (at->a_att_field != NULL &&
				   0 == strcmp(at->a_att_field, "inactive")) {
			return _INACTIVE;
		}
		pos++;
	}

	return _SENDRECV;
}

int
_sdp_switch_to_codec(sdp_message_t * local_sdp,
					 const char *preferred_codec, int compress_more)
{
	sdp_media_t *local_med;
	int pos;

	local_med = eXosip_get_audio_media(local_sdp);

	if (local_med == NULL)
		return AMSIP_SUCCESS;

	if (preferred_codec == NULL)
		return AMSIP_SUCCESS;

	if (compress_more == 0) {
		/* use ptime = 20 ? */
	} else {
		int done = 0;

		/* use ptime = 30 */
		pos = 0;
		while (!osip_list_eol(&local_med->a_attributes, pos)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&local_med->a_attributes,
												  pos);
			if (attr->a_att_field != NULL
				&& osip_strcasecmp(attr->a_att_field, "ptime") == 0) {
				if (attr->a_att_value != NULL
					&& strlen(attr->a_att_value) == 2
					&& osip_strcasecmp(attr->a_att_value, "30") != 0) {
					snprintf(attr->a_att_value, 2, "30");
					break;
				}
				done = 1;
			}
			pos++;
		}

		if (done == 0) {
			/* add ptime=30 */
			sdp_attribute_t *attr;
			int i = sdp_attribute_init(&attr);

			if (i == 0) {
				attr->a_att_field = osip_strdup("ptime");
				attr->a_att_value = osip_strdup("30");
				osip_list_add(&local_med->a_attributes, attr, -1);
			}
		}

		/* use speex in mode 8000 */
		pos = 0;
		while (!osip_list_eol(&local_med->a_attributes, pos)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&local_med->a_attributes,
												  pos);
			if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0
				&& strstr(attr->a_att_value, "speex") != NULL) {
				if (attr->a_att_value != NULL) {
					char *freq = strstr(attr->a_att_value, "16000");

					if (freq != NULL) {
						snprintf(freq, 5, "8000");
					}
				}
				break;
			}
			pos++;
		}
	}

	/* modify to use preferred codec */
	/* search for codec support and payload in the rtpmap */
	pos = 0;

	while (!osip_list_eol(&local_med->a_attributes, pos)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&local_med->a_attributes,
											  pos);
		if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
			/* search for each supported payload */
			int n;
			char codec[64];
			char subtype[64];
			char freq[64];

			if (attr->a_att_value == NULL
				|| strlen(attr->a_att_value) > 63) {
				pos++;
				continue;		/* avoid buffer overflow! */
			}
			n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
			if (n == 3 && osip_strcasecmp(subtype, preferred_codec) == 0) {
				/* codec should appear first in the list of payload */
				int pos2 = 0;

				while (!osip_list_eol(&local_med->m_payloads, pos2)) {
					char *payload_number;

					payload_number =
						(char *) osip_list_get(&local_med->m_payloads,
											   pos2);
					if (payload_number != NULL
						&& 0 == osip_strcasecmp(payload_number, codec)) {
						if (pos2 == 0) {	/* no change */
						} else {	/* move it to first position */
							osip_list_remove(&local_med->m_payloads, pos2);
							osip_list_add(&local_med->m_payloads,
										  payload_number, 0);

							return AMSIP_SUCCESS;
						}
					}
					pos2++;
				}
			}
		}
		pos++;
	}
	return AMSIP_SUCCESS;
}

#ifdef ENABLE_VIDEO
int _sdp_add_video(am_call_t * ca, sdp_message_t * local_sdp)
{
	sdp_media_t *local_med;
	sdp_attribute_t *attr;
	int i;
	char *payload;
	int _video_port;
	char port[10];
	char video_profile[32];

	strncpy(video_profile, _antisipc.video_profile, sizeof(video_profile));

	local_med = eXosip_get_video_media(local_sdp);

	if (local_med != NULL)
		return AMSIP_SUCCESS;	/* already a video call !! */

	for (i = 0; i < 5; i++) {
		if (_antisipc.video_codecs[i].enable > 0) {
			break;
		}
	}
	if (i == 5) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"no video codec configured\n"));
		return AMSIP_API_NOT_INITIALIZED;
	}

	i = sdp_media_init(&local_med);
	if (i != 0)
		return i;

	_video_port = ca->video_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_video_port = ca->video_candidate.stun_candidates[0].conn_port;
	if (_video_port < 1024)
		_video_port = ca->video_candidate.stun_candidates[0].rel_port;

	snprintf(port, 10, "%i", _video_port);
	local_med->m_media = osip_strdup("video");
	local_med->m_port = osip_strdup(port);
	local_med->m_proto = osip_strdup(video_profile);

	/* Add Video */
	for (i = 0; i < 5; i++) {
		if (_antisipc.video_codecs[i].enable > 0) {
			char tmp_payloads[64];
			char tmp_rtpmaps[256];
			int k;

			memset(tmp_payloads, 0, sizeof(tmp_payloads));
			memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
			snprintf(tmp_payloads, 64, "%i",
					 _antisipc.video_codecs[i].payload);

			payload = osip_strdup(tmp_payloads);
			osip_list_add(&local_med->m_payloads, payload, -1);

			k = sdp_attribute_init(&attr);
			if (k != 0) {
				sdp_media_free(local_med);
				return k;
			}
			attr->a_att_field = osip_strdup("rtpmap");
			snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps), "%i %s/%i",
					 _antisipc.video_codecs[i].payload,
					 _antisipc.video_codecs[i].name,
					 _antisipc.video_codecs[i].freq);
			attr->a_att_value = osip_strdup(tmp_rtpmaps);	/* ("34 H263-1998/90000"); */
			osip_list_add(&local_med->a_attributes, attr, -1);

			if (osip_strcasecmp(_antisipc.video_codecs[i].name, "H264") ==
				0) {
				k = sdp_attribute_init(&attr);
				if (k != 0) {
					sdp_media_free(local_med);
					return k;
				}
				attr->a_att_field = osip_strdup("fmtp");

				//;max-mbps=10000; max-fs=1792; max-br=525",
				if (_antisipc.video_codecs[i].mode==0)
					snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps), "%i profile-level-id=42800c; packetization-mode=0",
					_antisipc.video_codecs[i].payload);
				else if (_antisipc.video_codecs[i].mode==1)
					snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps), "%i profile-level-id=42800c; packetization-mode=1",
					_antisipc.video_codecs[i].payload);

				attr->a_att_value = osip_strdup(tmp_rtpmaps);
				osip_list_add(&local_med->a_attributes, attr, -1);

				if (_antisipc.video_codecs[i].vbr==0xF000)
				{
					k = sdp_attribute_init(&attr);
					if (k != 0) {
						sdp_media_free(local_med);
						return k;
					}
					attr->a_att_field = osip_strdup("ars");
					attr->a_att_value = NULL;
					osip_list_add(&local_med->a_attributes, attr, -1);
				}

			}
		}
	}

	if (_antisipc.use_turn_server != 0) {
		char conn[256];
		int pos;

		/* add all stun and local ip candidates */

		/* add all local ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->video_candidate.local_candidates[pos].conn_addr[0] ==
				'\0')
				break;
			i = sdp_attribute_init(&attr);
			if (i != 0) {
				sdp_media_free(local_med);
				return i;
			}

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256, "%i %i %s %i %s %i typ %s",
					 ca->video_candidate.local_candidates[pos].foundation,
					 ca->video_candidate.local_candidates[pos].
					 component_id,
					 ca->video_candidate.local_candidates[pos].transport,
					 ca->video_candidate.local_candidates[pos].priority,
					 ca->video_candidate.local_candidates[pos].conn_addr,
					 ca->video_candidate.local_candidates[pos].conn_port,
					 ca->video_candidate.local_candidates[pos].cand_type);
			attr->a_att_field = osip_strdup("candidate");
			attr->a_att_value = osip_strdup(conn);
			osip_list_add(&local_med->a_attributes, attr, -1);
		}

		/* add all stun ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->video_candidate.stun_candidates[pos].conn_addr[0] ==
				'\0')
				break;
			i = sdp_attribute_init(&attr);
			if (i != 0) {
				sdp_media_free(local_med);
				return i;
			}

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->video_candidate.stun_candidates[pos].foundation,
					 ca->video_candidate.stun_candidates[pos].component_id,
					 ca->video_candidate.stun_candidates[pos].transport,
					 ca->video_candidate.stun_candidates[pos].priority,
					 ca->video_candidate.stun_candidates[pos].conn_addr,
					 ca->video_candidate.stun_candidates[pos].conn_port,
					 ca->video_candidate.stun_candidates[pos].cand_type,
					 ca->video_candidate.stun_candidates[pos].rel_addr,
					 ca->video_candidate.stun_candidates[pos].rel_port);
			attr->a_att_field = osip_strdup("candidate");
			attr->a_att_value = osip_strdup(conn);
			osip_list_add(&local_med->a_attributes, attr, -1);
		}

		/* add all relay ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->video_candidate.relay_candidates[pos].conn_addr[0] ==
				'\0')
				break;
			i = sdp_attribute_init(&attr);
			if (i != 0) {
				sdp_media_free(local_med);
				return i;
			}

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->video_candidate.relay_candidates[pos].foundation,
					 ca->video_candidate.relay_candidates[pos].
					 component_id,
					 ca->video_candidate.relay_candidates[pos].transport,
					 ca->video_candidate.relay_candidates[pos].priority,
					 ca->video_candidate.relay_candidates[pos].conn_addr,
					 ca->video_candidate.relay_candidates[pos].conn_port,
					 ca->video_candidate.relay_candidates[pos].cand_type,
					 ca->video_candidate.relay_candidates[pos].rel_addr,
					 ca->video_candidate.relay_candidates[pos].rel_port);
			attr->a_att_field = osip_strdup("candidate");
			attr->a_att_value = osip_strdup(conn);
			osip_list_add(&local_med->a_attributes, attr, -1);
		}
	}

	if (_antisipc.video_codec_attr.download_bandwidth > 0) {
		sdp_bandwidth_t *bd;
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "%i",
				 _antisipc.video_codec_attr.download_bandwidth);
		i = sdp_bandwidth_init(&bd);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		bd->b_bwtype = osip_strdup("AS");
		bd->b_bandwidth = osip_strdup(tmp_b);
		osip_list_add(&local_med->b_bandwidths, bd, -1);
	}

	if (0 == osip_strcasecmp(video_profile, "RTP/SAVP")
		|| _antisipc.optionnal_encryption != 0) {
		char b64_text[41];
		char key[31];
		char *crypto_attr;
		memset(b64_text, 0, sizeof(b64_text));
		memset(key, 0, sizeof(key));
		eXosip_generate_random(key, 16);
		am_hexa_generate_random(key, 31, key, "key", "crypto");
		am_base64_encode(b64_text, key, 30);

		crypto_attr = osip_malloc(35+40+7);
		if (crypto_attr == NULL) {
			sdp_media_free(local_med);
			return i;
		}
		/*snprintf(crypto_attr, 35+40+7, "1 AES_CM_128_HMAC_SHA1_80 inline:%s|2^20", b64_text); */
		snprintf(crypto_attr, 35+40+7, "1 AES_CM_128_HMAC_SHA1_80 inline:%s", b64_text);

		i = sdp_attribute_init(&attr);
		if (i != 0) {
			osip_free(crypto_attr);
			sdp_media_free(local_med);
			return i;
		}
		attr->a_att_field = osip_strdup("crypto");
		attr->a_att_value = crypto_attr;
		osip_list_add(&local_med->a_attributes, attr, -1);
	}

	if (_antisipc.optionnal_encryption != 0) {
		i = sdp_attribute_init(&attr);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		attr->a_att_field = osip_strdup("encryption");
		attr->a_att_value = osip_strdup("optional");
		osip_list_add(&local_med->a_attributes, attr, -1);
	}

	osip_list_add(&local_sdp->m_medias, local_med, -1);
	return AMSIP_SUCCESS;
}


int _sdp_modify_video(am_call_t * ca, sdp_message_t * local_sdp)
{
	sdp_media_t *local_med;
	int pos;
	int _video_port;
	char port[10];

	local_med = eXosip_get_video_media(local_sdp);

	if (local_med == NULL)
		return AMSIP_BADPARAMETER;

	if (local_med->m_port[0] == '0') {
		_video_port = ca->video_candidate.stun_candidates[0].rel_port;
		if (_antisipc.stuntest.use_stun_mapped_port != 0)
			_video_port = ca->video_candidate.stun_candidates[0].conn_port;
		if (_video_port < 1024)
			_video_port = ca->video_candidate.stun_candidates[0].rel_port;

		snprintf(port, 10, "%i", _video_port);
		osip_free(local_med->m_port);
		local_med->m_port = osip_strdup(port);
	}

	/* change bandwidth attribute */
	pos = 0;
	while (!osip_list_eol(&local_med->b_bandwidths, pos)) {
		sdp_bandwidth_t *bandwidth;

		bandwidth =
			(sdp_bandwidth_t *) osip_list_get(&local_med->b_bandwidths,
											  pos);
		if (bandwidth->b_bwtype != NULL && bandwidth->b_bandwidth != NULL
			&& osip_strcasecmp(bandwidth->b_bwtype, "AS") == 0) {
			char tmp_b[256];

			memset(tmp_b, 0, sizeof(tmp_b));
			snprintf(tmp_b, sizeof(tmp_b), "%i",
					 _antisipc.video_codec_attr.download_bandwidth);

			if (atoi(bandwidth->b_bandwidth) ==
				_antisipc.video_codec_attr.download_bandwidth)
				return AMSIP_SUCCESS;	/* no change... */

			/* replace the bandwidth */
			osip_free(bandwidth->b_bandwidth);
			bandwidth->b_bandwidth = osip_strdup(tmp_b);
			return AMSIP_SUCCESS;
		}
	}

	if (_antisipc.video_codec_attr.download_bandwidth > 0) {
		sdp_bandwidth_t *bd;
		int i;
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "%i",
				 _antisipc.video_codec_attr.download_bandwidth);
		i = sdp_bandwidth_init(&bd);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		bd->b_bwtype = osip_strdup("AS");
		bd->b_bandwidth = osip_strdup(tmp_b);
		osip_list_add(&local_med->b_bandwidths, bd, -1);
	}
	return AMSIP_SUCCESS;
}

int _sdp_disable_video(am_call_t * ca, sdp_message_t * local_sdp)
{
	sdp_media_t *local_med;
	int pos;

	local_med = eXosip_get_video_media(local_sdp);

	if (local_med == NULL)
		return AMSIP_BADPARAMETER;

	osip_free(local_med->m_port);
	local_med->m_port = osip_strdup("0");

	/* change bandwidth attribute */
	pos = 0;
	while (!osip_list_eol(&local_med->b_bandwidths, pos)) {
		sdp_bandwidth_t *bandwidth;

		bandwidth =
			(sdp_bandwidth_t *) osip_list_get(&local_med->b_bandwidths,
											  pos);
		if (bandwidth->b_bwtype != NULL && bandwidth->b_bandwidth != NULL
			&& osip_strcasecmp(bandwidth->b_bwtype, "AS") == 0) {
			char tmp_b[256];

			memset(tmp_b, 0, sizeof(tmp_b));
			snprintf(tmp_b, sizeof(tmp_b), "%i",
					 _antisipc.video_codec_attr.download_bandwidth);

			if (atoi(bandwidth->b_bandwidth) ==
				_antisipc.video_codec_attr.download_bandwidth)
				return AMSIP_SUCCESS;	/* no change... */

			/* replace the bandwidth */
			osip_free(bandwidth->b_bandwidth);
			bandwidth->b_bandwidth = osip_strdup(tmp_b);
			return AMSIP_SUCCESS;
		}
	}

	if (_antisipc.video_codec_attr.download_bandwidth > 0) {
		sdp_bandwidth_t *bd;
		int i;
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "%i",
				 _antisipc.video_codec_attr.download_bandwidth);
		i = sdp_bandwidth_init(&bd);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		bd->b_bwtype = osip_strdup("AS");
		bd->b_bandwidth = osip_strdup(tmp_b);
		osip_list_add(&local_med->b_bandwidths, bd, -1);
	}
	return AMSIP_SUCCESS;
}

#endif

int _sdp_add_text(am_call_t * ca, sdp_message_t * local_sdp)
{
	sdp_media_t *local_med;
	sdp_attribute_t *attr;
	int i;
	char *payload;
	int _text_port;
	char port[10];
	char text_profile[32];

	strncpy(text_profile, _antisipc.text_profile, sizeof(text_profile));

	local_med = eXosip_get_media(local_sdp, "text");

	if (local_med != NULL)
		return AMSIP_SUCCESS;	/* already a text call !! */

	for (i = 0; i < 5; i++) {
		if (_antisipc.text_codecs[i].enable > 0) {
			break;
		}
	}
	if (i == 5) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"no text codec configured\n"));
		return AMSIP_API_NOT_INITIALIZED;
	}

	i = sdp_media_init(&local_med);
	if (i != 0)
		return i;

	_text_port = ca->text_candidate.stun_candidates[0].rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_text_port = ca->text_candidate.stun_candidates[0].conn_port;
	if (_text_port < 1024)
		_text_port = ca->text_candidate.stun_candidates[0].rel_port;

	snprintf(port, 10, "%i", _text_port);
	local_med->m_media = osip_strdup("text");
	local_med->m_port = osip_strdup(port);
	local_med->m_proto = osip_strdup(text_profile);

	/* Add text */
	for (i = 0; i < 5; i++) {
		if (_antisipc.text_codecs[i].enable > 0) {
			char tmp_payloads[64];
			char tmp_rtpmaps[256];
			int k;

			memset(tmp_payloads, 0, sizeof(tmp_payloads));
			memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
			snprintf(tmp_payloads, 64, "%i",
					 _antisipc.text_codecs[i].payload);

			payload = osip_strdup(tmp_payloads);
			osip_list_add(&local_med->m_payloads, payload, -1);

			k = sdp_attribute_init(&attr);
			if (k != 0) {
				sdp_media_free(local_med);
				return k;
			}
			attr->a_att_field = osip_strdup("rtpmap");
			snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps), "%i %s/%i",
					 _antisipc.text_codecs[i].payload,
					 _antisipc.text_codecs[i].name,
					 _antisipc.text_codecs[i].freq);
			attr->a_att_value = osip_strdup(tmp_rtpmaps);	/* ("34 H263-1998/90000"); */
			osip_list_add(&local_med->a_attributes, attr, -1);
		}
	}

	if (_antisipc.use_turn_server != 0) {
		char conn[256];
		int pos;

		/* add all stun and local ip candidates */

		/* add all local ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->text_candidate.local_candidates[pos].conn_addr[0] ==
				'\0')
				break;
			i = sdp_attribute_init(&attr);
			if (i != 0) {
				sdp_media_free(local_med);
				return i;
			}

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256, "%i %i %s %i %s %i typ %s",
					 ca->text_candidate.local_candidates[pos].foundation,
					 ca->text_candidate.local_candidates[pos].component_id,
					 ca->text_candidate.local_candidates[pos].transport,
					 ca->text_candidate.local_candidates[pos].priority,
					 ca->text_candidate.local_candidates[pos].conn_addr,
					 ca->text_candidate.local_candidates[pos].conn_port,
					 ca->text_candidate.local_candidates[pos].cand_type);
			attr->a_att_field = osip_strdup("candidate");
			attr->a_att_value = osip_strdup(conn);
			osip_list_add(&local_med->a_attributes, attr, -1);
		}

		/* add all stun ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->text_candidate.stun_candidates[pos].conn_addr[0] ==
				'\0')
				break;
			i = sdp_attribute_init(&attr);
			if (i != 0) {
				sdp_media_free(local_med);
				return i;
			}

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->text_candidate.stun_candidates[pos].foundation,
					 ca->text_candidate.stun_candidates[pos].component_id,
					 ca->text_candidate.stun_candidates[pos].transport,
					 ca->text_candidate.stun_candidates[pos].priority,
					 ca->text_candidate.stun_candidates[pos].conn_addr,
					 ca->text_candidate.stun_candidates[pos].conn_port,
					 ca->text_candidate.stun_candidates[pos].cand_type,
					 ca->text_candidate.stun_candidates[pos].rel_addr,
					 ca->text_candidate.stun_candidates[pos].rel_port);
			attr->a_att_field = osip_strdup("candidate");
			attr->a_att_value = osip_strdup(conn);
			osip_list_add(&local_med->a_attributes, attr, -1);
		}

		/* add all relay ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->text_candidate.relay_candidates[pos].conn_addr[0] ==
				'\0')
				break;
			i = sdp_attribute_init(&attr);
			if (i != 0) {
				sdp_media_free(local_med);
				return i;
			}

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->text_candidate.relay_candidates[pos].foundation,
					 ca->text_candidate.relay_candidates[pos].component_id,
					 ca->text_candidate.relay_candidates[pos].transport,
					 ca->text_candidate.relay_candidates[pos].priority,
					 ca->text_candidate.relay_candidates[pos].conn_addr,
					 ca->text_candidate.relay_candidates[pos].conn_port,
					 ca->text_candidate.relay_candidates[pos].cand_type,
					 ca->text_candidate.relay_candidates[pos].rel_addr,
					 ca->text_candidate.relay_candidates[pos].rel_port);
			attr->a_att_field = osip_strdup("candidate");
			attr->a_att_value = osip_strdup(conn);
			osip_list_add(&local_med->a_attributes, attr, -1);
		}
	}

	if (_antisipc.text_codec_attr.download_bandwidth > 0) {
		sdp_bandwidth_t *bd;
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "%i",
				 _antisipc.text_codec_attr.download_bandwidth);
		i = sdp_bandwidth_init(&bd);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		bd->b_bwtype = osip_strdup("AS");
		bd->b_bandwidth = osip_strdup(tmp_b);
		osip_list_add(&local_med->b_bandwidths, bd, -1);
	}


	if (0 == osip_strcasecmp(text_profile, "RTP/SAVP")
		|| _antisipc.optionnal_encryption != 0) {
		char b64_text[41];
		char key[31];
		char *crypto_attr;
		memset(b64_text, 0, sizeof(b64_text));
		memset(key, 0, sizeof(key));
		eXosip_generate_random(key, 16);
		am_hexa_generate_random(key, 31, key, "key", "crypto");
		am_base64_encode(b64_text, key, 30);

		crypto_attr = osip_malloc(35+40+7);
		if (crypto_attr == NULL) {
			sdp_media_free(local_med);
			return i;
		}
		/*snprintf(crypto_attr, 35+40+7, "1 AES_CM_128_HMAC_SHA1_80 inline:%s|2^20", b64_text); */
		snprintf(crypto_attr, 35+40+7, "1 AES_CM_128_HMAC_SHA1_80 inline:%s", b64_text);

		i = sdp_attribute_init(&attr);
		if (i != 0) {
			osip_free(crypto_attr);
			sdp_media_free(local_med);
			return i;
		}
		attr->a_att_field = osip_strdup("crypto");
		attr->a_att_value = crypto_attr;
		osip_list_add(&local_med->a_attributes, attr, -1);
	}

	if (_antisipc.optionnal_encryption != 0) {
		i = sdp_attribute_init(&attr);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		attr->a_att_field = osip_strdup("encryption");
		attr->a_att_value = osip_strdup("optional");
		osip_list_add(&local_med->a_attributes, attr, -1);
	}

	osip_list_add(&local_sdp->m_medias, local_med, -1);
	return AMSIP_SUCCESS;
}


int _sdp_modify_text(am_call_t * ca, sdp_message_t * local_sdp)
{
	sdp_media_t *local_med;
	int pos;

	local_med = eXosip_get_media(local_sdp, "text");

	if (local_med == NULL)
		return AMSIP_BADPARAMETER;

	/* change bandwidth attribute */
	pos = 0;
	while (!osip_list_eol(&local_med->b_bandwidths, pos)) {
		sdp_bandwidth_t *bandwidth;

		bandwidth =
			(sdp_bandwidth_t *) osip_list_get(&local_med->b_bandwidths,
											  pos);
		if (bandwidth->b_bwtype != NULL && bandwidth->b_bandwidth != NULL
			&& osip_strcasecmp(bandwidth->b_bwtype, "AS") == 0) {
			char tmp_b[256];

			memset(tmp_b, 0, sizeof(tmp_b));
			snprintf(tmp_b, sizeof(tmp_b), "%i",
					 _antisipc.text_codec_attr.download_bandwidth);

			if (atoi(bandwidth->b_bandwidth) ==
				_antisipc.text_codec_attr.download_bandwidth)
				return AMSIP_SUCCESS;	/* no change... */

			/* replace the bandwidth */
			osip_free(bandwidth->b_bandwidth);
			bandwidth->b_bandwidth = osip_strdup(tmp_b);
			return AMSIP_SUCCESS;
		}
	}

	if (_antisipc.text_codec_attr.download_bandwidth > 0) {
		sdp_bandwidth_t *bd;
		int i;
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "%i",
				 _antisipc.text_codec_attr.download_bandwidth);
		i = sdp_bandwidth_init(&bd);
		if (i != 0) {
			sdp_media_free(local_med);
			return i;
		}
		bd->b_bwtype = osip_strdup("AS");
		bd->b_bandwidth = osip_strdup(tmp_b);
		osip_list_add(&local_med->b_bandwidths, bd, -1);
	}
	return AMSIP_SUCCESS;
}

int _sdp_increase_versionid(sdp_message_t * local_sdp)
{
	int vid = 0;
	char *svid;

	if (local_sdp == NULL)
		return AMSIP_BADPARAMETER;
	if (local_sdp->o_sess_version == NULL)
		return AMSIP_SYNTAXERROR;

	vid = atoi(local_sdp->o_sess_version);
	if (vid < 0)
		return AMSIP_SYNTAXERROR;
	vid++;

	svid = (char *) osip_malloc(strlen(local_sdp->o_sess_version) + 3);
	snprintf(svid, strlen(local_sdp->o_sess_version) + 2, "%i", vid);

	osip_free(local_sdp->o_sess_version);
	local_sdp->o_sess_version = svid;

	return AMSIP_SUCCESS;
}
