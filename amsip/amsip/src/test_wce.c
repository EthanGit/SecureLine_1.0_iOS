/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012  Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "test_getopt.h"

#include <amsip/am_options.h>

#ifdef HARDWARE_TEST
#include <amsiptools/hardware_guid.h>
#endif

#if !defined(WIN32)
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>				/* dirname */
#endif

#include <osip2/osip.h>
#include <eXosip2/eXosip.h>

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


#if defined(_WIN32_WCE)

/* Parse a command line buffer into arguments */
static int ParseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	int argc;

	argc = 0;
	for (bufp = cmdline; *bufp;) {
		/* Skip leading whitespace */
		while (((CHAR) * bufp == ' ') || ((CHAR) * bufp == '\t')) {
			++bufp;
		}
		/* Skip over argument */
		if (*bufp == '"') {
			++bufp;
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && (*bufp != '"')) {
				++bufp;
			}
		} else {
			if (*bufp) {
				if (argv) {
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && !isspace(*bufp)) {
				++bufp;
			}
		}
		if (*bufp) {
			if (argv) {
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if (argv) {
		argv[argc] = NULL;
	}
	return (argc);
}

int main(int argc, const char *const argv[]);

int WINAPI
WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPWSTR szCmdLine,
		int sw)
{
	char **argv;
	int argc;
	char *cmdline;

	wchar_t *bufp;
	int nLen;

	/* Grab the command line */
	nLen = wcslen(szCmdLine) + 128 + 1;
	bufp = osip_malloc(sizeof(wchar_t) * nLen * 2);
	wcscpy(bufp, TEXT("\""));
	GetModuleFileName(NULL, bufp + 1, 128 - 3);
	wcscpy(bufp + wcslen(bufp), TEXT("\" "));
	wcsncpy(bufp + wcslen(bufp), szCmdLine, nLen - wcslen(bufp));
	nLen = wcslen(bufp) + 1;
	cmdline = osip_malloc(sizeof(char) * nLen);
	if (cmdline == NULL) {
		return -1;
	}
	WideCharToMultiByte(CP_ACP, 0, bufp, -1, cmdline, nLen, NULL, NULL);

	/* Parse it into argv and argc */
	argc = ParseCommandLine(cmdline, NULL);
	argv = malloc(sizeof(char *) * (argc + 1));
	if (argv == NULL) {
		return -1;
	}
	ParseCommandLine(cmdline, argv);

	main(argc, argv);

	return 0;
}

#endif

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
	char sip_to[256];
	char sip_proxy[256];
	char sip_identity[256];
	int sip_port;
	char sip_realm[256];
	char sip_username[256];
	char sip_password[256];
	int enable_srtpprofile;
	int i;

	int loop = 1;

#ifdef HARDWARE_TEST
	hdd_guid_t hg;
	proc_guid_t hp;
	mb_guid_t hm;
	printf("Welcome to %s program\n", argv[0]);
	printf("written by Aymeric Moizard <amoizard@gmail.com>\n");
	printf("Copyright 2005-2012 Aymeric Moizard\n");

	hdd_guid_get(&hg);
	if (hg.volume_name[0]!='\0')
	{
		printf("HDD name: %s\n", hg.volume_name);
		printf("HDD Hardware serial: %s\n", hg.volume_serial);
		printf("HDD GUID: %s\n",hg.volume_guid);
	}
	else 
		printf("HDD Sorry, not detected\n");

	proc_guid_get(&hp);
	if (hp.proc_name[0]!='\0')
	{
		printf("PROC name: %s\n", hp.proc_name);
		printf("PROC Hardware serial: %s\n", hp.proc_serial);
		printf("PROC GUID: %s\n",hp.proc_guid);
	}
	else 
		printf("PROC Sorry, not detected\n");

	mb_guid_get(&hm);
	if (hm.mb_name[0]!='\0')
	{
		printf("MB name: %s\n", hm.mb_name);
		printf("MB Hardware serial: %s\n", hm.mb_serial);
		printf("MB GUID: %s\n",hm.mb_guid);
	}
	else 
		printf("MB Sorry, not detected\n");
	return 0;
#endif

	debug_level = 6;
	use_rport = 1;
	audio_port = 8066;
	config_file[0] = '\0';
	log_file[0] = '\0';
	sip_to[0] = '\0';
	sip_identity[0] = '\0';
	sip_proxy[0] = '\0';
	sip_port = 5060;
	stun_server[0] = '\0';
	sip_realm[0] = '\0';
	sip_username[0] = '\0';
	sip_password[0] = '\0';
	enable_srtpprofile = 0;

#ifdef _WIN32_WCE
	snprintf(sip_realm, 256, "%s", "sip.antisip.com");
	snprintf(sip_username, 256, "%s", "test1");
	snprintf(sip_password, 256, "%s", "secret");
	snprintf(sip_proxy, 256, "%s", "sip:sip.antisip.com");
	snprintf(sip_identity, 256, "%s", "sip:test1@sip.antisip.com");
	//snprintf (stun_server, 256, "%s", "stun.goober.com");
	snprintf(sip_to, 256, "%s", "sip:antisip@sip.antisip.com");
	sip_port = 5060;
	debug_level = 6;
#endif

	am_init("antisip-test", debug_level);

	am_option_load_plugins("plug");

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
		case 't':
			snprintf(sip_to, 256, "%s", optarg);
			break;
		case 'r':
			snprintf(sip_realm, 256, "%s", optarg);
			break;
		case 'u':
			snprintf(sip_username, 256, "%s", optarg);
			break;
		case 'k':
			snprintf(sip_password, 256, "%s", optarg);
			break;
		case 'p':
			snprintf(sip_proxy, 256, "%s", optarg);
			break;
		case 'i':
			snprintf(sip_identity, 256, "%s", optarg);
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
			printf("Welcome to %s program\n", argv[0]);
			printf("written by Aymeric Moizard <amoizard@gmail.com>\n");
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
	am_option_set_rate(16000);
	//am_option_set_window_handle(NULL, 176, 144);
	//am_option_enable_preview(1);
	//am_option_enable_optionnal_encryption(0);
	am_option_enable_rport(use_rport);
	am_option_enable_echo_canceller(0, 0, 0);
	am_option_enable_101(0);
	if (enable_srtpprofile != 0) {
		am_option_set_audio_profile("RTP/SAVP");
		am_option_set_video_profile("RTP/SAVP");
		am_option_set_text_profile("RTP/SAVP");
	}

	am_option_debug(log_file, 6);

	am_option_set_initial_audio_port(audio_port);

	am_option_select_in_sound_card(0);
	am_option_select_out_sound_card(0);
	
	am_option_set_volume_in_sound_card(0, 60);
	am_option_set_volume_in_sound_card(0, 60);

#if 0
	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "avatar-events");
	codec.payload = 123;
	codec.enable = 0;
	codec.freq = 8000;
	codec.vbr = 1;
	codec.cng = 1;
	codec.mode = 6;
	am_codec_info_modify(&codec, 0);
#else
	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "speex");
	codec.payload = 99;
	codec.enable = 1;
	codec.freq = 16000;
	codec.vbr = 1;
	codec.cng = 1;
	codec.mode = 6;
	am_codec_info_modify(&codec, 0);
#endif

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
#if 1
		am_option_enable_turn_server(stun_server, 1);
#else
		am_option_enable_stun_server(stun_server, 1);
#endif
	}

	am_option_enable_symmetric_rtp(1);

	i = am_network_start("UDP", sip_port);

	if (i < 0) {
		printf("error: cannot start network for test service!\n");
		return -1;
	}
	am_option_enable_symmetric_rtp(1);

	if (sip_realm[0] != '\0')
		am_option_set_password(sip_realm, sip_username, sip_password);

	if (sip_identity[0] != '\0' && sip_proxy[0] != '\0') {
		/* i = am_register_send_star (sip_identity, sip_proxy); */
		i = am_register_start(sip_identity, sip_proxy, 120, 120);
		if (i <= 0) {
			printf("error: cannot start registration for echo service!\n");
		}
	}

	loop = 1;
	while (loop) {
		loop--;

		for (i = 0; i < 1000; i++) {
			eXosip_event_t evt;
			am_header_t to;
			am_messageinfo_t minfo;
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
			memset(&to, 0, sizeof(am_header_t));
			memset(&minfo, 0, sizeof(am_messageinfo_t));
			to.index = 0;
			osip_usleep(100000);
			k = am_event_get(&evt);
			if (k != AMSIP_TIMEOUT) {
				int e = am_message_get_messageinfo(evt.request, &minfo);

				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.cid,
						   evt.did, minfo.method, minfo.to);
				memset(&minfo, 0, sizeof(am_messageinfo_t));
				e = am_message_get_messageinfo(evt.response, &minfo);
				if (e >= 0)
					printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt.cid,
						   evt.did, minfo.answer_code, minfo.reason,
						   minfo.to);
				e = am_message_get_messageinfo(evt.ack, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.cid,
						   evt.did, minfo.method, minfo.to);

				memset(&minfo, 0, sizeof(am_messageinfo_t));
				e = am_message_get_messageinfo(evt.request, &minfo);
				if (evt.type == EXOSIP_CALL_ACK) {
#if 0
					static int done = 0;

					if (done == 0)
						am_session_play_file(2, "holdmusic.wav");
					done++;
#endif
				}
				if (evt.type == EXOSIP_CALL_INVITE) {
					char remote_audio_ipport[256];
					int direct_mode = 0;


					osip_header_t *replaces_header = NULL;

					k = osip_message_header_get_byname(evt.request,
													   "replaces", 0,
													   &replaces_header);
					if (k >= 0) {
						am_session_stop(1, 2, 486);
					} else {
						am_session_answer(evt.tid, evt.did, 180, 0);
					}
					am_session_answer(evt.tid, evt.did, 200, 1);
					direct_mode =
						am_session_get_audio_remote(evt.did,
													remote_audio_ipport);
					if (direct_mode == 0) {
						printf("MEDIA PROXY IS USED\n");
					} else if (direct_mode == 1) {
						printf("MEDIA PROXY IS NOT USED ANY MORE\n");
					} else {
						printf("NO INFORMATION ON MEDIA PROXY USE\n");
					}
				} else if (evt.type == EXOSIP_CALL_REINVITE) {
					k = am_message_get_audio_rtpdirection(evt.request);
					if (k == 1) {
						printf("Remote application put us on hold\n");
					}
				} else if (evt.type == EXOSIP_CALL_MESSAGE_NEW
						   && osip_strcasecmp(minfo.method, "REFER") == 0)
				{
					osip_header_t *refer_to = NULL;

					am_session_answer_request(evt.tid, evt.did, 202);

					k = osip_message_header_get_byname(evt.request,
													   "refer-to", 0,
													   &refer_to);
					if (k >= 0) {
						k = am_session_start(sip_identity,
											 refer_to->hvalue, sip_proxy,
											 NULL);
						printf("Call transfered to: %s\n",
							   refer_to->hvalue);
					} else {
					}
					am_session_stop(1, 2, 486);


				} else if (evt.type == EXOSIP_CALL_MESSAGE_NEW) {
					am_session_answer_request(evt.tid, evt.did, 501);
				} else if (evt.type == EXOSIP_MESSAGE_NEW
						   && osip_strcasecmp(minfo.method,
											  "MESSAGE") == 0) {
					am_bodyinfo_t binfo;

					memset(&binfo, 0, sizeof(am_bodyinfo_t));
					e = am_message_get_bodyinfo(evt.request, 0, &binfo);
					if (e >= 0) {
						printf("REQUEST  MESSAGE: %s\n", binfo.attachment);
						am_message_release_bodyinfo(&binfo);
					}
					am_message_answer(evt.tid, 200);
				}
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
				if (sip_identity[0] != '\0'
					&& sip_to[0] != '\0' && sip_proxy[0] != '\0') {
#if 1
					//k = am_session_start (sip_identity, "<sip:antisip@sip.antisip.com?Replaces=CALLID%3Bto-tag=aaaaa%3Bfrom-tag=bbbb>", sip_proxy, NULL);
					k = am_session_start(sip_identity, sip_to, sip_proxy,
										 NULL);
					if (k <= 0) {
						am_log(-OSIP_ERROR, "cannot start call!");
					}
#else
					osip_message_t *message;

					/*am_message_execute_uri (&message,
					   sip_identity, "<sip:joe@sip.antisip.com;method=REFER?Refer-to=sip:welcome%40sip.antisip.com&Call-Info=%3Chttp://sip.antisip.com%3E;purpose=rendering>",
					   sip_proxy, NULL,
					   NULL);
					 */
					ams_message_build(&message,
									  sip_identity, sip_to,
									  sip_proxy, NULL, " Hello Boy");
					if (message != NULL)
						ams_message_send(message);
#endif
				}
			} else if (i % 100 == 0) {
				struct am_audio_stats audio_stats;
				memset(&audio_stats, 0, sizeof(struct am_audio_stats));
				k = am_session_get_audio_statistics(2, &audio_stats);

#if 0
				if (k == 0) {
					if (audio_stats.pk_loss > 5) {
						printf
							("Too Many packet Loss: -> compress more!\n");
					} else {
						printf
							("no packet Loss detected: -> increase quality!\n");
					}

				}
#endif
#if 0
				am_session_send_rtp_dtmf(2, '0');
				am_session_send_rtp_dtmf(2, '2');
				am_session_send_rtp_dtmf(2, '3');
				am_session_send_rtp_dtmf(2, '4');
				am_session_send_rtp_dtmf(2, '5');
#endif
			} else if (i == 70) {
				am_session_send_dtmf(2, '0');
				/* am_session_send_rtp_dtmf (2, '1'); */
				/* am_session_play_file(2, "dtmf-4.wav"); */
			} else if (i == 140) {
#if 0
				char remote_audio_ipport[256];
				int direct_mode = 0;

				direct_mode =
					am_session_get_audio_remote(2, remote_audio_ipport);
				if (direct_mode == 0) {
					printf("MEDIA PROXY IS USED\n");
				} else if (direct_mode == 1) {
					printf("MEDIA PROXY IS NOT USED ANY MORE\n");
				} else {
					printf("NO INFORMATION ON MEDIA PROXY USE\n");
				}
#endif
				/* k = am_session_modify_bitrate (2, "speex", 1); */
				/* am_session_play_file(2, "ringback.wav"); */
			}
			if (i == 140) {
#ifdef ENABLE_VIDEO
				/* am_session_add_video (2); */
#endif
				/* am_player_start(1, "holdmusic.wav", 1); */

				//am_session_inactive(2);
				//am_session_refer(2, "sip:welcome@sip.antisip.com", sip_identity);
				/* am_session_turn_into_conference(2, "conf-3way1"); */
			}
			if (i == 180) {
				/* am_player_stop(0); */
				/* am_session_unmute(2); */
				/* am_session_add_in_conference(sip_identity, "sip:antisip@sip.antisip.com", sip_proxy, NULL, "conf-3way1"); */
			}
		}


	}
	//i = am_session_stop (1, 3, 486);
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

#if defined(ENABLE_VIDEO) && defined(WIN32)
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
