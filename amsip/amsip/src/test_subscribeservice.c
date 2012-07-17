/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "test_getopt.h"

#include "amsip/am_options.h"

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
		   "subscribe_service  [options]\n"
		   "    -h          : show options\n"
		   "    -d X        : set debug level\n"
		   "    -f config   : set config file\n"
		   "    -l logfile  : set log file\n"
		   "\n"
		   "    -r realm       : SIP realm\n"
		   "    -u username    : SIP login\n"
		   "    -k password    : SIP password\n"
		   "    -o port        : SIP port to use\n"
		   "    -s stun server : stun server to use\n"
		   "    -p proxy       : register to proxy\n"
		   "    -i identity    : local SIP identity\n"
		   "\n"
		   "    -t target      : target SIP url to subscribe\n");
	exit(0);
}

int main(int argc, const char *const argv[])
{
	test_getopt_t *opt;
	int rv;
	const char *optarg;
	char c;

	am_codec_info_t codec;

	int rid = 0;

	int debug_level;
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
	int i;

	am_messageinfo_t minfo;
	int loop = 10;

	debug_level = 0;
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

	if (argc > 1 && strlen(argv[1]) == 1 && 0 == strncmp(argv[1], "-", 2))
		usage();
	if (argc > 1 && strlen(argv[1]) >= 2 && 0 == strncmp(argv[1], "--", 2))
		usage();

	test_getopt_init(&opt, argc, argv);

#define __APP_BASEARGS "t:r:u:k:p:i:o:s:d:f:l:vVh?"

	while ((rv =
			test_getopt(opt, __APP_BASEARGS, &c, &optarg)) == GETOPT_SUCCESS) {
		switch (c) {
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

	if (debug_level > 0) {
		if (log_file[0] != '\0') {
			FILE *f;

			f = fopen(log_file, "w+");
			if (NULL == f) {
				printf("error with log file (%s)\n", strerror(errno));
				exit(0);
			}
			TRACE_INITIALIZE(debug_level, f);
		} else {
			TRACE_INITIALIZE(debug_level, NULL);
		}
	}

	printf("Welcome to %s program\n", argv[0]);
	am_init("subscribe-service", debug_level);
	am_reset("subscribe-service", debug_level);

	osip_free((void *) (opt->argv));
	osip_free((void *) opt);

	am_option_select_in_sound_card(0);
	am_option_select_out_sound_card(0);

	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "speex");
	codec.payload = 97;
	codec.enable = 1;
	codec.freq = 8000;
	codec.vbr = 1;
	codec.cng = 0;
	codec.mode = 3;
	am_codec_info_modify(&codec, 0);

	memset(&codec, 0, sizeof(codec));
	snprintf(codec.name, 64, "iLBC");
	codec.payload = 98;
	codec.enable = 1;
	codec.freq = 8000;
	codec.mode = 20;
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
	codec.enable = 1;
	codec.freq = 8000;
	am_codec_info_modify(&codec, 3);

	/* am_option_enable_sdp_in_ack (1); */


	if (stun_server[0] != '\0') {
#if 1
		am_option_enable_turn_server(stun_server, 1);
#else
		am_option_enable_stun_server(stun_server, 1);
#endif
	}

	/* am_option_enable_http_proxy ("80.118.132.69", 80, "213.91.9.219", 0); */
	i = am_network_start("UDP", sip_port);

	if (i < 0) {
		printf("error: cannot start network for subscribe service!\n");
		return -1;
	}

	if (sip_realm[0] != '\0')
		am_option_set_password(sip_realm, sip_username, sip_password);

	if (sip_identity[0] != '\0' && sip_proxy[0] != '\0') {
		rid = am_register_start(sip_identity, sip_proxy, 1800, 3600);
		if (rid <= 0) {
			printf
				("error: cannot start registration for subscribe service!\n");
		}
	}

	printf("Starting subscription to %s.\n", sip_to);

	if (sip_identity[0] != '\0' && sip_to[0] != '\0'
		&& sip_proxy[0] != '\0') {
		am_subscription_start(sip_identity, sip_to, sip_proxy, "presence",
							  "application/pidf+xml", 3600);
	}

	if (i < 0) {
		printf
			("error: cannot start subscription for subscribe service!\n");
		return -1;
	}

	while (loop) {
		int active_sid = 0;
		int active_did = 0;
		int e;

		loop--;

		for (i = 0; active_sid == 0; i++) {
			eXosip_event_t evt;
			am_header_t to;
			int k;

			memset(&evt, 0, sizeof(eXosip_event_t));
			memset(&to, 0, sizeof(am_header_t));
			to.index = 0;
			osip_usleep(100000);
			k = am_event_get(&evt);
			if (k >= 0) {
				e = am_message_get_messageinfo(evt.request, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);
				memset(&minfo, 0, sizeof(am_messageinfo_t));
				e = am_message_get_messageinfo(evt.response, &minfo);
				if (e >= 0)
					printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt.sid,
						   evt.did, minfo.answer_code, minfo.reason,
						   minfo.to);
				e = am_message_get_messageinfo(evt.ack, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);

				if (evt.type == EXOSIP_CALL_INVITE) {
					printf("Reject call From:");
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s\n", to.value);
					else
						printf(" ?\n");
					am_session_answer(evt.tid, evt.did, 486, 0);
				} else if (evt.sid > 0) {
					active_sid = evt.sid;
					active_did = evt.did;
				}

				am_event_release(&evt);
			}
		}

		for (i = 0; i < 600 && active_sid != 0; i++) {
			eXosip_event_t evt;
			am_header_t to;
			int k;

			memset(&evt, 0, sizeof(eXosip_event_t));
			memset(&to, 0, sizeof(am_header_t));
			to.index = 0;
			osip_usleep(100000);
			k = am_event_get(&evt);
			if (k >= 0) {
				e = am_message_get_messageinfo(evt.request, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);
				memset(&minfo, 0, sizeof(am_messageinfo_t));
				e = am_message_get_messageinfo(evt.response, &minfo);
				if (e >= 0)
					printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt.sid,
						   evt.did, minfo.answer_code, minfo.reason,
						   minfo.to);
				e = am_message_get_messageinfo(evt.ack, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);

				if (evt.type == EXOSIP_CALL_INVITE) {
					printf("Reject call From:");
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s\n", to.value);
					else
						printf(" ?\n");
					am_session_answer(evt.tid, evt.did, 486, 0);
				} else if (evt.type == EXOSIP_SUBSCRIPTION_REDIRECTED
						   || (evt.type ==
							   EXOSIP_SUBSCRIPTION_REQUESTFAILURE
							   && evt.response != NULL
							   && (evt.response->status_code == 407
								   || evt.response->status_code == 401))) {
				} else if (evt.sid > 0) {
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s)\n", to.value);
					else
						printf(" ?)\n");
				}
				am_event_release(&evt);
			}
		}

		for (i = 0; i < 600 && active_sid != 0; i++) {
			eXosip_event_t evt;
			am_header_t to;
			int k;

			memset(&evt, 0, sizeof(eXosip_event_t));
			memset(&to, 0, sizeof(am_header_t));
			to.index = 0;
			osip_usleep(100000);
			k = am_event_get(&evt);
			if (k >= 0) {
				e = am_message_get_messageinfo(evt.request, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);
				memset(&minfo, 0, sizeof(am_messageinfo_t));
				e = am_message_get_messageinfo(evt.response, &minfo);
				if (e >= 0)
					printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt.sid,
						   evt.did, minfo.answer_code, minfo.reason,
						   minfo.to);
				e = am_message_get_messageinfo(evt.ack, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);

				if (evt.type == EXOSIP_CALL_INVITE) {
					printf("reject new call From:");
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s\n", to.value);
					else
						printf(" ?\n");
					am_session_answer(evt.tid, evt.did, 486, 0);
				} else if (evt.type == EXOSIP_SUBSCRIPTION_REDIRECTED
						   || (evt.type ==
							   EXOSIP_SUBSCRIPTION_REQUESTFAILURE
							   && evt.response != NULL
							   && (evt.response->status_code == 407
								   || evt.response->status_code == 401))) {
				} else if (evt.sid > 0) {
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s)\n", to.value);
					else
						printf(" ?)\n");
				}
				am_event_release(&evt);
			}
		}


		if (active_sid > 0)
			i = am_register_stop(rid);
		/* am_subscription_refresh(active_did, "presence", "application/pidf+xml", 0); */

		for (i = 0; i < 40 && active_sid != 0; i++) {
			eXosip_event_t evt;
			am_header_t to;
			int k;

			memset(&evt, 0, sizeof(eXosip_event_t));
			memset(&to, 0, sizeof(am_header_t));
			to.index = 0;
			osip_usleep(100000);
			k = am_event_get(&evt);
			if (k >= 0) {
				e = am_message_get_messageinfo(evt.request, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);
				memset(&minfo, 0, sizeof(am_messageinfo_t));
				e = am_message_get_messageinfo(evt.response, &minfo);
				if (e >= 0)
					printf("RESPONSE (%i-%i) %i %s -- To: %s\n", evt.sid,
						   evt.did, minfo.answer_code, minfo.reason,
						   minfo.to);
				e = am_message_get_messageinfo(evt.ack, &minfo);
				if (e >= 0)
					printf("REQUEST  (%i-%i) %s -- To: %s\n", evt.sid,
						   evt.did, minfo.method, minfo.to);

				if (evt.type == EXOSIP_CALL_INVITE) {
					printf("reject new call From:");
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s\n", to.value);
					else
						printf(" ?\n");
					am_session_answer(evt.tid, evt.did, 486, 0);
				} else if (evt.type == EXOSIP_CALL_CLOSED
						   || evt.type == EXOSIP_CALL_RELEASED) {
					printf("subscription closed (From:");
					e = am_message_get_header(evt.response, "from", &to);
					if (e >= 0)
						printf(" %s)\n", to.value);
					else
						printf(" ?)\n");
					if (evt.sid == active_sid) {
						active_sid = 0;
						active_did = 0;
					}
				}
				am_event_release(&evt);
			}
		}

	}

	am_quit();
	return 0;
}
