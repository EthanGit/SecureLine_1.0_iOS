/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "test_getopt.h"

#include <amsip/am_options.h>

#if !defined(WIN32)
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>				/* dirname */
#endif

#include <osip2/osip.h>
#include <eXosip2/eXosip.h>

#include "test_account.h"

static void usage(void);

static void usage(void)
{
	printf(""
		   "test  [options]\n"
		   "    -h          : show options\n"
		   "    -d X        : set debug level\n"
		   "    -f config   : set config file\n"
		   "    -l logfile  : set log file\n"
		   "\n"
		   "    -b             : disable rport extension\n"
		   "    -r realm       : SIP realm\n"
		   "    -u username    : SIP login\n"
		   "    -k password    : SIP password\n"
		   "    -o port        : SIP port to use\n"
		   "    -s stun server : stun server to use\n"
		   "    -p proxy       : register to proxy\n"
		   "    -i identity    : local SIP identity\n"
		   "    -z             : enable SRTP profile\n"
		   "\n" "    -t target     : target SIP url to call\n");
	exit(0);
}

int handle_amsip_events(eXosip_event_t *evt, struct test_account *account)
{
	am_messageinfo_t minfo;
	int e;

	memset(&minfo, 0, sizeof(am_messageinfo_t));
	e = am_message_get_messageinfo(evt->request, &minfo);
	if (e >= 0)
		printf("REQUEST  (%i-%i) %s -- To: %s\n", evt->cid,
		evt->did, minfo.method, minfo.to);

	memset(&minfo, 0, sizeof(am_messageinfo_t));
	e = am_message_get_messageinfo(evt->response, &minfo);
	if (e >= 0)
		printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt->cid,
		evt->did, minfo.answer_code, minfo.reason,
		minfo.to);

	memset(&minfo, 0, sizeof(am_messageinfo_t));
	e = am_message_get_messageinfo(evt->ack, &minfo);
	if (e >= 0)
		printf("REQUEST  (%i-%i) %s -- To: %s\n", evt->cid,
		evt->did, minfo.method, minfo.to);

	memset(&minfo, 0, sizeof(am_messageinfo_t));
	e = am_message_get_messageinfo(evt->request, &minfo);


	if (evt->type == EXOSIP_CALL_ACK) {
	}
	if (evt->type == EXOSIP_CALL_INVITE) {

		osip_header_t *replaces_header = NULL;

		e = osip_message_header_get_byname(evt->request,
			"replaces", 0,
			&replaces_header);
		if (e >= 0) {
			e = am_session_find_by_replaces(evt->request);
			 /* in real use case, we have to find the DID (second parameter
			 must not be hardcoded. */
			am_session_stop(e, 2, 486);
		} else {
			am_session_answer(evt->tid, evt->did, 180, 0);
		}
		am_session_answer(evt->tid, evt->did, 200, 1);
		account->cid=evt->cid;
		account->did=evt->did;
	} else if (evt->type == EXOSIP_CALL_PROCEEDING || evt->type == EXOSIP_CALL_RINGING) {
		account->cid=evt->cid;
		account->did=evt->did;
	} else if (evt->type == EXOSIP_CALL_ANSWERED) {
		account->cid=evt->cid;
		account->did=evt->did;
	} else if (evt->type == EXOSIP_CALL_REINVITE) {
		e = am_message_get_audio_rtpdirection(evt->request);
		if (e == 1) {
			printf("Remote application put us on hold\n");
		}
	} else if (evt->type == EXOSIP_CALL_MESSAGE_NEW
		&& osip_strcasecmp(minfo.method, "REFER") == 0)
	{
		osip_header_t *refer_to = NULL;

		am_session_answer_request(evt->tid, evt->did, 202);

		e = osip_message_header_get_byname(evt->request,
			"refer-to", 0,
			&refer_to);
		if (e >= 0) {
			char sip_identity[256];
			char sip_proxy[256];

			snprintf(sip_proxy, sizeof(sip_proxy), "sip:%s", account->proxy);
			if (account->identity[0]!='\0')
				snprintf(sip_identity, sizeof(sip_identity), "sip:%s", account->identity);
			else if (account->domain[0]!='\0')
				snprintf(sip_identity, sizeof(sip_identity), "sip:%s@%s", account->username, account->domain);
			else
				snprintf(sip_identity, sizeof(sip_identity), "sip:%s@%s", account->username, account->proxy);

			e = am_session_start(sip_identity,
					refer_to->hvalue, sip_proxy, NULL);
			printf("Call transfered to: %s\n",
				refer_to->hvalue);
		} else {
		}
		am_session_stop(evt->cid, evt->did, 486);

	} else if (evt->type == EXOSIP_CALL_MESSAGE_NEW) {
		am_session_answer_request(evt->tid, evt->did, 501);
	} else if (evt->type == EXOSIP_MESSAGE_NEW
		&& osip_strcasecmp(minfo.method, "MESSAGE") == 0) {
			am_bodyinfo_t binfo;

			memset(&binfo, 0, sizeof(am_bodyinfo_t));
			e = am_message_get_bodyinfo(evt->request, 0, &binfo);
			if (e >= 0) {
				printf("REQUEST  MESSAGE: %s\n", binfo.attachment);
				am_message_release_bodyinfo(&binfo);
			}
			am_message_answer(evt->tid, 200);
	}
	return 0;
}

void on_new_image_cb(int pin, int width, int height, int format, int size, void *pixel)
{
	printf("on_new_image_cb: pin value:%i\n", pin);
}

int main(int argc, const char *const argv[])
{
	test_getopt_t *opt;
	int rv;
	const char *optarg;
	char c;

	am_codec_info_t codec;

#ifdef ENABLE_VIDEO
	am_video_codec_info_t video_codec;
#endif

	int debug_level;
	int use_rport;
	int audio_port;
	char log_file[256];
	char config_file[256];
	char stun_server[256];
	int sip_port;
	int enable_srtpprofile;
	int i;

	int loop = 1;
	int idx=0;

	debug_level = 6;
	use_rport = 1;
	audio_port = 8066;
	config_file[0] = '\0';
	log_file[0] = '\0';
	sip_port = 5060;
	stun_server[0] = '\0';
	enable_srtpprofile = 0;

	am_init("antisip-test", debug_level);

	am_option_load_plugins(".");
	//am_option_load_plugins("./amsip.app/Contents/PlugIns");
	am_option_load_plugins("/Library/Isphone/plugin/9.0.0/");

	if (argc > 1 && strlen(argv[1]) == 1 && 0 == strncmp(argv[1], "-", 2))
		usage();
	if (argc > 1 && strlen(argv[1]) >= 2 && 0 == strncmp(argv[1], "--", 2))
		usage();

	test_getopt_init(&opt, argc, argv);

#define __APP_BASEARGS "t:r:m::u:k:p:i:o:s:d:f:l:bzvVh?"

	while ((rv =
			test_getopt(opt, __APP_BASEARGS, &c, &optarg)) == GETOPT_SUCCESS) {
		switch (c) {
		case 'b':
			use_rport = 0;		/* disable rport */
			break;
		case 'm':
			audio_port = atoi(optarg);	/* change default media port */
			break;
		case 's':
			snprintf(stun_server, 256, "%s", optarg);
			break;
		case 'o':
			sip_port = atoi(optarg);
			break;
		case 'z':
			enable_srtpprofile = 1;;
			break;
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 'l':
			snprintf(log_file, 256, "%s", optarg);
			break;
		case 'f':
			snprintf(config_file, 256, "%s", optarg);
			break;
		case 'v':
		case 'V':
			printf("Welcome to the echo-service\n");
			printf("written by Aymeric Moizard <amoizard@osip.org>\n");
			printf("Copyright 2005-2012 Aymeric Moizard\n");
			exit(0);
			break;
		default:
			usage();
			exit(0);
		}
	}

	if (rv != GETOPT_EOF) {
		usage();
		exit(0);
	}


	osip_free((void *) (opt->argv));
	osip_free((void *) opt);


	am_reset("antisip-test", debug_level);
	am_option_debug(log_file, 6);
	am_option_set_rate(16000);
	am_option_set_image_callback(&on_new_image_cb);
	am_option_set_window_handle(NULL, 176, 144);
	am_option_enable_preview(1);

	//am_option_enable_optionnal_encryption(0);
	am_option_enable_rport(use_rport);
	am_option_enable_echo_canceller(0, 0, 0);
	am_option_enable_101(0);
	if (enable_srtpprofile != 0) {
		am_option_set_audio_profile("RTP/SAVP");
		am_option_set_video_profile("RTP/SAVP");
		am_option_set_text_profile("RTP/SAVP");
	}


	am_option_set_initial_audio_port(audio_port);

	am_option_select_in_sound_card(0);
	am_option_select_out_sound_card(0);
	
	am_option_set_volume_in_sound_card(0, 60);
	am_option_set_volume_in_sound_card(0, 60);

	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "speex");
	codec.payload = 99;
	codec.enable = 1;
	codec.freq = 16000;
	codec.vbr = 1;
	codec.cng = 1;
	codec.mode = 6;
	am_codec_info_modify(&codec, 0);

	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "iLBC");
	codec.payload = 97;
	codec.enable = 0;
	codec.freq = 8000;
	codec.mode = 30;
	am_codec_info_modify(&codec, 1);

	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "PCMU");
	codec.payload = 0;
	codec.enable = 1;
	codec.freq = 8000;
	am_codec_info_modify(&codec, 2);

	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "PCMA");
	codec.payload = 8;
	codec.enable = 0;
	codec.freq = 8000;
	am_codec_info_modify(&codec, 3);

#if 0
	snprintf(codec.name, 64, "GSM");
	codec.payload = 3;
	codec.enable = 1;
	codec.freq = 8000;
	codec.vbr = 0;
	codec.cng = 0;
	codec.mode = 0;
	am_codec_info_modify(&codec, 0);
#endif
#if 0
	snprintf(codec.name, 64, "g729");
	codec.payload = 18;
	codec.enable = 1;
	codec.freq = 8000;
	codec.vbr = 1;
	codec.cng = 1;
	codec.mode = 6;
	am_codec_info_modify(&codec, 0);
#endif

#ifdef ENABLE_VIDEO
	memset(&video_codec, 0, sizeof(video_codec));
	snprintf(video_codec.name, 64, "H263-1998");
	video_codec.payload = 115;
	video_codec.enable = 1;
	video_codec.freq = 90000;
	am_video_codec_info_modify(&video_codec, 0);

	{
		am_video_codec_attr_t codec_attr;

		codec_attr.ptime = 0;
		codec_attr.maxptime = 0;
		codec_attr.upload_bandwidth = 128;
		codec_attr.download_bandwidth = 128;
		am_video_codec_attr_modify(&codec_attr);
	}
#endif

	if (stun_server[0] != '\0') {
		am_option_enable_turn_server(stun_server, 1);
	}

	am_option_enable_symmetric_rtp(1);

	i = am_network_start("UDP", sip_port);

	if (i < 0) {
		printf("error: cannot start network for test service!\n");
		return -1;
	}
	am_option_enable_symmetric_rtp(1);

	for (i = 0; /* i < 1000 */; i++) {
		eXosip_event_t evt;
		int k;
		struct test_account *account;

		account = &test_accounts[idx];
		if (account->provider_info[0]=='\0')
			break;

		if (account->provider_info[0]!='\0' && i % 100 == 0)
		{
			char sip_identity[256];
			char sip_proxy[256];

			if (account->proxy[0]=='\0')
				continue; /* skip */

			if (account->username[0] != '\0' && account->password[0] != '\0')
				am_option_set_password(NULL, account->username, account->password);

			snprintf(sip_proxy, sizeof(sip_proxy), "sip:%s", account->proxy);
			if (account->identity[0]!='\0')
				snprintf(sip_identity, sizeof(sip_identity), "%s", account->identity);
			else if (account->domain[0]!='\0')
				snprintf(sip_identity, sizeof(sip_identity), "sip:%s@%s", account->username, account->domain);
			else
				snprintf(sip_identity, sizeof(sip_identity), "sip:%s@%s", account->username, account->proxy);
			account->rid = am_register_start(sip_identity, sip_proxy, 120, 120);

		}

		if (account->provider_info[0]!='\0' && i%100==20)
		{
			if (account->callee_number[0] != '\0') {
				char sip_identity[256];
				char sip_proxy[256];

				snprintf(sip_proxy, sizeof(sip_proxy), "sip:%s", account->proxy);
				if (account->identity[0]!='\0')
					snprintf(sip_identity, sizeof(sip_identity), "sip:%s", account->identity);
				else if (account->domain[0]!='\0')
					snprintf(sip_identity, sizeof(sip_identity), "sip:%s@%s", account->username, account->domain);
				else
					snprintf(sip_identity, sizeof(sip_identity), "sip:%s@%s", account->username, account->proxy);

				account->cid = am_session_start(sip_identity, account->callee_number, sip_proxy,
									 NULL);
				if (account->cid <= 0) {
					am_log(-OSIP_ERROR, "cannot start call!");
				}
			}
		}

		if (account->provider_info[0]!='\0' && i % 100 == 60)
		{
			if (account->rid>0)
				am_register_stop(account->rid);
			if (account->cid>0)
				am_session_stop(account->cid, account->did, 486);
			account->rid=0;

		}
		if (account->provider_info[0]!='\0' && i % 100 == 90)
		{
			idx++;
			continue;
		}


#if 0 //defined(ENABLE_VIDEO) && defined(WIN32)
		MSG msg;
		BOOL fGotMessage;

		if ((fGotMessage =
			 PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE)) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif

		memset(&evt, 0, sizeof(eXosip_event_t));
		osip_usleep(100000);
		k = am_event_get(&evt);
		if (k != AMSIP_TIMEOUT) {
			handle_amsip_events(&evt, account);
			am_event_release(&evt);
		}

		if (i % 10 == 0) {
			struct am_dtmf_event dtmf_event;
			memset(&dtmf_event, 0, sizeof(struct am_dtmf_event));
			k = am_session_get_dtmf_event(2, &dtmf_event);
			if (k == 0) {
				am_log(-OSIP_INFO1, "dtmf_event=%i (duration=%i)",
					   dtmf_event.dtmf, dtmf_event.duration);
			}
		}

		if (i == 40) {


		} else if (i % 100 == 0) {
			struct am_bandwidth_stats band_stats;
			memset(&band_stats, 0, sizeof(band_stats));
			k = am_session_get_audio_bandwidth(2, &band_stats);
		}
		else if (i == 140) {
			/* am_session_send_rtp_dtmf(2, '0'); */
			/* am_session_send_dtmf(2, '1'); */
			/* am_session_play_file(2, "dtmf-4.wav"); */
			/* am_session_inactive(2); */
			/* am_session_refer(2, "sip:welcome@sip.antisip.com", sip_identity); */
			/* am_session_turn_into_conference(2, "conf-3way1"); */
			/* am_player_stop(0); */
			/* am_session_mute(2); */
#ifdef ENABLE_VIDEO
			/* am_session_add_video (2); */
#endif
		}

#if defined(ENABLE_VIDEO) && defined(WIN32)
		{
			MSG msg;
			BOOL fGotMessage;

			if ((fGotMessage =
				 PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE)) != 0) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
#endif
	}

	//i = am_session_stop (2, 3, 486);
	am_reset("antisip-test", debug_level);

	for (i = 0; i < 300; i++) {
		eXosip_event_t evt;
		int k;

#if defined(ENABLE_VIDEO) && defined(WIN32)
		MSG msg;
		BOOL fGotMessage;

		if ((fGotMessage =
			 PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE)) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif

		memset(&evt, 0, sizeof(eXosip_event_t));
		osip_usleep(100000);
		k = am_event_get(&evt);
		if (k != AMSIP_TIMEOUT) {
			am_messageinfo_t minfo;
			int e;

			memset(&minfo, 0, sizeof(am_messageinfo_t));
			e = am_message_get_messageinfo(evt.request, &minfo);
			if (e >= 0)
				printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.cid, evt.did,
					   minfo.method, minfo.to);
			memset(&minfo, 0, sizeof(am_messageinfo_t));
			e = am_message_get_messageinfo(evt.response, &minfo);
			if (e >= 0)
				printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt.cid,
					   evt.did, minfo.answer_code, minfo.reason, minfo.to);
			e = am_message_get_messageinfo(evt.ack, &minfo);
			if (e >= 0)
				printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.cid, evt.did,
					   minfo.method, minfo.to);

			am_event_release(&evt);
		}
	}
	am_quit();

#if 0 //defined(ENABLE_VIDEO) && defined(WIN32)
	{
		MSG msg;
		BOOL fGotMessage;

		while ((fGotMessage =
				PeekMessage(&msg, (HWND) NULL, 0, 0, PM_REMOVE)) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
#endif

	return 0;
}
