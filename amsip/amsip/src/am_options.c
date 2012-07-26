/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include <mediastreamer2/mediastream.h>
#ifdef ENABLE_VIDEO
#include <mediastreamer2/msvideoout.h>
#include "mediastreamer2/msv4l.h"
#endif

#include "amsip/am_version.h"

#include "amsip/am_options.h"
#include <osipparser2/osip_port.h>

#include <eXosip2/eXosip.h>

#include "am_calls.h"

#include <limits.h>

#ifdef ENABLE_VIDEO
#include "am_video_start.h"
#endif

#ifdef EXOSIP4
#include "amsip-internal.h"

struct eXosip_t *amsip_eXosip;
#endif

static FILE *f = NULL;

// Check for memory leaks
#if 0
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

BOOL WINAPI DllMain( 
					 HINSTANCE hinstDLL,	// handle to DLL module
					 DWORD fdwReason,		// reason for calling function
					 LPVOID lpReserved		// reserved
				   )  
{
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
        //_crtBreakAlloc = 149;
        _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
       return 1;
    }
}
#endif

PPL_DECLARE (int) am_option_debug(const char *log_file, int debug_level)
{
	if (debug_level > 0) {
		if (log_file != NULL && log_file[0] != '\0') {
			f = fopen(log_file, "w+");
			if (NULL == f) {
				TRACE_INITIALIZE(debug_level, NULL);
				return -1;
			}
			TRACE_INITIALIZE(debug_level, f);
		} else {
			TRACE_INITIALIZE(debug_level, NULL);
		}
	}
	return 0;
}

PPL_DECLARE (int) am_quit(void)
{
	/* close all calls */
	am_call_t *ca;
	int k;

	_am_udpftp_stop_thread();

#if !defined (_WIN32_WCE)
	am_player_stop_all();
#endif

#ifdef ENABLE_VIDEO
	_antisipc.video_media->video_module_enable_preview(0);
#endif

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state != NOT_USED) {
			ca = &(_antisipc.calls[k]);
			am_session_stop(ca->cid, ca->did, 486);
			am_call_release(ca);
		}
	}

	eXosip_quit();
#ifdef EXOSIP4
	osip_free(amsip_eXosip);
	amsip_eXosip=NULL;
#endif

	_antisipc.audio_media->audio_module_quit();

	if (_antisipc.player_ticker != NULL)
		ms_ticker_destroy(_antisipc.player_ticker);

#ifdef ENABLE_VIDEO
	_antisipc.video_media->video_module_quit();
#endif

	memset(&_antisipc, 0, sizeof(struct antisipc));
	return 0;
}

PPL_DECLARE (int) am_reset(const char *name, int debug_level)
{
	int i;

	am_quit();

	TRACE_INITIALIZE(debug_level, f);
	memset(&_antisipc, 0, sizeof(struct antisipc));

	_antisipc.audio_media = &ms2_audio_module;
#ifdef ENABLE_VIDEO
	_antisipc.video_media = &ms2_video_module;
#endif

	_antisipc.use_rport = 1;
	_antisipc.use_udpkeepalive = 17000;
	_antisipc.dns_capabilities = 2;
	_antisipc.use_101 = 1;
	_antisipc.session_timers=0;
	_antisipc.enable_p_am_sessiontype=0;

	snprintf(_antisipc.audio_profile, sizeof(_antisipc.audio_profile),
			 "RTP/AVP");
	snprintf(_antisipc.video_profile, sizeof(_antisipc.video_profile),
			 "RTP/AVP");
	snprintf(_antisipc.text_profile, sizeof(_antisipc.text_profile),
			 "RTP/AVP");
	snprintf(_antisipc.udpftp_profile, sizeof(_antisipc.udpftp_profile),
			 "RTP/AVP");

#if !defined (_WIN32_WCE) && !defined(__MACH__)
	_antisipc.optionnal_encryption = 1;
#else
	_antisipc.optionnal_encryption = 0;
#endif
	_antisipc.enable_zrtp = 0;

	{
		struct am_srtp_info srtp_info;
		srtp_info.enable = 1;
		srtp_info.pos = 0;
		srtp_info.srtp_algo = AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80;
		am_option_set_option(AMSIP_OPTION_SET_SRTP_CRYPTO_SUITE, &srtp_info);
		srtp_info.enable = 0;
		srtp_info.srtp_algo = 0;
		for (i=1;i<10;i++) {
			srtp_info.pos = i;
			am_option_set_option(AMSIP_OPTION_SET_SRTP_CRYPTO_SUITE, &srtp_info);
		}
	}

	_antisipc.automatic_rfc5168 = 1;
	_antisipc.enable_sdpsetupparameter = 0;
	_antisipc.add_nortpproxy = 0;

	snprintf(_antisipc.supported_extensions,
			 sizeof(_antisipc.supported_extensions), "%s", "100rel");

	if (name == NULL || name[0] == '\0')
		snprintf(_antisipc.syslog_name, 256, "amsip");
	else
		snprintf(_antisipc.syslog_name, 256, "%s", name);

	_antisipc.port_range_min = 30250;

	snprintf(_antisipc.codecs[0].name, 64, "speex");
	_antisipc.codecs[0].payload = 97;
	_antisipc.codecs[0].enable = 0;
	_antisipc.codecs[0].freq = 8000;
	_antisipc.codecs[0].vbr = 1;
	_antisipc.codecs[0].mode = 3;

	snprintf(_antisipc.codecs[1].name, 64, "iLBC");
	_antisipc.codecs[1].payload = 98;
	_antisipc.codecs[1].enable = 0;
	_antisipc.codecs[1].freq = 8000;
	_antisipc.codecs[1].mode = 20;

	snprintf(_antisipc.codecs[2].name, 64, "PCMU");
	_antisipc.codecs[2].payload = 0;
	_antisipc.codecs[2].enable = 1;
	_antisipc.codecs[2].freq = 8000;

	snprintf(_antisipc.codecs[3].name, 64, "PCMA");
	_antisipc.codecs[3].payload = 8;
	_antisipc.codecs[3].enable = 1;
	_antisipc.codecs[3].freq = 8000;

	_antisipc.codec_attr.ptime = 0;
	_antisipc.codec_attr.bandwidth = 0;

	snprintf(_antisipc.video_codecs[0].name, 64, "H263-1998");
	_antisipc.video_codecs[0].payload = 115;
#ifdef ENABLE_VIDEO
	_antisipc.video_codecs[0].enable = 1;
#else
	_antisipc.video_codecs[0].enable = 0;
#endif
	_antisipc.video_codecs[0].freq = 90000;

	/* snprintf(_antisipc.supported_gruu, sizeof(_antisipc.supported_gruu), "%s", "\"<urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6>\""); */
	_antisipc.supported_gruu[0] = '\0';

#ifdef ENABLE_VIDEO
	_antisipc.video_codec_attr.ptime = 0;
	_antisipc.video_codec_attr.upload_bandwidth = 64;
	_antisipc.video_codec_attr.download_bandwidth = 256;
#endif

	snprintf(_antisipc.text_codecs[0].name, 64, "t140");
	_antisipc.text_codecs[0].payload = 110;
	_antisipc.text_codecs[0].enable = 1;
	_antisipc.text_codecs[0].freq = 1000;

	_antisipc.text_codec_attr.ptime = 0;
	_antisipc.text_codec_attr.upload_bandwidth = 0;
	_antisipc.text_codec_attr.download_bandwidth = 0;

	snprintf(_antisipc.udpftp_codecs[0].name, 64, "x-udpftp");
	_antisipc.udpftp_codecs[0].payload = 110;
	_antisipc.udpftp_codecs[0].enable = 1;
	_antisipc.udpftp_codecs[0].freq = 1000;

	_antisipc.udpftp_codec_attr.ptime = 0;
	_antisipc.udpftp_codec_attr.upload_bandwidth = 0;
	_antisipc.udpftp_codec_attr.download_bandwidth = 0;

	eXosip_quit();
#ifdef EXOSIP4
	osip_free(amsip_eXosip);
	amsip_eXosip=NULL;
	amsip_eXosip = eXosip_malloc();
#endif
	i = eXosip_init();
	if (i != 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot initialize eXosip");
		return -1;
	}

	eXosip_set_option(EXOSIP_OPT_UDP_KEEP_ALIVE,
					  &_antisipc.use_udpkeepalive);

	i = 1;
	eXosip_set_option(EXOSIP_OPT_UDP_LEARN_PORT, &i);

	am_option_set_user_agent(name);
	am_option_enable_101(0);

	i = _antisipc.audio_media->audio_module_reset(name, debug_level);
	if (i != 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot initialize audio module");
		return i;
	}

	//AMD_antisipc.player_ticker = ms_ticker_new_withname("amsip-player");

#ifdef ENABLE_VIDEO
	i = _antisipc.video_media->video_module_reset(name, debug_level);
	if (i != 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot initialize video module");
		return i;
	}
#endif

	_antisipc.text_dscp = 0x38;
	_antisipc.udpftp_dscp = 0x28;

	_am_udpftp_start_thread();

	return 0;
}

PPL_DECLARE (int) am_init(const char *name, int debug_level)
{
	int i;

	TRACE_INITIALIZE(debug_level, f);
	memset(&_antisipc, 0, sizeof(struct antisipc));

	_antisipc.audio_media = &ms2_audio_module;
#ifdef ENABLE_VIDEO
	_antisipc.video_media = &ms2_video_module;
#endif

	_antisipc.use_rport = 1;
	_antisipc.use_udpkeepalive = 17000;
	_antisipc.dns_capabilities = 2;
	_antisipc.use_101 = 1;
	_antisipc.session_timers=0;
	_antisipc.enable_p_am_sessiontype=0;

	snprintf(_antisipc.audio_profile, sizeof(_antisipc.audio_profile),
			 "RTP/AVP");
	snprintf(_antisipc.video_profile, sizeof(_antisipc.video_profile),
			 "RTP/AVP");
	snprintf(_antisipc.text_profile, sizeof(_antisipc.text_profile),
			 "RTP/AVP");
	snprintf(_antisipc.udpftp_profile, sizeof(_antisipc.udpftp_profile),
			 "RTP/AVP");
#if !defined (_WIN32_WCE) && !defined(__MACH__)
	_antisipc.optionnal_encryption = 1;
#else
	_antisipc.optionnal_encryption = 0;
#endif
	_antisipc.enable_zrtp = 0;

	{
		struct am_srtp_info srtp_info;
		srtp_info.enable = 1;
		srtp_info.pos = 0;
		srtp_info.srtp_algo = AMSIP_ALGO_AES_CM_128_HMAC_SHA1_80;
		am_option_set_option(AMSIP_OPTION_SET_SRTP_CRYPTO_SUITE, &srtp_info);
		srtp_info.enable = 0;
		srtp_info.srtp_algo = 0;
		for (i=1;i<10;i++) {
			srtp_info.pos = i;
			am_option_set_option(AMSIP_OPTION_SET_SRTP_CRYPTO_SUITE, &srtp_info);
		}
	}

	_antisipc.automatic_rfc5168 = 1;
	_antisipc.enable_sdpsetupparameter = 0;
	_antisipc.add_nortpproxy = 0;

	snprintf(_antisipc.supported_extensions,
			 sizeof(_antisipc.supported_extensions), "%s", "100rel");

	if (name == NULL || name[0] == '\0')
		snprintf(_antisipc.syslog_name, 256, "amsip");
	else
		snprintf(_antisipc.syslog_name, 256, "%s", name);

	_antisipc.port_range_min = 30250;

	snprintf(_antisipc.codecs[0].name, 64, "speex");
	_antisipc.codecs[0].payload = 97;
	_antisipc.codecs[0].enable = 0;
	_antisipc.codecs[0].freq = 8000;
	_antisipc.codecs[0].vbr = 1;
	_antisipc.codecs[0].mode = 3;

	snprintf(_antisipc.codecs[1].name, 64, "iLBC");
	_antisipc.codecs[1].payload = 98;
	_antisipc.codecs[1].enable = 0;
	_antisipc.codecs[1].freq = 8000;
	_antisipc.codecs[1].mode = 20;

	snprintf(_antisipc.codecs[2].name, 64, "PCMU");
	_antisipc.codecs[2].payload = 0;
	_antisipc.codecs[2].enable = 1;
	_antisipc.codecs[2].freq = 8000;

	snprintf(_antisipc.codecs[3].name, 64, "PCMA");
	_antisipc.codecs[3].payload = 8;
	_antisipc.codecs[3].enable = 1;
	_antisipc.codecs[3].freq = 8000;

	_antisipc.codec_attr.ptime = 0;
	_antisipc.codec_attr.bandwidth = 0;

	snprintf(_antisipc.video_codecs[0].name, 64, "H263-1998");
	_antisipc.video_codecs[0].payload = 115;
#ifdef ENABLE_VIDEO
	_antisipc.video_codecs[0].enable = 1;
#else
	_antisipc.video_codecs[0].enable = 0;
#endif
	_antisipc.video_codecs[0].freq = 90000;

#ifdef ENABLE_VIDEO
	_antisipc.video_codec_attr.ptime = 0;
	_antisipc.video_codec_attr.upload_bandwidth = 64;
	_antisipc.video_codec_attr.download_bandwidth = 256;
#endif

	snprintf(_antisipc.text_codecs[0].name, 64, "t140");
	_antisipc.text_codecs[0].payload = 110;
	_antisipc.text_codecs[0].enable = 1;
	_antisipc.text_codecs[0].freq = 1000;

	_antisipc.text_codec_attr.ptime = 0;
	_antisipc.text_codec_attr.upload_bandwidth = 0;
	_antisipc.text_codec_attr.download_bandwidth = 0;

	snprintf(_antisipc.udpftp_codecs[0].name, 64, "x-udpftp");
	_antisipc.udpftp_codecs[0].payload = 110;
	_antisipc.udpftp_codecs[0].enable = 1;
	_antisipc.udpftp_codecs[0].freq = 1000;

	_antisipc.udpftp_codec_attr.ptime = 0;
	_antisipc.udpftp_codec_attr.upload_bandwidth = 0;
	_antisipc.udpftp_codec_attr.download_bandwidth = 0;

#ifdef EXOSIP4
	amsip_eXosip = eXosip_malloc();
#endif
	i = eXosip_init();
	if (i != 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot initialize eXosip");
		return -1;
	}

	eXosip_set_option(EXOSIP_OPT_UDP_KEEP_ALIVE,
					  &_antisipc.use_udpkeepalive);

	i = 1;
	eXosip_set_option(EXOSIP_OPT_UDP_LEARN_PORT, &i);

	am_option_set_user_agent(name);
	am_option_enable_101(0);

	i = _antisipc.audio_media->audio_module_init(name, debug_level);
	if (i != 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot initialize audio module");
		return i;
	}

	//AMD_antisipc.player_ticker = ms_ticker_new_withname("amsip-player");
    
#ifdef ENABLE_VIDEO
	i = _antisipc.video_media->video_module_init(name, debug_level);
	if (i != 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "cannot initialize video module");
		return i;
	}
#endif

	_am_udpftp_start_thread();
	return 0;
}

PPL_DECLARE (int)
am_option_set_option(int opt, void *arg)
{
	int i = AMSIP_SUCCESS;
	switch(opt){

		case AMSIP_OPTION_TLS_CHECK_CERTIFICATE:
#ifndef DISABLE_SSL
			i = eXosip_tls_verify_certificate(*((int*)arg));
#endif
			break;
		case AMSIP_OPTION_TLS_SET_CERTIFICATES:
#ifndef DISABLE_SSL            

			{
                OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
                                      "############# am_option_set_option : DISABLE_SSL\r\n"));
				eXosip_tls_ctx_t *tls_info = (eXosip_tls_ctx_t *)arg;
				i = eXosip_set_tls_ctx(tls_info);
			}
#endif
			break;
			
		case AMSIP_OPTION_ENABLE_P_SESSION_TYPE:
			_antisipc.enable_p_am_sessiontype=0;
			if (*((int*)arg)>0)
				_antisipc.enable_p_am_sessiontype=1;
			break;
		case AMSIP_OPTION_ENABLE_AUTOMATIC_RFC5168:
			_antisipc.automatic_rfc5168 = 0;
			if (*((int*)arg)>0)
				_antisipc.automatic_rfc5168 = 1;
			break;
		case AMSIP_OPTION_ENABLE_SDPSETUPPARAMETER:
			_antisipc.enable_sdpsetupparameter = 0;
			if (*((int*)arg)>0)
				_antisipc.enable_sdpsetupparameter = 1;
			break;
		case AMSIP_OPTION_SET_SRTP_CRYPTO_SUITE:
			{
				struct am_srtp_info *srtp_info = (struct am_srtp_info *)arg;
				if (srtp_info->pos>=0 && srtp_info->pos<=5)
				{
					memcpy(&_antisipc.srtp_info[srtp_info->pos], srtp_info, sizeof(struct am_srtp_info));
				}
				break;
			}
		case AMSIP_OPTION_ENABLE_ZRTP:
			_antisipc.enable_zrtp = 0;
			if (*((int*)arg)>0)
				_antisipc.enable_zrtp = 1;
			break;
		case AMSIP_OPTION_SET_AUDIO_JITTER:
			_antisipc.audio_jitter = -1;
			if (*((int*)arg)>=0)
				_antisipc.audio_jitter = *((int*)arg);
			break;
		default:
			i=AMSIP_BADPARAMETER;
	}

	return i;
}

PPL_DECLARE (int)
am_option_enable_optionnal_encryption(int optionnal_encryption)
{
//#if !defined (_WIN32_WCE) && !defined(__MACH__)
#if !defined (_WIN32_WCE)
	_antisipc.optionnal_encryption = 0;
	if (optionnal_encryption != 0)
		_antisipc.optionnal_encryption = 1;
#else
	_antisipc.optionnal_encryption = 0;
#endif
	return 0;
}

PPL_DECLARE (int) am_option_add_nortpproxy(int add_nortpproxy)
{
	_antisipc.add_nortpproxy = 0;
	if (add_nortpproxy != 0)
		_antisipc.add_nortpproxy = 1;
	return 0;
}

PPL_DECLARE (int) am_option_set_dscp_value(int dscp_value)
{
	_antisipc.audio_dscp = dscp_value;
	_antisipc.video_dscp = dscp_value;
	_antisipc.text_dscp = dscp_value;
	_antisipc.udpftp_dscp = dscp_value;
	return 0;
}

PPL_DECLARE (int) am_option_set_sip_dscp(int dscp_value)
{
	return eXosip_set_option(EXOSIP_OPT_SET_DSCP, &dscp_value);
}


PPL_DECLARE (int) am_option_set_audio_dscp(int dscp_value)
{
	_antisipc.audio_dscp = dscp_value;
	return 0;
}

PPL_DECLARE (int) am_option_set_video_dscp(int dscp_value)
{
	_antisipc.video_dscp = dscp_value;
	return 0;
}

PPL_DECLARE (int) am_option_set_text_dscp(int dscp_value)
{
	_antisipc.text_dscp = dscp_value;
	return 0;
}

PPL_DECLARE (int) am_option_set_udpftp_dscp(int dscp_value)
{
	_antisipc.udpftp_dscp = dscp_value;
	return 0;
}

PPL_DECLARE (int) am_option_add_dns_cache(const char *host, const char *ip)
{
	struct eXosip_dns_cache entry;

	if (host == NULL)
		return -1;

	memset(&entry, 0, sizeof(struct eXosip_dns_cache));
	snprintf(entry.host, sizeof(entry.host), "%s", host);
	if (ip != NULL)
		snprintf(entry.ip, sizeof(entry.ip), "%s", ip);
	eXosip_set_option(EXOSIP_OPT_ADD_DNS_CACHE, (void *) &entry);
	return 0;
}

PPL_DECLARE (int)
am_option_set_supported_extensions(const char *supported_extensions)
{
	if (supported_extensions == NULL) {
		memset(_antisipc.supported_extensions, 0,
			   sizeof(_antisipc.supported_extensions));
		return 0;
	}

	snprintf(_antisipc.supported_extensions,
			 sizeof(_antisipc.supported_extensions), "%s",
			 supported_extensions);
	return 0;
}

PPL_DECLARE (int)
am_option_set_allowed_methods(const char *allowed_methods)
{
	if (allowed_methods == NULL) {
		memset(_antisipc.allowed_methods, 0,
			   sizeof(_antisipc.allowed_methods));
		return 0;
	}

	snprintf(_antisipc.allowed_methods, sizeof(_antisipc.allowed_methods),
			 "%s", allowed_methods);
	return 0;
}

PPL_DECLARE (int)
am_option_set_allowed_events(const char *allowed_events)
{
	if (allowed_events == NULL) {
		memset(_antisipc.allowed_events, 0,
			   sizeof(_antisipc.allowed_events));
		return 0;
	}

	snprintf(_antisipc.allowed_events, sizeof(_antisipc.allowed_methods),
			 "%s", allowed_events);
	return 0;
}

PPL_DECLARE (int) am_option_set_accepted_types(const char *accepted_types)
{
	if (accepted_types == NULL) {
		memset(_antisipc.accepted_types, 0,
			   sizeof(_antisipc.accepted_types));
		return 0;
	}

	snprintf(_antisipc.accepted_types, sizeof(_antisipc.accepted_types),
			 "%s", accepted_types);
	return 0;
}

PPL_DECLARE (int) am_option_set_input_video_size(int width, int height)
{
#ifdef ENABLE_VIDEO
	if (_antisipc.video_media->video_module_set_input_video_size != NULL)
		return _antisipc.video_media->video_module_set_input_video_size(width, height);
	return AMSIP_UNDEFINED_ERROR;
#else
	return -1;
#endif
}

PPL_DECLARE (int)
am_option_set_window_display(MSDisplayDesc *desc, long handle, int width, int height)
{
#ifdef ENABLE_VIDEO
	if (_antisipc.video_media->video_module_set_window_display != NULL)
		return _antisipc.video_media->video_module_set_window_display(desc, handle, width, height);
	return AMSIP_UNDEFINED_ERROR;
#endif
	return -1;
}

PPL_DECLARE (int)
am_option_set_window_handle(long handle, int width, int height)
{
#ifdef ENABLE_VIDEO
	if (_antisipc.video_media->video_module_set_window_handle != NULL)
		return _antisipc.video_media->video_module_set_window_handle(handle, width, height);
	return AMSIP_UNDEFINED_ERROR;
#endif
	return -1;
}

PPL_DECLARE (int)
am_option_set_window_preview_handle(long handle, int width, int height)
{
#ifdef ENABLE_VIDEO
	if (_antisipc.video_media->video_module_set_window_preview_handle != NULL)
		return _antisipc.video_media->video_module_set_window_preview_handle(handle, width, height);
	return AMSIP_UNDEFINED_ERROR;
#endif
	return -1;
}

PPL_DECLARE (int) am_option_set_nowebcam(const char *nowebcam_image)
{
#ifdef ENABLE_VIDEO
	if (_antisipc.video_media->video_module_set_nowebcam != NULL)
		return _antisipc.video_media->video_module_set_nowebcam(nowebcam_image);
	return AMSIP_UNDEFINED_ERROR;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

PPL_DECLARE (int) am_option_enable_preview(int enable)
{
#ifdef ENABLE_VIDEO
	int i = AMSIP_UNDEFINED_ERROR;
	eXosip_lock();
	if (_antisipc.video_media->video_module_enable_preview != NULL)
		i = _antisipc.video_media->video_module_enable_preview(enable);
	eXosip_unlock();
	return i;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

PPL_DECLARE (int) am_option_set_image_callback(on_video_module_new_image_cb func)
{
#ifdef ENABLE_VIDEO
	int i = AMSIP_UNDEFINED_ERROR;
	eXosip_lock();
	if (_antisipc.video_media->video_module_set_image_callback != NULL)
		i = _antisipc.video_media->video_module_set_image_callback(func);
	eXosip_unlock();
	return i;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

PPL_DECLARE (char *) am_option_get_version()
{
	return AM_VERSION;
}

PPL_DECLARE (int) am_option_set_user_agent(const char *user_agent)
{
	char timestamp[64];
	int i;

	/* replace space with - */
	snprintf(timestamp, 64, "%s", __DATE__);
	for (i = 0; timestamp[i] != '\0' && i < 64; i++) {
		if (timestamp[i] == ' ')
			timestamp[i] = '-';
	}

#define SHORT_USER_AGENT
    
#ifdef SHORT_USER_AGENT
	snprintf(_antisipc.user_agent, sizeof(_antisipc.user_agent),
			 "%s", user_agent);
#else
	if (strchr(user_agent, '/')!=NULL)
	{
		snprintf(_antisipc.user_agent, sizeof(_antisipc.user_agent),
				 "antisip/%s-%s %s", AM_VERSION, timestamp, user_agent);
	}
	else
	{
		snprintf(_antisipc.user_agent, sizeof(_antisipc.user_agent),
				 "antisip-%s/%s-%s", user_agent, AM_VERSION, timestamp);
	}
#endif
	eXosip_set_user_agent(_antisipc.user_agent);
	return 0;
}

PPL_DECLARE (int) am_option_set_initial_audio_port(int initial_port)
{
	if (initial_port == 0) {
		/* generate random port */
		initial_port =
			30250 +
			2 * (int) ((int) 15000 *
					   (osip_build_random_number() / (RAND_MAX + 1.0)));

		if (initial_port < 30250 || initial_port > 65000) {
			initial_port =
				30250 +
				2 * (int) ((int) 15000 *
						   (osip_build_random_number() /
							(UINT_MAX + 1.0)));
			if (initial_port < 30250 || initial_port > 65000) {
				initial_port = 30250;
			}
		}
	}

	_antisipc.port_range_min = initial_port;
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "Range audio port start at: %i\r\n",
						  initial_port));
	return 0;
}

PPL_DECLARE (int) am_option_enable_rport(int enable)
{
	if (enable)
		enable = 1;
	_antisipc.use_rport = enable;
	eXosip_set_option(EXOSIP_OPT_USE_RPORT, &_antisipc.use_rport);
	return 0;
}

PPL_DECLARE (int) am_option_enable_outbound_proxy(const char *outbound)
{
	if (outbound == NULL) {
		memset(_antisipc.outbound_proxy, 0, 256);
		return -1;
	}

	snprintf(_antisipc.outbound_proxy, 256, "%s", outbound);
	return 0;
}

PPL_DECLARE (int) am_option_enable_keepalive(int enable)
{
	if (enable < 0)
		enable = 0;
	_antisipc.use_udpkeepalive = enable;
	eXosip_set_option(EXOSIP_OPT_UDP_KEEP_ALIVE,
					  &_antisipc.use_udpkeepalive);
	return 0;
}

PPL_DECLARE (int) am_option_set_audio_profile(const char *profile)
{
	if (profile == NULL) {
		memset(_antisipc.audio_profile, 0,
			   sizeof(_antisipc.audio_profile));
		return -1;
	}

	snprintf(_antisipc.audio_profile, sizeof(_antisipc.audio_profile),
			 "%s", profile);
	return 0;
}

PPL_DECLARE (int) am_option_set_video_profile(const char *profile)
{
	if (profile == NULL) {
		memset(_antisipc.video_profile, 0,
			   sizeof(_antisipc.video_profile));
		return -1;
	}

	snprintf(_antisipc.video_profile, sizeof(_antisipc.video_profile),
			 "%s", profile);
	return 0;
}

PPL_DECLARE (int) am_option_set_text_profile(const char *profile)
{
	if (profile == NULL) {
		memset(_antisipc.text_profile, 0, sizeof(_antisipc.text_profile));
		return -1;
	}

	snprintf(_antisipc.text_profile, sizeof(_antisipc.text_profile), "%s",
			 profile);
	return 0;
}

PPL_DECLARE (int) am_option_set_udpftp_profile(const char *profile)
{
	if (profile == NULL) {
		memset(_antisipc.udpftp_profile, 0,
			   sizeof(_antisipc.udpftp_profile));
		return -1;
	}

	snprintf(_antisipc.udpftp_profile, sizeof(_antisipc.udpftp_profile),
			 "%s", profile);
	return 0;
}

PPL_DECLARE (int) am_option_enable_101(int enable)
{
	return 0;
}

PPL_DECLARE (int) am_option_enable_session_timers(int session_expires)
{
	_antisipc.session_timers = session_expires;
	return 0;
}

PPL_DECLARE (int) am_option_enable_sdp_in_ack(int enable)
{
	if (enable)
		enable = 1;
	_antisipc.do_sdp_in_ack = enable;
	return 0;
}


PPL_DECLARE (int)
am_option_enable_relay_server(const char *relay_server,
							  int use_relay_server)
{
	memset(_antisipc.relay_server, '\0', sizeof(_antisipc.relay_server));
	_antisipc.use_relay_server = 0;
	if (relay_server == NULL || relay_server[0] == '\0') {
		return 0;
	}
	_antisipc.use_relay_server = use_relay_server;
	snprintf(_antisipc.relay_server, 256, "%s", relay_server);
	return 0;
}

PPL_DECLARE (int)
am_option_enable_turn_server(const char *turn_server, int use_turn_server)
{
	memset(_antisipc.stun_server, '\0', sizeof(_antisipc.stun_server));
	_antisipc.use_stun_server = 0;
	_antisipc.use_turn_server = 0;
	if (turn_server == NULL || turn_server[0] == '\0') {
		return 0;
	}
	_antisipc.use_stun_server = use_turn_server;
	_antisipc.use_turn_server = use_turn_server;
	snprintf(_antisipc.stun_server, 256, "%s", turn_server);
	return 0;
}

PPL_DECLARE (int)
am_option_enable_stun_server(const char *stun_server, int use_stun_server)
{
	memset(_antisipc.stun_server, '\0', sizeof(_antisipc.stun_server));
	_antisipc.use_stun_server = 0;
	if (stun_server == NULL || stun_server[0] == '\0') {
		return 0;
	}
	_antisipc.use_stun_server = use_stun_server;
	snprintf(_antisipc.stun_server, 256, "%s", stun_server);
	return 0;
}

PPL_DECLARE (int)
am_option_set_ipv4_for_gateway(const char *ipv4_for_gateway)
{
	if (ipv4_for_gateway[0] != '\0' && _antisipc.ipv4_for_gateway != '\0'
		&& osip_strcasecmp(ipv4_for_gateway,
						   _antisipc.ipv4_for_gateway) == 0)
		return 0;
	if (ipv4_for_gateway[0] == '\0' && _antisipc.ipv4_for_gateway == '\0')
		return 0;

	if (ipv4_for_gateway[0] != '\0') {
		int mtu;

		/* create the cache entry */
		struct eXosip_dns_cache entry;
		memset(&entry, 0, sizeof(struct eXosip_dns_cache));
		snprintf(entry.host, sizeof(entry.host), "%s", ipv4_for_gateway);
		eXosip_set_option(EXOSIP_OPT_ADD_DNS_CACHE, (void *) &entry);

		mtu = 1492;			/* default maximum MTU */
		if (entry.ip[0] != '\0')
		{
			mtu = ms_discover_mtu(entry.ip);
			if (mtu > 1492 || mtu < 68)
				mtu = 1492;			/* default maximum MTU */
		}
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "MTU set to %i\r\n", mtu));
		ms_set_mtu(mtu);
	}

	memset(_antisipc.ipv4_for_gateway, '\0',
		   sizeof(_antisipc.ipv4_for_gateway));

	eXosip_set_option(EXOSIP_OPT_SET_IPV4_FOR_GATEWAY, ipv4_for_gateway);
	return 0;
}

PPL_DECLARE (int) am_option_set_dns_capabilities(int dns_capabilities)
{
	_antisipc.dns_capabilities = dns_capabilities;
	eXosip_set_option(EXOSIP_OPT_DNS_CAPABILITIES,
					  &_antisipc.dns_capabilities);
	return 0;
}

PPL_DECLARE (int) am_option_enable_symmetric_rtp(int enable)
{
	_antisipc.do_symmetric_rtp = enable;
	return 0;
}

PPL_DECLARE (int)
am_option_enable_vad(int enable, int vad_prob_start,
							 int vad_prob_continue)
{
	return am_option_conference_enable_vad(0, enable, vad_prob_start, vad_prob_continue);
}

PPL_DECLARE (int)
am_option_enable_agc(int enable, int agc_level, int max_gain)
{
	return am_option_conference_enable_agc(0, enable, agc_level, max_gain);
}

PPL_DECLARE (int)
am_option_set_denoise_level(int denoise_level)
{
	return am_option_conference_set_denoise_level(0, denoise_level);
}

PPL_DECLARE (int)
am_option_enable_echo_canceller(int enable, int frame_size, int tail_length)
{
	return am_option_conference_enable_echo_canceller(0, enable, frame_size, tail_length);
}

PPL_DECLARE (int)
am_option_conference_enable_vad(int conf_id, int enable, int vad_prob_start, int vad_prob_continue)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_enable_vad_conf != NULL)
		return _antisipc.audio_media->audio_module_enable_vad_conf(conf_id, enable, vad_prob_start, vad_prob_continue);
	return 0;
}

PPL_DECLARE (int)
am_option_conference_enable_agc(int conf_id, int enable, int agc_level, int max_gain)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_enable_agc_conf != NULL)
		return _antisipc.audio_media->audio_module_enable_agc_conf(conf_id, enable, agc_level, max_gain);
	return 0;
}

PPL_DECLARE (int)
am_option_conference_set_denoise_level(int conf_id, int denoise_level)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_denoise_level_conf != NULL)
		return _antisipc.audio_media->audio_module_set_denoise_level_conf(conf_id, denoise_level);
	return 0;
}

PPL_DECLARE (int)
am_option_conference_enable_echo_canceller(int conf_id, int enable, int frame_size, int tail_length)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_enable_echo_canceller_conf != NULL)
		return _antisipc.audio_media->audio_module_enable_echo_canceller_conf(conf_id, enable, frame_size, tail_length);
	return 0;
}

PPL_DECLARE (int) am_option_find_in_sound_card(struct am_sndcard *sndcard)
{
	return _antisipc.audio_media->audio_module_find_in_sound_card(sndcard);
}

PPL_DECLARE (int) am_option_find_out_sound_card(struct am_sndcard *sndcard)
{
	return _antisipc.audio_media->audio_module_find_out_sound_card(sndcard);
}

PPL_DECLARE (int) am_option_select_in_sound_card(int card)
{
	return am_option_conference_select_in_sound_card(0, card);
}

PPL_DECLARE (int) am_option_select_out_sound_card(int card)
{
	return am_option_conference_select_out_sound_card(0, card);
}

PPL_DECLARE (int)
am_option_select_in_custom_sound_card(MSSndCard * captcard)
{
	return am_option_conference_select_in_custom_sound_card(0, captcard);
}

PPL_DECLARE (int)
am_option_select_out_custom_sound_card(MSSndCard * playcard)
{
	return am_option_conference_select_out_custom_sound_card(0, playcard);
}

PPL_DECLARE (int) am_option_conference_select_in_sound_card(int conf_id, int card)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	return _antisipc.audio_media->audio_module_select_in_sound_card_conf(conf_id, card);
}

PPL_DECLARE (int) am_option_conference_select_out_sound_card(int conf_id, int card)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	return _antisipc.audio_media->audio_module_select_out_sound_card_conf(conf_id, card);
}

PPL_DECLARE (int)
am_option_conference_select_in_custom_sound_card(int conf_id, MSSndCard * captcard)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	return _antisipc.audio_media->audio_module_select_in_custom_sound_card_conf(conf_id, captcard);
}

PPL_DECLARE (int)
am_option_conference_select_out_custom_sound_card(int conf_id, MSSndCard * playcard)
{
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	return _antisipc.audio_media->audio_module_select_out_custom_sound_card_conf(conf_id, playcard);
}

PPL_DECLARE (int)
am_option_set_volume_out_sound_card(int card, int master, int percent)
{
	if (_antisipc.audio_media->audio_module_set_volume_out_sound_card!=NULL)
		return _antisipc.audio_media->audio_module_set_volume_out_sound_card(card, master, percent);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int)
am_option_get_volume_out_sound_card(int card, int master)
{
	if (_antisipc.audio_media->audio_module_get_volume_out_sound_card!=NULL)
		return _antisipc.audio_media->audio_module_get_volume_out_sound_card(card, master);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int)
am_option_set_volume_in_sound_card(int card, int percent)
{
	if (_antisipc.audio_media->audio_module_set_volume_in_sound_card!=NULL)
		return _antisipc.audio_media->audio_module_set_volume_in_sound_card(card, percent);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int)
am_option_get_volume_in_sound_card(int card)
{
	if (_antisipc.audio_media->audio_module_get_volume_in_sound_card!=NULL)
		return _antisipc.audio_media->audio_module_get_volume_in_sound_card(card);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int)
am_option_set_mute_out_sound_card(int card, int master, int val)
{
	if (_antisipc.audio_media->audio_module_set_mute_out_sound_card!=NULL)
		return _antisipc.audio_media->audio_module_set_mute_out_sound_card(card, master, val);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int)
am_option_set_mute_in_sound_card(int card, int val)
{
	if (_antisipc.audio_media->audio_module_set_mute_in_sound_card!=NULL)
		return _antisipc.audio_media->audio_module_set_mute_in_sound_card(card, val);
	return AMSIP_UNDEFINED_ERROR;
}

#ifdef ENABLE_VIDEO

PPL_DECLARE (int) am_option_find_camera(struct am_camera *camera)
{
	if (_antisipc.video_media->video_module_find_camera != NULL)
		return _antisipc.video_media->video_module_find_camera(camera);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int) am_option_select_camera(int card)
{
	if (_antisipc.video_media->video_module_select_camera != NULL)
		return _antisipc.video_media->video_module_select_camera(card);
	return AMSIP_UNDEFINED_ERROR;
}

PPL_DECLARE (int) am_option_set_selfview_mode(int mode)
{
	if (_antisipc.video_media->video_module_set_selfview_mode != NULL)
	    return _antisipc.video_media->video_module_set_selfview_mode(mode);
	return -1;
}

PPL_DECLARE (int) am_option_set_selfview_position(float posx, float posy, float size)
{
	if (_antisipc.video_media->video_module_set_selfview_position != NULL)
		return _antisipc.video_media->video_module_set_selfview_position(posx,posy,size);
	return -1;
}

PPL_DECLARE (int) am_option_get_selfview_position(float *posx, float *posy, float *size)
{
	if (_antisipc.video_media->video_module_get_selfview_position != NULL)
		return _antisipc.video_media->video_module_get_selfview_position(posx,posy,size);
	return -1;
}

PPL_DECLARE (int) am_option_set_selfview_scalefactor(float scalefactor)
{
	if (_antisipc.video_media->video_module_set_selfview_scalefactor != NULL)
		return _antisipc.video_media->video_module_set_selfview_scalefactor(scalefactor);
	return -1;
}

PPL_DECLARE (int) am_option_set_background_color(int red, int green, int blue)
{
	if (_antisipc.video_media->video_module_set_background_color != NULL)
		return _antisipc.video_media->video_module_set_background_color(red, green, blue);
	return -1;
}

PPL_DECLARE (int) am_option_set_video_option(int opt, void *arg)
{
	if (_antisipc.video_media->video_module_set_video_option != NULL)
		return _antisipc.video_media->video_module_set_video_option(opt, arg);
	return -1;
}

#endif

PPL_DECLARE (int)
am_option_set_password_for_username(const char *from_username, const char *realm, const char *login,
					   const char *passwd)
{
	if (login[0] != '\0' && passwd[0] != '\0') {
		eXosip_lock();
		eXosip_add_authentication_info(from_username, login, passwd, NULL, realm);
		eXosip_unlock();
		return 0;
	}
	return -1;
}

PPL_DECLARE (int)
am_option_set_password(const char *realm, const char *login,
					   const char *passwd)
{
	if (login[0] != '\0' && passwd[0] != '\0') {
		eXosip_lock();
		eXosip_add_authentication_info(login, login, passwd, NULL, realm);
		eXosip_unlock();
		return 0;
	}
	return -1;
}

PPL_DECLARE (int)
am_option_delete_password(const char *realm, const char *login)
{
	if (login[0] != '\0') {
		eXosip_lock();
		eXosip_remove_authentication_info(login, realm);
		eXosip_unlock();
		return 0;
	}
	return -1;
}

PPL_DECLARE (int)
am_option_remove_password()
{
	return eXosip_clear_authentication_info();
}

PPL_DECLARE (int) am_option_load_plugins(const char *directory)
{
	return ms_load_plugins(directory);
}

PPL_DECLARE (int) am_option_set_rate(int rate)
{
	return am_option_conference_set_rate(0, rate);
}

PPL_DECLARE (int) am_option_set_callback(unsigned int id, MSFilterNotifyFunc speex_pp_process, void *userdata)
{
	return am_option_conference_set_callback(0, id, speex_pp_process, userdata);
}

PPL_DECLARE (int) am_option_set_volume_gain(float capture_gain, float playback_gain)
{
	return am_option_conference_set_volume_gain(0, capture_gain, playback_gain);
}

PPL_DECLARE (int) am_option_set_echo_limitation(int enabled, float threshold, float speed, float force, int sustain)
{
	return am_option_conference_set_echo_limitation(0, enabled, threshold, speed, force, sustain);
}

PPL_DECLARE (int) am_option_set_noise_gate_threshold(int enabled, float threshold)
{
	return am_option_conference_set_noise_gate_threshold(0, enabled, threshold);
}

PPL_DECLARE (int) am_option_set_equalizer_state(int enable)
{
	return am_option_conference_set_equalizer_state(0, enable);
}

PPL_DECLARE (int) am_option_set_equalizer_params(float frequency, float gain, float width)
{
	return am_option_conference_set_equalizer_params(0, frequency, gain, width);
}

PPL_DECLARE (int) am_option_set_mic_equalizer_state(int enable)
{
	return am_option_conference_set_mic_equalizer_state(0, enable);
}

PPL_DECLARE (int) am_option_set_mic_equalizer_params(float frequency, float gain, float width)
{
	return am_option_conference_set_mic_equalizer_params(0, frequency, gain, width);
}

PPL_DECLARE (int) am_option_conference_set_rate(int conf_id, int rate)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_rate_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_rate_conf(conf_id, rate);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_callback(int conf_id, unsigned int id, MSFilterNotifyFunc speex_pp_process, void *userdata)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_callback_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_callback_conf(conf_id, id, speex_pp_process, userdata);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_volume_gain(int conf_id, float capture_gain, float playback_gain)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_volume_gain_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_volume_gain_conf(conf_id, capture_gain, playback_gain);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_echo_limitation(int conf_id, int enabled, float threshold, float speed, float force, int sustain)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_echo_limitation_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_echo_limitation_conf(conf_id, enabled, threshold, speed, force, sustain);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_noise_gate_threshold(int conf_id, int enabled, float threshold)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_noise_gate_threshold_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_noise_gate_threshold_conf(conf_id, enabled, threshold);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_equalizer_state(int conf_id, int enable)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_equalizer_state_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_equalizer_state_conf(conf_id, enable);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_equalizer_params(int conf_id, float frequency, float gain, float width)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_equalizer_params_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_equalizer_params_conf(conf_id, frequency, gain, width);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_mic_equalizer_state(int conf_id, int enable)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_mic_equalizer_state_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_mic_equalizer_state_conf(conf_id, enable);
	return i;
}

PPL_DECLARE (int) am_option_conference_set_mic_equalizer_params(int conf_id, float frequency, float gain, float width)
{
	int i = AMSIP_UNDEFINED_ERROR;
	conf_id = (conf_id>=AMSIP_CONF_MAX)?0:conf_id;
	conf_id = (conf_id<0)?0:conf_id;
	if (_antisipc.audio_media->audio_module_set_mic_equalizer_params_conf != NULL)
		i = _antisipc.audio_media->audio_module_set_mic_equalizer_params_conf(conf_id, frequency, gain, width);
	return i;
}

/**
 * Retreive a socket where data is written when a eXosip_event
 * is available in amsip/eXosip2 fifo.
 *
 */
PPL_DECLARE (int) am_option_geteventsocket(void)
{
	return eXosip_event_geteventsocket();
}


#ifdef ENABLE_RESIZE_AT_RUNTIME
/**
 * Resize information for rescaling in an SDL window.
 *
 */
PPL_DECLARE (int) am_option_resize_sdl_window(int w, int h)
{
	MSVideoSize sz;
	int val = 1;
	sz.width = w;
	sz.height = h;
	if (_antisipc.main_video_output != NULL)
		ms_filter_call_method(_antisipc.main_video_output,
							  MS_FILTER_SET_VIDEO_SIZE, &sz);
	if (_antisipc.main_video_output != NULL)
		ms_filter_call_method(_antisipc.main_video_output,
							  MS_VIDEO_OUT_SET_CORNER, &val);
	return 0;
}
#endif
