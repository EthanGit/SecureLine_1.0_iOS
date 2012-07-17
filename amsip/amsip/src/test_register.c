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

int main(int argc, const char *const argv[])
{
	test_getopt_t *opt;
	int rv;
	const char *optarg;
	char c;

	int debug_level;
	int use_rport;
	char log_file[256];
	char stun_server[256];
	char sip_proxy[256];
	char sip_identity[256];
	int sip_port;
	char sip_realm[256];
	char sip_username[256];
	char sip_password[256];
	int i;

	debug_level = 6;
	use_rport = 1;
	log_file[0] = '\0';
	sip_identity[0] = '\0';
	sip_proxy[0] = '\0';
	sip_port = 5060;
	stun_server[0] = '\0';
	sip_realm[0] = '\0';
	sip_username[0] = '\0';
	sip_password[0] = '\0';

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
	am_option_enable_rport(use_rport);
	am_option_enable_101(0);

	am_option_debug(log_file, 6);

	if (stun_server[0] != '\0') {
		am_option_enable_turn_server(stun_server, 1);
	}

	am_option_enable_symmetric_rtp(1);

	i = am_network_start("UDP", sip_port);

	if (i < 0) {
		printf("error: cannot start network for test service!\n");
		return -1;
	}

	if (sip_realm[0] != '\0')
		am_option_set_password(sip_realm, sip_username, sip_password);

	if (sip_identity[0] != '\0' && sip_proxy[0] != '\0') {
		/* i = am_register_send_star (sip_identity, sip_proxy); */
		i = am_register_start(sip_identity, sip_proxy, 120, 120);
		if (i <= 0) {
			printf("error: cannot start registration for echo service!\n");
		}
	}

	for (i = 0; i < 1000; i++) {
		eXosip_event_t evt;
		am_messageinfo_t minfo;
		int k;

		memset(&evt, 0, sizeof(eXosip_event_t));
		memset(&minfo, 0, sizeof(am_messageinfo_t));
		osip_usleep(100000);
		k = am_event_get(&evt);
		if (k >= 0) {
			int e = am_message_get_messageinfo(evt.request, &minfo);

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

			memset(&minfo, 0, sizeof(am_messageinfo_t));
			e = am_message_get_messageinfo(evt.request, &minfo);
			if (evt.type == EXOSIP_CALL_ACK) {
			}
			if (evt.type == EXOSIP_CALL_INVITE) {
				am_session_answer(evt.tid, evt.did, 486, 0);
			} else if (evt.type == EXOSIP_CALL_MESSAGE_NEW) {
				am_session_answer_request(evt.tid, evt.did, 501);
			} else if (evt.type == EXOSIP_MESSAGE_NEW
					   && osip_strcasecmp(minfo.method, "OPTIONS") == 0) {
				am_message_answer(evt.tid, 200);
			} else if (evt.type == EXOSIP_MESSAGE_NEW
					   && osip_strcasecmp(minfo.method, "PING") == 0) {
				am_message_answer(evt.tid, 200);
			} else if (evt.type == EXOSIP_MESSAGE_NEW
					   && osip_strcasecmp(minfo.method, "MESSAGE") == 0) {
				am_bodyinfo_t binfo;

				memset(&binfo, 0, sizeof(am_bodyinfo_t));
				e = am_message_get_bodyinfo(evt.request, 0, &binfo);
				if (e >= 0) {
					printf("REQUEST  MESSAGE: %s\n", binfo.attachment);
					am_message_release_bodyinfo(&binfo);
				}
				am_message_answer(evt.tid, 200);
			} else if (evt.type == EXOSIP_MESSAGE_NEW) {
				am_message_answer(evt.tid, 200);
			}
			am_event_release(&evt);
		}



	}
	//i = am_session_stop (1, 3, 486);
	am_reset("antisip-test", debug_level);

	for (i = 0; i < 300; i++) {
		eXosip_event_t evt;
		int k;

		memset(&evt, 0, sizeof(eXosip_event_t));
		osip_usleep(100000);
		k = am_event_get(&evt);
		if (k >= 0) {
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

	return 0;
}
