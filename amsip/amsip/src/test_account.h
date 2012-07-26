/*
amsip is a SIP library for softphone (SIP -rfc3261-)
Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
*/

struct test_account {
	char provider_info[512];
	char network_info[512];
	char proxy[512];
	char domain[512];
	char username[512];
	char transport[512];
	char password[512];
	char sip_id[512]; /* not used? */
	char identity[512];
	char callee_number[512];
	char callee_number_NOTEXIST[512];
	int rid;
	int cid;
	int did;
};

struct test_account test_accounts[] = {
	{"ANTISIP.COM",	"PUBLIC", "sip.antisip.com", "", "test1", "udp", "secret", "", "", "sip:welcome@sip.antisip.com", "", 0, 0 , 0},
	{"",	"", "", "", "", "", "", "", "", "", "", 0, 0, 0}
};
