/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "amsip/am_options.h"
#include "sdptools.h"

#include "am_calls.h"

extern struct antisipc _antisipc;	/* main handle */

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

PPL_DECLARE (int)
am_subscription_start(const char *identity, const char *url,
					  const char *proxy, const char *event,
					  const char *accept, int expires)
{
	osip_message_t *subscribe;
	int i;

	osip_from_t *to = NULL;
	osip_route_t *aroute = NULL;
	char route[256];

	memset(route, '\0', sizeof(route));

	if (proxy == NULL || proxy[0] == '\0')
		return AMSIP_BADPARAMETER;	/* there must be a proxy right now */

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL || to->url->host == NULL) {
		osip_from_free(to);
		return i;
	}

	/* check if we want to use the route or not */
	if (proxy != NULL && proxy[0] != '\0') {
		i = osip_route_init(&aroute);
		if (i != 0) {
			osip_from_free(to);
			return i;
		}
		i = osip_route_parse(aroute, proxy);
		if (i != 0 || aroute == NULL || aroute->url == NULL
			|| aroute->url->host == NULL) {
			osip_from_free(to);
			osip_route_free(aroute);
			return i;
		}

		if (0 != osip_strcasecmp(aroute->url->host, to->url->host)) {
			snprintf(route, 256, "<sip:%s;lr>", aroute->url->host);
		}
	}


	eXosip_lock();

	i = eXosip_subscribe_build_initial_request(&subscribe,
											   url, identity, route, event,
											   expires);


	if (i != 0) {
		eXosip_unlock();
		osip_route_free(aroute);
		osip_from_free(to);
		return i;
	}

	osip_message_set_accept(subscribe,
							accept /* "application/pidf+xml" */ );

	if (_antisipc.gruu_contact[0] != '\0') {
		/* replace with the gruu contact */
		osip_contact_t *co = NULL;
		osip_uri_t *uri_new = NULL;

		i = osip_uri_init(&uri_new);
		if (uri_new != NULL) {
			char *gruu_contact = osip_strdup(_antisipc.gruu_contact);

			i = -1;
			if (gruu_contact != NULL) {
				i = osip_uri_parse(uri_new, gruu_contact);
				osip_free(gruu_contact);
			}
			if (i == 0 && uri_new->host != NULL) {
				i = osip_message_get_contact(subscribe, 0, &co);
				if (i == 0 && co != NULL && co->url != NULL) {
					osip_uri_free(co->url);
					co->url = NULL;
				}
				if (i == 0 && co != NULL) {
					co->url = uri_new;
				} else
					osip_uri_free(uri_new);
			} else
				osip_uri_free(uri_new);
		}
	}

	if (aroute != NULL) {
		/* special check to modify username in request-uri if it was different
		   than to header */
		if (aroute->url->username != NULL && to->url->username != NULL
			&& 0 != osip_strcasecmp(aroute->url->username,
									to->url->username)) {
			/* modify request uri to use username from argument proxy */
			if (subscribe->req_uri->username != NULL)
				osip_free(subscribe->req_uri->username);
			subscribe->req_uri->username = aroute->url->username;
			aroute->url->username = NULL;
		}
	}

	i = eXosip_subscribe_send_initial_request(subscribe);
	eXosip_unlock();
	osip_route_free(aroute);
	osip_from_free(to);
	return i;
}

PPL_DECLARE (int)
am_subscription_refresh(int did, const char *event, const char *accept,
						int expires)
{
	osip_message_t *subscribe;
	int i;
	char exp[256];

	memset(exp, '\0', sizeof(exp));

	snprintf(exp, sizeof(exp), "%i", expires);

	eXosip_lock();
	i = eXosip_subscribe_build_refresh_request(did, &subscribe);

	if (i != 0) {
		eXosip_unlock();
		return i;
	}

	osip_message_set_accept(subscribe,
							accept /* "application/pidf+xml" */ );
	osip_message_set_header(subscribe, "Event", event);
	osip_message_set_expires(subscribe, exp);

	i = eXosip_subscribe_send_refresh_request(did, subscribe);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_subscription_build_refresh(int did, const char *event, const char *accept,
							  int expires, osip_message_t **subscribe)
{
	int i;
	char exp[256];

	memset(exp, '\0', sizeof(exp));

	snprintf(exp, sizeof(exp), "%i", expires);

	eXosip_lock();
	i = eXosip_subscribe_build_refresh_request(did, subscribe);

	if (i != 0) {
		eXosip_unlock();
		return i;
	}

	osip_message_set_accept(*subscribe,
							accept /* "application/pidf+xml" */ );
	osip_message_set_header(*subscribe, "Event", event);
	osip_message_set_expires(*subscribe, exp);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_subscription_send_refresh(int did, osip_message_t *subscribe)
{
	int i;

	if (subscribe==NULL)
		return AMSIP_BADPARAMETER;

	eXosip_lock();
	i = eXosip_subscribe_send_refresh_request(did, subscribe);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_subscription_remove(int did)
{
	int i;
	eXosip_lock();
	i=eXosip_subscribe_remove(did);
	if (i > 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_insubscription_answer(int tid, int did, int code)
{
	osip_message_t *answer;
	int i;

	eXosip_lock();
	i = eXosip_insubscription_build_answer(tid, code, &answer);
	if (i != 0) {
		eXosip_unlock();
		return i;
	}
	i = eXosip_insubscription_send_answer(tid, code, answer);
	if (i != 0) {
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_insubscription_build_answer(int tid, int code, osip_message_t ** answer)
{
	int i;

	eXosip_lock();
	i = eXosip_insubscription_build_answer(tid, code, answer);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_insubscription_send_answer(int tid, int code, osip_message_t * answer)
{
	int i;

	eXosip_lock();
	i = eXosip_insubscription_send_answer(tid, code, answer);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_insubscription_build_notify(int did, int subscription_status,
							   int subscription_reason,
							   osip_message_t ** notify)
{
	int i;

	eXosip_lock();
	i = eXosip_insubscription_build_notify(did, subscription_status,
										   subscription_reason, notify);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_insubscription_send_request(int did, osip_message_t * request)
{
	int i;

	eXosip_lock();
	i = eXosip_insubscription_send_request(did, request);
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int) am_insubscription_remove(int did)
{
	int i;

	eXosip_lock();
	i = eXosip_insubscription_remove(did);
	eXosip_unlock();
	return i;
}
