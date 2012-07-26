/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef DISABLE_CONF_MODE

#include "am_calls.h"

#include <ortp/ortp.h>
#include <ortp/telephonyevents.h>


#ifndef DISABLE_SRTP
#include <ortp/srtp.h>
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#ifdef ANDROID
#include <android/log.h>
//#define ENABLE_NOCONF_MODE
#endif

#if defined (_WIN32_WCE)
//#define DISABLE_VOLUME
#define DISABLE_EQUALIZER
#endif

#if TARGET_OS_IPHONE
#define DISABLE_EQUALIZER
#endif


#ifdef TWSMEDIASERVER
#define DISABLE_EQUALIZER
#define DISABLE_AEC
#endif

#include "mediastreamer2/dtmfgen.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilerec.h"

#ifndef DISABLE_VOLUME
#include "mediastreamer2/msvolume.h"
#endif
#ifndef DISABLE_EQUALIZER
#include "mediastreamer2/msequalizer.h"
#endif

#include <osip2/osip_time.h>
#include <osip2/osip_mt.h>

#include "sdptools.h"

#define KEEP_RTP_SESSION

static int audio_module_set_equalizer_state_conf(int conf_id, int enable);
static int audio_module_set_mic_equalizer_state_conf(int conf_id, int enable);

#ifdef TEST_SIMON
#define EQUALIZER_CONFIG_FILE "/tmp/eq.conf"
#define EQUALIZER_MIC_CONFIG_FILE "/tmp/mic_eq.conf"

static void configure_equalizer(MSFilter *f, const char *config_file){
	char buf[4096];
	FILE *file=fopen(config_file,"r");
	char *p;
	int enabled=0;
	char *gains;
	if (file==NULL){
		ms_error("Could not open %s\r\n",config_file);
		return;
	}
	if (fread(buf,1,sizeof(buf),file)<=0){
		ms_error("Could not read %s\r\n",config_file);
		fclose(file);
		return;
	}
	fclose(file);
	p=strstr(buf,"eq_active=");
	if (p){
		sscanf(buf,"eq_active=%i",&enabled);
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,"Equalizer state is %i\r\n",enabled));
	}else{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,"Could not parse config file, missing eq_active %s\r\n",config_file));
		return;
	}
	p=strstr(buf,"eq_gains=");
	if (p){
		char *e;
		p+=strlen("eq_gains=");
		e=strchr(p,'\n');
		if (e) *e='\0';
		gains=p;
		ms_message("Found eq_gains=%s\r\n",gains);
	}else return;
	ms_filter_call_method(f,MS_EQUALIZER_SET_ACTIVE,&enabled);
	if (enabled){
		if (gains){
			do{
				int bytes;
				MSEqualizerGain g;
				if (sscanf(gains,"%f:%f:%f %n",&g.frequency,&g.gain,&g.width,&bytes)>=3){
					ms_message("Read equalizer gains: %f(~%f) --> %f",g.frequency,g.width,g.gain);
					ms_filter_call_method(f,MS_EQUALIZER_SET_GAIN,&g);
					gains+=bytes;
				}else break;
			}while(1);
		}
	}
}
#endif

struct am_audio_info {
	char audio_record_file[256];
	int audio_enable_record;

	/* mediastreamer2 support */
	RtpSession *audio_rtp_session;
	RtpProfile *audio_recv_rtp_profile;
	RtpProfile *audio_send_rtp_profile;
	MSFilter *audio_encoder;
	MSFilter *audio_decoder[5];
	MSFilter *dtmfinband;
	MSFilter *audio_dec2conf;
	MSFilter *audio_conf2enc;
	MSFilter *audio_rtprecv;
	MSFilter *audio_rtpsend;
	MSFilter *audio_ice;
	MSFilter *audio_msjoin;
	MSFilter *audio_external_encoder;
	MSFilter *audio_f_recorder;
	MSFilter *audio_fileplayer;
	MSFilter *audio_fileplayer2;

	OrtpEvQueue *audio_dtmf_queue;
	OrtpEvQueue *audio_rtp_queue;
	int rfc2833_supported;
};

struct audio_module_ctx {

	int use_rate;
	int in_snd_card;
	int out_snd_card;

	MSTicker *ticker;
	MSFilter *soundread;
	MSFilter *soundwrite;
	MSFilter *resample_soundread;
	MSFilter *resample_soundwrite;
	MSFilter *conference;
	MSFilter *ec;
	MSFilter *dtmfplayback;
	MSFilter *volrecv;
	MSFilter *volsend;
	MSFilter *equalizer;
	MSFilter *mic_equalizer;
	struct am_audio_info ctx[MAX_NUMBER_OF_CALLS];
};

struct audio_module_ctx tws_ctx[AMSIP_CONF_MAX];

#ifndef DISABLE_SRTP
int
am_get_security_descriptions(RtpSession * rtp,
							 sdp_message_t * sdp_answer,
							 sdp_message_t * sdp_offer,
							 sdp_message_t * sdp_local,
							 sdp_message_t * sdp_remote, char *media_type);
#endif

#ifdef ANDROID
static void __amsip_logv_out(OrtpLogLevel lev, const char *fmt, va_list args){
	int prio;
	switch(lev){
	case ORTP_DEBUG:	prio = ANDROID_LOG_DEBUG;	break;
	case ORTP_MESSAGE:	prio = ANDROID_LOG_INFO;	break;
	case ORTP_WARNING:	prio = ANDROID_LOG_WARN;	break;
	case ORTP_ERROR:	prio = ANDROID_LOG_ERROR;	break;
	case ORTP_FATAL:	prio = ANDROID_LOG_FATAL;	break;
	default:		prio = ANDROID_LOG_DEFAULT;	break;
	}
	__android_log_vprint(prio, _antisipc.syslog_name, fmt, args);
}
#else
static void
__amsip_logv_out(OrtpLogLevel lev, const char *fmt, va_list args)
{
	const char *lname = "undef";
	char msg[512];
	int in = 0;

	switch (lev) {
	case ORTP_DEBUG:
		lname = "debug";
		break;
	case ORTP_MESSAGE:
		lname = "message";
		break;
	case ORTP_WARNING:
		lname = "warning";
		break;
	case ORTP_ERROR:
		lname = "error";
		break;
	case ORTP_FATAL:
		lname = "fatal";
		break;
	default:
		ortp_fatal("Bad level !");
	}
#ifdef WIN32
	in = _snprintf(msg, 511, "ortp-%s-", lname);
	_vsnprintf(msg + in, 511 - in, fmt, args);
#else
	in = snprintf(msg, 511, "ortp-%s-", lname);
	vsnprintf(msg + in, 511 - in, fmt, args);
#endif
	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO1, NULL, "%s: %s\n",
				_antisipc.syslog_name, msg));
}
#endif

static int audio_module_init_init()
{

	ortp_init();
	ortp_set_log_handler(__amsip_logv_out);
	ortp_set_log_level_mask(ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR |
							ORTP_FATAL);
	/* ortp_set_log_level_mask(ORTP_WARNING|ORTP_ERROR|ORTP_FATAL); */
	ms_init();
	am_filter_register();

	return 0;
}

static int audio_module_init_conf(int conf_id, const char *name, int debug_level)
{
	memset(&tws_ctx[conf_id], 0, sizeof(tws_ctx[conf_id]));
	tws_ctx[conf_id].use_rate = 8000;
	_antisipc.audio_dscp = 0x38;
	_antisipc.do_symmetric_rtp = 1;

	/* create ticker */
	tws_ctx[conf_id].ticker = ms_ticker_new_withname("amsip-audio");
#ifndef ENABLE_NOCONF_MODE
	tws_ctx[conf_id].conference = ms_filter_new(MS_CONF_ID);
#else
	tws_ctx[conf_id]..conference = ms_filter_new(MS_NOCONF_ID);
#endif

	if (tws_ctx[conf_id].conference != NULL) {
		int agc_level = 0;

		ms_filter_call_method(tws_ctx[conf_id].conference, MS_FILTER_ENABLE_AGC,
							  &agc_level);
	}
	tws_ctx[conf_id].dtmfplayback = ms_filter_new(MS_DTMF_GEN_ID);


#ifndef DISABLE_VOLUME
	tws_ctx[conf_id].volrecv = ms_filter_new(MS_VOLUME_ID);
	tws_ctx[conf_id].volsend = ms_filter_new(MS_VOLUME_ID);

#else
	tws_ctx[conf_id].volrecv = ms_filter_new(MS_TEE_ID);
	tws_ctx[conf_id].volsend = ms_filter_new(MS_TEE_ID);
#endif
#ifndef DISABLE_AEC
	tws_ctx[conf_id].ec = ms_filter_new(MS_SPEEX_EC_ID);
#endif

	tws_ctx[conf_id].resample_soundread = ms_filter_new(MS_RESAMPLE_ID);
	tws_ctx[conf_id].resample_soundwrite = ms_filter_new(MS_RESAMPLE_ID);

#ifndef DISABLE_EQUALIZER
	tws_ctx[conf_id].equalizer=ms_filter_new(MS_EQUALIZER_ID);
	tws_ctx[conf_id].mic_equalizer=ms_filter_new(MS_EQUALIZER_ID);
	audio_module_set_equalizer_state_conf(conf_id, 0);
	audio_module_set_mic_equalizer_state_conf(conf_id, 0);
#ifdef TEST_SIMON
	configure_equalizer(tws_ctx[conf_id].equalizer,EQUALIZER_CONFIG_FILE);
	configure_equalizer(tws_ctx[conf_id].mic_equalizer,EQUALIZER_MIC_CONFIG_FILE);
#endif
#else
	tws_ctx[conf_id].equalizer=ms_filter_new(MS_TEE_ID);
	tws_ctx[conf_id].mic_equalizer=ms_filter_new(MS_TEE_ID);
#endif
	_antisipc.audio_media->audio_module_select_out_sound_card_conf(conf_id, -1);
	_antisipc.audio_media->audio_module_select_in_sound_card_conf(conf_id, -1);
	if (tws_ctx[conf_id].soundread == NULL || tws_ctx[conf_id].soundwrite == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "No sound card found\r\n"));
		/* return -1; */
	}

	return 0;
}

static int audio_module_init(const char *name, int debug_level)
{
	int k;
	audio_module_init_init();
	for (k=0; k < AMSIP_CONF_MAX; k++)
		audio_module_init_conf(k, name, debug_level);

	return 0;
}

static int audio_module_reset(const char *name, int debug_level)
{
	MSSndCardManager *scm;
	int k;

	scm = ms_snd_card_manager_get();
	if (scm != NULL)
		ms_snd_card_manager_reload(scm);

	for (k=0; k < AMSIP_CONF_MAX; k++)
		audio_module_init_conf(k, name, debug_level);

	return 0;
}

static int audio_module_quit_conf(int conf_id)
{
	ms_filter_unlink(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
	ms_filter_unlink(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
	ms_filter_unlink(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread, 0);
	ms_filter_unlink(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
		ms_filter_unlink(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer, 0);
		ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
		ms_filter_unlink(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
	} else {
		ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].equalizer,
						 0);
		ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].volsend,
						 0);
	}
	ms_filter_unlink(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);
	ms_filter_unlink(tws_ctx[conf_id].equalizer, 0, tws_ctx[conf_id].resample_soundwrite,0);
	ms_filter_unlink(tws_ctx[conf_id].resample_soundwrite,0,tws_ctx[conf_id].soundwrite,0);
	if (tws_ctx[conf_id].ec != NULL)
		ms_filter_destroy(tws_ctx[conf_id].ec);
	if (tws_ctx[conf_id].volrecv != NULL)
		ms_filter_destroy(tws_ctx[conf_id].volrecv);
	if (tws_ctx[conf_id].volsend != NULL)
		ms_filter_destroy(tws_ctx[conf_id].volsend);
	if (tws_ctx[conf_id].equalizer != NULL)
		ms_filter_destroy(tws_ctx[conf_id].equalizer);
	if (tws_ctx[conf_id].mic_equalizer != NULL)
		ms_filter_destroy(tws_ctx[conf_id].mic_equalizer);
	if (tws_ctx[conf_id].dtmfplayback != NULL)
		ms_filter_destroy(tws_ctx[conf_id].dtmfplayback);
	if (tws_ctx[conf_id].ticker != NULL)
		ms_ticker_destroy(tws_ctx[conf_id].ticker);
	if (tws_ctx[conf_id].conference != NULL)
		ms_filter_destroy(tws_ctx[conf_id].conference);
	if (tws_ctx[conf_id].soundread != NULL)
		ms_filter_destroy(tws_ctx[conf_id].soundread);
	if (tws_ctx[conf_id].soundwrite != NULL)
		ms_filter_destroy(tws_ctx[conf_id].soundwrite);
	if (tws_ctx[conf_id].resample_soundread != NULL)
		ms_filter_destroy(tws_ctx[conf_id].resample_soundread);
	if (tws_ctx[conf_id].resample_soundwrite != NULL)
		ms_filter_destroy(tws_ctx[conf_id].resample_soundwrite);

	memset(&tws_ctx[conf_id], 0, sizeof(struct audio_module_ctx));
	return 0;
}

static int audio_module_quit()
{
	int k;
	for (k=0; k < AMSIP_CONF_MAX; k++)
		audio_module_quit_conf (k);

	return 0;
}

static int audio_module_set_rate_conf(int conf_id, int rate)
{
	int val;
	int i;
	if (rate != 8000 && rate != 16000 && rate != 32000 && rate != 44100
		&& rate != 48000)
		return -1;

	if (tws_ctx[conf_id].use_rate == rate)
		return 0;

	tws_ctx[conf_id].use_rate = rate;
	if (tws_ctx[conf_id].soundread != NULL && tws_ctx[conf_id].resample_soundread != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].soundread, MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);

		val = tws_ctx[conf_id].use_rate;
		i = ms_filter_call_method(tws_ctx[conf_id].soundread,
								  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (i != 0) {
			val = tws_ctx[conf_id].use_rate;
		}
		ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
							  MS_FILTER_SET_SAMPLE_RATE, &val);
		ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
	}

	if (tws_ctx[conf_id].soundwrite != NULL && tws_ctx[conf_id].resample_soundwrite != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].soundwrite,
							  MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);

		val = tws_ctx[conf_id].use_rate;
		i = ms_filter_call_method(tws_ctx[conf_id].soundwrite,
								  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (i != 0) {
			val = tws_ctx[conf_id].use_rate;
		}
		ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
							  MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
		ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE, &val);

		if (tws_ctx[conf_id].equalizer != NULL) {
			ms_filter_call_method(tws_ctx[conf_id].equalizer,
								  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
		}
		if (tws_ctx[conf_id].mic_equalizer != NULL) {
			ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,
								  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
		}
	}

	if (tws_ctx[conf_id].conference != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].conference,
							  MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
	}

	if (tws_ctx[conf_id].dtmfplayback != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].dtmfplayback, MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
	}

	if (tws_ctx[conf_id].ec != NULL) {
		int val = (128 * tws_ctx[conf_id].use_rate / 8000);

		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);

		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_FRAMESIZE,
							  (void *) &val);
		val = 2048;
		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_FILTERLENGTH,
							  (void *) &val);
#if TARGET_OS_IPHONE
		val = (5 * 128 * tws_ctx[conf_id].use_rate) / 8000;
		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_PLAYBACKDELAY,
							  &val);
#elif defined(ANDROID)
		val = (10 * 128 * tws_ctx[conf_id].use_rate) / 8000;
		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_PLAYBACKDELAY,
							  &val);
#endif
	}
	return 0;
}

static int
audio_module_enable_echo_canceller_conf(int conf_id, int enable, int frame_size,
								   int tail_length)
{
#ifdef DISABLE_AEC
	return 0;
#else

	if (enable == 0 && tws_ctx[conf_id].ec == NULL)
		return 0;				/* already disabled */

	if (tws_ctx[conf_id].conference == NULL || tws_ctx[conf_id].ticker == NULL
		|| tws_ctx[conf_id].volrecv == NULL || tws_ctx[conf_id].volsend == NULL 
		|| tws_ctx[conf_id].dtmfplayback == NULL
		|| tws_ctx[conf_id].soundread == NULL || tws_ctx[conf_id].soundwrite == NULL
		|| tws_ctx[conf_id].resample_soundread == NULL || tws_ctx[conf_id].resample_soundwrite == NULL
		|| tws_ctx[conf_id].equalizer == NULL || tws_ctx[conf_id].mic_equalizer == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (enable != 0) {
		/* check values */
		if (frame_size < 128) {
			frame_size = 160;
		}
		if (frame_size % 160 != 0 && frame_size % 128 != 0) {
			frame_size = 160;
		}

		if (tail_length == 0) {
			tail_length = 2096;
		}
		if (tail_length < 512) {
			tail_length = 512;
		}
		if (tail_length > 512 * 20) {
			tail_length = 10240;	/* should really be less than 4096 in most case */
		}
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	ms_filter_unlink(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
	ms_filter_unlink(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
	ms_filter_unlink(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread, 0);
	ms_filter_unlink(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
		ms_filter_unlink(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer, 0);
		ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
		ms_filter_unlink(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
	} else {
		ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].equalizer,
						 0);
		ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].volsend,
						 0);
	}
	ms_filter_unlink(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);
	ms_filter_unlink(tws_ctx[conf_id].equalizer, 0, tws_ctx[conf_id].resample_soundwrite,0);
	ms_filter_unlink(tws_ctx[conf_id].resample_soundwrite, 0, tws_ctx[conf_id].soundwrite,0);

	if (enable != 0 && tws_ctx[conf_id].ec != NULL) {
		/* already enabled */
	} else if (tws_ctx[conf_id].ec == NULL)
		tws_ctx[conf_id].ec = ms_filter_new(MS_SPEEX_EC_ID);
	else {
		ms_filter_destroy(tws_ctx[conf_id].ec);
		tws_ctx[conf_id].ec = NULL;
	}

	if (tws_ctx[conf_id].ec != NULL) {
		int val = (frame_size * tws_ctx[conf_id].use_rate) / 8000;

		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);

		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_FRAMESIZE,
							  (void *) &val);
		val = tail_length;
		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_FILTERLENGTH,
							  (void *) &val);

#if TARGET_OS_IPHONE
		val = (5 * frame_size * tws_ctx[conf_id].use_rate) / 8000;
		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_PLAYBACKDELAY,
							  &val);
#elif defined(ANDROID)
		val = (10 * frame_size * tws_ctx[conf_id].use_rate) / 8000;
		ms_filter_call_method(tws_ctx[conf_id].ec, MS_FILTER_SET_PLAYBACKDELAY,
							  &val);
#endif
	}

	ms_filter_link(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
	ms_filter_link(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_link(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
		ms_filter_link(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer, 0);
	} else
		ms_filter_link(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].equalizer, 0);
	ms_filter_link(tws_ctx[conf_id].equalizer, 0, tws_ctx[conf_id].resample_soundwrite, 0);
	ms_filter_link(tws_ctx[conf_id].resample_soundwrite, 0, tws_ctx[conf_id].soundwrite, 0);

	ms_filter_link(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread, 0);
	ms_filter_link(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_link(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
		ms_filter_link(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
	} else
	{
		ms_filter_link(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].volsend,
					   0);
	}
	ms_filter_link(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);

	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);

	return 0;
#endif
}

static int
audio_module_enable_vad_conf(int conf_id, int enable, int vad_prob_start,
								int vad_prob_continue)
{
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	if (tws_ctx[conf_id].conference != NULL) {
		int val = enable;
		ms_filter_call_method(tws_ctx[conf_id].conference,
							  MS_FILTER_ENABLE_VAD, &val);
		val = vad_prob_start;
		ms_filter_call_method(tws_ctx[conf_id].conference,
							  MS_FILTER_SET_VAD_PROB_START, &val);
		val = vad_prob_continue;
		ms_filter_call_method(tws_ctx[conf_id].conference,
							  MS_FILTER_SET_VAD_PROB_CONTINUE, &val);
	}
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
}

static int audio_module_enable_agc_conf(int conf_id, int enable, int agc_level, int max_gain)
{
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	if (tws_ctx[conf_id].conference != NULL) {
		if (enable <= 0)
			agc_level = 0;

		ms_filter_call_method(tws_ctx[conf_id].conference, MS_FILTER_ENABLE_AGC,
							  &agc_level);
		ms_filter_call_method(tws_ctx[conf_id].conference, MS_FILTER_SET_MAX_GAIN,
							  &max_gain);
	}
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
}

static int audio_module_set_denoise_level_conf(int conf_id, int denoise_level)
{
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	if (tws_ctx[conf_id].conference != NULL) {

		ms_filter_call_method(tws_ctx[conf_id].conference, MS_FILTER_SET_DENOISE_LEVEL,
							  &denoise_level);
	}
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
}

static int
audio_module_set_callback_conf(int conf_id, unsigned int id,
						  MSFilterNotifyFunc speex_pp_process,
						  void *userdata)
{
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	if (id == MS_CONF_SPEEX_PREPROCESS_MIC
		|| id == MS_CONF_CHANNEL_VOLUME) {
		if (tws_ctx[conf_id].conference != NULL) {
			ms_filter_set_notify_callback(tws_ctx[conf_id].conference,
										  speex_pp_process, userdata);
		}
	} else if (id == MS_SPEEX_EC_ECHO_STATE
			   || id == MS_SPEEX_EC_PREPROCESS_MIC) {
		if (tws_ctx[conf_id].ec != NULL) {
			ms_filter_set_notify_callback(tws_ctx[conf_id].ec, speex_pp_process,
										  userdata);
		}
	}
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
}

static int audio_module_set_volume_gain_conf(int conf_id, float capture_gain, float playback_gain)
{
#ifndef DISABLE_VOLUME
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].volrecv == NULL || tws_ctx[conf_id].volsend == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);

	ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_GAIN,&playback_gain);
	ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_GAIN,&capture_gain);
	
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

static int audio_module_set_echo_limitation_conf(int conf_id, int enabled,
									 float threshold,
									 float speed,
									 float force,
									 int sustain)
{
#ifndef DISABLE_VOLUME
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].volrecv == NULL || tws_ctx[conf_id].volsend == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);

	if (enabled==0)
	{
		ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_PEER,NULL);
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_PEER,NULL);
	}
	else
	{
		/* mic gain is controlled by receveid RTP */
		ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_PEER,NULL);
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_PEER,tws_ctx[conf_id].volrecv);

		ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_EA_THRESHOLD,&threshold);
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_EA_THRESHOLD,&threshold);
		
		ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_EA_SPEED,&speed);
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_EA_SPEED,&speed);
		
		ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_EA_FORCE,&force);
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_EA_FORCE,&force);

		ms_filter_call_method(tws_ctx[conf_id].volrecv,MS_VOLUME_SET_EA_SUSTAIN,&sustain);
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_EA_SUSTAIN,&sustain);
	}

	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

static int audio_module_set_noise_gate_threshold_conf(int conf_id, int enabled, float threshold)
{
#ifndef DISABLE_VOLUME
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].volrecv == NULL || tws_ctx[conf_id].volsend == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);

	ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_ENABLE_NOISE_GATE,&enabled);
	if (threshold>0)
	{
		ms_filter_call_method(tws_ctx[conf_id].volsend,MS_VOLUME_SET_NOISE_GATE_THRESHOLD,&threshold);
	}

	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

static int audio_module_set_equalizer_state_conf(int conf_id, int enable)
{
#ifndef DISABLE_EQUALIZER
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].equalizer == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (tws_ctx[conf_id].equalizer)
	{
		ms_filter_call_method(tws_ctx[conf_id].equalizer,MS_FILTER_SET_SAMPLE_RATE,&tws_ctx[conf_id].use_rate);
		ms_filter_call_method(tws_ctx[conf_id].equalizer,MS_EQUALIZER_SET_ACTIVE,&enable);
	}

	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

static int audio_module_set_equalizer_params_conf(int conf_id, float frequency, float gain, float width)
{
#ifndef DISABLE_EQUALIZER
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].equalizer == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (tws_ctx[conf_id].equalizer){
		MSEqualizerGain d;
		d.frequency=frequency;
		d.gain=gain;
		d.width=width;
		ms_filter_call_method(tws_ctx[conf_id].equalizer,MS_EQUALIZER_SET_GAIN,&d);
	}

	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}


static int audio_module_set_mic_equalizer_state_conf(int conf_id, int enable)
{
#ifndef DISABLE_EQUALIZER
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].mic_equalizer == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (tws_ctx[conf_id].mic_equalizer){
		ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,MS_FILTER_SET_SAMPLE_RATE,&tws_ctx[conf_id].use_rate);
		ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,MS_EQUALIZER_SET_ACTIVE,&enable);
	}

	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

static int audio_module_set_mic_equalizer_params_conf(int conf_id, float frequency, float gain, float width)
{
#ifndef DISABLE_EQUALIZER
	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL
		|| tws_ctx[conf_id].mic_equalizer == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (tws_ctx[conf_id].mic_equalizer){
		MSEqualizerGain d;
		d.frequency=frequency;
		d.gain=gain;
		d.width=width;
		ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,MS_EQUALIZER_SET_GAIN,&d);
	}


	return 0;
#else
	return AMSIP_UNDEFINED_ERROR;
#endif
}

static int audio_module_find_in_sound_card(struct am_sndcard *sndcard)
{
	MSSndCard *captcard = NULL;
	int card = sndcard->card;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	int k = 0;

	if (card > 50)
		return -1;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSSndCard *acard = (MSSndCard *) elem->data;

		if (acard->capabilities == MS_SND_CARD_CAP_CAPTURE
			|| acard->capabilities == 3) {
			if (k == card) {
				captcard = acard;
				ms_message("am_audio_module.c: found captcard = %s %s.",
						   acard->desc->driver_type, acard->name);
				break;
			}
			k++;
		}
	}
	if (captcard == NULL)
		return AMSIP_NODEVICE;

	sndcard->capabilities = captcard->capabilities;
	snprintf(sndcard->name, sizeof(sndcard->name), "%s", captcard->name);
	snprintf(sndcard->driver_type, sizeof(sndcard->driver_type), "%s",
			 captcard->desc->driver_type);
	return 0;
}

static int audio_module_find_out_sound_card(struct am_sndcard *sndcard)
{
	MSSndCard *playcard = NULL;
	int card = sndcard->card;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	int k = 0;

	if (card > 50)
		return -1;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSSndCard *acard = (MSSndCard *) elem->data;

		if (acard->capabilities == MS_SND_CARD_CAP_PLAYBACK
			|| acard->capabilities == 3) {
			if (k == card) {
				playcard = acard;
				ms_message("am_audio_module.c: found playcard = %s %s.",
						   acard->desc->driver_type, acard->name);
				break;
			}
			k++;
		}
	}

	if (playcard == NULL)
		return AMSIP_NODEVICE;

	sndcard->capabilities = playcard->capabilities;
	snprintf(sndcard->name, sizeof(sndcard->name), "%s", playcard->name);
	snprintf(sndcard->driver_type, sizeof(sndcard->driver_type), "%s",
			 playcard->desc->driver_type);
	return 0;
}

static int audio_module_select_in_sound_card_conf(int conf_id, int card)
{
	MSSndCard *captcard = NULL;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	int k = 0;
	MSFilter *soundread;
	int i;
	int val;

	if (card > 20)
		return -1;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSSndCard *acard = (MSSndCard *) elem->data;

		if (acard->capabilities == MS_SND_CARD_CAP_CAPTURE
			|| acard->capabilities == 3) {
			if (k == card) {
				captcard = acard;
				ms_message("am_audio_module.c: select captcard = %s %s.",
						   acard->desc->driver_type, acard->name);
				break;
			}
			k++;
		}
	}

	if (captcard == NULL) {
		elem = ms_snd_card_manager_get_list(ms_snd_card_manager_get());
		for (; elem != NULL; elem = elem->next) {
			MSSndCard *acard = (MSSndCard *) elem->data;

			if (acard->capabilities == MS_SND_CARD_CAP_CAPTURE
				|| acard->capabilities == 3) {
				captcard = acard;
				ms_message
					("am_audio_module.c: select default captcard = %s %s.",
					 acard->desc->driver_type, acard->name);
				break;
			}
		}
	}

	if (captcard == NULL)
		return AMSIP_NODEVICE;

	soundread = ms_snd_card_create_reader(captcard);
	if (soundread == NULL)
		return -1;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	if (tws_ctx[conf_id].soundread != NULL) {
		ms_filter_unlink(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread,
						 0);
		ms_filter_unlink(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer,
						 0);
		if (tws_ctx[conf_id].ec != NULL) {
			ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
			ms_filter_unlink(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
		} else
		{
			ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0,
							 tws_ctx[conf_id].volsend, 0);
		}
		ms_filter_unlink(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);
		ms_filter_destroy(tws_ctx[conf_id].soundread);
	}

	tws_ctx[conf_id].soundread = soundread;

	ms_filter_call_method(tws_ctx[conf_id].soundread, MS_FILTER_SET_SAMPLE_RATE,
						  &tws_ctx[conf_id].use_rate);

	val = tws_ctx[conf_id].use_rate;
	i = ms_filter_call_method(tws_ctx[conf_id].soundread, MS_FILTER_GET_SAMPLE_RATE,
							  &val);
	if (i != 0) {
		val = tws_ctx[conf_id].use_rate;
	}
	ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
						  MS_FILTER_SET_SAMPLE_RATE, &val);
	ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
						  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						  &tws_ctx[conf_id].use_rate);

	ms_filter_link(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread, 0);
	ms_filter_link(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_link(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
		ms_filter_link(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
	} else
	{
		ms_filter_link(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].volsend,
					   0);
	}
	ms_filter_link(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);

	if (card < 0)
#ifdef WIN32
	{
		tws_ctx[conf_id].in_snd_card = WAVE_MAPPER;
	}
#else
	{
		tws_ctx[conf_id].in_snd_card = -1;
	}
#endif
	else
		tws_ctx[conf_id].in_snd_card = card;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}


	return 0;
}

static int audio_module_select_out_sound_card_conf(int conf_id, int card)
{
	MSSndCard *playcard = NULL;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	int k = 0;
	MSFilter *soundwrite;
	int i;
	int val;

	if (card > 20)
		return -1;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSSndCard *acard = (MSSndCard *) elem->data;

		if (acard->capabilities == MS_SND_CARD_CAP_PLAYBACK
			|| acard->capabilities == 3) {
			if (k == card) {
				playcard = acard;
				ms_message("am_audio_module.c: select playcard = %s %s.",
						   acard->desc->driver_type, acard->name);
				break;
			}
			k++;
		}
	}

	if (playcard == NULL) {
		elem = ms_snd_card_manager_get_list(ms_snd_card_manager_get());
		for (; elem != NULL; elem = elem->next) {
			MSSndCard *acard = (MSSndCard *) elem->data;

			if (acard->capabilities == MS_SND_CARD_CAP_PLAYBACK
				|| acard->capabilities == 3) {
				playcard = acard;
				ms_message
					("am_audio_module.c: select default playcard = %s %s.",
					 acard->desc->driver_type, acard->name);
				break;
			}
		}
	}

	if (playcard == NULL)
		return AMSIP_NODEVICE;

	soundwrite = ms_snd_card_create_writer(playcard);
	if (soundwrite == NULL)
		return -1;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	if (tws_ctx[conf_id].soundwrite != NULL) {
		ms_filter_unlink(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
		ms_filter_unlink(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
		if (tws_ctx[conf_id].ec != NULL) {
			ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
			ms_filter_unlink(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer,
							 0);
		} else
			ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0,
							 tws_ctx[conf_id].equalizer, 0);
		ms_filter_unlink(tws_ctx[conf_id].equalizer, 0,
						 tws_ctx[conf_id].resample_soundwrite, 0);
		ms_filter_unlink(tws_ctx[conf_id].resample_soundwrite, 0,
						 tws_ctx[conf_id].soundwrite, 0);
		ms_filter_destroy(tws_ctx[conf_id].soundwrite);
	}
	tws_ctx[conf_id].soundwrite = soundwrite;

	ms_filter_call_method(tws_ctx[conf_id].soundwrite, MS_FILTER_SET_SAMPLE_RATE,
						  &tws_ctx[conf_id].use_rate);

	val = tws_ctx[conf_id].use_rate;
	i = ms_filter_call_method(tws_ctx[conf_id].soundwrite,
							  MS_FILTER_GET_SAMPLE_RATE, &val);
	if (i != 0) {
		val = tws_ctx[conf_id].use_rate;
	}
	ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
						  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
	ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
						  MS_FILTER_SET_OUTPUT_SAMPLE_RATE, &val);

	if (tws_ctx[conf_id].equalizer != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].equalizer,
							  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
	}

	ms_filter_link(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
	ms_filter_link(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_link(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
		ms_filter_link(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer, 0);
	} else
		ms_filter_link(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].equalizer, 0);
	ms_filter_link(tws_ctx[conf_id].equalizer, 0, tws_ctx[conf_id].resample_soundwrite, 0);
	ms_filter_link(tws_ctx[conf_id].resample_soundwrite, 0, tws_ctx[conf_id].soundwrite, 0);

	if (card < 0)
#ifdef WIN32
	{
		tws_ctx[conf_id].out_snd_card = WAVE_MAPPER;
	}
#else
	{
		tws_ctx[conf_id].out_snd_card = -1;
	}
#endif
	else
		tws_ctx[conf_id].out_snd_card = card;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	return 0;
}

/**
 * Configure amsip to use a specific audio card for recording audio
 *
 * @param card        MSSndCard object pointer
 */

static int audio_module_select_in_custom_sound_card_conf(int conf_id, MSSndCard * captcard)
{
	int k;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	MSFilter *soundread;
	int i;
	int val;

	if (captcard == NULL) {
		for (; elem != NULL; elem = elem->next) {
			MSSndCard *acard = (MSSndCard *) elem->data;

			if (acard->capabilities == MS_SND_CARD_CAP_CAPTURE
				|| acard->capabilities == 3) {
				captcard = acard;
				ms_message
					("am_audio_module.c: select default captcard = %s %s.",
					 acard->desc->driver_type, acard->name);
				break;
			}
		}
	}

	if (captcard == NULL)
		return AMSIP_NODEVICE;


	soundread = ms_snd_card_create_reader(captcard);
	if (soundread == NULL)
		return -1;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	if (tws_ctx[conf_id].soundread != NULL) {
		ms_filter_unlink(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread,
						 0);
		ms_filter_unlink(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer,
						 0);
		if (tws_ctx[conf_id].ec != NULL) {
			ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
			ms_filter_unlink(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
		} else
		{
			ms_filter_unlink(tws_ctx[conf_id].mic_equalizer, 0,
							 tws_ctx[conf_id].volsend, 0);
		}
		ms_filter_unlink(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);
		ms_filter_destroy(tws_ctx[conf_id].soundread);
	}

	tws_ctx[conf_id].soundread = soundread;

	ms_filter_call_method(tws_ctx[conf_id].soundread, MS_FILTER_SET_SAMPLE_RATE,
						  &tws_ctx[conf_id].use_rate);

	val = tws_ctx[conf_id].use_rate;
	i = ms_filter_call_method(tws_ctx[conf_id].soundread, MS_FILTER_GET_SAMPLE_RATE,
							  &val);
	if (i != 0) {
		val = tws_ctx[conf_id].use_rate;
	}
	ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
						  MS_FILTER_SET_SAMPLE_RATE, &val);  
	ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,
						  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
	ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
						  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						  &tws_ctx[conf_id].use_rate);

	ms_filter_link(tws_ctx[conf_id].soundread, 0, tws_ctx[conf_id].resample_soundread, 0);
	ms_filter_link(tws_ctx[conf_id].resample_soundread, 0, tws_ctx[conf_id].mic_equalizer, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_link(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].ec, 1);
		ms_filter_link(tws_ctx[conf_id].ec, 1, tws_ctx[conf_id].volsend, 0);
	} else
	{
		ms_filter_link(tws_ctx[conf_id].mic_equalizer, 0, tws_ctx[conf_id].volsend,
					   0);
	}
	ms_filter_link(tws_ctx[conf_id].volsend, 0, tws_ctx[conf_id].conference, 0);

	tws_ctx[conf_id].in_snd_card = -2;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	return 0;
}

static int audio_module_select_out_custom_sound_card_conf(int conf_id, MSSndCard * playcard)
{
	int k;
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	MSFilter *soundwrite;
	int i;
	int val;

	if (playcard == NULL) {
		for (; elem != NULL; elem = elem->next) {
			MSSndCard *acard = (MSSndCard *) elem->data;

			if (acard->capabilities == MS_SND_CARD_CAP_PLAYBACK
				|| acard->capabilities == 3) {
				playcard = acard;
				ms_message
					("am_audio_module.c: select default playcard = %s %s.",
					 acard->desc->driver_type, acard->name);
				break;
			}
		}
	}

	if (playcard == NULL)
		return AMSIP_NODEVICE;

	soundwrite = ms_snd_card_create_writer(playcard);
	if (soundwrite == NULL)
		return -1;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	if (tws_ctx[conf_id].soundwrite != NULL) {
		ms_filter_unlink(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
		ms_filter_unlink(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
		if (tws_ctx[conf_id].ec != NULL) {
			ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
			ms_filter_unlink(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer,
							 0);
		} else
			ms_filter_unlink(tws_ctx[conf_id].dtmfplayback, 0,
							 tws_ctx[conf_id].equalizer, 0);
		ms_filter_unlink(tws_ctx[conf_id].equalizer, 0, tws_ctx[conf_id].resample_soundwrite, 0);
		ms_filter_unlink(tws_ctx[conf_id].resample_soundwrite, 0, tws_ctx[conf_id].soundwrite, 0);
		ms_filter_destroy(tws_ctx[conf_id].soundwrite);
	}

	tws_ctx[conf_id].soundwrite = soundwrite;

	ms_filter_call_method(tws_ctx[conf_id].soundwrite, MS_FILTER_SET_SAMPLE_RATE,
						  &tws_ctx[conf_id].use_rate);

	val = tws_ctx[conf_id].use_rate;
	i = ms_filter_call_method(tws_ctx[conf_id].soundwrite,
							  MS_FILTER_GET_SAMPLE_RATE, &val);
	if (i != 0) {
		val = tws_ctx[conf_id].use_rate;
	}
	ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
						  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
	ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
						  MS_FILTER_SET_OUTPUT_SAMPLE_RATE, &val);

	if (tws_ctx[conf_id].equalizer != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].equalizer,
							  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
	}

	ms_filter_link(tws_ctx[conf_id].conference, 0, tws_ctx[conf_id].volrecv, 0);
	ms_filter_link(tws_ctx[conf_id].volrecv, 0, tws_ctx[conf_id].dtmfplayback, 0);
	if (tws_ctx[conf_id].ec != NULL) {
		ms_filter_link(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].ec, 0);
		ms_filter_link(tws_ctx[conf_id].ec, 0, tws_ctx[conf_id].equalizer, 0);
	} else
		ms_filter_link(tws_ctx[conf_id].dtmfplayback, 0, tws_ctx[conf_id].equalizer, 0);
	ms_filter_link(tws_ctx[conf_id].equalizer, 0, tws_ctx[conf_id].resample_soundwrite, 0);
	ms_filter_link(tws_ctx[conf_id].resample_soundwrite, 0, tws_ctx[conf_id].soundwrite, 0);

	tws_ctx[conf_id].out_snd_card = -2;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
			continue;
		if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
			continue;
		ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		break;
	}

	return 0;
}

static MSSndCard *_audio_card_get_card(int card, int capabilities)
{
	const MSList *elem =
		ms_snd_card_manager_get_list(ms_snd_card_manager_get());
	int k = 0;

	if (card > 20)
		return NULL;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSSndCard *acard = (MSSndCard *) elem->data;

		if (acard->capabilities == capabilities
			|| acard->capabilities == 3) {
			if (k == card) {
				ms_message("am_audio_module.c: _audio_card_get_card = %s %s.",
						   acard->desc->driver_type, acard->name);
				return acard;
			}
			k++;
		}
	}

	return NULL;
}

static int audio_module_set_volume_out_sound_card(int card, int mixer, int percent)
{
	MSSndCard *playcard = _audio_card_get_card(card, MS_SND_CARD_CAP_PLAYBACK);

	if (playcard == NULL)
		return AMSIP_NODEVICE;

	if (mixer==0)
		ms_snd_card_set_level(playcard, MS_SND_CARD_MASTER, percent);
	else if (mixer==1)
		ms_snd_card_set_level(playcard, MS_SND_CARD_PLAYBACK, percent);
	else
		return -1;
	return 0;
}

static int audio_module_get_volume_out_sound_card(int card, int mixer)
{
	MSSndCard *playcard = _audio_card_get_card(card, MS_SND_CARD_CAP_PLAYBACK);

	if (playcard == NULL)
		return AMSIP_NODEVICE;

	if (mixer==0)
		return ms_snd_card_get_level(playcard, MS_SND_CARD_MASTER);
	else if (mixer==1)
		return ms_snd_card_get_level(playcard, MS_SND_CARD_PLAYBACK);
	
	return -1;
}

static int audio_module_set_volume_in_sound_card(int card, int percent)
{
	MSSndCard *captcard = _audio_card_get_card(card, MS_SND_CARD_CAP_CAPTURE);

	if (captcard == NULL)
		return AMSIP_NODEVICE;

	ms_snd_card_set_level(captcard, MS_SND_CARD_CAPTURE, percent);
	return 0;
}

static int audio_module_get_volume_in_sound_card(int card)
{
	MSSndCard *captcard = _audio_card_get_card(card, MS_SND_CARD_CAP_CAPTURE);

	if (captcard == NULL)
		return AMSIP_NODEVICE;

	return ms_snd_card_get_level(captcard, MS_SND_CARD_CAPTURE);
}

static int audio_module_set_mute_out_sound_card(int card, int mixer, int val)
{
	MSSndCard *playcard = _audio_card_get_card(card, MS_SND_CARD_CAP_PLAYBACK);

	if (playcard == NULL)
		return AMSIP_NODEVICE;

	if (mixer==0)
		return ms_snd_card_set_control(playcard, MS_SND_CARD_MASTER_MUTE, 0);
	else if (mixer==1)
		return ms_snd_card_set_control(playcard, MS_SND_CARD_PLAYBACK_MUTE, 0);
	return -1;
}

static int audio_module_set_mute_in_sound_card(int card, int val)
{
	MSSndCard *captcard = _audio_card_get_card(card, MS_SND_CARD_CAP_CAPTURE);

	if (captcard == NULL)
		return AMSIP_NODEVICE;

	return ms_snd_card_set_control(captcard, MS_SND_CARD_CAPTURE_MUTE, 0);
}

static int audio_module_session_mute(am_call_t * ca)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to mute\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_rtpsend != NULL) {
		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
		ms_filter_call_method_noarg(ainfo->audio_rtpsend,
									MS_RTP_SEND_MUTE_MIC);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	}

	return AMSIP_SUCCESS;
}

static int audio_module_session_unmute(am_call_t * ca)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to unmute\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_rtpsend != NULL) {
		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
		ms_filter_call_method_noarg(ainfo->audio_rtpsend,
									MS_RTP_SEND_UNMUTE_MIC);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	}

	return AMSIP_SUCCESS;
}

static int
audio_module_session_get_bandwidth_statistics(am_call_t * ca,
											  struct am_bandwidth_stats
											  *band_stats)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	memset(band_stats, 0, sizeof(struct am_bandwidth_stats));
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : no available statistics\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_rtp_session != NULL) {
		RtpStream *rtpstream;
		rtp_stats_t *stats;

		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
		rtpstream = &ainfo->audio_rtp_session->rtp;
		stats = &rtpstream->stats;
		rtp_stats_display(stats, "amsip audio statistics");

		band_stats->incoming_received = (int) stats->packet_recv;
		band_stats->incoming_expected = (int) stats->packet_recv;
		band_stats->incoming_packetloss = (int) stats->cum_packet_loss;
		band_stats->incoming_outoftime = (int) stats->outoftime;
		band_stats->incoming_notplayed = 0;
		band_stats->incoming_discarded = (int) stats->discarded;

		band_stats->outgoing_sent = (int) stats->packet_sent;

		band_stats->download_rate =
			rtp_session_compute_recv_bandwidth(ainfo->audio_rtp_session);
		band_stats->upload_rate =
			rtp_session_compute_send_bandwidth(ainfo->audio_rtp_session);

		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	}

	return AMSIP_SUCCESS;
}

static OrtpEvent *
audio_module_session_get_audio_rtp_events(am_call_t * ca)
{
	int conf_id = ca->conf_id;
	OrtpEvent *evt;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return NULL;
	}

	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return NULL;	/* no sound cards!! */
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : no available events\n",
							  ca->did, ca->enable_audio));
		return NULL;
	}

	if (ainfo->audio_rtp_session == NULL) {
		return NULL;	/* no RTP session? */
	}
	if (ainfo->audio_rtp_queue == NULL) {
		return NULL;	/* no video queue for RTCP? */
	}
	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);

	evt = ortp_ev_queue_get(ainfo->audio_rtp_queue);

	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);

	return evt;
}

static int
audio_module_session_set_audio_zrtp_sas_verified (am_call_t *ca)
{
	int conf_id = ca->conf_id;
	int verified=1;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : no available events\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_rtp_session == NULL) {
		return AMSIP_WRONG_STATE;	/* no RTP session? */
	}
	if (ainfo->audio_rtp_queue == NULL) {
		return AMSIP_WRONG_STATE;	/* no video queue for RTCP? */
	}
	if (ainfo->audio_rtp_session->rtp.tr == NULL) {
		return AMSIP_WRONG_STATE;	/* no transport? */
	}
	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);

	if (strcmp(ainfo->audio_rtp_session->rtp.tr->name, "ZRTP")==0) {
		ortp_transport_set_option(ainfo->audio_rtp_session->rtp.tr, 2, &verified);
	}

	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);

	return AMSIP_SUCCESS;
}

static int
audio_module_session_get_audio_statistics(am_call_t * ca,
										  struct am_audio_stats
										  *audio_stats)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	memset(audio_stats, 0, sizeof(struct am_audio_stats));
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL || tws_ctx[conf_id].conference == NULL) {
		return AMSIP_NODEVICE;	/* no sound cards!! */
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : no available statistics\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_rtp_session != NULL) {
		RtpStream *rtpstream;
		rtp_stats_t *stats;
		int k;
#if 0
		MSList *it;
		MSList *filters;
#endif
		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);

#if 0
		filters=ms_filter_find_neighbours(ainfo->audio_rtprecv);
		for(it=filters;it!=NULL;it=it->next){
			ms_filter_call_method((MSFilter*)it->data,
				MS_FILTER_GET_STATISTICS, (void *) NULL);
		}
		if (ainfo->audio_ice!=NULL)
		{
			ms_filter_call_method((MSFilter*)ainfo->audio_ice,
				MS_FILTER_GET_STATISTICS, (void *) NULL);
		}
#endif

		rtpstream = &ainfo->audio_rtp_session->rtp;
		stats = &rtpstream->stats;
		rtp_stats_display(stats, "amsip audio statistics");

		audio_stats->incoming_received = (int) stats->packet_recv;
		audio_stats->incoming_expected = (int) stats->packet_recv;
		audio_stats->incoming_packetloss = (int) stats->cum_packet_loss;
		audio_stats->incoming_outoftime = (int) stats->outoftime;
		audio_stats->incoming_notplayed = 0;
		audio_stats->incoming_discarded = (int) stats->discarded;

		audio_stats->outgoing_sent = (int) stats->packet_sent;

		audio_stats->sndcard_recorded = 0;
		audio_stats->sndcard_played = 0;
		audio_stats->sndcard_discarded = 0;

		k = 0;
		audio_stats->sndcard_recorded =
			ms_filter_call_method(tws_ctx[conf_id].soundread,
								  MS_FILTER_GET_STAT_INPUT, (void *) &k);
		audio_stats->sndcard_played =
			ms_filter_call_method(tws_ctx[conf_id].soundwrite,
								  MS_FILTER_GET_STAT_OUTPUT, (void *) &k);
		audio_stats->sndcard_discarded =
			ms_filter_call_method(tws_ctx[conf_id].soundwrite,
								  MS_FILTER_GET_STAT_DISCARDED,
								  (void *) &k);

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Sound card statistics rec=%i, played=%i, discared=%i\n",
							  audio_stats->sndcard_recorded,
							  audio_stats->sndcard_played,
							  audio_stats->sndcard_discarded));

		audio_stats->msconf_processed = 0;
		audio_stats->msconf_missed = 0;
		audio_stats->msconf_discarded = 0;

		/* find out on wich pin this call is connected */
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (&_antisipc.calls[k] == ca) {
				int pin = 1 + (k * 2);

				audio_stats->msconf_discarded =
					ms_filter_call_method(tws_ctx[conf_id].conference,
										  MS_FILTER_GET_STAT_DISCARDED,
										  (void *) &pin);
				audio_stats->msconf_missed =
					ms_filter_call_method(tws_ctx[conf_id].conference,
										  MS_FILTER_GET_STAT_MISSED,
										  (void *) &pin);
				audio_stats->msconf_processed =
					ms_filter_call_method(tws_ctx[conf_id].conference,
										  MS_FILTER_GET_STAT_OUTPUT,
										  (void *) &pin);
				break;
			}
		}

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "mixer card statistics processed=%i, missed=%i, discarded=%i\n",
							  audio_stats->msconf_processed,
							  audio_stats->msconf_missed,
							  audio_stats->msconf_discarded));

		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);

		audio_stats->proposed_action = STAT_ENOUGH_BANDWIDTH;

		audio_stats->pk_loss = 0;
		if (audio_stats->incoming_received > 100) {
			audio_stats->pk_loss =
				((audio_stats->incoming_received * 100) /
				 (audio_stats->incoming_received +
				  audio_stats->incoming_packetloss));
			audio_stats->pk_loss = 100 - audio_stats->pk_loss;

			if (audio_stats->pk_loss > 5) {
				audio_stats->proposed_action = STAT_NOT_ENOUGH_BANDWIDTH;
				OSIP_TRACE(osip_trace
						   (__FILE__, __LINE__, OSIP_WARNING, NULL,
							"RTP statistics pk_loss=%i\n",
							audio_stats->pk_loss));
			} else {
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "RTP statistics pk_loss=%i\n",
									  audio_stats->pk_loss));
			}
		} else {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
								  "no RTP statistics - retry later\n"));
		}

	}

	return AMSIP_SUCCESS;
}

static int
audio_module_session_get_dtmf_event(am_call_t * ca,
									struct am_dtmf_event *dtmf_event)
{
	OrtpEvent *evt;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_dtmf_queue == NULL) {
		return AMSIP_WRONG_STATE;	/* no incoming audio? */
	}

	evt = ortp_ev_queue_get(ainfo->audio_dtmf_queue);

	while (evt != NULL) {

		if (ortp_event_get_type(evt) == ORTP_EVENT_TELEPHONE_EVENT) {
			OrtpEventData *evd;

			evd = ortp_event_get_data(evt);

			switch (evd->info.telephone_event) {

			case TEV_DTMF_1:
				dtmf_event->dtmf = '1';
				break;
			case TEV_DTMF_2:
				dtmf_event->dtmf = '2';
				break;
			case TEV_DTMF_3:
				dtmf_event->dtmf = '3';
				break;
			case TEV_DTMF_4:
				dtmf_event->dtmf = '4';
				break;
			case TEV_DTMF_5:
				dtmf_event->dtmf = '5';
				break;
			case TEV_DTMF_6:
				dtmf_event->dtmf = '6';
				break;
			case TEV_DTMF_7:
				dtmf_event->dtmf = '7';
				break;
			case TEV_DTMF_8:
				dtmf_event->dtmf = '8';
				break;
			case TEV_DTMF_9:
				dtmf_event->dtmf = '9';
				break;
			case TEV_DTMF_0:
				dtmf_event->dtmf = '0';
				break;
			case TEV_DTMF_STAR:
				dtmf_event->dtmf = '*';
				break;
			case TEV_DTMF_POUND:
				dtmf_event->dtmf = '#';
				break;
			case TEV_DTMF_A:
				dtmf_event->dtmf = 'A';
				break;
			case TEV_DTMF_B:
				dtmf_event->dtmf = 'B';
				break;
			case TEV_DTMF_C:
				dtmf_event->dtmf = 'C';
				break;
			case TEV_DTMF_D:
				dtmf_event->dtmf = 'D';
				break;

			default:
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
									  "call (did:%i) bad dtmf received (%d)\n",
									  ca->did, evd->info.telephone_event));
				ortp_event_destroy(evt);
				return AMSIP_SYNTAXERROR;
			}

			dtmf_event->duration = 250;
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
								  "call (did:%i) DTMF (rtp-telephone event received\n",
								  ca->did));
			ortp_event_destroy(evt);
			return AMSIP_SUCCESS;
		}

		ortp_event_destroy(evt);
		evt = ortp_ev_queue_get(ainfo->audio_dtmf_queue);
	}

	return AMSIP_UNDEFINED_ERROR;
}

static int audio_module_session_send_inband_dtmf(am_call_t * ca, char dtmf_number)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL) {
		return AMSIP_UNDEFINED_ERROR;
	}

	if (ainfo->audio_rtpsend == NULL) {
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].dtmfplayback != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].dtmfplayback, MS_DTMF_GEN_PUT,
							  &dtmf_number);
	}
	if (ainfo->dtmfinband != NULL) {
		ms_filter_call_method(ainfo->dtmfinband, MS_DTMF_GEN_PUT,
							  &dtmf_number);
	}

	/* FIXME: should unlock after add_audio_mixing:
	   VERIFY that it cannot lock the app. */
	return AMSIP_UNDEFINED_ERROR;
}

static int audio_module_session_send_rtp_dtmf(am_call_t * ca, char dtmf_number)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL) {
		return AMSIP_UNDEFINED_ERROR;
	}

	if (ainfo->audio_rtpsend == NULL) {
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->rfc2833_supported>0) {
		ms_filter_call_method(ainfo->audio_rtpsend, MS_RTP_SEND_SEND_DTMF,
							  &dtmf_number);
	}
	else
	{
		// backup to in-band tone?
		if (ainfo->dtmfinband != NULL) {
			ms_filter_call_method(ainfo->dtmfinband, MS_DTMF_GEN_PUT,
								  &dtmf_number);
		}
	}

	if (tws_ctx[conf_id].dtmfplayback != NULL) {
		ms_filter_call_method(tws_ctx[conf_id].dtmfplayback, MS_DTMF_GEN_PUT,
							  &dtmf_number);
	}

	/* FIXME: should unlock after add_audio_mixing:
	   VERIFY that it cannot lock the app. */
	return AMSIP_SUCCESS;
}

static int
audio_module_session_play_file(am_call_t * ca, const char *wav_file,
							   int repeat,
							   MSFilterNotifyFunc cb_fileplayer_eof)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	int val;
	int i;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL) {
		return AMSIP_UNDEFINED_ERROR;
	}

	if (ainfo->audio_fileplayer2 == NULL) {
		return AMSIP_WRONG_STATE;
	}

	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	ms_filter_call_method_noarg(ainfo->audio_fileplayer2,
								MS_FILE_PLAYER_CLOSE);
	i = ms_filter_call_method(ainfo->audio_fileplayer2,
							  MS_FILE_PLAYER_OPEN, (void *) wav_file);
	if (i < 0) {
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
		return AMSIP_FILE_NOT_EXIST;
	}
	//val = -2;                     /* special value to play file only once and close */
	ms_filter_call_method(ainfo->audio_fileplayer2, MS_FILE_PLAYER_LOOP,
						  (void *) &repeat);

	ms_filter_call_method(ainfo->audio_fileplayer2,
						  MS_FILTER_GET_SAMPLE_RATE, (void *) &val);
	if (val != tws_ctx[conf_id].use_rate) {
		ms_warning
			("Wrong wav format: (need file with rate/channel -> %i:%i)",
			 tws_ctx[conf_id].use_rate, 1);
		ms_filter_call_method_noarg(ainfo->audio_fileplayer2,
									MS_FILE_PLAYER_CLOSE);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
		return AMSIP_WRONG_FORMAT;
	}
	ms_filter_call_method(ainfo->audio_fileplayer2,
						  MS_FILTER_GET_NCHANNELS, (void *) &val);
	if (val != 1) {
		ms_warning
			("Wrong wav format: (need file with rate/channel -> %i:%i)",
			 tws_ctx[conf_id].use_rate, 1);

		ms_filter_call_method_noarg(ainfo->audio_fileplayer2,
									MS_FILE_PLAYER_CLOSE);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
		return AMSIP_WRONG_FORMAT;
	}

	ms_filter_call_method_noarg(ainfo->audio_fileplayer2,
								MS_FILE_PLAYER_START);
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);

	return AMSIP_SUCCESS;
}

static int audio_module_session_record(am_call_t * ca, const char *recfile)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL) {
		return AMSIP_UNDEFINED_ERROR;
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to record\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}
	if (ainfo->audio_enable_record > 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "already recording RTP stream (%i:val=%i)\n",
							  ca->did, ainfo->audio_enable_record));
		return AMSIP_SUCCESS;
	}

	snprintf(ainfo->audio_record_file, sizeof(ainfo->audio_record_file),
			 "%s", recfile);
	if (ainfo->audio_f_recorder != NULL && ca->local_sendrecv != _SENDONLY) {
		int i;

		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
		i = ms_filter_call_method(ainfo->audio_f_recorder,
								  MS_FILE_REC_OPEN,
								  (void *) ainfo->audio_record_file);
		if (i < 0) {
			ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
			return AMSIP_NO_RIGHTS;
		}
		ms_filter_call_method_noarg(ainfo->audio_f_recorder,
									MS_FILE_REC_START);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	}
	ainfo->audio_enable_record = 1;

	return AMSIP_SUCCESS;
}

static int audio_module_session_stop_record(am_call_t * ca)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL) {
		return AMSIP_UNDEFINED_ERROR;
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i) : impossible to stop recording\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}
	if (ainfo->audio_enable_record == 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "not currently recording RTP stream (%i:val=%i)\n",
							  ca->did, ainfo->audio_enable_record));
		return AMSIP_SUCCESS;
	}

	memset(ainfo->audio_record_file, '\0',
		   sizeof(ainfo->audio_record_file));
	ainfo->audio_enable_record = 0;
	if (ainfo->audio_f_recorder != NULL && ca->local_sendrecv != _SENDONLY) {
		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
		ms_filter_call_method_noarg(ainfo->audio_f_recorder,
									MS_FILE_REC_STOP);
		ms_filter_call_method_noarg(ainfo->audio_f_recorder,
									MS_FILE_REC_CLOSE);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	}

	return AMSIP_SUCCESS;
}

static int
audio_module_session_add_external_rtpdata(am_call_t * ca,
										  MSFilter * external_rtpdata)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].ticker == NULL) {
		return AMSIP_UNDEFINED_ERROR;
	}

	if (ca->enable_audio <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio not started for this call (%i:val=%i)\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_external_encoder != NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "External filter already set. (%i:val=%i)\n",
							  ca->did, ca->enable_audio));
		return AMSIP_WRONG_STATE;
	}

	ainfo->audio_external_encoder = external_rtpdata;
	if (ainfo->audio_msjoin != NULL) {
		ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
		ms_filter_link(ainfo->audio_external_encoder, 1,
					   ainfo->audio_msjoin, 0);
		ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
	}

	return AMSIP_SUCCESS;
}

static void
audio_on_timestamp_jump(RtpSession * s, uint32_t * ts, void *user_data)
{
	ms_warning
		("Remote phone is sending audio data with a future timestamp: %u",
		 *ts);
	rtp_session_resync(s);
}

static void payload_type_changed(RtpSession * session, void *data)
{
	am_call_t *ca = (am_call_t *) data;
	int payload;
	RtpProfile *prof;
	PayloadType *pt;
	int conf_id = ca->conf_id;

	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return;
	}

	payload = rtp_session_get_recv_payload_type(session);
	prof = rtp_session_get_recv_profile(session);
	pt = rtp_profile_get_payload(prof, payload);

	if (pt != NULL) {
		MSFilter *dec = ms_filter_create_decoder(pt->mime_type);

		if (dec != NULL) {

			int k;

			/* search index of elements */
			for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
				if (&_antisipc.calls[k] == ca) {
					ms_filter_unlink(ainfo->audio_rtprecv, 0,
									 ainfo->audio_decoder[0], 0);
					if (ainfo->audio_dec2conf != NULL) {
						ms_filter_unlink(ainfo->audio_decoder[0], 0,
										 ainfo->audio_dec2conf, 0);
						ms_filter_unlink(ainfo->audio_dec2conf, 0,
										 tws_ctx[conf_id].conference, 1 + (k * 2));
						ms_filter_postprocess(ainfo->audio_dec2conf);
						ms_filter_destroy(ainfo->audio_dec2conf);
						ainfo->audio_dec2conf = NULL;
					} else {
						ms_filter_unlink(ainfo->audio_decoder[0], 0,
										 tws_ctx[conf_id].conference, 1 + (k * 2));
					}
					ms_filter_postprocess(ainfo->audio_decoder[0]);
					ms_filter_destroy(ainfo->audio_decoder[0]);
					ainfo->audio_decoder[0] = dec;
					ms_filter_link(ainfo->audio_rtprecv, 0,
								   ainfo->audio_decoder[0], 0);
                    //Force to use new payload rate for timestamp
                    ms_filter_call_method(ainfo->audio_rtprecv, MS_RTP_RECV_SET_SESSION,
                                          session);
					//if (pt->clock_rate==8000 && tws_ctx[conf_id].use_rate==16000)
					if (pt->clock_rate != tws_ctx[conf_id].use_rate) {
						ainfo->audio_dec2conf = ms_filter_new(MS_RESAMPLE_ID);
						if (ainfo->audio_dec2conf != NULL) {
							ms_filter_call_method(ainfo->audio_dec2conf,
												  MS_FILTER_SET_SAMPLE_RATE,
												  &pt->clock_rate);
							ms_filter_call_method(ainfo->audio_dec2conf,
												  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
												  &tws_ctx[conf_id].use_rate);

							ms_filter_preprocess(ainfo->audio_dec2conf,
												 tws_ctx[conf_id].ticker);
						}
					}
					ms_filter_call_method(ainfo->audio_decoder[0],
										  MS_FILTER_SET_SAMPLE_RATE,
										  &pt->clock_rate);
					if (ainfo->audio_dec2conf != NULL) {
						ms_filter_link(ainfo->audio_decoder[0], 0,
									   ainfo->audio_dec2conf, 0);
						ms_filter_link(ainfo->audio_dec2conf, 0,
									   tws_ctx[conf_id].conference, 1 + (k * 2));
					} else {
						ms_filter_link(ainfo->audio_decoder[0], 0,
									   tws_ctx[conf_id].conference, 1 + (k * 2));
					}
					ms_filter_preprocess(ainfo->audio_decoder[0],
										 tws_ctx[conf_id].ticker);
				}
			}
		} else {
			ms_warning("No decoder found for %s", pt->mime_type);
		}
	} else {
		ms_warning("No payload defined with number %i", payload);
	}

}

static int dtmf_tab[16] =
	{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#', 'A', 'B',
'C',
	'D'
};

static void ms2_on_dtmf_received(RtpSession * s, int dtmf, void *user_data)
{
	MSFilter *dtmfplayback = (MSFilter *) user_data;

	if (dtmf > 15) {
		ms_warning("Unsupported telephone-event type.");
		return;
	}
	ms_message("Receiving dtmf %c.", dtmf_tab[dtmf]);
	if (dtmfplayback != NULL) {
		ms_filter_call_method(dtmfplayback, MS_DTMF_GEN_PUT, &dtmf_tab[dtmf]);
	}
}


static RtpSession *am_create_duplex_rtpsession(am_call_t * ca, int locport,
											   const char *remip,
											   int remport, int jitt_comp)
{
	RtpSession *rtpr;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return NULL;
	}

	if (ainfo->audio_rtp_session==NULL)
	{
		rtpr = rtp_session_new(RTP_SESSION_SENDRECV);
		rtp_session_set_recv_buf_size(rtpr, 1500);
		rtp_session_set_recv_profile(rtpr, ainfo->audio_recv_rtp_profile);
		rtp_session_set_send_profile(rtpr, ainfo->audio_send_rtp_profile);
		rtp_session_set_local_addr(rtpr, "0.0.0.0", locport);
		if (remport > 0)
			rtp_session_set_remote_addr(rtpr, remip, remport);
		rtp_session_set_dscp(rtpr, _antisipc.audio_dscp);
		rtp_session_set_scheduling_mode(rtpr, 0);
		rtp_session_set_blocking_mode(rtpr, 0);
#ifdef ENABLE_NOCONF_MODE
		if (_antisipc.audio_jitter==0)
			_antisipc.audio_jitter=-1;
#endif
#if TARGET_OS_IPHONE
		if (_antisipc.audio_jitter==0)
			_antisipc.audio_jitter=-1;
#endif
		if (_antisipc.audio_jitter==0) {
			rtp_session_enable_jitter_buffer(rtpr, FALSE);
			rtp_session_enable_adaptive_jitter_compensation(rtpr, FALSE);
		} else if (_antisipc.audio_jitter<0) {
			JBParameters jbp;
			jbp.min_size = RTP_DEFAULT_JITTER_TIME;  /* NOT IMPLEMENTED , useless*/
			jbp.nom_size = RTP_DEFAULT_JITTER_TIME; /* jitter compensation */
			jbp.max_size = -1; /* NOT IMPLEMENTED , useless*/
			jbp.max_packets = 15; /* maximum number of packet allowed to be queued 15x10ms or 15x20ms, etc... */
			jbp.adaptive = TRUE;
			rtp_session_set_jitter_buffer_params(rtpr, &jbp);
			rtp_session_enable_jitter_buffer(rtpr, TRUE);
		} else { /* positive value */
			JBParameters jbp;
			jbp.min_size = _antisipc.audio_jitter;  /* NOT IMPLEMENTED , useless*/
			jbp.nom_size = _antisipc.audio_jitter; /* jitter compensation */
			jbp.max_size = -1; /* NOT IMPLEMENTED , useless*/
			jbp.max_packets = (_antisipc.audio_jitter*3<15*20)?15:_antisipc.audio_jitter*3/20; /* maximum number of packet allowed to be queued 15x10ms or 15x20ms, etc... */
			jbp.adaptive = FALSE;
			rtp_session_set_jitter_buffer_params(rtpr, &jbp);
			rtp_session_enable_jitter_buffer(rtpr, TRUE);
		}

		rtp_session_signal_connect(rtpr, "timestamp_jump",
								   (RtpCallback) audio_on_timestamp_jump, 0);
		rtp_session_set_ssrc_changed_threshold(rtpr, 5);
	}
	else
	{
		rtpr = ainfo->audio_rtp_session;
		rtp_session_flush_sockets(rtpr);
		rtp_session_resync(rtpr);

		rtp_session_signal_disconnect_by_callback(rtpr, "timestamp_jump",
								   (RtpCallback) audio_on_timestamp_jump);
		rtp_session_signal_disconnect_by_callback(rtpr, "telephone-event",
								   (RtpCallback) ms2_on_dtmf_received);
		rtp_session_signal_disconnect_by_callback(rtpr, "payload_type_changed",
								   (RtpCallback) payload_type_changed);

		rtp_session_set_recv_buf_size(rtpr, 1500);

		rtp_session_set_recv_profile(rtpr, ainfo->audio_recv_rtp_profile);
		rtp_session_set_send_profile(rtpr, ainfo->audio_send_rtp_profile);
		rtp_session_set_local_addr(rtpr, "0.0.0.0", locport);
		if (remport > 0)
			rtp_session_set_remote_addr(rtpr, remip, remport);
		rtp_session_set_dscp(rtpr, _antisipc.audio_dscp);
		rtp_session_set_scheduling_mode(rtpr, 0);
		rtp_session_set_blocking_mode(rtpr, 0);
#ifdef ENABLE_NOCONF_MODE
		if (_antisipc.audio_jitter==0)
			_antisipc.audio_jitter=-1;
#endif
#if TARGET_OS_IPHONE
		if (_antisipc.audio_jitter==0)
			_antisipc.audio_jitter=-1;
#endif
		if (_antisipc.audio_jitter==0) {
			rtp_session_enable_jitter_buffer(rtpr, FALSE);
			rtp_session_enable_adaptive_jitter_compensation(rtpr, FALSE);
		} else if (_antisipc.audio_jitter<0) {
			JBParameters jbp;
			jbp.min_size = RTP_DEFAULT_JITTER_TIME;  /* NOT IMPLEMENTED , useless*/
			jbp.nom_size = RTP_DEFAULT_JITTER_TIME; /* jitter compensation */
			jbp.max_size = -1; /* NOT IMPLEMENTED , useless*/
			jbp.max_packets = 15; /* maximum number of packet allowed to be queued 15x10ms or 15x20ms, etc... */
			jbp.adaptive = TRUE;
			rtp_session_set_jitter_buffer_params(rtpr, &jbp);
			rtp_session_enable_jitter_buffer(rtpr, TRUE);
		} else { /* positive value */
			JBParameters jbp;
			jbp.min_size = _antisipc.audio_jitter;  /* NOT IMPLEMENTED , useless*/
			jbp.nom_size = _antisipc.audio_jitter; /* jitter compensation */
			jbp.max_size = -1; /* NOT IMPLEMENTED , useless*/
			jbp.max_packets = (_antisipc.audio_jitter*3<15*20)?15:_antisipc.audio_jitter*3/20; /* maximum number of packet allowed to be queued 15x10ms or 15x20ms, etc... */
			jbp.adaptive = FALSE;
			rtp_session_set_jitter_buffer_params(rtpr, &jbp);
			rtp_session_enable_jitter_buffer(rtpr, TRUE);
		}

		rtp_session_signal_connect(rtpr, "timestamp_jump",
								   (RtpCallback) audio_on_timestamp_jump, 0);
		rtp_session_set_ssrc_changed_threshold(rtpr, 5);
	}
	return rtpr;
}

static void audio_stream_graph_reset(am_call_t * ca)
{
	struct am_audio_info *ainfo;
	if (ca == NULL)
		return;
	ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return;
	}

	if (ainfo->audio_rtp_session != NULL
		&& ainfo->audio_dtmf_queue != NULL)
		rtp_session_unregister_event_queue(ainfo->audio_rtp_session,
										   ainfo->audio_dtmf_queue);
	if (ainfo->audio_rtp_session != NULL
		&& ainfo->audio_rtp_queue != NULL)
		rtp_session_unregister_event_queue(ainfo->audio_rtp_session,
										   ainfo->audio_rtp_queue);
	
	if (ainfo->audio_rtp_session != NULL
		&& ainfo->audio_rtp_session->rtp.tr!=NULL
		&& strcmp(ainfo->audio_rtp_session->rtp.tr->name, "ZRTP")!=0)
	{
		rtp_session_set_transports(ainfo->audio_rtp_session, NULL, NULL);
	}
#if !defined(KEEP_RTP_SESSION)
	if (ainfo->audio_rtp_session != NULL)
		rtp_session_destroy(ainfo->audio_rtp_session);
#endif
	if (ainfo->audio_rtpsend != NULL)
		ms_filter_destroy(ainfo->audio_rtpsend);
	if (ainfo->audio_rtprecv != NULL)
		ms_filter_destroy(ainfo->audio_rtprecv);
	if (ainfo->audio_ice != NULL)
		ms_filter_destroy(ainfo->audio_ice);
	if (ainfo->audio_msjoin != NULL)
		ms_filter_destroy(ainfo->audio_msjoin);
	if (ainfo->audio_encoder != NULL)
		ms_filter_destroy(ainfo->audio_encoder);
	if (ainfo->audio_decoder[0] != NULL)
		ms_filter_destroy(ainfo->audio_decoder[0]);
	if (ainfo->dtmfinband!= NULL)
		ms_filter_destroy(ainfo->dtmfinband);
	if (ainfo->audio_dec2conf != NULL)
		ms_filter_destroy(ainfo->audio_dec2conf);
	if (ainfo->audio_conf2enc != NULL)
		ms_filter_destroy(ainfo->audio_conf2enc);
	if (ainfo->audio_fileplayer != NULL)
		ms_filter_destroy(ainfo->audio_fileplayer);

#if !defined(KEEP_RTP_SESSION)
	ainfo->audio_rtp_session = NULL;
#endif
	ainfo->audio_rtpsend = NULL;
	ainfo->audio_rtprecv = NULL;
	ainfo->audio_ice = NULL;
	ainfo->audio_msjoin = NULL;
	ainfo->audio_encoder = NULL;
	ainfo->audio_decoder[0] = NULL;
	ainfo->dtmfinband = NULL;
	ainfo->audio_dec2conf = NULL;
	ainfo->audio_conf2enc = NULL;
	ainfo->audio_fileplayer = NULL;
}

static int audio_stream_start_send_recv(am_call_t * ca, PayloadType * pt)
{
	int conf_id = ca->conf_id;
	RtpSession *rtps;

	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	rtps = ainfo->audio_rtp_session;
	if (rtps == NULL)
		return -1;

	ainfo->audio_rtpsend = am_filter_new_rtpsend();
	ms_filter_call_method(ainfo->audio_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->audio_rtprecv = am_filter_new_rtprecv();
	ms_filter_call_method(ainfo->audio_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

#if 0
	if (ca->audio_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
#endif
		ainfo->audio_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->audio_ice != NULL) {
			ms_filter_call_method(ainfo->audio_ice, MS_ICE_SET_SESSION,
								  rtps);
			ms_filter_call_method(ainfo->audio_ice,
								  MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->audio_checklist);
		}
#if 0
	}
#endif

	ainfo->audio_msjoin = ms_filter_new(MS_JOIN_ID);

	if (ainfo->audio_dtmf_queue == NULL)
		ainfo->audio_dtmf_queue = ortp_ev_queue_new();
	if (ainfo->audio_dtmf_queue != NULL)
		rtp_session_register_event_queue(ainfo->audio_rtp_session,
										 ainfo->audio_dtmf_queue);
	if (ainfo->audio_rtp_queue == NULL)
		ainfo->audio_rtp_queue = ortp_ev_queue_new();
	if (ainfo->audio_rtp_queue != NULL)
		rtp_session_register_event_queue(ainfo->audio_rtp_session,
										 ainfo->audio_rtp_queue);

	rtp_session_signal_connect(rtps, "telephone-event",
							   (RtpCallback) ms2_on_dtmf_received,
							   (uintptr_t) tws_ctx[conf_id].dtmfplayback);
	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) payload_type_changed,
							   (uintptr_t) ca);

	if ((ainfo->audio_encoder == NULL)
		|| (ainfo->audio_decoder[0] == NULL)) {
		/* big problem: we have not a registered codec for this payload... */
		audio_stream_graph_reset(ca);
		if (pt == NULL)
			ms_error
				("am_ms2_win32.c: No decoder available for payload (NULL).");
		else
			ms_error
				("am_ms2_win32.c: No decoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}

	/* give the sound filters some properties */
#if 0
	if (pt->clock_rate == 16000 && rate == 8000) {
		ms_error
			("am_ms2_win32.c: 16kHz codecs are not supported in this configuration.");
		return -1;
	}
#endif

	/* give the encoder/decoder some parameters */
	ms_filter_call_method(ainfo->audio_encoder, MS_FILTER_SET_SAMPLE_RATE,
						  &pt->clock_rate);
	ms_filter_call_method(ainfo->audio_decoder[0],
						  MS_FILTER_SET_SAMPLE_RATE, &pt->clock_rate);

	if (pt->send_fmtp != NULL)
		ms_filter_call_method(ainfo->audio_encoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->send_fmtp);
	if (pt->recv_fmtp != NULL)
		ms_filter_call_method(ainfo->audio_decoder[0], MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);

	return 0;
}

static int audio_stream_start_recv_only(am_call_t * ca, PayloadType * pt)
{
	int conf_id = ca->conf_id;
	RtpSession *rtps;

	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	rtps = ainfo->audio_rtp_session;
	if (rtps == NULL)
		return -1;

	ainfo->audio_rtpsend = am_filter_new_rtpsend();
	ms_filter_call_method(ainfo->audio_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->audio_rtprecv = am_filter_new_rtprecv();
	ms_filter_call_method(ainfo->audio_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

#if 0
	if (ca->audio_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
#endif
		ainfo->audio_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->audio_ice != NULL) {
			ms_filter_call_method(ainfo->audio_ice, MS_ICE_SET_SESSION,
								  rtps);
			ms_filter_call_method(ainfo->audio_ice,
								  MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->audio_checklist);
		}
#if 0
	}
#endif

	ainfo->audio_msjoin = ms_filter_new(MS_JOIN_ID);

	if (ainfo->audio_dtmf_queue == NULL)
		ainfo->audio_dtmf_queue = ortp_ev_queue_new();
	if (ainfo->audio_dtmf_queue != NULL)
		rtp_session_register_event_queue(ainfo->audio_rtp_session,
										 ainfo->audio_dtmf_queue);
	if (ainfo->audio_rtp_queue == NULL)
		ainfo->audio_rtp_queue = ortp_ev_queue_new();
	if (ainfo->audio_rtp_queue != NULL)
		rtp_session_register_event_queue(ainfo->audio_rtp_session,
										 ainfo->audio_rtp_queue);

	rtp_session_signal_connect(rtps, "telephone-event",
							   (RtpCallback) ms2_on_dtmf_received,
							   (uintptr_t) tws_ctx[conf_id].dtmfplayback);
	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) payload_type_changed,
							   (uintptr_t) ca);

	/* creates the couple of encoder/decoder */
	if (ainfo->audio_decoder == NULL) {
		/* big problem: we have not a registered codec for this payload... */
		audio_stream_graph_reset(ca);
		if (pt == NULL)
			ms_error
				("am_ms2_win32.c: No decoder available for payload (NULL)");
		else
			ms_error
				("am_ms2_win32.c: No decoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}

	/* give the encoder/decoder some parameters */
	ms_filter_call_method(ainfo->audio_decoder[0],
						  MS_FILTER_SET_SAMPLE_RATE, &pt->clock_rate);

	if (pt->recv_fmtp != NULL)
		ms_filter_call_method(ainfo->audio_decoder[0], MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);

	return 0;
}

static int
audio_stream_start_send_only(am_call_t * ca, PayloadType * pt,
							 char *file_to_stream)
{
	int conf_id = ca->conf_id;
	RtpSession *rtps;
	int rate = tws_ctx[conf_id].use_rate;

	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	rtps = ainfo->audio_rtp_session;
	if (rtps == NULL)
		return -1;

	ainfo->audio_rtpsend = am_filter_new_rtpsend();
	ms_filter_call_method(ainfo->audio_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->audio_rtprecv = am_filter_new_rtprecv();
	ms_filter_call_method(ainfo->audio_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

#if 0
	if (ca->audio_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
#endif
		ainfo->audio_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->audio_ice != NULL) {
			ms_filter_call_method(ainfo->audio_ice, MS_ICE_SET_SESSION,
								  rtps);
			ms_filter_call_method(ainfo->audio_ice,
								  MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->audio_checklist);
		}
#if 0
	}
#endif

	ainfo->audio_msjoin = ms_filter_new(MS_JOIN_ID);

	if (ainfo->audio_dtmf_queue == NULL)
		ainfo->audio_dtmf_queue = ortp_ev_queue_new();
	if (ainfo->audio_dtmf_queue != NULL)
		rtp_session_register_event_queue(ainfo->audio_rtp_session,
										 ainfo->audio_dtmf_queue);
	if (ainfo->audio_rtp_queue == NULL)
		ainfo->audio_rtp_queue = ortp_ev_queue_new();
	if (ainfo->audio_rtp_queue != NULL)
		rtp_session_register_event_queue(ainfo->audio_rtp_session,
										 ainfo->audio_rtp_queue);

	/* creates the local part */
	ainfo->audio_fileplayer = ms_filter_new(MS_FILE_PLAYER_ID);
	ms_filter_call_method(ainfo->audio_fileplayer, MS_FILE_PLAYER_OPEN,
						  (void *) file_to_stream);
	ms_filter_call_method_noarg(ainfo->audio_fileplayer,
								MS_FILE_PLAYER_START);

	if (ainfo->audio_encoder == NULL) {
		/* big problem: we have not a registered codec for this payload... */
		audio_stream_graph_reset(ca);
		if (pt == NULL)
			ms_error
				("am_ms2_win32.c: No decoder available for payload (NULL).");
		else
			ms_error
				("am_ms2_win32.c: No decoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}
#if 0
	/* give the sound filters some properties */
	if (pt->clock_rate == 16000 && rate == 8000) {
		ms_error
			("am_ms2_win32.c: 16kHz codecs are not supported in this configuration.");
		return -1;
	}
#endif

	ms_filter_call_method(ainfo->audio_fileplayer,
						  MS_FILTER_GET_SAMPLE_RATE, &rate);

	// rate is the one of the file/not the one of the conf bridge.
	if (ainfo->dtmfinband == NULL)
		ainfo->dtmfinband = ms_filter_new(MS_DTMF_GEN_ID);
	if (ainfo->dtmfinband != NULL) {
		ms_filter_call_method(ainfo->dtmfinband,
							  MS_FILTER_SET_SAMPLE_RATE, &rate);
	}

	if (ainfo->audio_conf2enc == NULL)
		ainfo->audio_conf2enc = ms_filter_new(MS_RESAMPLE_ID);
	if (ainfo->audio_conf2enc != NULL) {
		ms_filter_call_method(ainfo->audio_conf2enc,
							  MS_FILTER_SET_SAMPLE_RATE, &rate);
		ms_filter_call_method(ainfo->audio_conf2enc,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
							  &pt->clock_rate);
	}

	/* give the encoder/decoder some parameters */
	ms_filter_call_method(ainfo->audio_encoder, MS_FILTER_SET_SAMPLE_RATE,
						  &pt->clock_rate);

	if (pt->send_fmtp != NULL)
		ms_filter_call_method(ainfo->audio_encoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->send_fmtp);

	return 0;
}

static int am_ms2_detach(am_call_t * ca)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_fileplayer != NULL) {
		/* unlink for mode sendonly */
		if (ainfo->audio_conf2enc != NULL) {
			ms_filter_unlink(ainfo->audio_fileplayer, 0,
				ainfo->dtmfinband, 0);
			ms_filter_unlink(ainfo->dtmfinband, 0,
							 ainfo->audio_conf2enc, 0);
			ms_filter_unlink(ainfo->audio_conf2enc, 0, ainfo->audio_encoder,
							 0);
		} else {
			ms_filter_unlink(ainfo->audio_fileplayer, 0,
							 ainfo->dtmfinband, 0);
			ms_filter_unlink(ainfo->dtmfinband, 0,
							 ainfo->audio_encoder, 0);
		}
		ms_filter_unlink(ainfo->audio_encoder, 0, ainfo->audio_msjoin, 0);
		if (ainfo->audio_external_encoder != NULL)
			ms_filter_unlink(ainfo->audio_external_encoder, 1,
							 ainfo->audio_msjoin, 0);
		ms_filter_unlink(ainfo->audio_msjoin, 0, ainfo->audio_rtpsend, 0);
	} else if (ainfo->audio_encoder == NULL) {
		int k;

		/* search index of elements */
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (&_antisipc.calls[k] == ca) {
				ms_filter_unlink(ainfo->audio_rtprecv, 0,
								 ainfo->audio_decoder[0], 0);
				if (ainfo->audio_dec2conf != NULL) {
					ms_filter_unlink(ainfo->audio_decoder[0], 0,
									 ainfo->audio_dec2conf, 0);
					ms_filter_unlink(ainfo->audio_dec2conf, 0,
									 tws_ctx[conf_id].conference, 1 + (k * 2));
				} else
					ms_filter_unlink(ainfo->audio_decoder[0], 0,
									 tws_ctx[conf_id].conference, 1 + (k * 2));
			}
		}
	} else {
		int k;

		/* search index of elements */
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (&_antisipc.calls[k] == ca) {
				ms_filter_unlink(ainfo->audio_rtprecv, 0,
								 ainfo->audio_decoder[0], 0);
				if (ainfo->audio_dec2conf != NULL) {
					ms_filter_unlink(ainfo->audio_decoder[0], 0,
									 ainfo->audio_dec2conf, 0);
					ms_filter_unlink(ainfo->audio_dec2conf, 0,
									 tws_ctx[conf_id].conference, 1 + (k * 2));
				} else
					ms_filter_unlink(ainfo->audio_decoder[0], 0,
									 tws_ctx[conf_id].conference, 1 + (k * 2));

				if (ainfo->audio_conf2enc != NULL) {
					ms_filter_unlink(tws_ctx[conf_id].conference, 1 + (k * 2),
									 ainfo->dtmfinband, 0);
					ms_filter_unlink(ainfo->dtmfinband, 0,
									 ainfo->audio_conf2enc, 0);
					ms_filter_unlink(ainfo->audio_conf2enc, 0,
									 ainfo->audio_encoder, 0);
				} else {
					ms_filter_unlink(tws_ctx[conf_id].conference, 1 + (k * 2),
									 ainfo->dtmfinband, 0);
					ms_filter_unlink(ainfo->dtmfinband, 0,
									 ainfo->audio_encoder, 0);
				}
				ms_filter_unlink(ainfo->audio_encoder, 0,
								 ainfo->audio_msjoin, 0);
				if (ainfo->audio_external_encoder != NULL)
					ms_filter_unlink(ainfo->audio_external_encoder, 1,
									 ainfo->audio_msjoin, 0);
				ms_filter_unlink(ainfo->audio_msjoin, 0,
								 ainfo->audio_rtpsend, 0);

				ms_filter_unlink(ainfo->audio_fileplayer2, 0,
								 tws_ctx[conf_id].conference, 1 + (k * 2) + 1);
				ms_filter_unlink(tws_ctx[conf_id].conference, 1 + (k * 2) + 1,
								 ainfo->audio_f_recorder, 0);
			}
		}
	}

	if (ainfo->audio_ice != NULL)
		ms_ticker_detach(tws_ctx[conf_id].ticker, ainfo->audio_ice);
	return 0;
}

static int am_ms2_attach(am_call_t * ca, int k)
{
	int conf_id = ca->conf_id;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_fileplayer != NULL) {
		int val=8000;
		ms_filter_call_method(ainfo->audio_fileplayer,
							  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (ainfo->audio_conf2enc != NULL) {
			ms_filter_link(ainfo->audio_fileplayer, 0, ainfo->dtmfinband,
						   0);
			ms_filter_link(ainfo->dtmfinband, 0, ainfo->audio_conf2enc,
						   0);
			ms_filter_link(ainfo->audio_conf2enc, 0, ainfo->audio_encoder, 0);
			ms_filter_call_method(ainfo->audio_conf2enc,
									MS_FILTER_SET_SAMPLE_RATE,
									&val);
		} else {
			ms_filter_link(ainfo->audio_fileplayer, 0,
						   ainfo->dtmfinband, 0);
			ms_filter_link(ainfo->dtmfinband, 0,
						   ainfo->audio_encoder, 0);
		}
		ms_filter_call_method(ainfo->dtmfinband,
							  MS_FILTER_SET_SAMPLE_RATE, &val);
		ms_filter_link(ainfo->audio_encoder, 0, ainfo->audio_msjoin, 0);
		if (ainfo->audio_external_encoder != NULL)
			ms_filter_link(ainfo->audio_external_encoder, 1,
						   ainfo->audio_msjoin, 0);
		ms_filter_link(ainfo->audio_msjoin, 0, ainfo->audio_rtpsend, 0);
	} else if (ainfo->audio_encoder == NULL) {
		/* detach soundread from ticker */
		ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);

		/* TODO: DO NOT PLAY WHEN ACTIVE CALL EXIST */
		ms_filter_link(ainfo->audio_rtprecv, 0, ainfo->audio_decoder[0],
					   0);
		if (ainfo->audio_dec2conf != NULL) {
			ms_filter_link(ainfo->audio_decoder[0], 0, ainfo->audio_dec2conf,
						   0);
			ms_filter_link(ainfo->audio_dec2conf, 0, tws_ctx[conf_id].conference,
						   1 + (k * 2));
			/* it might have been configured with other conference */
			ms_filter_call_method(ainfo->audio_dec2conf,
									MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
									&tws_ctx[conf_id].use_rate);
		} else
			ms_filter_link(ainfo->audio_decoder[0], 0, tws_ctx[conf_id].conference,
						   1 + (k * 2));
	} else {
		/* detach soundread from ticker */
		ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);

		ms_filter_link(ainfo->audio_rtprecv, 0, ainfo->audio_decoder[0],
					   0);
		if (ainfo->audio_dec2conf != NULL) {
			ms_filter_link(ainfo->audio_decoder[0], 0, ainfo->audio_dec2conf,
						   0);
			ms_filter_link(ainfo->audio_dec2conf, 0, tws_ctx[conf_id].conference,
						   1 + (k * 2));
			ms_filter_call_method(ainfo->audio_dec2conf,
									MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
									&tws_ctx[conf_id].use_rate);
		} else
			ms_filter_link(ainfo->audio_decoder[0], 0, tws_ctx[conf_id].conference,
						   1 + (k * 2));

		if (ainfo->audio_conf2enc != NULL) {
			
			ms_filter_link(tws_ctx[conf_id].conference, 1 + (k * 2),
						   ainfo->dtmfinband, 0);
			ms_filter_link(ainfo->dtmfinband, 0,
						   ainfo->audio_conf2enc, 0);
			ms_filter_link(ainfo->audio_conf2enc, 0, ainfo->audio_encoder, 0);
			ms_filter_call_method(ainfo->audio_conf2enc,
									MS_FILTER_SET_SAMPLE_RATE,
									&tws_ctx[conf_id].use_rate);
		} else {
			ms_filter_link(tws_ctx[conf_id].conference, 1 + (k * 2),
						   ainfo->dtmfinband, 0);
			ms_filter_link(ainfo->dtmfinband, 0,
						   ainfo->audio_encoder, 0);
		}
		ms_filter_call_method(ainfo->dtmfinband,
							  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);

		ms_filter_link(ainfo->audio_encoder, 0, ainfo->audio_msjoin, 0);
		if (ainfo->audio_external_encoder != NULL)
			ms_filter_link(ainfo->audio_external_encoder, 1,
						   ainfo->audio_msjoin, 0);
		ms_filter_link(ainfo->audio_msjoin, 0, ainfo->audio_rtpsend, 0);

		ms_filter_link(ainfo->audio_fileplayer2, 0, tws_ctx[conf_id].conference,
					   1 + (k * 2) + 1);
		ms_filter_link(tws_ctx[conf_id].conference, 1 + (k * 2) + 1,
					   ainfo->audio_f_recorder, 0);

	}

	if (ainfo->audio_fileplayer != NULL)
		ms_ticker_attach(tws_ctx[conf_id].ticker, ainfo->audio_fileplayer);
	else if (ainfo->audio_encoder == NULL)
		/* TODO: DO NOT PLAY WHEN ACTIVE CALL EXIST */
	{
		int i;
		int val;

		val = tws_ctx[conf_id].use_rate;
		i = ms_filter_call_method(tws_ctx[conf_id].soundread,
								  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (i != 0) {
			val = tws_ctx[conf_id].use_rate;
		}
		ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
							  MS_FILTER_SET_SAMPLE_RATE, &val);
		ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
							  
		if (tws_ctx[conf_id].mic_equalizer != NULL) {
			ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,
								  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
		}

		val = tws_ctx[conf_id].use_rate;
		i = ms_filter_call_method(tws_ctx[conf_id].soundwrite,
								  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (i != 0) {
			val = tws_ctx[conf_id].use_rate;
		}
		ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
							  MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
		ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE, &val);

		if (tws_ctx[conf_id].equalizer != NULL) {
			ms_filter_call_method(tws_ctx[conf_id].equalizer,
								  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
		}

		ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
	} else {
		/* attach soundread to ticker */
		int val;
		int i;
		val = tws_ctx[conf_id].use_rate;
		i = ms_filter_call_method(tws_ctx[conf_id].soundread,
								  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (i != 0) {
			val = tws_ctx[conf_id].use_rate;
		}
		ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
							  MS_FILTER_SET_SAMPLE_RATE, &val);
		ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
		if (tws_ctx[conf_id].mic_equalizer != NULL) {
			ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,
								  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
		}

		val = tws_ctx[conf_id].use_rate;
		i = ms_filter_call_method(tws_ctx[conf_id].soundwrite,
								  MS_FILTER_GET_SAMPLE_RATE, &val);
		if (i != 0) {
			val = tws_ctx[conf_id].use_rate;
		}
		ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
							  MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
		ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
							  MS_FILTER_SET_OUTPUT_SAMPLE_RATE, &val);

		if (tws_ctx[conf_id].equalizer != NULL) {
			ms_filter_call_method(tws_ctx[conf_id].equalizer,
								  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
		}

		ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
		/* ms_ticker_attach(tws_ctx[conf_id].ticker,ca->audio_rtprecv); */
	}
	if (ainfo->audio_ice != NULL)
		ms_ticker_attach(tws_ctx[conf_id].ticker, ainfo->audio_ice);
	return 0;
}

static int
_am_add_fmtp_parameter_encoders(am_call_t * ca, sdp_message_t * sdp_answer,
								PayloadType * pt)
{
	int pos;

	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (sdp_answer == NULL)
		return -1;

	if (ainfo->audio_encoder == NULL)
		return -1;

	pos = 0;
	while (!osip_list_eol(&sdp_answer->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&sdp_answer->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "audio")) {
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
						if ((0 == osip_strcasecmp(subtype, pt->mime_type)
							&& pt->clock_rate == atoi(freq))
							|| (0 == osip_strcasecmp(subtype, "g722")
							&& (atoi(freq)==8000 || atoi(freq)==16000))) {
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
							ms_filter_call_method(ainfo->audio_encoder,
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
						ms_filter_call_method(ainfo->audio_encoder,
											  MS_FILTER_ADD_ATTR,
											  (void *) ptattr);
					}
					pos3++;
				}
			}
			return p_number;
		} else {
			/* skip non audio */
		}

		pos++;
	}

	return -1;
}

static int
_am_prepare_audio_decoders(am_call_t * ca, sdp_message_t * sdp,
						   sdp_media_t * med, int *decoder_payload)
{
	int conf_id = ca->conf_id;
	int pos2 = 0;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}


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
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__,
								OSIP_WARNING, NULL,
								"audio profile/recv payload=%i %s/%s\n",
								p_number, subtype, freq));
					if (0 == osip_strcasecmp(subtype, "telephone-event")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_telephone_event));
					} else if (0 == osip_strcasecmp(subtype, "PCMU")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_pcmu8000));
					} else if (0 == osip_strcasecmp(subtype, "PCMA")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_pcma8000));
					} else if (0 == osip_strcasecmp(subtype, "speex")) {
						if (strstr(freq, "32000") != NULL) {
							rtp_profile_set_payload(ainfo->
													audio_recv_rtp_profile,
													payload,
													payload_type_clone
													(&payload_type_speex_uwb));
						} else if (strstr(freq, "16000") != NULL) {
							rtp_profile_set_payload(ainfo->
													audio_recv_rtp_profile,
													payload,
													payload_type_clone
													(&payload_type_speex_wb));
						} else {
							rtp_profile_set_payload(ainfo->
													audio_recv_rtp_profile,
													payload,
													payload_type_clone
													(&payload_type_speex_nb));
						}
					} else if (0 == osip_strcasecmp(subtype, "ilbc")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_ilbc));
					} else if (0 == osip_strcasecmp(subtype, "gsm")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_gsm));
					} else if (0 == osip_strcasecmp(subtype, "g723")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7231));
					} else if (0 == osip_strcasecmp(subtype, "g7221")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7221));
					} else if (0 == osip_strcasecmp(subtype, "g722")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g722));
					} else if (0 == osip_strcasecmp(subtype, "g726-40")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_40));
					} else if (0 == osip_strcasecmp(subtype, "g726-32")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_32));
					} else if (0 == osip_strcasecmp(subtype, "g726-24")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_24));
					} else if (0 == osip_strcasecmp(subtype, "g726-16")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_16));
					} else if (0 == osip_strcasecmp(subtype, "g729")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729));
					} else if (0 == osip_strcasecmp(subtype, "g729d")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729d));
					} else if (0 == osip_strcasecmp(subtype, "g729e")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729e));
					} else if (0 == osip_strcasecmp(subtype, "g7291")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7291));
					} else if (0 == osip_strcasecmp(subtype, "g728")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g728));
					} else if (0 == osip_strcasecmp(subtype, "amr")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_amr));
					} else if (0 == osip_strcasecmp(subtype, "amr-wb")) {
						rtp_profile_set_payload(ainfo->audio_recv_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_amrwb));
					}

					/* step 2: create decoders: */
					if (ainfo->audio_decoder[0] == NULL) {
						PayloadType *pt;

						pt = rtp_profile_get_payload(ainfo->
													 audio_recv_rtp_profile,
													 p_number);
						if (pt != NULL && pt->mime_type != NULL) {
							ainfo->audio_decoder[0] =
								ms_filter_create_decoder(pt->mime_type);
							if (ainfo->audio_decoder[0] != NULL) {
								/* all fine! */
								*decoder_payload = p_number;
								OSIP_TRACE(osip_trace
										   (__FILE__, __LINE__,
											OSIP_WARNING, NULL,
											"Decoder created for payload=%i %s\n",
											p_number, pt->mime_type));

								if (pt->clock_rate != tws_ctx[conf_id].use_rate) {
									ainfo->audio_dec2conf = ms_filter_new(MS_RESAMPLE_ID);
									ms_filter_call_method(ainfo->audio_dec2conf,
														  MS_FILTER_SET_SAMPLE_RATE, &pt->clock_rate);
									ms_filter_call_method(ainfo->audio_dec2conf,
														  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
														  &tws_ctx[conf_id].use_rate);
								}
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

static int _am_complete_decoders_configuration(am_call_t * ca,
											   sdp_message_t * sdp_remote,
											   sdp_media_t * med)
{
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	int payload = -1;
	int pos3;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return 0;
	}
	if (ainfo->audio_decoder[0] == NULL)
		return 0;
		
	if (osip_strncasecmp(ainfo->audio_decoder[0]->desc->enc_fmt, "amr", 4)!=0
		&&osip_strncasecmp(ainfo->audio_decoder[0]->desc->enc_fmt, "amr-wb", 4)!=0)
		return 0;

	pos3=0;
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
			n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
			if (n == 3)
				payload = atoi(codec);
			if (n == 3) {
				if (0 == osip_strcasecmp(subtype, ainfo->audio_decoder[0]->desc->enc_fmt)) {
					/* found! */
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__,
								OSIP_WARNING, NULL,
								"%s encoder rtpmap found payload=%i\n", subtype, payload));
					break;
				}
			}
		}
		payload=-1;
		pos3++;
	}

	if (payload==-1)
		return 0;

	pos3=0;
	while (!osip_list_eol(&med->a_attributes, pos3)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&med->a_attributes,
											  pos3);
		if (osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
			if (attr->a_att_value != NULL
				&& payload == atoi(attr->a_att_value)) {
				ms_filter_call_method(ainfo->audio_decoder[0],
									  MS_FILTER_ADD_FMTP,
									  (void *) attr->
									  a_att_value);
			}
		}
		pos3++;
	}
	return 0;
}

#define REMOTE_PREFERED_CODEC 0
#define LOCAL_PREFERED_CODEC 1

//static int audio_policy = LOCAL_PREFERED_CODEC;
static int audio_policy = REMOTE_PREFERED_CODEC;

static PayloadType *_am_prepare_audio_encoder_LOCAL_PREFERED_CODEC(int index, am_call_t * ca,
											  sdp_message_t * sdp_remote,
											  sdp_media_t * med,
											  int *encoder_payload)
{
	int pos2 = 0;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return NULL;
	}

	*encoder_payload = -1;
	/* use first local supported payload */


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

				if (strstr(attr->a_att_value, "729a/")!=NULL
					||strstr(attr->a_att_value, "729A/")!=NULL) {
						pos3++;
						continue; /* bypass linksys wrong "g729a" information */
				}

				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3)
					payload = atoi(codec);
				if (n == 3 && p_number == payload) {
					PayloadType *pt_index;
					char rtpmap_tmp[128];

					/* use it if it match our local order preference */
					if (osip_strcasecmp(subtype, _antisipc.codecs[index].name)!=0)
						break;
					if (osip_strcasecmp(subtype, "g722")!=0)
					{
						if (_antisipc.codecs[index].freq!=atoi(freq))
							break;
					} else {
						if (atoi(freq)!=8000 && atoi(freq)!=16000)
							break;
					}

					found_rtpmap = 1;
					memset(rtpmap_tmp, '\0', sizeof(rtpmap_tmp));
					snprintf(rtpmap_tmp, sizeof(rtpmap_tmp), "%s/%s",
						subtype, freq);
					if (osip_strcasecmp(subtype, "g722")==0 && atoi(freq)==8000)
						snprintf(rtpmap_tmp, sizeof(rtpmap_tmp), "%s/%s",
								 subtype, "16000");

					/* rtp_profile are added with numbers that comes from the
					local_sdp: the current p_number could be different!! */

					/* create an encoder: pt will exist if a decoder was found
					or if it's PCMU/PCMA */
					pt_index =
						rtp_profile_get_payload(ainfo->audio_recv_rtp_profile,
						p_number);
					pt = rtp_profile_get_payload_from_rtpmap(ainfo->
						audio_recv_rtp_profile,
						rtpmap_tmp);
					if (pt_index != pt && pt != NULL) {
						OSIP_TRACE(osip_trace
							(__FILE__, __LINE__, OSIP_WARNING, NULL,
							"Payload for %s is different in offer(%i) and answer(%i)\n",
							rtpmap_tmp, -1, p_number));
					}

					if (pt != NULL && pt->mime_type != NULL) {

						if (ca->local_sendrecv != _RECVONLY) {
							ainfo->audio_encoder =
								ms_filter_create_encoder(pt->mime_type);
							if (ainfo->audio_encoder != NULL) {
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

#if 0
		if (found_rtpmap == 0) {
			/* missing rtpmap attribute in remote_sdp!!! */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
				"Missing rtpmap attribute in remote SDP for payload=%i\n",
				p_number));
			pt = NULL;
			if (p_number == 0 || p_number == 8 || p_number == 3
				|| p_number == 18 || p_number == 4 || p_number == 9) {
					pt = rtp_profile_get_payload(ainfo->audio_recv_rtp_profile,
						p_number);
			}

			if (pt != NULL && pt->mime_type != NULL) {
				if (ca->local_sendrecv != _RECVONLY) {
					ainfo->audio_encoder =
						ms_filter_create_encoder(pt->mime_type);
					if (ainfo->audio_encoder != NULL) {
						*encoder_payload = p_number;
						return pt;
					}
				} else {
					*encoder_payload = p_number;
					return pt;
				}
			}
		}
#endif

		pos2++;
	}

	return NULL;
}

static PayloadType *_am_prepare_audio_encoder(am_call_t * ca,
											  sdp_message_t * sdp_remote,
											  sdp_media_t * med,
											  int *encoder_payload)
{
	int pos2 = 0;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return NULL;
	}

	if (audio_policy == LOCAL_PREFERED_CODEC)
	{
		int i;
		for (i = 0; i < 5; i++) {
			PayloadType *pt;
			if (_antisipc.codecs[i].enable == 0)
				continue;
			pt = _am_prepare_audio_encoder_LOCAL_PREFERED_CODEC(i, ca, sdp_remote, med, encoder_payload);
			if (pt!=NULL)
				return pt;
		}
	}

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
				
				if (strstr(attr->a_att_value, "729a/")!=NULL
					||strstr(attr->a_att_value, "729A/")!=NULL) {
					pos3++;
					continue; /* bypass linksys wrong "g729a" information */
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
					if (osip_strcasecmp(subtype, "g722")==0 && atoi(freq)==8000)
						snprintf(rtpmap_tmp, sizeof(rtpmap_tmp), "%s/%s",
								 subtype, "16000");

					/* rtp_profile are added with numbers that comes from the
					   local_sdp: the current p_number could be different!! */

					/* create an encoder: pt will exist if a decoder was found
					   or if it's PCMU/PCMA */
					pt_index =
						rtp_profile_get_payload(ainfo->audio_recv_rtp_profile,
												p_number);
					pt = rtp_profile_get_payload_from_rtpmap(ainfo->
															 audio_recv_rtp_profile,
															 rtpmap_tmp);
					if (pt_index != pt && pt != NULL) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_WARNING, NULL,
									"Payload for %s is different in offer(%i) and answer(%i)\n",
									rtpmap_tmp, -1, p_number));
					}

					if (pt != NULL && pt->mime_type != NULL) {

						if (ca->local_sendrecv != _RECVONLY) {
							ainfo->audio_encoder =
								ms_filter_create_encoder(pt->mime_type);
							if (ainfo->audio_encoder != NULL) {
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
			if (p_number == 0 || p_number == 8 || p_number == 3
				|| p_number == 18 || p_number == 4 || p_number == 9) {
				pt = rtp_profile_get_payload(ainfo->audio_recv_rtp_profile,
											 p_number);
			}

			if (pt != NULL && pt->mime_type != NULL) {
				if (ca->local_sendrecv != _RECVONLY) {
					ainfo->audio_encoder =
						ms_filter_create_encoder(pt->mime_type);
					if (ainfo->audio_encoder != NULL) {
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
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return NULL;
	}

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

		if (0 == osip_strcasecmp(med->m_media, "audio")) {
			_am_prepare_audio_decoders(ca, local_sdp, med,
									   decoder_payload);
		} else {
			/* skip non audio */
		}

		pos++;
	}


	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "audio")) {
			_am_complete_decoders_configuration(ca, remote_sdp, med);
		} else {
			/* skip non audio */
		}

		pos++;
	}

	/* find encoder that we do support AND offer */
	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "audio")) {
			PayloadType *pt =
				_am_prepare_audio_encoder(ca, remote_sdp, med,
										  encoder_payload);
			if (pt != NULL && ainfo->audio_encoder != NULL) {
				/* step 3: set fmtp parameters */
				_am_add_fmtp_parameter_encoders(ca, sdp_answer, pt);
				/* when payload is not supported in sdp_answer, we might
				   want to set encoder parameter from sdp_offer... */
			}
			return pt;
		} else {
			/* skip non audio */
		}
		pos++;
	}

	return NULL;
}

static int _am_audio_build_encoder_profile(am_call_t * ca, sdp_message_t * sdp_remote)
{
	sdp_media_t *med_remote=NULL;
	int pos;
	int pos2;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return -1;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "building audio encoder profile\n"));

	pos = 0;
	while (!osip_list_eol(&sdp_remote->m_medias, pos)) {

		med_remote = (sdp_media_t *) osip_list_get(&sdp_remote->m_medias, pos);

		if (0 == osip_strcasecmp(med_remote->m_media, "audio")
			&& 0 != osip_strcasecmp(med_remote->m_port, "0")) {
			break;
		}

		med_remote=NULL;
		pos++;
	}

	if (med_remote!=NULL)
	{
		pos2 = 0;
		while (!osip_list_eol(&med_remote->a_attributes, pos2)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&med_remote->a_attributes,
												  pos2);
			if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
				/* search for each supported payload */
				int n;
				int payload = 0;
				char codec[64];
				char subtype[64];
				char freq[64];

				if (attr->a_att_value == NULL
					|| strlen(attr->a_att_value) > 63) {
					pos2++;
					continue;	/* avoid buffer overflow! */
				}

				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3) {

					payload = atoi(codec);
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__,
								OSIP_WARNING, NULL,
								"audio profile/send payload=%i %s/%s\n",
								payload, subtype, freq));
					if (0 == osip_strcasecmp(subtype, "telephone-event")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
											payload,
											payload_type_clone
											(&payload_type_telephone_event));
					} else if (0 == osip_strcasecmp(subtype, "PCMU")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_pcmu8000));
					} else if (0 == osip_strcasecmp(subtype, "PCMA")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_pcma8000));
					} else if (0 == osip_strcasecmp(subtype, "speex")) {
						if (strstr(freq, "32000") != NULL) {
							rtp_profile_set_payload(ainfo->
													audio_send_rtp_profile,
													payload,
													payload_type_clone
													(&payload_type_speex_uwb));
						} else if (strstr(freq, "16000") != NULL) {
							rtp_profile_set_payload(ainfo->
													audio_send_rtp_profile,
													payload,
													payload_type_clone
													(&payload_type_speex_wb));
						} else {
							rtp_profile_set_payload(ainfo->
													audio_send_rtp_profile,
													payload,
													payload_type_clone
													(&payload_type_speex_nb));
						}
					} else if (0 == osip_strcasecmp(subtype, "ilbc")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_ilbc));
					} else if (0 == osip_strcasecmp(subtype, "gsm")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_gsm));
					} else if (0 == osip_strcasecmp(subtype, "g723")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7231));
					} else if (0 == osip_strcasecmp(subtype, "g7221")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7221));
					} else if (0 == osip_strcasecmp(subtype, "g722")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g722));
					} else if (0 == osip_strcasecmp(subtype, "g726-40")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_40));
					} else if (0 == osip_strcasecmp(subtype, "g726-32")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_32));
					} else if (0 == osip_strcasecmp(subtype, "g726-24")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_24));
					} else if (0 == osip_strcasecmp(subtype, "g726-16")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g726_16));
					} else if (0 == osip_strcasecmp(subtype, "g729")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729));
					} else if (0 == osip_strcasecmp(subtype, "g729d")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729d));
					} else if (0 == osip_strcasecmp(subtype, "g729e")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729e));
					} else if (0 == osip_strcasecmp(subtype, "g7291")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7291));
					} else if (0 == osip_strcasecmp(subtype, "g728")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g728));
					} else if (0 == osip_strcasecmp(subtype, "amr")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_amr));
					} else if (0 == osip_strcasecmp(subtype, "amr-wb")) {
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_amrwb));
					}
				}
			}
			pos2++;
		}

		//complete with missing rtpmaps
		pos2 = 0;
		while (!osip_list_eol(&med_remote->m_payloads, pos2)) {
			char *p_char = (char *) osip_list_get(&med_remote->m_payloads, pos2);
			int payload = atoi(p_char);
			if (payload>=0 && payload<RTP_PROFILE_MAX_PAYLOADS)
			{
				PayloadType *pt = rtp_profile_get_payload(ainfo->audio_send_rtp_profile,
					payload);
				if (pt==NULL)
				{
					if (payload==0)
					{
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_pcmu8000));
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/add missing send payload=%i PCMU/8000\n",
									payload));
					}
					else if (payload==3)
					{
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_gsm));
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/add missing send payload=%i GSM/8000\n",
									payload));
					}
					else if (payload==4)
					{
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g7231));
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/add missing send payload=%i G723/8000\n",
									payload));
					}
					else if (payload==8)
					{
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_pcma8000));
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/add missing send payload=%i PCMA/8000\n",
									payload));
					}
					else if (payload==18)
					{
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g729));
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/add missing send payload=%i g729/8000\n",
									payload));
					}
					else if (payload==9)
					{
						rtp_profile_set_payload(ainfo->audio_send_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_g722));
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/add missing send payload=%i g722/8000\n",
									payload));
					}
					else
					{
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__,
									OSIP_WARNING, NULL,
									"audio profile/unkown missing send payload=%i NOT ADDED\n",
									payload));
					}
				}
			}
			pos2++;
		}
	}

	return 0;
}

static int _am_check_telephone_event_support(am_call_t * ca, sdp_message_t * sdp_remote)
{
	sdp_media_t *med_remote=NULL;
	int pos;
	int pos2;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return -1;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "Checking rfc2833 support\n"));
	ainfo->rfc2833_supported=0;

	pos = 0;
	while (!osip_list_eol(&sdp_remote->m_medias, pos)) {

		med_remote = (sdp_media_t *) osip_list_get(&sdp_remote->m_medias, pos);

		if (0 == osip_strcasecmp(med_remote->m_media, "audio")
			&& 0 != osip_strcasecmp(med_remote->m_port, "0")) {
			break;
		}

		med_remote=NULL;
		pos++;
	}

	if (med_remote!=NULL)
	{
		pos2 = 0;
		while (!osip_list_eol(&med_remote->a_attributes, pos2)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&med_remote->a_attributes,
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

				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3) {
					if (0 == osip_strcasecmp(subtype, "telephone-event")) {
						ainfo->rfc2833_supported=1;
						OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
											  "rfc2833 support enabled - %i -\n", atoi(codec)));
						break;
					}
				}
			}
			pos2++;
		}
	}

	return 0;
}

static int is_video_enabled(sdp_message_t * sdp_answer)
{
	int pos;

	if (sdp_answer == NULL)
		return -1;

	pos = 0;
	while (!osip_list_eol(&sdp_answer->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&sdp_answer->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "video")
			&& 0 != osip_strcasecmp(med->m_port, "0")) {
			/* should check attribute... */
			return 0;
		}

		pos++;
	}

	return -1;
}

int os_sound_init()
{
	return 0;
}

static int audio_module_sound_init(am_call_t * ca, int idx)
{
	struct am_audio_info *ainfo;

	ca->audio_ctx = &tws_ctx[0].ctx[idx];

	ainfo = (struct am_audio_info *)ca->audio_ctx;
	ainfo->audio_dtmf_queue = ortp_ev_queue_new();
	ainfo->audio_rtp_queue = ortp_ev_queue_new();
	return 0;
}

static int audio_module_sound_release(am_call_t * ca)
{
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->audio_fileplayer2 != NULL)
		ms_filter_destroy(ainfo->audio_fileplayer2);
	ainfo->audio_fileplayer2 = NULL;

	if (ainfo->audio_f_recorder != NULL)
		ms_filter_call_method_noarg(ainfo->audio_f_recorder,
									MS_FILE_REC_CLOSE);
	if (ainfo->audio_f_recorder != NULL)
		ms_filter_destroy(ainfo->audio_f_recorder);
	ainfo->audio_f_recorder = NULL;

	if (ainfo->audio_dtmf_queue != NULL)
		ortp_ev_queue_destroy(ainfo->audio_dtmf_queue);
	ainfo->audio_dtmf_queue = NULL;

	if (ainfo->audio_rtp_queue != NULL)
		ortp_ev_queue_destroy(ainfo->audio_rtp_queue);
	ainfo->audio_rtp_queue = NULL;

	/* thoses filter must be realeased after the session is closed */
	ainfo->audio_external_encoder = NULL;

	ainfo->audio_enable_record = 0;

	if (ainfo->audio_rtp_session != NULL)
		rtp_session_destroy(ainfo->audio_rtp_session);
	ainfo->audio_rtp_session = NULL;

	ca->audio_ctx = NULL;
	return 0;
}


static int audio_module_sound_close(am_call_t * ca)
{
	int conf_id = ca->conf_id;
	int k;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}


	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"audio_module_sound_close - RTP stream cid=%i did=%i\n",
				ca->cid, ca->did));

	ca->enable_audio = -1;
	/* stop mediastreamer thread */

	/* stop mediastreamer session */

	/* print statistics */
	if (ainfo->audio_rtp_session != NULL)
		rtp_stats_display(&ainfo->audio_rtp_session->rtp.stats,
						  "end of session");

	{
		int reconnect_soundread = 0;

		if (ainfo->audio_fileplayer != NULL)
			ms_ticker_detach(tws_ctx[conf_id].ticker, ainfo->audio_fileplayer);
		else if (tws_ctx[conf_id].soundread != NULL) {
			ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
			reconnect_soundread = 1;
		} else {
			ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
			reconnect_soundread = 1;
			/* ms_ticker_detach(tws_ctx[conf_id].ticker,ca->audio_rtprecv); */
		}

		am_ms2_detach(ca);

		audio_stream_graph_reset(ca);

		if (reconnect_soundread == 1) {
			/* if other calls are active, attach the soundread device! */
			for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
				int i;
				int val;
				if (tws_ctx[conf_id].ctx[k].audio_fileplayer != NULL)
					continue;
				if (tws_ctx[conf_id].ctx[k].audio_encoder == NULL)
					continue;

				val = tws_ctx[conf_id].use_rate;
				i = ms_filter_call_method(tws_ctx[conf_id].soundread,
										  MS_FILTER_GET_SAMPLE_RATE, &val);
				if (i != 0) {
					val = tws_ctx[conf_id].use_rate;
				}
				ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
									  MS_FILTER_SET_SAMPLE_RATE, &val);
				ms_filter_call_method(tws_ctx[conf_id].resample_soundread,
									  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
									  &tws_ctx[conf_id].use_rate);
				if (tws_ctx[conf_id].mic_equalizer != NULL) {
					ms_filter_call_method(tws_ctx[conf_id].mic_equalizer,
										  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
				}

				val = tws_ctx[conf_id].use_rate;
				i = ms_filter_call_method(tws_ctx[conf_id].soundwrite,
										  MS_FILTER_GET_SAMPLE_RATE, &val);
				if (i != 0) {
					val = tws_ctx[conf_id].use_rate;
				}
				ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
									  MS_FILTER_SET_SAMPLE_RATE,
									  &tws_ctx[conf_id].use_rate);
				ms_filter_call_method(tws_ctx[conf_id].resample_soundwrite,
									  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
									  &val);

				if (tws_ctx[conf_id].equalizer != NULL) {
					ms_filter_call_method(tws_ctx[conf_id].equalizer,
										  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
				}

				ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
				break;
			}
		}
	}

	if (ainfo->audio_recv_rtp_profile != NULL) {
		rtp_profile_destroy(ainfo->audio_recv_rtp_profile);
		ainfo->audio_recv_rtp_profile = NULL;
	}
	if (ainfo->audio_send_rtp_profile != NULL) {
		rtp_profile_destroy(ainfo->audio_send_rtp_profile);
		ainfo->audio_send_rtp_profile = NULL;
	}
#if 0
	if (ainfo->audio_fileplayer2 != NULL)
		ms_filter_call_method_noarg(ainfo->audio_fileplayer2,
									MS_FILE_PLAYER_STOP);
#endif

	if (ainfo->audio_f_recorder != NULL)
		ms_filter_call_method_noarg(ainfo->audio_f_recorder,
									MS_FILE_REC_STOP);

	return 0;
}

static int
audio_module_sound_start(am_call_t * ca, sdp_message_t * sdp_answer,
						 sdp_message_t * sdp_offer,
						 sdp_message_t * sdp_local,
						 sdp_message_t * sdp_remote, int local_port,
						 char *remote_ip, int remote_port,
						 int setup_passive)
{
	int conf_id = ca->conf_id;
	PayloadType *pt;
	int jitter;
	int encoder_payload = -1;
	int decoder_payload = -1;
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ca->p_am_sessiontype[0]!='\0' && strstr(ca->p_am_sessiontype, "audio")==NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "audio disabled using P-AM-ST private header\n"));
		return AMSIP_WRONG_STATE;
	}

	if (tws_ctx[conf_id].soundread == NULL || tws_ctx[conf_id].soundwrite == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio sound cards\n"));
		return AMSIP_NODEVICE;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "audio_module_sound_start - RTP stream cid=%i did=%i\n",
						  ca->cid, ca->did));

	if (ca->local_sendrecv == _INACTIVE) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "audio_module_sound_start - RTP stream is inactive cid=%i did=%i\n",
							  ca->cid, ca->did));
		return -1;
	}

#if !defined(KEEP_RTP_SESSION)
	ainfo->audio_rtp_session = NULL;
#endif

	if (sdp_remote==NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "audio_module_sound_start - avoid ICMP\n"));
		ainfo->audio_recv_rtp_profile = (RtpProfile *) rtp_profile_new("RtpProfile1");
		ainfo->audio_send_rtp_profile = (RtpProfile *) rtp_profile_new("RtpProfile2");
		jitter = 30;
		ainfo->audio_rtp_session =
			am_create_duplex_rtpsession(ca, local_port, NULL, 0,
										jitter);
		ca->enable_audio = 1;
		return 0;
	}

	if (ca->audio_candidate.stun_candidates[0].conn_port > 0)
		local_port = ca->audio_candidate.stun_candidates[0].rel_port;
	if (ca->audio_candidate.relay_candidates[0].conn_port > 0)
		local_port = ca->audio_candidate.relay_candidates[0].rel_port;


	/* open mediastreamer session */

	ainfo->audio_recv_rtp_profile =
		(RtpProfile *) rtp_profile_new("RtpProfile1");
	ainfo->audio_send_rtp_profile =
		(RtpProfile *) rtp_profile_new("RtpProfile2");

	pt = _am_prepare_coders(ca, sdp_answer, sdp_offer, sdp_local,
							sdp_remote, remote_ip, &decoder_payload,
							&encoder_payload);

	_am_check_telephone_event_support(ca, sdp_remote);
	_am_audio_build_encoder_profile(ca, sdp_remote);

	if (ainfo->audio_fileplayer2 == NULL)
		ainfo->audio_fileplayer2 = ms_filter_new(MS_FILE_PLAYER_ID);

	if (pt == NULL) {
		audio_module_sound_close(ca);
		return -1;
	}

	if (ainfo->dtmfinband == NULL)
		ainfo->dtmfinband = ms_filter_new(MS_DTMF_GEN_ID);
	if (ainfo->dtmfinband != NULL) {
		ms_filter_call_method(ainfo->dtmfinband,
							  MS_FILTER_SET_SAMPLE_RATE, &tws_ctx[conf_id].use_rate);
	}

	if (pt->clock_rate != tws_ctx[conf_id].use_rate) {
		ainfo->audio_conf2enc = ms_filter_new(MS_RESAMPLE_ID);
		if (ainfo->audio_conf2enc != NULL) {
			ms_filter_call_method(ainfo->audio_conf2enc,
								  MS_FILTER_SET_SAMPLE_RATE,
								  &tws_ctx[conf_id].use_rate);
			ms_filter_call_method(ainfo->audio_conf2enc,
								  MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
								  &pt->clock_rate);
		}
	}

	jitter = 30;
	if (is_video_enabled(sdp_answer) == 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "audio_module_sound_start - Adjusting jitter for video call=%i did=%i\n",
							  ca->cid, ca->did));
		jitter = 60;
	}

	ainfo->audio_rtp_session =
		am_create_duplex_rtpsession(ca, local_port, remote_ip, remote_port,
									jitter);

	if (!ainfo->audio_rtp_session) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "Could not initialize audio session for %s port %d\n",
							  remote_ip, remote_port));
		if (ca->local_sendrecv == _SENDONLY) {
		}
		audio_module_sound_close(ca);
		return -1;
	}
#ifndef DISABLE_SRTP
	am_get_security_descriptions(ainfo->audio_rtp_session, sdp_answer,
								 sdp_offer, sdp_local, sdp_remote,
								 "audio");
#endif
	if (ainfo->audio_rtp_session->rtp.tr==NULL
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
      rtp_session_set_transports(ainfo->audio_rtp_session, rtpt, rtcpt );
      ortp_transport_set_option(rtpt, 1, &ca->call_direction);
      ortp_transport_set_option(rtcpt, 1, &ca->call_direction);
    }
	}

	/* RTP */
	if (encoder_payload >= 0)
		rtp_session_set_send_payload_type(ainfo->audio_rtp_session,
										  encoder_payload);
	if (encoder_payload < 0 && decoder_payload >= 0)
		rtp_session_set_send_payload_type(ainfo->audio_rtp_session,
										  decoder_payload);
	if (decoder_payload >= 0)
		rtp_session_set_recv_payload_type(ainfo->audio_rtp_session,
										  decoder_payload);
	if (encoder_payload >= 0 && decoder_payload < 0)
		rtp_session_set_send_payload_type(ainfo->audio_rtp_session,
										  encoder_payload);

	if (ca->local_sendrecv == _SENDONLY) {
		/* send a wav file!  ca->wav_file */
		audio_stream_start_send_only(ca, pt, ca->hold_wav_file);
	} else if (ca->local_sendrecv == _RECVONLY) {
		audio_stream_start_recv_only(ca, pt);
	} else {
		audio_stream_start_send_recv(ca, pt);
	}
	/* set sdes (local user) info */

	/* check if symmetric RTP is required
	   with antisip to antisip softphone, we use STUN connectivity check
	 */

	if (ca->audio_checklist.cand_pairs[0].remote_candidate.conn_addr[0]=='\0') {
		if (_antisipc.do_symmetric_rtp == 1) {
			rtp_session_set_symmetric_rtp(ainfo->audio_rtp_session, 1);
		}
	}

	if (ainfo->audio_f_recorder == NULL) {
		ainfo->audio_f_recorder = ms_filter_new(MS_FILE_REC_ID);
		ms_filter_call_method(ainfo->audio_f_recorder,
							  MS_FILTER_SET_SAMPLE_RATE,
							  &tws_ctx[conf_id].use_rate);
	}

	if (ainfo->audio_f_recorder != NULL && ainfo->audio_enable_record > 0
		&& ca->local_sendrecv != _SENDONLY) {
		ms_filter_call_method_noarg(ainfo->audio_f_recorder,
									MS_FILE_REC_START);
	}

	ca->enable_audio = 1;
	/* should start here the mediastreamer thread */

	{
		/* build graph for each kind of session */
		int k;

		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "audio_module_sound_start: searching for call to attach\n"));
		for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
			if (_antisipc.calls[k].state == NOT_USED)
				continue;

			if (&_antisipc.calls[k] == ca) {
				am_ms2_attach(&_antisipc.calls[k], k);
				OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,	"audio_module_sound_start: attaching call\n"));

				break;
			}
		}

		if (ainfo->audio_fileplayer == NULL
			&& ainfo->audio_encoder!=NULL)
		{
			for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
				if (_antisipc.calls[k].state == NOT_USED)
					continue;
				if (&_antisipc.calls[k] != ca) {
					struct am_audio_info *tmpinfo = _antisipc.calls[k].audio_ctx;
					if (tmpinfo != NULL
						&& tmpinfo->audio_fileplayer == NULL
						&& tmpinfo->audio_encoder == NULL
						&& tmpinfo->audio_rtprecv != NULL)
					{
						ms_ticker_detach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);
						am_ms2_detach(&_antisipc.calls[k]);
						ms_ticker_attach(tws_ctx[conf_id].ticker, tws_ctx[conf_id].soundread);

						OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
											  "audio_module_sound_start: detaching recvonly stream\n"));
					}
				}
			}
		}
	}

#if 0
	ms_mutex_lock(&tws_ctx[conf_id].ticker->lock);
	ms_ticker_print_graphs(tws_ctx[conf_id].ticker);
	ms_mutex_unlock(&tws_ctx[conf_id].ticker->lock);
#endif
	return 0;
}


static int
audio_module_session_detach_conf(am_call_t * ca)
{
#ifdef TWSMEDIASERVER
	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) 
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}


	ca->enable_audio = -1;

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,	"audio_module_session_detach_conf: detaching call\n"));

	if (ainfo->audio_fileplayer != NULL)
	{
		ms_ticker_detach(tws_ctx[ca->conf_id].ticker, ainfo->audio_fileplayer);
		am_ms2_detach(ca);
	}
	else if (tws_ctx[ca->conf_id].soundread != NULL) {
		ms_ticker_detach(tws_ctx[ca->conf_id].ticker, tws_ctx[ca->conf_id].soundread);
		am_ms2_detach(ca);
		ms_ticker_attach(tws_ctx[ca->conf_id].ticker, tws_ctx[ca->conf_id].soundread);
	}

#endif
	return 0;
}

static int
audio_module_session_attach_conf(am_call_t * ca, int conf_id)
{
#ifdef TWSMEDIASERVER
	int k;

	struct am_audio_info *ainfo = (struct am_audio_info *)ca->audio_ctx;

	if (ainfo == NULL) 
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"No audio ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	ca->conf_id = conf_id; 
	ca->enable_audio = 1;

	for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
		if (_antisipc.calls[k].state == NOT_USED)
			continue;
		if (&_antisipc.calls[k] == ca) {

			/* if 2 conference use different rate, then need to prepare the resampler */

			if (ainfo->audio_encoder != NULL && ainfo->audio_conf2enc == NULL) {
				ainfo->audio_conf2enc = ms_filter_new(MS_RESAMPLE_ID);
				ms_filter_call_method(ainfo->audio_conf2enc,
					MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
					&tws_ctx[conf_id].use_rate);
				ms_filter_call_method(ainfo->audio_conf2enc,
					MS_FILTER_SET_SAMPLE_RATE,
					&tws_ctx[conf_id].use_rate);
			}
			if (ainfo->audio_decoder[0] != NULL && ainfo->audio_dec2conf == NULL) {
				ainfo->audio_dec2conf = ms_filter_new(MS_RESAMPLE_ID);
				ms_filter_call_method(ainfo->audio_dec2conf,
					MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
					&tws_ctx[conf_id].use_rate);
				ms_filter_call_method(ainfo->audio_dec2conf,
					MS_FILTER_SET_SAMPLE_RATE,
					&tws_ctx[conf_id].use_rate);
			}

			am_ms2_attach(&_antisipc.calls[k], k);

			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,	"audio_module_session_attach_conf: attaching call\n"));
			break;
		}
	}

#endif
	return 0;
}


struct audio_module ms2_audio_module = {
	"MS2AUDIO",					/* module name */
	"Mediastreamer2 audio default module",	/* descriptive text */
	"audio",					/* media type */

	audio_module_init,
	audio_module_reset,
	audio_module_quit,

	audio_module_set_volume_out_sound_card,
	audio_module_get_volume_out_sound_card,
	audio_module_set_volume_in_sound_card,
	audio_module_get_volume_in_sound_card,
	audio_module_set_mute_out_sound_card,
	audio_module_set_mute_in_sound_card,
	audio_module_find_out_sound_card,
	audio_module_find_in_sound_card,

	audio_module_select_in_sound_card_conf,
	audio_module_select_out_sound_card_conf,
	audio_module_select_in_custom_sound_card_conf,
	audio_module_select_out_custom_sound_card_conf,

	audio_module_set_rate_conf,
	audio_module_enable_echo_canceller_conf,
	audio_module_enable_vad_conf,
	audio_module_enable_agc_conf,
	audio_module_set_denoise_level_conf,
	audio_module_set_callback_conf,
	audio_module_set_volume_gain_conf,	
	audio_module_set_echo_limitation_conf,
	audio_module_set_noise_gate_threshold_conf,
	audio_module_set_equalizer_state_conf,
	audio_module_set_equalizer_params_conf,
	audio_module_set_mic_equalizer_state_conf,
	audio_module_set_mic_equalizer_params_conf,

	audio_module_sound_init,
	audio_module_sound_release,
	audio_module_sound_start,
	audio_module_sound_close,

	audio_module_session_mute,
	audio_module_session_unmute,
	audio_module_session_get_bandwidth_statistics,
	audio_module_session_get_audio_rtp_events,
	audio_module_session_set_audio_zrtp_sas_verified,
	audio_module_session_get_audio_statistics,

	audio_module_session_get_dtmf_event,
	audio_module_session_send_inband_dtmf,
	audio_module_session_send_rtp_dtmf,
	audio_module_session_play_file,
	audio_module_session_record,
	audio_module_session_stop_record,
	audio_module_session_add_external_rtpdata,
	audio_module_session_detach_conf,
	audio_module_session_attach_conf,
};

#endif
