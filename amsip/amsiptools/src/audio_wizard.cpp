/*
  The amsip program is a modular SIP softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include <amsiptools/audio_wizard.h>

#include <mediastreamer2/mscommon.h>
#include "mediastreamer2/msticker.h"
#include <mediastreamer2/mssndcard.h>
#include <mediastreamer2/msfileplayer.h>
#include <mediastreamer2/msfilerec.h>

#include <speex/speex_preprocess.h>

MSTicker *ticker=NULL;
MSFilter *conference=NULL;
MSFilter *soundread=NULL;
MSFilter *soundwrite=NULL;
MSFilter *resample_soundread=NULL;
MSFilter *resample_soundwrite=NULL;
MSFilter *fileplayer=NULL;
MSFilter *filerecorder=NULL;
MSFilter *resample_to_conf=NULL;
MSFilter *resample_from_conf=NULL;
MSFilter *ec=NULL;
int enable_ec=0;
int agc_level=20000;
int max_gain=15;
int frame_size=160;
int tail_length=1280;
int sample_rate=16000;
int codec_rate=16000;

static int amsiptools_wizard_detach()
{
	if (ticker==NULL)
		return 0;

	ms_ticker_detach(ticker, soundread);

	ms_filter_unlink(soundread, 0, resample_soundread, 0);
	ms_filter_unlink(resample_soundread, 0, ec, 1);
	ms_filter_unlink (ec, 1, conference, 0);
	ms_filter_unlink(conference, 0, ec, 0);
	ms_filter_unlink(ec, 0, resample_soundwrite, 0);
	ms_filter_unlink(resample_soundwrite, 0, soundwrite, 0);

	if (resample_to_conf!=NULL)
		ms_filter_unlink(resample_to_conf, 0, conference, 1);
	if (resample_from_conf!=NULL)
		ms_filter_unlink(conference, 1, resample_from_conf, 0);

	if (filerecorder!=NULL)
	{
		ms_filter_unlink(resample_from_conf, 0, filerecorder, 0);
		ms_filter_call_method_noarg (filerecorder, MS_FILE_REC_STOP);
		ms_filter_call_method_noarg (filerecorder, MS_FILE_REC_CLOSE);
	}
	if (fileplayer!=NULL)
	{
		ms_filter_unlink(fileplayer, 0, resample_to_conf, 0);
	}
	return 0;
}

static int amsiptools_wizard_release()
{
	if (ticker==NULL)
		return 0;
	if (ticker!=NULL)
		ms_ticker_destroy(ticker);
	if (conference!=NULL)
		ms_filter_destroy(conference);
	if (soundread!=NULL)
		ms_filter_destroy(soundread);
	if (soundwrite!=NULL)
		ms_filter_destroy(soundwrite);
	if (resample_soundread!=NULL)
		ms_filter_destroy(resample_soundread);
	if (resample_soundwrite!=NULL)
		ms_filter_destroy(resample_soundwrite);
	if (fileplayer!=NULL)
		ms_filter_destroy(fileplayer);
	if (filerecorder!=NULL)
		ms_filter_destroy(filerecorder);
	if (resample_to_conf!=NULL)
		ms_filter_destroy(resample_to_conf);
	if (resample_from_conf!=NULL)
		ms_filter_destroy(resample_from_conf);
	if (ec!=NULL)
		ms_filter_destroy(ec);

	ticker=NULL;
	conference=NULL;
	soundread=NULL;
	soundwrite=NULL;
	resample_soundread=NULL;
	resample_soundwrite=NULL;
	fileplayer=NULL;
	filerecorder=NULL;
	resample_to_conf=NULL;
	resample_from_conf=NULL;
	ec=NULL;

	return 0;
}

int amsiptools_wizard_stop()
{
	amsiptools_wizard_detach();
	amsiptools_wizard_release();
	return 0;
}

#if 0

int amsiptools_wizard_startplay(const char *in_card, const char *out_card, char *wavfile)
{
	MSSndCardManager *manager;
	MSSndCard *playcard;
	MSSndCard *captcard;
	int val;
	int i;

	amsiptools_wizard_stop();

	manager = ms_snd_card_manager_get ();
	playcard = ms_snd_card_manager_get_card(manager, out_card);
	if (playcard==NULL)
		return -1;

	captcard = ms_snd_card_manager_get_card(manager, in_card);
	if (captcard==NULL)
		return -1;

	ticker = ms_ticker_new ();
	ms_ticker_set_name(ticker, "amsiptools-playwizard");
	conference = ms_filter_new (MS_CONF_ID);
	if (conference == NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	ec = ms_filter_new (MS_SPEEX_EC_ID);
	if (ec == NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	val=(frame_size*sample_rate)/8000;
	ms_filter_call_method (ec, MS_FILTER_SET_FRAMESIZE,
		(void *) &val);
	val=tail_length;
	ms_filter_call_method (ec, MS_FILTER_SET_FILTERLENGTH,
		(void *) &val);

	ms_filter_call_method (ec, MS_FILTER_SET_SAMPLE_RATE,
		&sample_rate);

#if 0
	val = (7*frame_size*sample_rate)/8000;
	ms_filter_call_method (ec, MS_FILTER_SET_PLAYBACKDELAY,
		&val);
#endif

	ms_filter_call_method (conference, MS_FILTER_SET_SAMPLE_RATE,
		&sample_rate);

	ms_filter_call_method (conference, MS_FILTER_ENABLE_AGC, &agc_level);
	ms_filter_call_method (conference, MS_FILTER_SET_MAX_GAIN, &max_gain);

	soundread = ms_snd_card_create_reader (captcard);
	soundwrite = ms_snd_card_create_writer (playcard);
	resample_soundread = ms_filter_new (MS_RESAMPLE_ID);
	resample_soundwrite = ms_filter_new (MS_RESAMPLE_ID);

	if (soundread==NULL || soundwrite==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}
	if (resample_soundread==NULL || resample_soundwrite==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* Retreive supported rate */
	i = ms_filter_call_method (soundread,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	i = ms_filter_call_method (soundread,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);
	if (i<0)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* resample from soundread to msconf */
	ms_filter_call_method (resample_soundread,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &val);
	ms_filter_call_method (resample_soundread,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &sample_rate);

	/* Retreive supported rate */
	ms_filter_call_method (soundwrite,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	i = ms_filter_call_method (soundwrite,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);
	if (i<0)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* resample from soundread to msconf */
	ms_filter_call_method (resample_soundwrite,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	ms_filter_call_method (resample_soundwrite,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &val);

	resample_to_conf = ms_filter_new (MS_RESAMPLE_ID);

	if (resample_to_conf==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	fileplayer = ms_filter_new(MS_FILE_PLAYER_ID);
	if (fileplayer==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	ms_filter_call_method (fileplayer, MS_FILE_PLAYER_OPEN,
							 (void *) wavfile);

	val=8000;
	ms_filter_call_method (fileplayer,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);

	ms_filter_call_method (resample_to_conf,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &val);
	ms_filter_call_method (resample_to_conf,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &sample_rate);

	val = -2; /* special value to play file only once and close */
	ms_filter_call_method (fileplayer, MS_FILE_PLAYER_LOOP, (void *) &val);
	ms_filter_call_method_noarg (fileplayer, MS_FILE_PLAYER_START);

	ms_filter_link(soundread, 0, resample_soundread, 0);
	ms_filter_link(resample_soundread, 0, ec, 1);
	ms_filter_link (ec, 1, conference, 0);
	ms_filter_link(conference, 0, ec, 0);
	ms_filter_link(ec, 0, resample_soundwrite, 0);
	ms_filter_link(resample_soundwrite, 0, soundwrite, 0);

	ms_filter_link(resample_to_conf, 0, conference, 1);

	ms_filter_link(fileplayer, 0, resample_to_conf, 0);

	ms_ticker_attach(ticker, soundread);

	ms_mutex_lock (&ticker->lock);
	//ms_filter_set_notify_callback(conference,&_we_wizard_vumeter_play, this);
	ms_mutex_unlock (&ticker->lock);
	return 0;
}

int amsiptools_wizard_startrecord(const char *in_card, const char *out_card, char *wavfile)
{
	MSSndCardManager *manager;
	MSSndCard *playcard;
	MSSndCard *captcard;
	int val;
	int i;

	amsiptools_wizard_stop();
	current_agc_gain=-999;
	current_loudness=-999;

	manager = ms_snd_card_manager_get ();
	playcard = ms_snd_card_manager_get_card(manager, out_card);
	if (playcard==NULL)
		return -1;

	captcard = ms_snd_card_manager_get_card(manager, in_card);
	if (captcard==NULL)
		return -1;

	ticker = ms_ticker_new ();
	ms_ticker_set_name(ticker, "amsiptools-recordwizard");
	conference = ms_filter_new (MS_CONF_ID);
	if (conference == NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	ec = ms_filter_new (MS_SPEEX_EC_ID);
	if (ec == NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	val=(frame_size*sample_rate)/8000;
	ms_filter_call_method (ec, MS_FILTER_SET_FRAMESIZE,
		(void *) &val);
	val=tail_length;
	ms_filter_call_method (ec, MS_FILTER_SET_FILTERLENGTH,
		(void *) &val);

	ms_filter_call_method (ec, MS_FILTER_SET_SAMPLE_RATE,
		&sample_rate);

#if 0
	val = (7*frame_size*sample_rate)/8000;
	ms_filter_call_method (ec, MS_FILTER_SET_PLAYBACKDELAY,
		&val);
#endif

	ms_filter_call_method (conference, MS_FILTER_SET_SAMPLE_RATE,
		&sample_rate);

	ms_filter_call_method (conference, MS_FILTER_ENABLE_AGC, &agc_level);
	ms_filter_call_method (conference, MS_FILTER_SET_MAX_GAIN, &max_gain);

	soundread = ms_snd_card_create_reader (captcard);
	soundwrite = ms_snd_card_create_writer (playcard);
	resample_soundread = ms_filter_new (MS_RESAMPLE_ID);
	resample_soundwrite = ms_filter_new (MS_RESAMPLE_ID);

	if (soundread==NULL || soundwrite==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}
	if (resample_soundread==NULL || resample_soundwrite==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* Retreive supported rate */
	i = ms_filter_call_method (soundread,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	i = ms_filter_call_method (soundread,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);
	if (i<0)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* resample from soundread to msconf */
	ms_filter_call_method (resample_soundread,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &val);
	ms_filter_call_method (resample_soundread,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &sample_rate);

	/* Retreive supported rate */
	ms_filter_call_method (soundwrite,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	i = ms_filter_call_method (soundwrite,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);
	if (i<0)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* resample from soundread to msconf */
	ms_filter_call_method (resample_soundwrite,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	ms_filter_call_method (resample_soundwrite,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &val);


	resample_to_conf = ms_filter_new (MS_RESAMPLE_ID);
	resample_from_conf = ms_filter_new (MS_RESAMPLE_ID);

	if (resample_to_conf==NULL || resample_from_conf==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	ms_filter_call_method (resample_from_conf,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	ms_filter_call_method (resample_from_conf,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &codec_rate);

	ms_filter_call_method (resample_to_conf,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &codec_rate);
	ms_filter_call_method (resample_to_conf,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &sample_rate);

	filerecorder = ms_filter_new(MS_FILE_REC_ID);
	ms_filter_call_method (filerecorder,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &codec_rate);

	ms_filter_call_method (filerecorder, MS_FILE_REC_OPEN,
							 (void *) wavfile);
	ms_filter_call_method_noarg (filerecorder, MS_FILE_REC_START);

	ms_filter_link(soundread, 0, resample_soundread, 0);
	ms_filter_link(resample_soundread, 0, ec, 1);
	ms_filter_link (ec, 1, conference, 0);
	ms_filter_link(conference, 0, ec, 0);
	ms_filter_link(ec, 0, resample_soundwrite, 0);
	ms_filter_link(resample_soundwrite, 0, soundwrite, 0);

	ms_filter_link(resample_to_conf, 0, conference, 1);
	ms_filter_link(conference, 1, resample_from_conf, 0);

	ms_filter_link(resample_from_conf, 0, filerecorder, 0);

	ms_ticker_attach(ticker, soundread);

	ms_mutex_lock (&ticker->lock);
	ms_filter_set_notify_callback(conference,&vumeter_mic, NULL);
	ms_mutex_unlock (&ticker->lock);
	return 0;
}

#endif


static int current_agc_gain=-999;
static int current_loudness=-999;

static void vumeter_mic(void *userdata ,unsigned int id, void *arg)
{
	if (id == MS_CONF_SPEEX_PREPROCESS_MIC)
	{
		SpeexPreprocessState *speex_pp=(SpeexPreprocessState *)arg;

		speex_preprocess_ctl(speex_pp, SPEEX_PREPROCESS_GET_AGC_LOUDNESS, &current_loudness);
		speex_preprocess_ctl(speex_pp, SPEEX_PREPROCESS_GET_AGC_GAIN, &current_agc_gain);
	}
}

int amsiptools_wizard_get_mic_agc_gain()
{
	int val;
	if (ticker==NULL)
		return current_agc_gain;
	ms_mutex_lock(&ticker->lock);
	val = current_agc_gain;
	ms_mutex_unlock(&ticker->lock);
	return val;
}

int amsiptools_wizard_get_mic_loudness()
{
	int val;
	if (ticker==NULL)
		return current_loudness;
	ms_mutex_lock(&ticker->lock);
	val = current_loudness;
	ms_mutex_unlock(&ticker->lock);
	return val;
}

int amsiptools_wizard_play(char *wavfile)
{
	int i;
	int val;

	if (ticker==NULL)
		return 0;

	if (resample_to_conf==NULL)
	{
		resample_to_conf = ms_filter_new (MS_RESAMPLE_ID);

		if (resample_to_conf==NULL)
		{
			return -1;
		}
	}

	if (fileplayer==NULL)
	{
		fileplayer = ms_filter_new(MS_FILE_PLAYER_ID);
		if (fileplayer==NULL)
		{
			if (resample_to_conf!=NULL)
				ms_filter_destroy(resample_to_conf);
			resample_to_conf=NULL;
			return -1;
		}

		ms_ticker_detach(ticker, soundread);
		ms_filter_call_method (fileplayer, MS_FILE_PLAYER_OPEN,
								 (void *) wavfile);
		ms_filter_link(fileplayer, 0, resample_to_conf, 0);
		ms_filter_link(resample_to_conf, 0, conference, 1);
	}
	else
	{
		ms_ticker_detach(ticker, soundread);
		ms_filter_call_method_noarg (fileplayer, MS_FILE_PLAYER_STOP);
		ms_filter_call_method (fileplayer, MS_FILE_PLAYER_OPEN,
								 (void *) wavfile);
	}

	val=8000;
	ms_filter_call_method (fileplayer,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);

	ms_filter_call_method (resample_to_conf,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &val);
	ms_filter_call_method (resample_to_conf,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &sample_rate);

	val = -2; /* special value to play file only once and close */
	ms_filter_call_method (fileplayer, MS_FILE_PLAYER_LOOP, (void *) &val);
	i = ms_filter_call_method_noarg (fileplayer, MS_FILE_PLAYER_START);

	ms_ticker_attach(ticker, soundread);
	return i;
}

int amsiptools_wizard_start(const char *in_card, const char *out_card)
{
	MSSndCardManager *manager;
	MSSndCard *playcard;
	MSSndCard *captcard;
	int val;
	int i;

	amsiptools_wizard_stop();
	current_agc_gain=-999;
	current_loudness=-999;

	manager = ms_snd_card_manager_get ();
	playcard = ms_snd_card_manager_get_card(manager, out_card);
	if (playcard==NULL)
		return -1;

	captcard = ms_snd_card_manager_get_card(manager, in_card);
	if (captcard==NULL)
		return -1;

	ticker = ms_ticker_new ();
	ms_ticker_set_name(ticker, "amsiptools-wizard");
	conference = ms_filter_new (MS_CONF_ID);
	if (conference == NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	ec = ms_filter_new (MS_SPEEX_EC_ID);
	if (ec == NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	val=(frame_size*sample_rate)/8000;
	ms_filter_call_method (ec, MS_FILTER_SET_FRAMESIZE,
		(void *) &val);
	val=tail_length;
	ms_filter_call_method (ec, MS_FILTER_SET_FILTERLENGTH,
		(void *) &val);

	ms_filter_call_method (ec, MS_FILTER_SET_SAMPLE_RATE,
		&sample_rate);

#if 0
	val = (7*frame_size*sample_rate)/8000;
	ms_filter_call_method (ec, MS_FILTER_SET_PLAYBACKDELAY,
		&val);
#endif

	ms_filter_call_method (conference, MS_FILTER_SET_SAMPLE_RATE,
		&sample_rate);

	ms_filter_call_method (conference, MS_FILTER_ENABLE_AGC, &agc_level);
	ms_filter_call_method (conference, MS_FILTER_SET_MAX_GAIN, &max_gain);

	soundread = ms_snd_card_create_reader (captcard);
	soundwrite = ms_snd_card_create_writer (playcard);
	resample_soundread = ms_filter_new (MS_RESAMPLE_ID);
	resample_soundwrite = ms_filter_new (MS_RESAMPLE_ID);

	if (soundread==NULL || soundwrite==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}
	if (resample_soundread==NULL || resample_soundwrite==NULL)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* Retreive supported rate */
	i = ms_filter_call_method (soundread,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	i = ms_filter_call_method (soundread,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);
	if (i<0)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* resample from soundread to msconf */
	ms_filter_call_method (resample_soundread,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &val);
	ms_filter_call_method (resample_soundread,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &sample_rate);

	/* Retreive supported rate */
	ms_filter_call_method (soundwrite,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	i = ms_filter_call_method (soundwrite,
						 MS_FILTER_GET_SAMPLE_RATE,
						 &val);
	if (i<0)
	{
		amsiptools_wizard_release();
		return -1;
	}

	/* resample from soundread to msconf */
	ms_filter_call_method (resample_soundwrite,
						 MS_FILTER_SET_SAMPLE_RATE,
						 &sample_rate);
	ms_filter_call_method (resample_soundwrite,
						 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
						 &val);




	//ms_filter_call_method (resample_to_conf,
	//					 MS_FILTER_SET_SAMPLE_RATE,
	//					 &codec_rate);
	//ms_filter_call_method (resample_to_conf,
	//					 MS_FILTER_SET_OUTPUT_SAMPLE_RATE,
	//					 &sample_rate);

	//filerecorder = ms_filter_new(MS_FILE_REC_ID);
	//ms_filter_call_method (filerecorder,
	//					 MS_FILTER_SET_SAMPLE_RATE,
	//					 &codec_rate);

	//ms_filter_call_method (filerecorder, MS_FILE_REC_OPEN,
	//						 (void *) wavfile);
	//ms_filter_call_method_noarg (filerecorder, MS_FILE_REC_START);

	ms_filter_link(soundread, 0, resample_soundread, 0);
	ms_filter_link(resample_soundread, 0, ec, 1);
	ms_filter_link (ec, 1, conference, 0);
	ms_filter_link(conference, 0, ec, 0);
	ms_filter_link(ec, 0, resample_soundwrite, 0);
	ms_filter_link(resample_soundwrite, 0, soundwrite, 0);

	//ms_filter_link(conference, 1, resample_from_conf, 0);
	//ms_filter_link(resample_from_conf, 0, filerecorder, 0);

	ms_ticker_attach(ticker, soundread);

	ms_mutex_lock (&ticker->lock);
	ms_filter_set_notify_callback(conference,&vumeter_mic, NULL);
	ms_mutex_unlock (&ticker->lock);
	return 0;
}
