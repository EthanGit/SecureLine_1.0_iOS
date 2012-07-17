/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include <osipparser2/osip_md5.h>
#include "am_calls.h"

#include <ortp/ortp.h>


#ifndef DISABLE_SRTP
#include <ortp/srtp.h>
#endif

#include "mediastreamer2/dtmfgen.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilerec.h"

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

#define HASHLEN 16
typedef char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN + 1];

void CvtHex(HASH Bin, HASHHEX Hex);

int
am_hexa_generate_random(char *val, int val_size, char *str1, char *str2,
					  char *str3)
{
	osip_MD5_CTX Md5Ctx;
	HASH HA1;
	HASHHEX Key;

	osip_MD5Init(&Md5Ctx);
	osip_MD5Update(&Md5Ctx, (unsigned char *) str1, (unsigned int)strlen(str1));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) str2, (unsigned int)strlen(str2));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) str3, (unsigned int)strlen(str3));
	osip_MD5Final((unsigned char *) HA1, &Md5Ctx);
	CvtHex(HA1, Key);
	osip_strncpy(val, Key, val_size - 1);
	return 0;
}

#if 0
static const unsigned char alpha[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
#else
static const unsigned char alpha[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#endif

static unsigned char b64_convert(unsigned char v)
{
	if (v >= 'A' && v <= 'Z')
		return (v - 'A');
	if (v >= 'a' && v <= 'z')
		return (v - 'a' + 26);
	if (v >= '0' && v <= '9')
		return (v - '0' + 52);
	if (v == '+')
		return 62;
	if (v == '-')
		return 63;
	if (v == '=')
		return 64;

	/* 63 can be '/' or '-' */
	if (v == '/')
		return 63;
	return 65;
}

static void b642a(const unsigned char from[], unsigned char to[])
{
	to[0] = (from[0] << 2) | (from[1] >> 4);
	if (from[3] == 64) {
		if (from[2] == 64) {
			to[1] = '\0';
			return;
		}
		to[1] = (from[1] << 4) | (from[2] >> 2);
		to[2] = '\0';
		return;
	}
	to[1] = (from[1] << 4) | (from[2] >> 2);
	to[2] = (from[2] << 6) | from[3];
	return;
}

static void
a2b64(const unsigned char *from, int from_len, unsigned char *to)
{
	*((unsigned long *) to) = 0;
	to[0] = alpha[from[0] >> 2];
	to[1] =
		alpha[(((unsigned char) (from[0] << 6)) >> 2) | (from[1] >> 4)];
	if (from_len == 1) {
		to[2] = to[3] = '=';
		return;
	}
	to[2] =
		alpha[(((unsigned char) (from[1] << 4)) >> 2) | (from[2] >> 6)];
	if (from_len == 2) {
		to[3] = '=';
		return;
	}
	to[3] = alpha[((unsigned char) (from[2] << 2)) >> 2];
}

int
am_base64_encode(char *b64_text, const char *target_key, size_t target_key_size)
{
    int i=0;
	int j=0;

	for (i=0;i<target_key_size-3;i=i+3)
	{
		a2b64((const unsigned char *)(target_key+i),3,(unsigned char *)(b64_text+j));
		j=j+4;
	}
	if (target_key[i+1]=='\0')
	{
		a2b64((const unsigned char *)(target_key+i),1,(unsigned char *)(b64_text+j));
		j=j+4;
	}
	else if (target_key[i+2]=='\0')
	{
		a2b64((const unsigned char *)(target_key+i),2,(unsigned char *)(b64_text+j));
		j=j+4;
	}
	else if (target_key[i+3]=='\0')
	{
		a2b64((const unsigned char *)(target_key+i),3,(unsigned char *)(b64_text+j));
		j=j+4;
	}

	return j;
}

static int
am_base64_decode(char *target_key, char *b64_text, size_t b64_size)
{
	unsigned char from[4], to[4] = "   ";
	int cnt = 0;
	int i = 0, bad = 0, c;

	while (b64_size > 0) {
		if ((c = b64_convert(b64_text[0])) == 65) {
			if (bad > 500)
				return -1;
			bad++;
			continue;
		}
		from[i] = c;
		i = (i + 1) % 4;
		if (!i) {
			b642a(from, to);
			strcat(target_key, (const char *)to);
			/* printf("%s\n",target_key); */
		}
		b64_size--;
		b64_text++;
		cnt++;
	}

	return cnt;
}

#ifndef DISABLE_SRTP

#define MAX_KEY_LEN      64
#define MASTER_KEY_LEN   30


static int
am_use_security_descriptions(RtpSession * rtpr, char *local_crypto, char *remote_crypto)
{
	srtp_t srtp;
	srtp_policy_t policy;
#if 0
	static err_status_t status = err_status_fail;
#endif
	RtpTransport *rtpt;
	RtpTransport *rtcpt;
	int i;
	char input_key[MASTER_KEY_LEN * 2 + 1];
	size_t len;

	char crypto[256];

	char *beg;
	char *end;

	char local_key[MAX_KEY_LEN];
	char remote_key[MAX_KEY_LEN];
	char *remote_wsh=NULL;
	char *local_wsh=NULL;

	memset(local_key, 0, sizeof(local_key));
	memset(remote_key, 0, sizeof(remote_key));

#if 0
	if (status != err_status_ok) {
		status = ortp_srtp_init();
		if (status) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								  "srtp initialization failed with error code %d\n",
								  status));
			return -1;
		}
	}
#endif

	/* wsh  = "WSH=" 2*DIGIT    ; minimum value is 64 */
	local_wsh = osip_strcasestr(local_crypto, "wsh=");
	remote_wsh = osip_strcasestr(remote_crypto, "wsh=");

	snprintf(crypto, sizeof(crypto), "%s", local_crypto);

	if (osip_strcasestr(local_crypto, "AES_CM_128_HMAC_SHA1_80") != NULL)
	{
		crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
		crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
	}
	else if (osip_strcasestr(local_crypto, "AES_CM_128_HMAC_SHA1_32") != NULL)
	{
		crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);
		crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtcp);
	}
	else if (osip_strcasestr(local_crypto, "AES_CM_128_NULL_AUTH") != NULL)
	{
		crypto_policy_set_aes_cm_128_null_auth(&policy.rtp);
		crypto_policy_set_aes_cm_128_null_auth(&policy.rtcp);
	}
	else if (osip_strcasestr(local_crypto, "NULL_CIPHER_HMAC_SHA1_80") != NULL)
	{
		crypto_policy_set_null_cipher_hmac_sha1_80(&policy.rtp);
		crypto_policy_set_null_cipher_hmac_sha1_80(&policy.rtcp);
	}
#if 0
	/* not yet supported by libsrtp */
	else if (osip_strcasestr(local_crypto, "AES_CM_192_HMAC_SHA1_80") != NULL)
	{
		crypto_policy_set_aes_cm_192_hmac_sha1_80(&policy.rtp);
		crypto_policy_set_aes_cm_192_hmac_sha1_80(&policy.rtcp);
	}
	else if (osip_strcasestr(local_crypto, "AES_CM_192_HMAC_SHA1_32") != NULL)
	{
		crypto_policy_set_aes_cm_192_hmac_sha1_32(&policy.rtp);
		crypto_policy_set_aes_cm_192_hmac_sha1_32(&policy.rtcp);
	}
#endif
	else if (osip_strcasestr(local_crypto, "AES_CM_256_HMAC_SHA1_80") != NULL)
	{
		crypto_policy_set_aes_cm_256_hmac_sha1_80(&policy.rtp);
		crypto_policy_set_aes_cm_256_hmac_sha1_80(&policy.rtcp);
	}
	else if (osip_strcasestr(local_crypto, "AES_CM_256_HMAC_SHA1_32") != NULL)
	{
		crypto_policy_set_aes_cm_256_hmac_sha1_32(&policy.rtp);
		crypto_policy_set_aes_cm_256_hmac_sha1_32(&policy.rtcp);
	}
	
	else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
								"unknown crypto suite detected (%s)\n",
								crypto));
		return -1;
	}

	policy.next = NULL;
	policy.rtcp.sec_serv = sec_serv_none;	/* we don't do RTCP anyway */

	octet_string_set_to_zero((uint8_t *) input_key, MASTER_KEY_LEN * 2);

	beg = osip_strcasestr(crypto, "inline:");
	if (beg == NULL)
		return -1;
	beg = beg + 7;
	end = strchr(beg, '|');
	if (end == NULL) {
		int k = 0;

		while (beg[k] != '\0') {
			if (beg[k] == ' ')
				beg[k] = '\0';
			k++;
		}
		osip_strncpy(input_key, beg, strlen(beg));
	} else
		osip_strncpy(input_key, beg, end - beg);

	len = am_base64_decode(local_key, input_key, strlen(input_key));

	/* check that hex string is the right length */
	if (len < strlen(input_key)) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "error: too few digits in key/salt\n"));
		return -1;
	}
#if 0
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
						  "local key: %s\n", local_key));
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
						  "set local master key/salt to %s/",
						  octet_string_hex_string(local_key, 16)));
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_ERROR, NULL, "%s\n",
				octet_string_hex_string(local_key + 16, 14)));
#endif


	snprintf(crypto, sizeof(crypto), "%s", remote_crypto);

	octet_string_set_to_zero((uint8_t *) input_key, MASTER_KEY_LEN * 2);

	beg = osip_strcasestr(crypto, "inline:");
	if (beg == NULL)
		return -1;
	beg = beg + 7;
	end = strchr(beg, '|');
	if (end == NULL) {
		int k = 0;

		while (beg[k] != '\0') {
			if (beg[k] == ' ')
				beg[k] = '\0';
			k++;
		}
		osip_strncpy(input_key, beg, strlen(beg));
	} else
		osip_strncpy(input_key, beg, end - beg);

	len = am_base64_decode(remote_key, input_key, strlen(input_key));

	/* check that hex string is the right length */
	if (len < strlen(input_key)) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "error: too few digits in key/salt\n"));
		return -1;
	}
#if 0
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
						  "remote key: %s\n", remote_key));
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
						  "set remote master key/salt to %s/",
						  octet_string_hex_string(local_key, 16)));
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_ERROR, NULL, "%s\n",
				octet_string_hex_string(local_key + 16, 14)));
#endif

	i = ortp_srtp_create(&srtp, NULL);
	if (i != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "srtp create failed\n"));
		return -1;
	}

	policy.key = (uint8_t *) remote_key;
	policy.ssrc.type = ssrc_any_inbound;
	policy.ssrc.value = 0;
	policy.window_size = 0;
	if (remote_wsh!=NULL)
		policy.window_size = atoi(remote_wsh+4);
	policy.allow_repeat_tx = 0;
	policy.ekt=NULL;

	i = ortp_srtp_add_stream(srtp, &policy);
	if (i != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "srtp_add_stream failed\n"));
		return -1;
	}

	policy.key = (uint8_t *) local_key;
	policy.ssrc.type = ssrc_specific;
	policy.ssrc.value = rtpr->snd.ssrc;
	policy.window_size = 0;
	if (local_wsh!=NULL)
		policy.window_size = atoi(local_wsh+4);
	policy.allow_repeat_tx = 0;
	policy.ekt=NULL;

	i = ortp_srtp_add_stream(srtp, &policy);
	if (i != 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "srtp_add_stream failed\n"));
		return -1;
	}

	rtpt = ortp_transport_new("SRTP");
	rtcpt = NULL;
	
	if (rtpt == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "oRTP compiled without SRTP support\n"));
		ortp_srtp_dealloc(srtp);
		return -1;
	}

	rtpt->data = srtp;
	rtp_session_set_transports(rtpr, rtpt, NULL /* rtcpt */ );

	return 0;
}

int
am_get_security_descriptions(RtpSession * rtp,
							 sdp_message_t * sdp_answer,
							 sdp_message_t * sdp_offer,
							 sdp_message_t * sdp_local,
							 sdp_message_t * sdp_remote, char *media_type)
{
	/* get local & remote "crypto" attribute */
	sdp_media_t *answer_med = NULL;
	sdp_media_t *offer_med = NULL;
	sdp_attribute_t *answer_attr = NULL;
	sdp_attribute_t *offer_attr = NULL;
	int pos;
	int idx = -1;

	if (sdp_local == NULL)
		return -1;
	if (sdp_remote == NULL)
		return -1;
	if (sdp_offer == NULL)
		return -1;
	if (sdp_answer == NULL)
		return -1;
	if (rtp == NULL)
		return -1;

	pos = 0;
	while (!osip_list_eol(&sdp_answer->m_medias, pos)) {
		sdp_media_t *med = NULL;

		med = (sdp_media_t *) osip_list_get(&sdp_answer->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, media_type)
			&& 0 == osip_strcasecmp(med->m_proto, "RTP/SAVP")) {
			answer_med = med;
			break;
		}
		pos++;
	}

	pos = 0;
	while (!osip_list_eol(&sdp_offer->m_medias, pos)) {
		sdp_media_t *med = NULL;

		med = (sdp_media_t *) osip_list_get(&sdp_offer->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, media_type)
			&& 0 == osip_strcasecmp(med->m_proto, "RTP/SAVP")) {
			offer_med = med;
			break;
		}
		pos++;
	}

	if (offer_med == NULL && answer_med == NULL) {
		if (_antisipc.optionnal_encryption == 0)
			return 0;

		/* support optionnal encryption in RTP/AVP profile */
		pos = 0;
		while (!osip_list_eol(&sdp_answer->m_medias, pos)) {
			sdp_media_t *med = NULL;

			med =
				(sdp_media_t *) osip_list_get(&sdp_answer->m_medias, pos);

			if (0 == osip_strcasecmp(med->m_media, media_type)
				&& 0 == osip_strcasecmp(med->m_proto, "RTP/AVP")) {
				answer_med = med;
				break;
			}
			pos++;
		}

		pos = 0;
		while (!osip_list_eol(&sdp_offer->m_medias, pos)) {
			sdp_media_t *med = NULL;

			med = (sdp_media_t *) osip_list_get(&sdp_offer->m_medias, pos);

			if (0 == osip_strcasecmp(med->m_media, media_type)
				&& 0 == osip_strcasecmp(med->m_proto, "RTP/AVP")) {
				offer_med = med;
				break;
			}
			pos++;
		}

	}

	if (offer_med == NULL || answer_med == NULL)
		return 0;				/* RTP/SAVP not used */

	pos = 0;
	while (!osip_list_eol(&answer_med->a_attributes, pos)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&answer_med->a_attributes,
											  pos);
		if (attr->a_att_field != NULL
			&& osip_strcasecmp(attr->a_att_field, "crypto") == 0) {
			answer_attr = attr;
			break;
		}
		pos++;
	}

	if (answer_attr == NULL || answer_attr->a_att_field == NULL)
		return -1;

	idx = atoi(answer_attr->a_att_field);
	if (idx < 0)
		return -1;

	pos = 0;
	while (!osip_list_eol(&offer_med->a_attributes, pos)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&offer_med->a_attributes,
											  pos);
		if (attr->a_att_field != NULL
			&& osip_strcasecmp(attr->a_att_field, "crypto") == 0
			&& idx == atoi(attr->a_att_field)) {
			offer_attr = attr;
			break;
		}
		pos++;
	}


	if (offer_attr == NULL || offer_attr->a_att_field == NULL)
		return -1;

	/* set policy */
	if (sdp_offer == sdp_local)
		return am_use_security_descriptions(rtp,
											offer_attr->a_att_value,
											answer_attr->a_att_value);
	else
		return am_use_security_descriptions(rtp,
											answer_attr->a_att_value,
											offer_attr->a_att_value);

	return 0;
}
#endif
