/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "amsip/am_options.h"
#include "sdptools.h"

#include "am_calls.h"

#include <ortp/ortp.h>
#include <ortp/telephonyevents.h>

#include "mediastreamer2/dtmfgen.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilerec.h"
#include "mediastreamer2/msv4l.h"
#ifdef ENABLE_VIDEO
#include "mediastreamer2/msvideo.h"
#endif

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

struct antisipc _antisipc;		/* main handle */

int am_session_add_offer(am_call_t * ca, char *tmp);

int am_session_add_offer(am_call_t * ca, char *tmp)
{
	char port[10];
	char rtpmaps[2048];
	char payloads[128];
	char setup[64];
	char audio_profile[32];
	char localip[128];

	int _port = 0;
	int i;

	tmp[0] = '\0';

	strncpy(audio_profile, _antisipc.audio_profile, sizeof(audio_profile));

	strncpy(setup, "passive", sizeof(setup));
	if (ca == NULL) {
		return OSIP_BADPARAMETER;
	}

	_port = ca->audio_candidate.stun_candidates->rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_port = ca->audio_candidate.stun_candidates->conn_port;
	if (_port < 1024)
		_port = ca->audio_candidate.stun_candidates->rel_port;

	if (_antisipc.use_relay_server != 0
		&& ca->audio_candidate.relay_candidates->conn_addr[0] != '\0') {
		_port = ca->audio_candidate.relay_candidates->conn_port;
	}

	if (_antisipc.stuntest.use_setup_both != 0)
		strncpy(setup, "both", sizeof(setup));

	snprintf(port, 10, "%i", _port);

	eXosip_guess_localip(AF_INET, localip, 128);

#ifdef LOOPBACK_TEST
	snprintf(localip, sizeof(localip), "%s", "192.168.2.50");
#endif

	if (_antisipc.use_relay_server != 0
		&& ca->audio_candidate.relay_candidates->conn_addr[0] != '\0') {
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n",
				 ca->versionid,
				 ca->audio_candidate.relay_candidates->conn_addr,
				 ca->audio_candidate.relay_candidates->conn_addr);
	} else if (_antisipc.use_turn_server != 0
			   && _antisipc.stun_firewall[0] != '\0') {
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
	} else if (_antisipc.stun_firewall[0] != '\0'
			   && _antisipc.stuntest.use_stun_mapped_ip != 0)
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
	else
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip, localip);

	if (_antisipc.use_turn_server != 0) {
		strcat(tmp, "a=ice-pwd:");
		strcat(tmp, ca->ice_pwd);
		strcat(tmp, "\r\n");
		strcat(tmp, "a=ice-ufrag:");
		strcat(tmp, ca->ice_ufrag);
		strcat(tmp, "\r\n");
	}

	strcat(tmp, "m=audio ");
	strcat(tmp, port);
	strcat(tmp, " ");
	strcat(tmp, audio_profile);

	memset(rtpmaps, 0, sizeof(rtpmaps));
	memset(payloads, 0, sizeof(payloads));
	for (i = 0; i < 5; i++) {
		if (ms_filter_codec_supported(_antisipc.codecs[i].name)
			&& _antisipc.codecs[i].enable > 0) {
			char tmp_payloads[64];
			char tmp_rtpmaps[256];

			memset(tmp_payloads, 0, sizeof(tmp_payloads));
			memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
			snprintf(tmp_payloads, 64, "%i", _antisipc.codecs[i].payload);
			strcat(payloads, " ");
			strcat(payloads, tmp_payloads);

			snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
					 "a=rtpmap:%i %s/%i\r\n", _antisipc.codecs[i].payload,
					 _antisipc.codecs[i].name, _antisipc.codecs[i].freq);
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

			if (osip_strcasecmp(_antisipc.codecs[i].name, "speex") == 0) {
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

			if (osip_strcasecmp(_antisipc.codecs[i].name, "AMR") == 0
				||osip_strcasecmp(_antisipc.codecs[i].name, "AMR-WB") == 0) {

				if ((_antisipc.codecs[i].mode&0x70) == 0)
				{
					snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
							 "a=fmtp:%i octet-align=1\r\n",
							 _antisipc.codecs[i].payload);
				}
				else
				{
					snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
							 "a=fmtp:%i octet-align=%i;interleaving=%i\r\n",
							 _antisipc.codecs[i].payload,
							 (_antisipc.codecs[i].mode&0x10)==0x10?1:0,
							 (_antisipc.codecs[i].mode&0x20)==0x20?1:0);
				}
				strcat(rtpmaps, tmp_rtpmaps);
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

	if (_antisipc.use_turn_server != 0) {
		char conn[256];
		int pos;

		/* add all stun and local ip candidates */

		/* add all local ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->audio_candidate.local_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256, "a=candidate:%i %i %s %i %s %i typ %s\r\n",
					 ca->audio_candidate.local_candidates[pos].foundation,
					 ca->audio_candidate.local_candidates[pos].
					 component_id,
					 ca->audio_candidate.local_candidates[pos].transport,
					 ca->audio_candidate.local_candidates[pos].priority,
					 ca->audio_candidate.local_candidates[pos].conn_addr,
					 ca->audio_candidate.local_candidates[pos].conn_port,
					 ca->audio_candidate.local_candidates[pos].cand_type);
			strcat(tmp, conn);
		}

		/* add all stun ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->audio_candidate.stun_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->audio_candidate.stun_candidates[pos].foundation,
					 ca->audio_candidate.stun_candidates[pos].component_id,
					 ca->audio_candidate.stun_candidates[pos].transport,
					 ca->audio_candidate.stun_candidates[pos].priority,
					 ca->audio_candidate.stun_candidates[pos].conn_addr,
					 ca->audio_candidate.stun_candidates[pos].conn_port,
					 ca->audio_candidate.stun_candidates[pos].cand_type,
					 ca->audio_candidate.stun_candidates[pos].rel_addr,
					 ca->audio_candidate.stun_candidates[pos].rel_port);
			strcat(tmp, conn);
		}

		/* add all relay ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->audio_candidate.relay_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->audio_candidate.relay_candidates[pos].foundation,
					 ca->audio_candidate.relay_candidates[pos].
					 component_id,
					 ca->audio_candidate.relay_candidates[pos].transport,
					 ca->audio_candidate.relay_candidates[pos].priority,
					 ca->audio_candidate.relay_candidates[pos].conn_addr,
					 ca->audio_candidate.relay_candidates[pos].conn_port,
					 ca->audio_candidate.relay_candidates[pos].cand_type,
					 ca->audio_candidate.relay_candidates[pos].rel_addr,
					 ca->audio_candidate.relay_candidates[pos].rel_port);
			strcat(tmp, conn);
		}
	}

	strcat(tmp, rtpmaps);
	strcat(tmp, "a=rtpmap:101 telephone-event/8000\r\n");
	strcat(tmp, "a=fmtp:101 0-11\r\n");
	if (_antisipc.enable_sdpsetupparameter>0) {
		strcat(tmp, "a=setup:");
		strcat(tmp, setup);
		strcat(tmp, "\r\n");
	}
	if (_antisipc.codec_attr.ptime > 0) {
		char tmp_ptime[256];

		memset(tmp_ptime, 0, sizeof(tmp_ptime));
		snprintf(tmp_ptime, sizeof(tmp_ptime), "a=ptime:%i\r\n",
				 _antisipc.codec_attr.ptime);
		strcat(tmp, tmp_ptime);
	}


	if (0 == osip_strcasecmp(audio_profile, "RTP/SAVP")
		|| _antisipc.optionnal_encryption != 0) {
		int idx;
		int tag=1;
		for (idx=0;idx<10;idx++) {
			if (_antisipc.srtp_info[idx].srtp_algo > 0 && _antisipc.srtp_info[idx].enable>0)
			{
				char b64_text[41];
				char key[31];
				strcat(tmp, "a=crypto:");
				snprintf(key, sizeof(key), "%i ", tag);
				strcat(tmp, key);
				tag++;
				if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80)
					strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_32)
					strcat(tmp, "AES_CM_128_HMAC_SHA1_32 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_NULL_AUTH)
					strcat(tmp, "AES_CM_128_NULL_AUTH inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_NULL_CIPHER_HMAC_SHA1_80)
					strcat(tmp, "NULL_CIPHER_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_80)
					strcat(tmp, "AES_CM_256_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_32)
					strcat(tmp, "AES_CM_256_HMAC_SHA1_32 inline:");
				else
					strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
				memset(b64_text, 0, sizeof(b64_text));
				memset(key, 0, sizeof(key));
				eXosip_generate_random(key, 16);
				am_hexa_generate_random(key, 31, key, "key", "crypto");
				am_base64_encode(b64_text, key, 30);

				strcat(tmp, b64_text);
				/* strcat(tmp, "|2^20"); */
				strcat(tmp, "\r\n");
			}
		}
	}
	if (_antisipc.optionnal_encryption != 0)
		strcat(tmp, "a=encryption:optional\r\n");

	if (_antisipc.add_nortpproxy != 0)
		strcat(tmp, "a=nortpproxy:yes\r\n");


	ca->versionid++;
	return OSIP_SUCCESS;
}

static int am_session_add_text_offer(am_call_t * ca, char *tmp)
{
	char port[10];
	char rtpmaps[2048];
	char payloads[128];
	char setup[64];
	char text_profile[32];
	char localip[128];

	int _port = 0;
	int i;

	tmp[0] = '\0';

	strncpy(text_profile, _antisipc.text_profile, sizeof(text_profile));

	strncpy(setup, "passive", sizeof(setup));
	if (ca == NULL) {
		return OSIP_BADPARAMETER;
	}

	_port = ca->text_candidate.stun_candidates->rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_port = ca->text_candidate.stun_candidates->conn_port;
	if (_port < 1024)
		_port = ca->text_candidate.stun_candidates->rel_port;

	if (_antisipc.use_relay_server != 0
		&& ca->text_candidate.relay_candidates->conn_addr[0] != '\0') {
		_port = ca->text_candidate.relay_candidates->conn_port;
	}

	if (_antisipc.stuntest.use_setup_both != 0)
		strncpy(setup, "both", sizeof(setup));

	snprintf(port, 10, "%i", _port);

	eXosip_guess_localip(AF_INET, localip, 128);

	if (_antisipc.use_relay_server != 0
		&& ca->text_candidate.relay_candidates->conn_addr[0] != '\0') {
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n",
				 ca->versionid,
				 ca->text_candidate.relay_candidates->conn_addr,
				 ca->text_candidate.relay_candidates->conn_addr);
	} else if (_antisipc.use_turn_server != 0
			   && _antisipc.stun_firewall[0] != '\0') {
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
	} else if (_antisipc.stun_firewall[0] != '\0'
			   && _antisipc.stuntest.use_stun_mapped_ip != 0)
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
	else
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip, localip);

	if (_antisipc.use_turn_server != 0) {
		strcat(tmp, "a=ice-pwd:");
		strcat(tmp, ca->ice_pwd);
		strcat(tmp, "\r\n");
		strcat(tmp, "a=ice-ufrag:");
		strcat(tmp, ca->ice_ufrag);
		strcat(tmp, "\r\n");
	}

	strcat(tmp, "m=text ");
	strcat(tmp, port);
	strcat(tmp, " ");
	strcat(tmp, text_profile);

	memset(rtpmaps, 0, sizeof(rtpmaps));
	memset(payloads, 0, sizeof(payloads));
	for (i = 0; i < 5; i++) {
		if (ms_filter_codec_supported(_antisipc.text_codecs[i].name)
			&& _antisipc.text_codecs[i].enable > 0) {
			char tmp_payloads[64];
			char tmp_rtpmaps[256];

			memset(tmp_payloads, 0, sizeof(tmp_payloads));
			memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
			snprintf(tmp_payloads, 64, "%i",
					 _antisipc.text_codecs[i].payload);
			strcat(payloads, " ");
			strcat(payloads, tmp_payloads);

			snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
					 "a=rtpmap:%i %s/%i\r\n",
					 _antisipc.text_codecs[i].payload,
					 _antisipc.text_codecs[i].name,
					 _antisipc.text_codecs[i].freq);
			strcat(rtpmaps, tmp_rtpmaps);

#if 0
			if (osip_strcasecmp(_antisipc.text_codecs[i].name, "t140") ==
				0) {
				if (_antisipc.text_codecs[i].cps > 0) {
					if (_antisipc.text_codecs[i].mode > 0)
						snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
								 "a=fmtp:%i cps=%i\r\n",
								 _antisipc.text_codecs[i].payload,
								 _antisipc.text_codecs[i].cps);
					strcat(rtpmaps, tmp_rtpmaps);
				}
			}
#endif
		}
	}

	if (payloads[0] != '\0')
		strcat(tmp, payloads);

	strcat(tmp, "\r\n");

	if (_antisipc.text_codec_attr.download_bandwidth > 0) {
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
				 _antisipc.text_codec_attr.download_bandwidth);
		strcat(tmp, tmp_b);
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

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256, "a=candidate:%i %i %s %i %s %i typ %s\r\n",
					 ca->text_candidate.local_candidates[pos].foundation,
					 ca->text_candidate.local_candidates[pos].component_id,
					 ca->text_candidate.local_candidates[pos].transport,
					 ca->text_candidate.local_candidates[pos].priority,
					 ca->text_candidate.local_candidates[pos].conn_addr,
					 ca->text_candidate.local_candidates[pos].conn_port,
					 ca->text_candidate.local_candidates[pos].cand_type);
			strcat(tmp, conn);
		}

		/* add all stun ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->text_candidate.stun_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->text_candidate.stun_candidates[pos].foundation,
					 ca->text_candidate.stun_candidates[pos].component_id,
					 ca->text_candidate.stun_candidates[pos].transport,
					 ca->text_candidate.stun_candidates[pos].priority,
					 ca->text_candidate.stun_candidates[pos].conn_addr,
					 ca->text_candidate.stun_candidates[pos].conn_port,
					 ca->text_candidate.stun_candidates[pos].cand_type,
					 ca->text_candidate.stun_candidates[pos].rel_addr,
					 ca->text_candidate.stun_candidates[pos].rel_port);
			strcat(tmp, conn);
		}

		/* add all relay ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->text_candidate.relay_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->text_candidate.relay_candidates[pos].foundation,
					 ca->text_candidate.relay_candidates[pos].component_id,
					 ca->text_candidate.relay_candidates[pos].transport,
					 ca->text_candidate.relay_candidates[pos].priority,
					 ca->text_candidate.relay_candidates[pos].conn_addr,
					 ca->text_candidate.relay_candidates[pos].conn_port,
					 ca->text_candidate.relay_candidates[pos].cand_type,
					 ca->text_candidate.relay_candidates[pos].rel_addr,
					 ca->text_candidate.relay_candidates[pos].rel_port);
			strcat(tmp, conn);
		}
	}

	strcat(tmp, rtpmaps);
	if (_antisipc.enable_sdpsetupparameter>0) {
		strcat(tmp, "a=setup:");
		strcat(tmp, setup);
		strcat(tmp, "\r\n");
	}

	if (0 == osip_strcasecmp(text_profile, "RTP/SAVP")
		|| _antisipc.optionnal_encryption != 0) {
		int idx;
		int tag=1;
		for (idx=0;idx<10;idx++) {
			if (_antisipc.srtp_info[idx].srtp_algo > 0 && _antisipc.srtp_info[idx].enable>0)
			{
				char b64_text[41];
				char key[31];
				strcat(tmp, "a=crypto:");
				snprintf(key, sizeof(key), "%i ", tag);
				strcat(tmp, key);
				tag++;
				if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80)
					strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_32)
					strcat(tmp, "AES_CM_128_HMAC_SHA1_32 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_NULL_AUTH)
					strcat(tmp, "AES_CM_128_NULL_AUTH inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_NULL_CIPHER_HMAC_SHA1_80)
					strcat(tmp, "NULL_CIPHER_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_80)
					strcat(tmp, "AES_CM_256_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_32)
					strcat(tmp, "AES_CM_256_HMAC_SHA1_32 inline:");
				else
					strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
				memset(b64_text, 0, sizeof(b64_text));
				memset(key, 0, sizeof(key));
				eXosip_generate_random(key, 16);
				am_hexa_generate_random(key, 31, key, "key", "crypto");
				am_base64_encode(b64_text, key, 30);

				strcat(tmp, b64_text);
				/* strcat(tmp, "|2^20"); */
				strcat(tmp, "\r\n");
			}
		}
	}
	if (_antisipc.optionnal_encryption != 0)
		strcat(tmp, "a=encryption:optional\r\n");

	if (_antisipc.add_nortpproxy != 0)
		strcat(tmp, "a=nortpproxy:yes\r\n");

	ca->versionid++;
	return OSIP_SUCCESS;
}


static int am_session_add_udpftp_offer(am_call_t * ca, char *tmp)
{
	char port[10];
	char rtpmaps[2048];
	char payloads[128];
	char setup[64];
	char udpftp_profile[32];
	char localip[128];

	int _port = 0;
	int i;

	tmp[0] = '\0';

	strncpy(udpftp_profile, _antisipc.udpftp_profile,
			sizeof(udpftp_profile));

	strncpy(setup, "passive", sizeof(setup));
	if (ca == NULL) {
		return OSIP_BADPARAMETER;
	}

	_port = ca->udpftp_candidate.stun_candidates->rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_port = ca->udpftp_candidate.stun_candidates->conn_port;
	if (_port < 1024)
		_port = ca->udpftp_candidate.stun_candidates->rel_port;

	if (_antisipc.use_relay_server != 0
		&& ca->udpftp_candidate.relay_candidates->conn_addr[0] != '\0') {
		_port = ca->udpftp_candidate.relay_candidates->conn_port;
	}

	if (_antisipc.stuntest.use_setup_both != 0)
		strncpy(setup, "both", sizeof(setup));

	snprintf(port, 10, "%i", _port);

	eXosip_guess_localip(AF_INET, localip, 128);

	if (_antisipc.use_relay_server != 0
		&& ca->udpftp_candidate.relay_candidates->conn_addr[0] != '\0') {
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n",
				 ca->versionid,
				 ca->udpftp_candidate.relay_candidates->conn_addr,
				 ca->udpftp_candidate.relay_candidates->conn_addr);
	} else if (_antisipc.use_turn_server != 0
			   && _antisipc.stun_firewall[0] != '\0') {
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
	} else if (_antisipc.stun_firewall[0] != '\0'
			   && _antisipc.stuntest.use_stun_mapped_ip != 0)
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n"
				 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
	else
		snprintf(tmp, 4096,
				 "v=0\r\n"
				 "o=amsip 0 %i IN IP4 %s\r\n"
				 "s=talk\r\n"
				 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip, localip);

	if (_antisipc.use_turn_server != 0) {
		strcat(tmp, "a=ice-pwd:");
		strcat(tmp, ca->ice_pwd);
		strcat(tmp, "\r\n");
		strcat(tmp, "a=ice-ufrag:");
		strcat(tmp, ca->ice_ufrag);
		strcat(tmp, "\r\n");
	}

	strcat(tmp, "m=x-udpftp ");
	strcat(tmp, port);
	strcat(tmp, " ");
	strcat(tmp, udpftp_profile);

	memset(rtpmaps, 0, sizeof(rtpmaps));
	memset(payloads, 0, sizeof(payloads));
	for (i = 0; i < 5; i++) {
		if (ms_filter_codec_supported(_antisipc.udpftp_codecs[i].name)
			&& _antisipc.udpftp_codecs[i].enable > 0) {
			char tmp_payloads[64];
			char tmp_rtpmaps[256];

			memset(tmp_payloads, 0, sizeof(tmp_payloads));
			memset(tmp_rtpmaps, 0, sizeof(tmp_rtpmaps));
			snprintf(tmp_payloads, 64, "%i",
					 _antisipc.udpftp_codecs[i].payload);
			strcat(payloads, " ");
			strcat(payloads, tmp_payloads);

			snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
					 "a=rtpmap:%i %s/%i\r\n",
					 _antisipc.udpftp_codecs[i].payload,
					 _antisipc.udpftp_codecs[i].name,
					 _antisipc.udpftp_codecs[i].freq);
			strcat(rtpmaps, tmp_rtpmaps);

#if 0
			if (osip_strcasecmp(_antisipc.udpftp_codecs[i].name, "t140") ==
				0) {
				if (_antisipc.udpftp_codecs[i].cps > 0) {
					if (_antisipc.udpftp_codecs[i].mode > 0)
						snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
								 "a=fmtp:%i cps=%i\r\n",
								 _antisipc.udpftp_codecs[i].payload,
								 _antisipc.udpftp_codecs[i].cps);
					strcat(rtpmaps, tmp_rtpmaps);
				}
			}
#endif
		}
	}

	if (payloads[0] != '\0')
		strcat(tmp, payloads);

	strcat(tmp, "\r\n");

	if (_antisipc.udpftp_codec_attr.download_bandwidth > 0) {
		char tmp_b[256];

		memset(tmp_b, 0, sizeof(tmp_b));
		snprintf(tmp_b, sizeof(tmp_b), "b=AS:%i\r\n",
				 _antisipc.udpftp_codec_attr.download_bandwidth);
		strcat(tmp, tmp_b);
	}

	if (_antisipc.use_turn_server != 0) {
		char conn[256];
		int pos;

		/* add all stun and local ip candidates */

		/* add all local ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->udpftp_candidate.local_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256, "a=candidate:%i %i %s %i %s %i typ %s\r\n",
					 ca->udpftp_candidate.local_candidates[pos].foundation,
					 ca->udpftp_candidate.local_candidates[pos].
					 component_id,
					 ca->udpftp_candidate.local_candidates[pos].transport,
					 ca->udpftp_candidate.local_candidates[pos].priority,
					 ca->udpftp_candidate.local_candidates[pos].conn_addr,
					 ca->udpftp_candidate.local_candidates[pos].conn_port,
					 ca->udpftp_candidate.local_candidates[pos].cand_type);
			strcat(tmp, conn);
		}

		/* add all stun ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->udpftp_candidate.stun_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->udpftp_candidate.stun_candidates[pos].foundation,
					 ca->udpftp_candidate.stun_candidates[pos].
					 component_id,
					 ca->udpftp_candidate.stun_candidates[pos].transport,
					 ca->udpftp_candidate.stun_candidates[pos].priority,
					 ca->udpftp_candidate.stun_candidates[pos].conn_addr,
					 ca->udpftp_candidate.stun_candidates[pos].conn_port,
					 ca->udpftp_candidate.stun_candidates[pos].cand_type,
					 ca->udpftp_candidate.stun_candidates[pos].rel_addr,
					 ca->udpftp_candidate.stun_candidates[pos].rel_port);
			strcat(tmp, conn);
		}

		/* add all relay ip candidates */
		for (pos = 0; pos < 10; pos++) {
			if (ca->udpftp_candidate.relay_candidates[pos].conn_addr[0] ==
				'\0')
				break;

			memset(conn, '\0', sizeof(conn));
			snprintf(conn, 256,
					 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
					 ca->udpftp_candidate.relay_candidates[pos].foundation,
					 ca->udpftp_candidate.relay_candidates[pos].
					 component_id,
					 ca->udpftp_candidate.relay_candidates[pos].transport,
					 ca->udpftp_candidate.relay_candidates[pos].priority,
					 ca->udpftp_candidate.relay_candidates[pos].conn_addr,
					 ca->udpftp_candidate.relay_candidates[pos].conn_port,
					 ca->udpftp_candidate.relay_candidates[pos].cand_type,
					 ca->udpftp_candidate.relay_candidates[pos].rel_addr,
					 ca->udpftp_candidate.relay_candidates[pos].rel_port);
			strcat(tmp, conn);
		}
	}

	strcat(tmp, rtpmaps);
	if (_antisipc.enable_sdpsetupparameter>0) {
		strcat(tmp, "a=setup:");
		strcat(tmp, setup);
		strcat(tmp, "\r\n");
	}

	if (0 == osip_strcasecmp(udpftp_profile, "RTP/SAVP")
		|| _antisipc.optionnal_encryption != 0) {
		int idx;
		int tag=1;
		for (idx=0;idx<10;idx++) {
			if (_antisipc.srtp_info[idx].srtp_algo > 0 && _antisipc.srtp_info[idx].enable>0)
			{
				char b64_text[41];
				char key[31];
				strcat(tmp, "a=crypto:");
				snprintf(key, sizeof(key), "%i ", tag);
				strcat(tmp, key);
				tag++;
				if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80)
					strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_32)
					strcat(tmp, "AES_CM_128_HMAC_SHA1_32 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_NULL_AUTH)
					strcat(tmp, "AES_CM_128_NULL_AUTH inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_NULL_CIPHER_HMAC_SHA1_80)
					strcat(tmp, "NULL_CIPHER_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_80)
					strcat(tmp, "AES_CM_256_HMAC_SHA1_80 inline:");
				else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_32)
					strcat(tmp, "AES_CM_256_HMAC_SHA1_32 inline:");
				else
					strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
				memset(b64_text, 0, sizeof(b64_text));
				memset(key, 0, sizeof(key));
				eXosip_generate_random(key, 16);
				am_hexa_generate_random(key, 31, key, "key", "crypto");
				am_base64_encode(b64_text, key, 30);

				strcat(tmp, b64_text);
				/* strcat(tmp, "|2^20"); */
				strcat(tmp, "\r\n");
			}
		}
	}
	if (_antisipc.optionnal_encryption != 0)
		strcat(tmp, "a=encryption:optional\r\n");

	if (_antisipc.add_nortpproxy != 0)
		strcat(tmp, "a=nortpproxy:yes\r\n");

	ca->versionid++;
	return OSIP_SUCCESS;
}

PPL_DECLARE (int)
am_session_start(const char *identity, const char *url, const char *proxy,
				 const char *outbound_proxy)
{
	return am_session_add_in_conference(identity, url, proxy,
										outbound_proxy, NULL);
}

#ifdef ENABLE_VIDEO
PPL_DECLARE (int)
am_session_start_with_video(const char *identity, const char *url,
							const char *proxy, const char *outbound_proxy)
{
	osip_message_t *invite;
	int i;

	am_call_t *ca;

	osip_from_t *to = NULL;

	char route[256];

	memset(route, '\0', sizeof(route));

	if (proxy == NULL || proxy[0] == '\0')
		return OSIP_BADPARAMETER;	/* there must be a proxy right now */

	if (url == NULL || url[0] == '\0')
		return OSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	/* check if we want to use the route or not */

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */

	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */
	}

	eXosip_lock();
	ca = _am_calls_new_audio_connection();
	if (ca == NULL) {
		osip_from_free(to);
		eXosip_unlock();
		return AMSIP_TOOMUCHCALL;
	}

	/* TODO: handle outbound proxy */
	i = eXosip_call_build_initial_invite(&invite, url, identity, route,
										 "Talk");

	osip_from_free(to);

	if (i != 0) {
		ca->state = NOT_USED;
		eXosip_unlock();
		return i;
	}

	if (_antisipc.gruu_contact[0] != '\0') {
		/* replace with the gruu contact */
		osip_contact_t *co = NULL;
		osip_uri_t *uri_new = NULL;

		i = osip_uri_init(&uri_new);
		if (uri_new != NULL) {
			char *gruu_contact = osip_strdup(_antisipc.gruu_contact);

			i = -1;
			if (gruu_contact != NULL) {
				i = osip_uri_parse(uri_new, gruu_contact);
				osip_free(gruu_contact);
			}
			if (i == 0 && uri_new->host != NULL) {
				i = osip_message_get_contact(invite, 0, &co);
				if (i >= 0 && co != NULL && co->url != NULL) {
					osip_uri_free(co->url);
					co->url = NULL;
				}
				if (i >= 0 && co != NULL) {
					co->url = uri_new;
				} else
					osip_uri_free(uri_new);
			} else
				osip_uri_free(uri_new);
		}
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);
	if (_antisipc.allowed_methods[0] != '\0')
		osip_message_set_allow(invite, _antisipc.allowed_methods);
	if (_antisipc.allowed_events[0] != '\0')
		osip_message_set_header(invite, "Allow-Events", _antisipc.allowed_events);

	/* add sdp body */
	if (!_antisipc.do_sdp_in_ack) {
		sdp_message_t *local_sdp;
		char tmp[4096];
		char *local_tmp;

		i = am_session_add_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(invite);
			eXosip_unlock();
			return i;
		}
		i = sdp_message_init(&local_sdp);
		if (i != 0) {
			osip_message_free(invite);
			eXosip_unlock();
			return i;
		}
		i = sdp_message_parse(local_sdp, tmp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			eXosip_unlock();
			return i;
		}
		i = _sdp_add_video(ca, local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			eXosip_unlock();
			return i;
		}
		sdp_message_to_str(local_sdp, &local_tmp);

		osip_message_set_body(invite, local_tmp, strlen(local_tmp));
		osip_free(local_tmp);
		osip_message_set_content_type(invite, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = local_sdp;
#else
		sdp_message_free(local_sdp);
#endif
	}

	if (_antisipc.enable_p_am_sessiontype>0)
	{
		osip_message_set_header(invite, "P-AM-ST", "audio;video");
		snprintf(ca->p_am_sessiontype, sizeof(ca->p_am_sessiontype), "audio;video");
	}

	i = eXosip_call_send_initial_invite(invite);
	if (i > 0) {
		ca->cid = i;

		/* avoid ICMP/be prepare to play data */
		if (ca->local_sdp!=NULL)
		{
			char offer_ip[256];
			int offer_port = 0;
			int k;
			_calls_get_ip_port(ca->local_sdp, "audio", offer_ip, &offer_port);
			k = _antisipc.audio_media->audio_module_sound_start(ca, NULL,
																ca->local_sdp,
																ca->local_sdp,
																NULL,
																offer_port,
																NULL,
																0,
																0);
			if (k == 0) {
				ca->enable_audio = 1;	/* audio is started */
			}
		}
	} else {
		ca->state = NOT_USED;
	}
	eXosip_unlock();
	return i;
}
#endif

PPL_DECLARE (int)
am_session_start_text_conversation(const char *identity, const char *url,
								   const char *proxy,
								   const char *outbound_proxy)
{
	osip_message_t *invite;
	int i;

	am_call_t *ca;

	osip_from_t *to = NULL;

	char route[256];

	memset(route, '\0', sizeof(route));

	if (proxy == NULL || proxy[0] == '\0')
		return AMSIP_BADPARAMETER;	/* there must be a proxy right now */

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	/* check if we want to use the route or not */

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */

	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */
	}

	eXosip_lock();
	ca = _am_calls_new_audio_connection();
	if (ca == NULL) {
		osip_from_free(to);
		eXosip_unlock();
		return AMSIP_TOOMUCHCALL;
	}

	/* TODO: handle outbound proxy */
	i = eXosip_call_build_initial_invite(&invite, url, identity, route,
										 "Talk");

	osip_from_free(to);

	if (i != 0) {
		ca->state = NOT_USED;
		eXosip_unlock();
		return i;
	}

	if (_antisipc.gruu_contact[0] != '\0') {
		/* replace with the gruu contact */
		osip_contact_t *co = NULL;
		osip_uri_t *uri_new = NULL;

		i = osip_uri_init(&uri_new);
		if (uri_new != NULL) {
			char *gruu_contact = osip_strdup(_antisipc.gruu_contact);

			i = -1;
			if (gruu_contact != NULL) {
				i = osip_uri_parse(uri_new, gruu_contact);
				osip_free(gruu_contact);
			}
			if (i == 0 && uri_new->host != NULL) {
				i = osip_message_get_contact(invite, 0, &co);
				if (i >= 0 && co != NULL && co->url != NULL) {
					osip_uri_free(co->url);
					co->url = NULL;
				}
				if (i >= 0 && co != NULL) {
					co->url = uri_new;
				} else
					osip_uri_free(uri_new);
			} else
				osip_uri_free(uri_new);
		}
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);
	if (_antisipc.allowed_methods[0] != '\0')
		osip_message_set_allow(invite, _antisipc.allowed_methods);
	if (_antisipc.allowed_events[0] != '\0')
		osip_message_set_header(invite, "Allow-Events", _antisipc.allowed_events);

	/* add sdp body */
	if (!_antisipc.do_sdp_in_ack) {
		char tmp[4096];

		i = am_session_add_text_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(invite);
			return i;
		}

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_message_set_content_type(invite, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(invite);
#else
		sdp_message_free(local_sdp);
#endif
	}

	if (_antisipc.enable_p_am_sessiontype>0)
	{
		osip_message_set_header(invite, "P-AM-ST", "chat");
		snprintf(ca->p_am_sessiontype, sizeof(ca->p_am_sessiontype), "chat");
	}

	i = eXosip_call_send_initial_invite(invite);
	if (i > 0) {
		ca->cid = i;
	} else {
		ca->state = NOT_USED;
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_start_file_transfer(const char *identity, const char *url,
							   const char *proxy,
							   const char *outbound_proxy)
{
	osip_message_t *invite;
	int i;

	am_call_t *ca;

	osip_from_t *to = NULL;

	char route[256];

	memset(route, '\0', sizeof(route));

	if (proxy == NULL || proxy[0] == '\0')
		return AMSIP_BADPARAMETER;	/* there must be a proxy right now */

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	/* check if we want to use the route or not */

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */

	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */
	}

	eXosip_lock();
	ca = _am_calls_new_audio_connection();
	if (ca == NULL) {
		osip_from_free(to);
		eXosip_unlock();
		return AMSIP_TOOMUCHCALL;
	}

	/* TODO: handle outbound proxy */
	i = eXosip_call_build_initial_invite(&invite, url, identity, route,
										 "Talk");

	osip_from_free(to);

	if (i != 0) {
		ca->state = NOT_USED;
		eXosip_unlock();
		return i;
	}

	if (_antisipc.gruu_contact[0] != '\0') {
		/* replace with the gruu contact */
		osip_contact_t *co = NULL;
		osip_uri_t *uri_new = NULL;

		i = osip_uri_init(&uri_new);
		if (uri_new != NULL) {
			char *gruu_contact = osip_strdup(_antisipc.gruu_contact);

			i = -1;
			if (gruu_contact != NULL) {
				i = osip_uri_parse(uri_new, gruu_contact);
				osip_free(gruu_contact);
			}
			if (i == 0 && uri_new->host != NULL) {
				i = osip_message_get_contact(invite, 0, &co);
				if (i >= 0 && co != NULL && co->url != NULL) {
					osip_uri_free(co->url);
					co->url = NULL;
				}
				if (i >= 0 && co != NULL) {
					co->url = uri_new;
				} else
					osip_uri_free(uri_new);
			} else
				osip_uri_free(uri_new);
		}
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);
	if (_antisipc.allowed_methods[0] != '\0')
		osip_message_set_allow(invite, _antisipc.allowed_methods);
	if (_antisipc.allowed_events[0] != '\0')
		osip_message_set_header(invite, "Allow-Events", _antisipc.allowed_events);

	/* add sdp body */
	if (!_antisipc.do_sdp_in_ack) {
		char tmp[4096];

		i = am_session_add_udpftp_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(invite);
			return i;
		}

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_message_set_content_type(invite, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(invite);
#else
		sdp_message_free(local_sdp);
#endif
	}

	if (_antisipc.enable_p_am_sessiontype>0)
	{
		osip_message_set_header(invite, "P-AM-ST", "x-udpftp");
		snprintf(ca->p_am_sessiontype, sizeof(ca->p_am_sessiontype), "x-udpftp");
	}

	i = eXosip_call_send_initial_invite(invite);
	if (i > 0) {
		ca->cid = i;
	} else {
		ca->state = NOT_USED;
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_add_in_conference(const char *identity, const char *url,
							 const char *proxy, const char *outbound_proxy,
							 const char *conf_name)
{
	osip_message_t *invite;
	int i;

	am_call_t *ca;

	osip_from_t *to = NULL;
	char port[10];
	char rtpmaps[2048];
	char payloads[128];
	char setup[64];
	char audio_profile[32];

	int _port = 0;

	char route[256];

	memset(route, '\0', sizeof(route));

	if (proxy == NULL || proxy[0] == '\0')
		return AMSIP_BADPARAMETER;	/* there must be a proxy right now */

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	/* check if we want to use the route or not */

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */

	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
		/* no pre-route set route! */
		if (outbound_proxy != NULL && outbound_proxy[0] != '\0')
			snprintf(route, 256, "%s", outbound_proxy);	/* must include "lr" */
	}

	strncpy(audio_profile, _antisipc.audio_profile, sizeof(audio_profile));

	strncpy(setup, "passive", sizeof(setup));
	eXosip_lock();
	ca = _am_calls_new_audio_connection();
	if (ca == NULL) {
		osip_from_free(to);
		eXosip_unlock();
		return AMSIP_TOOMUCHCALL;
	}

	_port = ca->audio_candidate.stun_candidates->rel_port;
	if (_antisipc.stuntest.use_stun_mapped_port != 0)
		_port = ca->audio_candidate.stun_candidates->conn_port;
	if (_port < 1024)
		_port = ca->audio_candidate.stun_candidates->rel_port;

	if (_antisipc.use_relay_server != 0
		&& ca->audio_candidate.relay_candidates->conn_addr[0] != '\0') {
		_port = ca->audio_candidate.relay_candidates->conn_port;
	}

	if (_antisipc.stuntest.use_setup_both != 0)
		strncpy(setup, "both", sizeof(setup));

	snprintf(port, 10, "%i", _port);

	/* TODO: handle outbound proxy */
	if (conf_name != NULL && conf_name[0] != '\0')
		i = eXosip_call_build_initial_invite(&invite,
											 url, identity, route, "Talk");
	else
		i = eXosip_call_build_initial_invite(&invite,
											 url, identity, route, "Talk");

	osip_from_free(to);

	if (i != 0) {
		ca->state = NOT_USED;
		eXosip_unlock();
		return i;
	}

	if (_antisipc.session_timers>0)
	{
		char session_exp[10];
		memset(session_exp, 0, sizeof(session_exp));
		snprintf(session_exp, sizeof(session_exp)-1, "%i", _antisipc.session_timers);
		osip_message_set_header(invite, "Session-Expires", session_exp);
	}

	if (_antisipc.gruu_contact[0] != '\0') {
		/* replace with the gruu contact */
		osip_contact_t *co = NULL;
		osip_uri_t *uri_new = NULL;

		i = osip_uri_init(&uri_new);
		if (uri_new != NULL) {
			char *gruu_contact = osip_strdup(_antisipc.gruu_contact);

			i = -1;
			if (gruu_contact != NULL) {
				i = osip_uri_parse(uri_new, gruu_contact);
				osip_free(gruu_contact);
			}
			if (i == 0 && uri_new->host != NULL) {
				i = osip_message_get_contact(invite, 0, &co);
				if (i >= 0 && co != NULL && co->url != NULL) {
					osip_uri_free(co->url);
					co->url = NULL;
				}
				if (i >= 0 && co != NULL) {
					co->url = uri_new;
				} else
					osip_uri_free(uri_new);
			} else
				osip_uri_free(uri_new);
		}
	}

	if (conf_name != NULL && conf_name[0] != '\0') {
		osip_contact_t *co = NULL;

		i = osip_message_get_contact(invite, 0, &co);
		if (i < 0 || co == NULL || co->url == NULL) {
			ca->state = NOT_USED;
			osip_message_free(invite);
			eXosip_unlock();
			return i;
		}
		/* replace username with conference name */

		if (_antisipc.gruu_contact[0] != '\0')
			osip_uri_uparam_add(co->url, osip_strdup("grid"),
								osip_strdup(conf_name));
		else {
			if (co->url->username != NULL)
				osip_free(co->url->username);
			co->url->username = osip_strdup(conf_name);
		}
		osip_contact_param_add(co, osip_strdup("isfocus"), NULL);
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);
	if (_antisipc.allowed_methods[0] != '\0')
		osip_message_set_allow(invite, _antisipc.allowed_methods);
	if (_antisipc.allowed_events[0] != '\0')
		osip_message_set_header(invite, "Allow-Events", _antisipc.allowed_events);

	/* add sdp body */
	if (!_antisipc.do_sdp_in_ack) {
		char tmp[4096];
		char localip[128];

		eXosip_guess_localip(AF_INET, localip, 128);

		if (_antisipc.use_relay_server != 0
			&& ca->audio_candidate.relay_candidates->conn_addr[0] != '\0')
		{
			snprintf(tmp, 4096,
					 "v=0\r\n"
					 "o=amsip 0 %i IN IP4 %s\r\n"
					 "s=talk\r\n"
					 "c=IN IP4 %s\r\n"
					 "t=0 0\r\n",
					 ca->versionid,
					 ca->audio_candidate.relay_candidates->conn_addr,
					 ca->audio_candidate.relay_candidates->conn_addr);
		} else if (_antisipc.use_turn_server != 0
				   && _antisipc.stun_firewall[0] != '\0') {
			snprintf(tmp, 4096,
					 "v=0\r\n"
					 "o=amsip 0 %i IN IP4 %s\r\n"
					 "s=talk\r\n"
					 "c=IN IP4 %s\r\n"
					 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
		} else if (_antisipc.stun_firewall[0] != '\0'
				   && _antisipc.stuntest.use_stun_mapped_ip != 0)
			snprintf(tmp, 4096,
					 "v=0\r\n"
					 "o=amsip 0 %i IN IP4 %s\r\n"
					 "s=talk\r\n"
					 "c=IN IP4 %s\r\n"
					 "t=0 0\r\n", ca->versionid, localip, _antisipc.stun_firewall);
		else
			snprintf(tmp, 4096,
					 "v=0\r\n"
					 "o=amsip 0 %i IN IP4 %s\r\n"
					 "s=talk\r\n"
					 "c=IN IP4 %s\r\n" "t=0 0\r\n", ca->versionid, localip, localip);

		if (_antisipc.use_turn_server != 0) {
			strcat(tmp, "a=ice-pwd:");
			strcat(tmp, ca->ice_pwd);
			strcat(tmp, "\r\n");
			strcat(tmp, "a=ice-ufrag:");
			strcat(tmp, ca->ice_ufrag);
			strcat(tmp, "\r\n");
		}

		strcat(tmp, "m=audio ");
		strcat(tmp, port);
		strcat(tmp, " ");
		strcat(tmp, audio_profile);

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

				if (osip_strcasecmp(_antisipc.codecs[i].name, "AMR") == 0
					||osip_strcasecmp(_antisipc.codecs[i].name, "AMR-WB") == 0) {

					if ((_antisipc.codecs[i].mode&0x70) == 0)
					{
						snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
								 "a=fmtp:%i octet-align=1\r\n",
								 _antisipc.codecs[i].payload);
					}
					else
					{
						snprintf(tmp_rtpmaps, sizeof(tmp_rtpmaps),
								 "a=fmtp:%i octet-align=%i;interleaving=%i\r\n",
								 _antisipc.codecs[i].payload,
								 (_antisipc.codecs[i].mode&0x10)==0x00?1:0,
								 (_antisipc.codecs[i].mode&0x20)==0x20?1:0);
					}
					strcat(rtpmaps, tmp_rtpmaps);
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

		if (_antisipc.use_turn_server != 0) {
			char conn[256];
			int pos;

			/* add all stun and local ip candidates */

			/* add all local ip candidates */
			for (pos = 0; pos < 10; pos++) {
				if (ca->audio_candidate.local_candidates[pos].
					conn_addr[0] == '\0')
					break;

				memset(conn, '\0', sizeof(conn));
				snprintf(conn, 256,
						 "a=candidate:%i %i %s %i %s %i typ %s\r\n",
						 ca->audio_candidate.local_candidates[pos].
						 foundation,
						 ca->audio_candidate.local_candidates[pos].
						 component_id,
						 ca->audio_candidate.local_candidates[pos].
						 transport,
						 ca->audio_candidate.local_candidates[pos].
						 priority,
						 ca->audio_candidate.local_candidates[pos].
						 conn_addr,
						 ca->audio_candidate.local_candidates[pos].
						 conn_port,
						 ca->audio_candidate.local_candidates[pos].
						 cand_type);
				strcat(tmp, conn);
			}

			/* add all stun ip candidates */
			for (pos = 0; pos < 10; pos++) {
				if (ca->audio_candidate.stun_candidates[pos].
					conn_addr[0] == '\0')
					break;

				memset(conn, '\0', sizeof(conn));
				snprintf(conn, 256,
						 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
						 ca->audio_candidate.stun_candidates[pos].
						 foundation,
						 ca->audio_candidate.stun_candidates[pos].
						 component_id,
						 ca->audio_candidate.stun_candidates[pos].
						 transport,
						 ca->audio_candidate.stun_candidates[pos].priority,
						 ca->audio_candidate.stun_candidates[pos].
						 conn_addr,
						 ca->audio_candidate.stun_candidates[pos].
						 conn_port,
						 ca->audio_candidate.stun_candidates[pos].
						 cand_type,
						 ca->audio_candidate.stun_candidates[pos].rel_addr,
						 ca->audio_candidate.stun_candidates[pos].
						 rel_port);
				strcat(tmp, conn);
			}

			/* add all relay ip candidates */
			for (pos = 0; pos < 10; pos++) {
				if (ca->audio_candidate.relay_candidates[pos].
					conn_addr[0] == '\0')
					break;

				memset(conn, '\0', sizeof(conn));
				snprintf(conn, 256,
						 "a=candidate:%i %i %s %i %s %i typ %s raddr %s rport %i\r\n",
						 ca->audio_candidate.relay_candidates[pos].
						 foundation,
						 ca->audio_candidate.relay_candidates[pos].
						 component_id,
						 ca->audio_candidate.relay_candidates[pos].
						 transport,
						 ca->audio_candidate.relay_candidates[pos].
						 priority,
						 ca->audio_candidate.relay_candidates[pos].
						 conn_addr,
						 ca->audio_candidate.relay_candidates[pos].
						 conn_port,
						 ca->audio_candidate.relay_candidates[pos].
						 cand_type,
						 ca->audio_candidate.relay_candidates[pos].
						 rel_addr,
						 ca->audio_candidate.relay_candidates[pos].
						 rel_port);
				strcat(tmp, conn);
			}
		}

		strcat(tmp, rtpmaps);
		strcat(tmp, "a=rtpmap:101 telephone-event/8000\r\n");
		strcat(tmp, "a=fmtp:101 0-11\r\n");
		if (_antisipc.enable_sdpsetupparameter>0) {
			strcat(tmp, "a=setup:");
			strcat(tmp, setup);
			strcat(tmp, "\r\n");
		}

		if (_antisipc.codec_attr.ptime > 0) {
			char tmp_ptime[256];

			memset(tmp_ptime, 0, sizeof(tmp_ptime));
			snprintf(tmp_ptime, sizeof(tmp_ptime), "a=ptime:%i\r\n",
					 _antisipc.codec_attr.ptime);
			strcat(tmp, tmp_ptime);
		}

		if (0 == osip_strcasecmp(audio_profile, "RTP/SAVP")
			|| _antisipc.optionnal_encryption != 0) {
			int idx;
			int tag=1;
			for (idx=0;idx<10;idx++) {
				if (_antisipc.srtp_info[idx].srtp_algo > 0 && _antisipc.srtp_info[idx].enable>0)
				{
					char b64_text[41];
					char key[31];
					strcat(tmp, "a=crypto:");
					snprintf(key, sizeof(key), "%i ", tag);
					strcat(tmp, key);
					tag++;
					if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80)
						strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
					else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_HMAC_SHA1_32)
						strcat(tmp, "AES_CM_128_HMAC_SHA1_32 inline:");
					else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_128_NULL_AUTH)
						strcat(tmp, "AES_CM_128_NULL_AUTH inline:");
					else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_NULL_CIPHER_HMAC_SHA1_80)
						strcat(tmp, "NULL_CIPHER_HMAC_SHA1_80 inline:");
					else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_80)
						strcat(tmp, "AES_CM_256_HMAC_SHA1_80 inline:");
					else if (_antisipc.srtp_info[idx].srtp_algo == AMSIP_ALGO_AES_CM_256_HMAC_SHA1_32)
						strcat(tmp, "AES_CM_256_HMAC_SHA1_32 inline:");
					else
						strcat(tmp, "AES_CM_128_HMAC_SHA1_80 inline:");
					memset(b64_text, 0, sizeof(b64_text));
					memset(key, 0, sizeof(key));
					eXosip_generate_random(key, 16);
					am_hexa_generate_random(key, 31, key, "key", "crypto");
					am_base64_encode(b64_text, key, 30);

					strcat(tmp, b64_text);
					/* strcat(tmp, "|2^20"); */
					/* strcat(tmp, " WSH=256"); */
					strcat(tmp, "\r\n");
				}
			}
		}
		if (_antisipc.optionnal_encryption != 0)
			strcat(tmp, "a=encryption:optional\r\n");

		if (_antisipc.add_nortpproxy != 0)
			strcat(tmp, "a=nortpproxy:yes\r\n");

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_message_set_content_type(invite, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(invite);
#endif
		ca->versionid++;
	}

	if (_antisipc.enable_p_am_sessiontype>0)
	{
		osip_message_set_header(invite, "P-AM-ST", "audio");
		snprintf(ca->p_am_sessiontype, sizeof(ca->p_am_sessiontype), "audio");
	}

	i = eXosip_call_send_initial_invite(invite);
	if (i > 0) {
		ca->cid = i;

		/* avoid ICMP/be prepare to play data */
		if (ca->local_sdp!=NULL)
		{
			char offer_ip[256];
			int offer_port = 0;
			int k;
			_calls_get_ip_port(ca->local_sdp, "audio", offer_ip, &offer_port);
			k = _antisipc.audio_media->audio_module_sound_start(ca, NULL,
																ca->local_sdp,
																ca->local_sdp,
																NULL,
																offer_port,
																NULL,
																0,
																0);
			if (k == 0) {
				ca->enable_audio = 1;	/* audio is started */
			}
		}

	} else {
		ca->state = NOT_USED;
	}
	eXosip_unlock();
	return i;
}


PPL_DECLARE (int)
am_session_conference_answer(int conf_id, int tid, int did, int code, int enable_audio)
{
	osip_message_t *answer = NULL;
	sdp_message_t *sdp = NULL;
	am_call_t *ca;
	int i;

	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;

	eXosip_lock();

	ca = _am_calls_find_audio_connection(tid, did);

	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	ca->conf_id = conf_id;

	i = eXosip_call_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_call_send_answer(tid, 400, NULL);
		eXosip_unlock();
		return i;
	}

	if (code > 100 && code < 300)
	{
		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(answer, _antisipc.supported_extensions);
		if (_antisipc.allowed_methods[0] != '\0')
			osip_message_set_allow(answer, _antisipc.allowed_methods);
		if (_antisipc.allowed_events[0] != '\0')
			osip_message_set_header(answer, "Allow-Events", _antisipc.allowed_events);
	}

	if (enable_audio == 0) {
		i = eXosip_call_send_answer(tid, code, answer);

		eXosip_unlock();
		return i;
	}

	if (code == 183) {
		osip_message_set_require(answer, "100rel");
		osip_message_set_header(answer, "RSeq", "1");
	}

	sdp = eXosip_get_remote_sdp(did);
	if (sdp == NULL) {
		char tmp[4096];

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Add offer in 200ok\n"));
		i = am_session_add_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}
		osip_message_set_body(answer, tmp, strlen(tmp));
		osip_message_set_content_type(answer, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif
	} else {
		sdp_message_free(sdp);
		i = sdp_complete_200ok(ca, answer);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}
#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif

		if (enable_audio != 0)
			_calls_start_audio_with_id(tid, did, answer);
#ifdef ENABLE_VIDEO
		if (code >= 200 && code <= 299)
			_calls_start_video_with_id(tid, did, answer);
#endif
		if (code >= 200 && code <= 299)
			_calls_start_text_with_id(tid, did, answer);
		if (code >= 200 && code <= 299)
			_calls_start_udpftp_with_id(tid, did, answer);
	}

	i = eXosip_call_send_answer(tid, code, answer);

	eXosip_unlock();
	return i;
}


PPL_DECLARE (int)
am_session_answer(int tid, int did, int code, int enable_audio)
{
	osip_message_t *answer = NULL;
	sdp_message_t *sdp = NULL;
	am_call_t *ca;
	int i;

	eXosip_lock();

	ca = _am_calls_find_audio_connection(tid, did);

	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = eXosip_call_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_call_send_answer(tid, 400, NULL);
		eXosip_unlock();
		return i;
	}

	if (code > 100 && code < 300)
	{
		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(answer, _antisipc.supported_extensions);
		if (_antisipc.allowed_methods[0] != '\0')
			osip_message_set_allow(answer, _antisipc.allowed_methods);
		if (_antisipc.allowed_events[0] != '\0')
			osip_message_set_header(answer, "Allow-Events", _antisipc.allowed_events);
	}

	if (enable_audio == 0) {
		i = eXosip_call_send_answer(tid, code, answer);

		eXosip_unlock();
		return i;
	}

	if (code == 183) {
		osip_message_set_require(answer, "100rel");
		osip_message_set_header(answer, "RSeq", "1");
	}

	sdp = eXosip_get_remote_sdp(did);
	if (sdp == NULL) {
		char tmp[4096];

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Add offer in 200ok\n"));
		i = am_session_add_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}
		osip_message_set_body(answer, tmp, strlen(tmp));
		osip_message_set_content_type(answer, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif
	} else {
		sdp_message_free(sdp);
		i = sdp_complete_200ok(ca, answer);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}
#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif

		if (enable_audio != 0)
			_calls_start_audio_with_id(tid, did, answer);
#ifdef ENABLE_VIDEO
		if (code >= 200 && code <= 299)
			_calls_start_video_with_id(tid, did, answer);
#endif
		if (code >= 200 && code <= 299)
			_calls_start_text_with_id(tid, did, answer);
		if (code >= 200 && code <= 299)
			_calls_start_udpftp_with_id(tid, did, answer);
	}

	i = eXosip_call_send_answer(tid, code, answer);

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_conference_answer_with_video (int conf_id, int tid, int did, int code, int enable_audio, int enable_video)
{
	osip_message_t *answer = NULL;
	sdp_message_t *sdp = NULL;
	am_call_t *ca;
	int i;

	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;

	eXosip_lock();

	ca = _am_calls_find_audio_connection(tid, did);

	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	ca->conf_id = conf_id;

	i = eXosip_call_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_call_send_answer(tid, 400, NULL);
		eXosip_unlock();
		return i;
	}

	if (code > 100 && code < 300)
	{
		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(answer, _antisipc.supported_extensions);
		if (_antisipc.allowed_methods[0] != '\0')
			osip_message_set_allow(answer, _antisipc.allowed_methods);
		if (_antisipc.allowed_events[0] != '\0')
			osip_message_set_header(answer, "Allow-Events", _antisipc.allowed_events);
	}

	if (enable_audio == 0 && enable_video == 0) {
		i = eXosip_call_send_answer(tid, code, answer);
		i = eXosip_call_send_answer(tid, code, NULL);
		eXosip_unlock();
		return i;
	}

	if (code == 183) {
		osip_message_set_require(answer, "100rel");
		osip_message_set_header(answer, "RSeq", "1");
	}

	sdp = eXosip_get_remote_sdp(did);
	if (sdp == NULL) {
		char tmp[4096];

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Add offer in 200ok\n"));
		i = am_session_add_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}
		osip_message_set_body(answer, tmp, strlen(tmp));
		osip_message_set_content_type(answer, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif
	} else {
		sdp_message_free(sdp);
		i = sdp_complete_200ok(ca, answer);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif

		if (enable_audio > 0)
			_calls_start_audio_with_id(tid, did, answer);
#ifdef ENABLE_VIDEO
		if (enable_video > 0)
			_calls_start_video_with_id(tid, did, answer);
#endif
		if (code >= 200 && code <= 299)
			_calls_start_text_with_id(tid, did, answer);
		if (code >= 200 && code <= 299)
			_calls_start_udpftp_with_id(tid, did, answer);
	}

	i = eXosip_call_send_answer(tid, code, answer);

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_answer_with_video(int tid, int did, int code, int enable_audio, int enable_video)
{
	osip_message_t *answer = NULL;
	sdp_message_t *sdp = NULL;
	am_call_t *ca;
	int i;

	eXosip_lock();

	ca = _am_calls_find_audio_connection(tid, did);

	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = eXosip_call_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_call_send_answer(tid, 400, NULL);
		eXosip_unlock();
		return i;
	}

	if (code > 100 && code < 300)
	{
		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(answer, _antisipc.supported_extensions);
		if (_antisipc.allowed_methods[0] != '\0')
			osip_message_set_allow(answer, _antisipc.allowed_methods);
		if (_antisipc.allowed_events[0] != '\0')
			osip_message_set_header(answer, "Allow-Events", _antisipc.allowed_events);
	}

	if (enable_audio == 0 && enable_video == 0) {
		i = eXosip_call_send_answer(tid, code, answer);
		i = eXosip_call_send_answer(tid, code, NULL);
		eXosip_unlock();
		return i;
	}

	if (code == 183) {
		osip_message_set_require(answer, "100rel");
		osip_message_set_header(answer, "RSeq", "1");
	}

	sdp = eXosip_get_remote_sdp(did);
	if (sdp == NULL) {
		char tmp[4096];

		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"Add offer in 200ok\n"));
		i = am_session_add_offer(ca, tmp);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}
		osip_message_set_body(answer, tmp, strlen(tmp));
		osip_message_set_content_type(answer, "application/sdp");

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif
	} else {
		sdp_message_free(sdp);
		i = sdp_complete_200ok(ca, answer);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 415, NULL);
			eXosip_unlock();
			return i;
		}

#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = eXosip_get_sdp_info(answer);
#endif

		if (enable_audio > 0)
			_calls_start_audio_with_id(tid, did, answer);
#ifdef ENABLE_VIDEO
		if (enable_video > 0)
			_calls_start_video_with_id(tid, did, answer);
#endif
		if (code >= 200 && code <= 299)
			_calls_start_text_with_id(tid, did, answer);
		if (code >= 200 && code <= 299)
			_calls_start_udpftp_with_id(tid, did, answer);
	}

	i = eXosip_call_send_answer(tid, code, answer);

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_redirect(int tid, int did, int code, const char *url)
{
	osip_message_t *answer = NULL;
	int i;

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	eXosip_lock();
	i = eXosip_call_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_call_send_answer(tid, 500, NULL);
	} else {
		char *header = osip_strdup(url);

		i = osip_message_set_contact(answer, header);
		osip_free(header);
		if (i != 0) {
			osip_message_free(answer);
			eXosip_call_send_answer(tid, 500, NULL);
		} else
			i = eXosip_call_send_answer(tid, code, answer);
	}

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_refer(int did, const char *url, const char *referred_by)
{
	osip_message_t *refer;
	char *header;
	int i;


	eXosip_lock();
	header = osip_strdup(url);
	i = eXosip_call_build_refer(did, header, &refer);
	osip_free(header);
	eXosip_unlock();

	if (i != 0)
		return i;

	if (referred_by != NULL)
		osip_message_set_header(refer, "Referred-By", referred_by);

	eXosip_lock();
	i = eXosip_call_send_request(did, refer);
	if (i > 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_get_referto(int did, char *refer_to, size_t refer_to_len)
{
	int i;
	eXosip_lock();
	i = eXosip_call_get_referto(did, refer_to, refer_to_len);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_find_by_replaces(osip_message_t * request)
{
	int i = AMSIP_UNDEFINED_ERROR;
	osip_header_t *header;

	if (request == NULL)
		return AMSIP_BADPARAMETER;

	eXosip_lock();

	osip_message_header_get_byname(request, "replaces", 0, &header);
	if (header != NULL && header->hvalue != NULL)
		i = eXosip_call_find_by_replaces(header->hvalue);

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_send_notify(int did, const char *sub_state,
						const char *content_type, const char *body,
						int size)
{
	osip_message_t *request;
	int i;

	if (sub_state==NULL)
		return AMSIP_BADPARAMETER;
	
	eXosip_lock();

	i = eXosip_call_build_request(did, "NOTIFY", &request);
	if (i == 0) {
		osip_message_set_header(request, "Event", "refer");
		osip_message_set_header(request, "Subscription-State", sub_state);

		if (body != NULL && size > 0)
		{
			if (content_type==NULL)
				osip_message_set_content_type (request, "message/sipfrag;version=2.0");
			else
				osip_message_set_content_type(request, content_type);
		}		
		if (body != NULL && size > 0)
			osip_message_set_body(request, body, size);

		i = eXosip_call_send_request(did, request);
	}

	eXosip_unlock();

	return i;
}

PPL_DECLARE (int) am_session_answer_request(int tid, int did, int code)
{
	osip_message_t *answer = NULL;
	int i;

	eXosip_lock();

	i = eXosip_call_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_call_send_answer(tid, 400, NULL);
		eXosip_unlock();
		return i;
	}

	if (code == 420) {
		/* TODO: add unsupported header */
	}

	i = eXosip_call_send_answer(tid, code, answer);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_send_request(int did, const char *method,
						const char *content_type, const char *body,
						int size)
{
	osip_message_t *request;
	int i;

	eXosip_lock();

	i = eXosip_call_build_request(did, method, &request);
	if (i == 0) {
		if (content_type != NULL)
			osip_message_set_content_type(request, content_type);
		if (body != NULL && size > 0)
			osip_message_set_body(request, body, size);

		i = eXosip_call_send_request(did, request);
	}

	eXosip_unlock();

	return i;
}

PPL_DECLARE (int) am_session_build_req(int did, const char *method, osip_message_t **request)
{
	int i;

	eXosip_lock();

	i = eXosip_call_build_request(did, method, request);
	if (i == 0) {
	}

	eXosip_unlock();

	return i;
}

PPL_DECLARE (int) am_session_send_req(int did, const char *method, osip_message_t *request)
{
	int i=0;

	eXosip_lock();

	if (request==NULL)
		i = eXosip_call_build_request(did, method, &request);
	if (i == 0) {

		i = eXosip_call_send_request(did, request);
	}

	eXosip_unlock();

	return i;
}

PPL_DECLARE (int)
am_session_modify_bitrate(int did, const char *preferred_codec,
						  int compress_more)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	{
		char *tmp = NULL;

		i = _sdp_switch_to_codec(local_sdp, preferred_codec,
								 compress_more);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}

		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

		i = sdp_message_to_str(local_sdp, &tmp);

		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = local_sdp;
#else
		sdp_message_free(local_sdp);
#endif

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_hold(int did, const char *wav_file)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	snprintf(ca->hold_wav_file, 256, "%s", wav_file);

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	{
		char *tmp = NULL;

		i = _sdp_hold_call(local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

#if 0
		if (strstr(ca->remote_useragent, "HiPath") != NULL) {
			/* fix the IP address to 0.0.0.0 and port to 0 */
			sdp_connection_t *conn;
			sdp_media_t *med;

			conn = eXosip_get_connection(local_sdp, "audio");
			if (conn != NULL && conn->c_addr != NULL) {
				osip_free(conn->c_addr);
				conn->c_addr = osip_strdup("0.0.0.0");
			}
			med = eXosip_get_media(local_sdp, "audio");
			if (med != NULL && med->m_port != NULL) {
				osip_free(med->m_port);
				med->m_port = osip_strdup("0");
			}
		}
#endif

		i = sdp_message_to_str(local_sdp, &tmp);
		sdp_message_free(local_sdp);
		if (i != 0) {
			osip_message_free(invite);
			return i;
		}
		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_inactive(int did)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	{
		char *tmp = NULL;

		i = _sdp_inactive_call(local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

#ifdef NORTEL_FIX
		{
			/* fix the IP address to 0.0.0.0 and port to 0 */
			sdp_connection_t *conn;
			sdp_media_t *med;

			conn = eXosip_get_connection(local_sdp, "audio");
			if (conn != NULL && conn->c_addr != NULL) {
				osip_free(conn->c_addr);
				conn->c_addr = osip_strdup("0.0.0.0");
			}
			med = eXosip_get_media(local_sdp, "audio");
			if (med != NULL && med->m_port != NULL) {
				osip_free(med->m_port);
				med->m_port = osip_strdup("0");
			}
		}
#endif

		i = sdp_message_to_str(local_sdp, &tmp);
		sdp_message_free(local_sdp);
		if (i != 0) {
			osip_message_free(invite);
			return i;
		}
		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_inactive_0_0_0_0(int did)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	{
		char *tmp = NULL;

		i = _sdp_inactive_call(local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

		{
			/* fix the IP address to 0.0.0.0 and port to 0 */
			sdp_connection_t *conn;
			sdp_media_t *med;

			conn = eXosip_get_connection(local_sdp, "audio");
			if (conn != NULL && conn->c_addr != NULL) {
				osip_free(conn->c_addr);
				conn->c_addr = osip_strdup("0.0.0.0");
			}
			med = eXosip_get_media(local_sdp, "audio");
			if (med != NULL && med->m_port != NULL) {
				osip_free(med->m_port);
				med->m_port = osip_strdup("0");
			}
		}

		i = sdp_message_to_str(local_sdp, &tmp);
		sdp_message_free(local_sdp);
		if (i != 0) {
			osip_message_free(invite);
			return i;
		}
		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_off_hold(int did)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}
#if 0
	snprintf(port, 10, "%i", 10504);
#endif

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	/* add sdp body */
	{
		char *tmp = NULL;

		i = _sdp_off_hold_call(local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}

		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

		i = sdp_message_to_str(local_sdp, &tmp);

		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = local_sdp;
#else
		sdp_message_free(local_sdp);
#endif

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

#ifdef ENABLE_VIDEO

PPL_DECLARE (int)
am_session_reject_video(int did, int _reject_video)
{
	am_call_t *ca;

	eXosip_lock();

	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	ca->reject_video = _reject_video;

	eXosip_unlock();
	return 0;
}

PPL_DECLARE (int) am_session_add_video(int did)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;
	sdp_media_t *local_med;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	for (i = 0; i < 5; i++) {
		if (_antisipc.video_codecs[i].enable > 0) {
			break;
		}
	}
	if (i == 5) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"no video codec configured\n"));
		eXosip_unlock();
		return AMSIP_API_NOT_INITIALIZED;
	}

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	/* check if video has already been added */
	local_med = eXosip_get_video_media(local_sdp);

	/* reset the reject_video parameter automatically */
	ca->reject_video = 0;

	{
		/* add sdp video body */
		char *tmp = NULL;

		if (local_med != NULL)
			i = _sdp_modify_video(ca, local_sdp);
		else
			i = _sdp_add_video(ca, local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}

		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

		i = sdp_message_to_str(local_sdp, &tmp);

		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = local_sdp;
#else
		sdp_message_free(local_sdp);
#endif

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	if (_antisipc.enable_p_am_sessiontype>0)
	{
		int pos = 0;
		while (!osip_list_eol(&invite->headers, pos)) {
			osip_header_t *head = osip_list_get(&invite->headers, pos);
			if (head != NULL && 0 == osip_strcasecmp(head->hname, "P-AM-ST")) {
				i = osip_list_remove(&invite->headers, pos);
				osip_header_free(head);
				break;
			}
			pos++;
		}

		osip_message_set_header(invite, "P-AM-ST", "audio;video");
		snprintf(ca->p_am_sessiontype, sizeof(ca->p_am_sessiontype), "audio;video");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_stop_video(int did)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;
	sdp_media_t *local_med;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	for (i = 0; i < 5; i++) {
		if (_antisipc.video_codecs[i].enable > 0) {
			break;
		}
	}
	if (i == 5) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, OSIP_WARNING, NULL,
					"no video codec configured\n"));
		eXosip_unlock();
		return AMSIP_API_NOT_INITIALIZED;
	}

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	/* check if video has already been added */
	local_med = eXosip_get_video_media(local_sdp);


	if (local_med == NULL) {
		sdp_message_free(local_sdp);
		osip_message_free(invite);
		return AMSIP_WRONG_STATE;
	}

	{
		/* add sdp video body */
		char *tmp = NULL;

		i = _sdp_disable_video(ca, local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}

		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

		i = sdp_message_to_str(local_sdp, &tmp);
		sdp_message_free(local_sdp);
		if (i != 0) {
			osip_message_free(invite);
			return i;
		}
		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_adapt_video_bitrate(int did, float lost)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to adapt video bitrate\n",
							  did));
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.video_media->
		video_module_session_adapt_video_bitrate(ca, lost);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_get_video_bandwidth(int did,
							   struct am_bandwidth_stats *band_stats)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	memset(band_stats, 0, sizeof(struct am_bandwidth_stats));
	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): can't get video statistics\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.video_media->video_module_session_get_bandwidth_statistics(ca, band_stats);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (OrtpEvent*)
am_session_get_audio_rtp_events(int did)
{
	OrtpEvent *evt=NULL;
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"no call with did (%i): can't get audio rtp events\n",
			did));
		return NULL;
	}

	evt = _antisipc.audio_media->audio_module_session_get_audio_rtp_events(ca);
	eXosip_unlock();
	return evt;
}

PPL_DECLARE (int)
am_session_set_audio_zrtp_sas_verified(int did)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"no call with did (%i): can't set audio ZRTP sas verified\n",
			did));
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->audio_module_session_set_audio_zrtp_sas_verified(ca);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (OrtpEvent*)
am_session_get_video_rtp_events(int did)
{
	OrtpEvent *evt=NULL;
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"no call with did (%i): can't get video rtp events\n",
			did));
		return NULL;
	}

	evt = _antisipc.video_media->video_module_session_get_video_rtp_events(ca);
	eXosip_unlock();
	return evt;
}

PPL_DECLARE (int)
am_session_set_video_zrtp_sas_verified(int did)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
			"no call with did (%i): can't set video ZRTP sas verified\n",
			did));
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.video_media->video_module_session_set_video_zrtp_sas_verified(ca);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_release_rtp_events(OrtpEvent *evt)
{
	ortp_event_destroy(evt);
	return AMSIP_SUCCESS;
}

PPL_DECLARE (int) am_session_send_vfu(int did)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): can't send VFU\n",
							  did));
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.video_media->video_module_session_send_vfu(ca);
	eXosip_unlock();
	return i;
}
#endif

PPL_DECLARE (int) am_session_add_t140(int did)
{
	osip_message_t *invite;
	int i;
	am_call_t *ca;

	sdp_message_t *local_sdp = NULL;
	sdp_media_t *local_med;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

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

	local_sdp = eXosip_get_local_sdp(did);
	eXosip_unlock();
	if (local_sdp == NULL && ca->local_sdp != NULL) {
		sdp_message_clone(ca->local_sdp, &local_sdp);
	}

	if (local_sdp == NULL) {
		return AMSIP_WRONG_STATE;
	}

	eXosip_lock();
	i = eXosip_call_build_request(did, "INVITE", &invite);
	eXosip_unlock();

	if (i != 0) {
		sdp_message_free(local_sdp);
		return i;
	}

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(invite, _antisipc.supported_extensions);

	/* check if video has already been added */
	local_med = eXosip_get_media(local_sdp, "text");

	{
		/* add sdp video body */
		char *tmp = NULL;

		if (local_med != NULL)
			i = _sdp_modify_text(ca, local_sdp);
		else
			i = _sdp_add_text(ca, local_sdp);
		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}

		_sdp_increase_versionid(local_sdp);
		ca->versionid++;

		i = sdp_message_to_str(local_sdp, &tmp);

		if (i != 0) {
			sdp_message_free(local_sdp);
			osip_message_free(invite);
			return i;
		}
#ifndef DISABLE_LOCAL_SDP
		if (ca->local_sdp != NULL)
			sdp_message_free(ca->local_sdp);
		ca->local_sdp = local_sdp;
#else
		sdp_message_free(local_sdp);
#endif

		osip_message_set_body(invite, tmp, strlen(tmp));
		osip_free(tmp);
		osip_message_set_content_type(invite, "application/sdp");
	}

	eXosip_lock();
	i = eXosip_call_send_request(did, invite);
	if (i >= 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_stop(int cid, int did, int code)
{
	int i;

	eXosip_lock();
	call_stop_sound(cid, did);
	i = eXosip_call_terminate(cid, did);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_play_file(int did, const char *wav_file)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->audio_module_session_play_file(ca, wav_file,
															  -2, NULL);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_playfile(int did, const char *wav_file,
									  int repeat,
									  MSFilterNotifyFunc cb_fileplayer_eof)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->audio_module_session_play_file(ca, wav_file,
															  repeat,
															  cb_fileplayer_eof);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_send_inband_dtmf(int did, char dtmf_number)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->audio_module_session_send_inband_dtmf(ca,
																	 dtmf_number);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_send_rtp_dtmf(int did, char dtmf_number)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->audio_module_session_send_rtp_dtmf(ca,
																  dtmf_number);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_send_dtmf_with_duration(int did, char dtmf_number, int duration)
{
	am_call_t *ca;
	osip_message_t *info;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = eXosip_call_build_info(ca->did, &info);
	if (i == 0) {
		char dtmf_body[1000];

		snprintf(dtmf_body, 999, "Signal=%c\r\nDuration=%i\r\n",
				 dtmf_number, duration);
		osip_message_set_content_type(info, "application/dtmf-relay");
		osip_message_set_body(info, dtmf_body, strlen(dtmf_body));
		i = eXosip_call_send_request(ca->did, info);
	}

	eXosip_unlock();

	return i;
}

PPL_DECLARE (int) am_session_send_dtmf(int did, char dtmf_number)
{
	return am_session_send_dtmf_with_duration(did, dtmf_number, 250);
}

PPL_DECLARE (int)
am_session_get_dtmf_event(int did, struct am_dtmf_event *dtmf_event)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->audio_module_session_get_dtmf_event(ca,
																   dtmf_event);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_get_audio_remote(int did, char *remote_info)
{
	am_call_t *ca;
	int val;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	if (ca == NULL) {
		eXosip_unlock();
		return AMSIP_NOTFOUND;
	}

	snprintf(remote_info, 256, "%s", ca->audio_rtp_remote_addr);

	val = ca->audio_rtp_in_direct_mode;
	eXosip_unlock();
	return val;
}

PPL_DECLARE (int)
am_session_get_udpftp_bandwidth(int did,
								struct am_bandwidth_stats *band_stats)
{
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	memset(band_stats, 0, sizeof(struct am_bandwidth_stats));
	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): can't get statistics\n",
							  did));
		return AMSIP_NOTFOUND;
	}

	if (ca->enable_udpftp <= 0) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "udpftp not started for this call (%i:val=%i) : no available statistics\n",
							  ca->did, ca->enable_udpftp));
		return AMSIP_WRONG_STATE;
	}

	if (ca->udpftp_rtp_session != NULL) {
		RtpStream *rtpstream;
		rtp_stats_t *stats;

		ms_mutex_lock(&ca->udpftp_ticker->lock);
		rtpstream = &ca->udpftp_rtp_session->rtp;
		stats = &rtpstream->stats;
		rtp_stats_display(stats, "amsip statistics");

		band_stats->incoming_received = (int) stats->packet_recv;
		band_stats->incoming_expected = (int) stats->packet_recv;
		band_stats->incoming_packetloss = (int) stats->cum_packet_loss;
		band_stats->incoming_outoftime = (int) stats->outoftime;
		band_stats->incoming_notplayed = 0;
		band_stats->incoming_discarded = (int) stats->discarded;

		band_stats->outgoing_sent = (int) stats->packet_sent;

		band_stats->download_rate =
			rtp_session_compute_recv_bandwidth(ca->udpftp_rtp_session);
		band_stats->upload_rate =
			rtp_session_compute_send_bandwidth(ca->udpftp_rtp_session);

		ms_mutex_unlock(&ca->udpftp_ticker->lock);
	}

	eXosip_unlock();

	return AMSIP_SUCCESS;
}

PPL_DECLARE (int)
am_session_get_text_bandwidth(int did,
							  struct am_bandwidth_stats *band_stats)
{
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	memset(band_stats, 0, sizeof(struct am_bandwidth_stats));
	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): can't get statistics\n",
							  did));
		return AMSIP_NOTFOUND;
	}

	if (ca->enable_text <= 0) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "text not started for this call (%i:val=%i) : no available statistics\n",
							  ca->did, ca->enable_text));
		return AMSIP_WRONG_STATE;
	}

	if (ca->text_rtp_session != NULL) {
		RtpStream *rtpstream;
		rtp_stats_t *stats;

		ms_mutex_lock(&ca->text_ticker->lock);
		rtpstream = &ca->text_rtp_session->rtp;
		stats = &rtpstream->stats;
		rtp_stats_display(stats, "amsip statistics");

		band_stats->incoming_received = (int) stats->packet_recv;
		band_stats->incoming_expected = (int) stats->packet_recv;
		band_stats->incoming_packetloss = (int) stats->cum_packet_loss;
		band_stats->incoming_outoftime = (int) stats->outoftime;
		band_stats->incoming_notplayed = 0;
		band_stats->incoming_discarded = (int) stats->discarded;

		band_stats->outgoing_sent = (int) stats->packet_sent;

		band_stats->download_rate =
			rtp_session_compute_recv_bandwidth(ca->text_rtp_session);
		band_stats->upload_rate =
			rtp_session_compute_send_bandwidth(ca->text_rtp_session);

		ms_mutex_unlock(&ca->text_ticker->lock);
	}

	eXosip_unlock();

	return AMSIP_SUCCESS;
}

PPL_DECLARE (int)
am_session_get_audio_bandwidth(int did,
							   struct am_bandwidth_stats *band_stats)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	memset(band_stats, 0, sizeof(struct am_bandwidth_stats));
	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): can't get statistics\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.audio_media->
		audio_module_session_get_bandwidth_statistics(ca, band_stats);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_get_audio_statistics(int did,
								struct am_audio_stats *audio_stats)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	memset(audio_stats, 0, sizeof(struct am_audio_stats));
	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): can't get statistics\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.audio_media->
		audio_module_session_get_audio_statistics(ca, audio_stats);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_record(int did, const char *recfile)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to record\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.audio_media->audio_module_session_record(ca, recfile);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_stop_record(int did)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to record\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.audio_media->audio_module_session_stop_record(ca);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_mute(int did)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to mute\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.audio_media->audio_module_session_mute(ca);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_unmute(int did)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to unmute\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	i = _antisipc.audio_media->audio_module_session_unmute(ca);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_get_udpftp(int did, MST140Block * _blk)
{
	int i = -1;
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to get udpftp\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	if (ca->enable_udpftp <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to get udpftp\n",
							  did, ca->enable_udpftp));
		eXosip_unlock();
		return AMSIP_WRONG_STATE;
	}

	if (ca->udpftp_decoder != NULL) {
		ms_mutex_lock(&ca->udpftp_ticker->lock);
		i = ms_filter_call_method(ca->udpftp_decoder, MS_FILTER_TEXT_GET,
								  _blk);
		ms_mutex_unlock(&ca->udpftp_ticker->lock);

	}

	if (_blk->size > 0)
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "Incoming udpftp: %s\n", _blk->utf8_string));

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_send_udpftp(int did, MST140Block * _blk)
{
	int i = -1;
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to send udpftp\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	if (ca->enable_udpftp <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to get udpftp\n",
							  did, ca->enable_udpftp));
		eXosip_unlock();
		return AMSIP_WRONG_STATE;
	}

	if (ca->udpftp_encoder != NULL) {
		ms_mutex_lock(&ca->udpftp_ticker->lock);
		i = ms_filter_call_method(ca->udpftp_encoder, MS_FILTER_TEXT_SEND,
								  _blk);
		ms_mutex_unlock(&ca->udpftp_ticker->lock);
	}

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_get_text(int did, MST140Block * _blk)
{
	int i = -1;
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to get text\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	if (ca->enable_text <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to get text\n",
							  did, ca->enable_text));
		eXosip_unlock();
		return AMSIP_WRONG_STATE;
	}

	if (ca->text_decoder != NULL) {
		ms_mutex_lock(&ca->text_ticker->lock);
		i = ms_filter_call_method(ca->text_decoder, MS_FILTER_TEXT_GET,
								  _blk);
		ms_mutex_unlock(&ca->text_ticker->lock);

	}

	if (_blk->size > 0)
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "Incoming text: %s\n", _blk->utf8_string));

	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_session_send_text(int did, MST140Block * _blk)
{
	int i = -1;
	am_call_t *ca;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to send text\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	if (ca->enable_text <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to send text\n",
							  did, ca->enable_text));
		eXosip_unlock();
		return AMSIP_WRONG_STATE;
	}

	if (ca->text_encoder != NULL) {
		ms_mutex_lock(&ca->text_ticker->lock);
		i = ms_filter_call_method(ca->text_encoder, MS_FILTER_TEXT_SEND,
								  _blk);
		ms_mutex_unlock(&ca->text_ticker->lock);
	}

	eXosip_unlock();
	return i;
}

PPL_DECLARE (void) am_free_t140block(MST140Block * _blk)
{
	if (_blk == NULL)
		return;
	osip_free(_blk->utf8_string);
}

PPL_DECLARE (int)
am_session_add_external_rtpdata(int did, MSFilter * external_rtpdata)
{
	am_call_t *ca;
	int i;

	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to add secondary codec\n",
							  did));
		return AMSIP_NOTFOUND;
	}

	i = _antisipc.audio_media->
		audio_module_session_add_external_rtpdata(ca, external_rtpdata);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_session_conference_detach(int cid, int did)
{
	am_call_t *ca;

	eXosip_lock();

	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to am_session_conference_detach\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	

	if (_antisipc.audio_media->audio_module_session_detach_conf!=NULL)
		_antisipc.audio_media->audio_module_session_detach_conf(ca);

	eXosip_unlock();

	return 0;

}


PPL_DECLARE (int)
am_session_conference_attach(int conf_id, int cid, int did)
{
	am_call_t *ca;

	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	
	eXosip_lock();

	ca = _am_calls_find_audio_connection(-2, did);

	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i): impossible to am_session_conference_attach\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	
	/* ca->conf_id = conf_id; */
	if (_antisipc.audio_media->audio_module_session_attach_conf!=NULL)
		_antisipc.audio_media->audio_module_session_attach_conf(ca, conf_id);
	eXosip_unlock();
	
	return 0;

}
