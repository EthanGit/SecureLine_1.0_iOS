/*
 * SIP Registration Agent -- by ww@styx.org
 * 
 * This program is Free Software, released under the GNU General
 * Public License v2.0 http://www.gnu.org/licenses/gpl
 *
 * This program will register to a SIP proxy using the contact
 * supplied on the command line. This is useful if, for some 
 * reason your SIP client cannot register to the proxy itself.
 * For example, if your SIP client registers to Proxy A, but
 * you want to be able to recieve calls that arrive at Proxy B,
 * you can use this program to register the client's contact
 * information to Proxy B.
 *
 * This program requires the eXosip library. To compile,
 * assuming your eXosip installation is in /usr/local,
 * use something like:
 *
 *
 */

#include <osip2/osip_mt.h>
#include <eXosip2/eXosip.h>
#include <winsock2.h>

#include "amsip-internal.h"
struct eXosip_t *amsip_eXosip;

#define PROG_NAME "sipreg"
#define PROG_VER  "1.0"
#define UA_STRING "SipReg v" PROG_VER
#define SYSLOG_FACILITY LOG_DAEMON

static void syslog_wrapper(int a, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}

#define LOG_INFO 0
#define LOG_ERR 0
#define LOG_WARNING 0
#define LOG_DEBUG 0


static void usage(void);
#ifdef OSIP_MT
static void *register_proc(void *arg);
#endif

static void usage(void)
{
	printf("Usage: " PROG_NAME " [required_options] [optional_options]\n"
		   "\n\t[required_options]\n"
		   "\t-r --proxy\tsip:proxyhost[:port]\n"
		   "\t-u --from\tsip:user@host[:port]\n"
		   "\n\t[optional_options]\n"
		   "\t-c --contact\tsip:user@host[:port]\n"
		   "\t-d --debug (log to stderr and do not fork)\n"
		   "\t-e --expiry\tnumber (default 3600)\n"
		   "\t-f --firewallip\tN.N.N.N\n"
		   "\t-h --help\n"
		   "\t-l --localip\tN.N.N.N (force local IP address)\n"
		   "\t-p --port\tnumber (default 5060)\n"
		   "\t-U --username\tauthentication username\n"
		   "\t-P --password\tauthentication password\n");
}

typedef struct regparam_t {
	int regid;
	int expiry;
	int auth;
} regparam_t;

static void *register_proc(void *arg)
{
	struct regparam_t *regparam = arg;
	int reg;

	for (;;) {
		osip_usleep((regparam->expiry / 2) * 1000000);
		eXosip_lock();
		reg = eXosip_register_send_register(regparam->regid, NULL);
		if (0 > reg) {
			fprintf(stdout, "eXosip_register: error while registring");
			exit(1);
		}
		regparam->auth = 0;
		eXosip_unlock();
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int port = 5060;
	char *contact = NULL;
	char *fromuser = NULL;
	const char *localip = NULL;
	const char *firewallip = NULL;
	char *proxy = NULL;
	char *username = NULL;
	char *password = NULL;
	struct regparam_t regparam = { 0, 3600, 0 };
	struct osip_thread *register_thread;
	int debug = 5;

	proxy = osip_strdup("sip:sip.antisip.com");
	fromuser = osip_strdup("sip:test8@sip.antisip.com");
	username = osip_strdup("test8");
	password = osip_strdup("secret");

	if (!proxy || !fromuser) {
		usage();
		exit(1);
	}

	syslog_wrapper(LOG_INFO, UA_STRING " up and running\n");
	syslog_wrapper(LOG_INFO, "proxy: %s\n", proxy);
	syslog_wrapper(LOG_INFO, "fromuser: %s\n", fromuser);
	syslog_wrapper(LOG_INFO, "contact: %s\n", contact);
	syslog_wrapper(LOG_INFO, "expiry: %d\n", regparam.expiry);
	syslog_wrapper(LOG_INFO, "local port: %d\n", port);

	amsip_eXosip = eXosip_malloc();

	if (debug > 0)
		TRACE_INITIALIZE(6, NULL);

	if (eXosip_init()) {
		syslog_wrapper(LOG_ERR, "eXosip_init failed\n");
		exit(1);
	}
	if (eXosip_listen_addr(IPPROTO_UDP, NULL, port, AF_INET, 0)) {
		syslog_wrapper(LOG_ERR, "eXosip_listen_addr failed\n");
		exit(1);
	}

	if (localip) {
		syslog_wrapper(LOG_INFO, "local address: %s\n", localip);
		eXosip_masquerade_contact(localip, port);
	}

	if (firewallip) {
		syslog_wrapper(LOG_INFO, "firewall address: %s:%i\n", firewallip, port);
		eXosip_masquerade_contact(firewallip, port);
	}

	eXosip_set_user_agent(UA_STRING);

	if (username && password) {
		syslog_wrapper(LOG_INFO, "username: %s\n", username);
		syslog_wrapper(LOG_INFO, "password: [removed]\n");
		if (eXosip_add_authentication_info
			(username, username, password, NULL, NULL)) {
			syslog_wrapper(LOG_ERR, "eXosip_add_authentication_info failed\n");
			exit(1);
		}
	}

	{
		osip_message_t *reg = NULL;
		int i;

		regparam.regid =
			eXosip_register_build_initial_register(fromuser, proxy, contact,
												   regparam.expiry * 2, &reg);
		if (regparam.regid < 1) {
			syslog_wrapper(LOG_ERR,
						   "eXosip_register_build_initial_register failed\n");
			exit(1);
		}
		i = eXosip_register_send_register(regparam.regid, reg);
		if (i != 0) {
			syslog_wrapper(LOG_ERR, "eXosip_register_send_register failed\n");
			exit(1);
		}
	}

#ifdef OSIP_MT
	register_thread = osip_thread_create(20000, register_proc, &regparam);
	if (register_thread == NULL) {
		syslog_wrapper(LOG_ERR, "pthread_create failed\n");
		exit(1);
	}
#endif

	for (;;) {
		eXosip_event_t *event;

		if (!(event = eXosip_event_wait(0, 1))) {
#ifndef OSIP_MT
			eXosip_execute();
			eXosip_automatic_action();
#endif
			osip_usleep(10000);
			continue;
		}
#ifndef OSIP_MT
		eXosip_execute();
#endif

		eXosip_automatic_action();
		switch (event->type) {
		case EXOSIP_REGISTRATION_NEW:
			syslog_wrapper(LOG_INFO, "received new registration\n");
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			syslog_wrapper(LOG_INFO, "registrered successfully\n");
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			regparam.auth = 1;
			break;
		case EXOSIP_REGISTRATION_TERMINATED:
			printf("Registration terminated\n");
			break;
		default:
			syslog_wrapper(LOG_DEBUG,
						   "recieved unknown eXosip event (type, did, cid) = (%d, %d, %d)\n",
						   event->type, event->did, event->cid);

		}
		eXosip_event_free(event);
	}

	eXosip_quit();
	osip_free(amsip_eXosip);
}
