/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include "amsip/am_message.h"

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

/**
* This method build and send a SIP message.
*
* The url argument can be used to specify the method: default
* will be MESSAGE.
*
* @param message The message to build.
* @param identity
* @param url
* @param proxy
* @param outbound_proxy
* @param buf
*/
PPL_DECLARE (int)
am_message_execute_uri(osip_message_t ** message,
					   const char *identity, const char *url,
					   const char *proxy, const char *outbound_proxy,
					   const char *buf)
{
	osip_message_t *msg;
	int i;
	osip_from_t *to = NULL;
	char route[256];
	char method[256];

	memset(route, '\0', sizeof(route));

	*message = NULL;

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
	}

	/* search for "method" parameter */
	snprintf(method, sizeof(method), "MESSAGE");
	{
		int pos = 0;
		size_t pname_len;
		osip_uri_param_t *u_param;

		pname_len = strlen("method");
		while (!osip_list_eol(&to->url->url_params, pos)) {
			size_t len;

			u_param = (osip_uri_param_t *) osip_list_get(&to->url->url_params, pos);
			len = strlen(u_param->gname);
			if (pname_len == len
				&& osip_strncasecmp(u_param->gname, "method", pname_len) == 0
				&& u_param->gvalue!=NULL) {
					snprintf(method, sizeof(method), "%s", u_param->gvalue);
					break;
			}
			pos++;
		}
	}

	osip_from_free(to);

	i = eXosip_message_build_request(&msg, method, url, identity, route);
	if (i != 0) {
		return i;
	}

	if (buf != NULL) {
		osip_message_set_body(msg, buf, strlen(buf));
	}

	*message = msg;
	return AMSIP_SUCCESS;
}

/**
* This method build and send a MESSAGE message.
* @param message The message to build.
* @param identity
* @param url
* @param proxy
* @param outbound_proxy
* @param buf
*/
PPL_DECLARE (int)
ams_message_build(osip_message_t ** message,
				  const char *identity, const char *url,
				  const char *proxy, const char *outbound_proxy,
				  const char *buf)
{
	osip_message_t *msg;
	int i;
	osip_from_t *to = NULL;
	char route[256];

	memset(route, '\0', sizeof(route));

	*message = NULL;

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
	}

	osip_from_free(to);

	i = eXosip_message_build_request(&msg, "MESSAGE", url, identity,
									 route);
	if (i != 0) {
		return i;
	}

	if (buf != NULL) {
		osip_message_set_body(msg, buf, strlen(buf));
	}

	*message = msg;
	return AMSIP_SUCCESS;
}

/**
* This method send SIP messages.
* @param message The message to send.
*/
PPL_DECLARE (int) ams_message_send(osip_message_t * message)
{
	int i;
	osip_header_t *hdr = NULL;

	osip_message_get_expires(message, 0, &hdr);
	if (hdr == NULL) {
		osip_message_set_expires(message, "120");
	}

	if (osip_list_size(&message->bodies) > 0) {
		if (message->content_type == NULL) {
			osip_message_set_content_type(message, "text/plain");
		}
	}

	eXosip_lock();
	i = eXosip_message_send_request(message);
	eXosip_unlock();
	return i;
}

/**
* This method build and send a MESSAGE message.
* @param identity
* @param url
* @param proxy
* @param outbound_proxy
* @param buf
*/
PPL_DECLARE (int)
am_message_send(const char *identity, const char *url,
				const char *proxy, const char *outbound_proxy,
				const char *buf)
{
	osip_message_t *message;
	int i;
	osip_from_t *to = NULL;
	char route[256];

	memset(route, '\0', sizeof(route));

	if (url == NULL || url[0] == '\0')
		return AMSIP_BADPARAMETER;

	i = osip_from_init(&to);
	if (i != 0)
		return i;
	i = osip_from_parse(to, url);
	if (i != 0 || to == NULL || to->url == NULL) {
		osip_from_free(to);
		return i;
	}

	if (proxy[0] != '\0' && to->url != NULL && to->url->host != NULL
		&& strlen(proxy) > 4
		&& 0 == osip_strcasecmp(proxy + 4, to->url->host))
	{
		/* no pre-route set route! */
	} else if (proxy[0] != '\0') {
		snprintf(route, 256, "<%s;lr>", proxy);
	}

	osip_from_free(to);

	i = eXosip_message_build_request(&message, "MESSAGE", url, identity,
									 route);
	if (i != 0) {
		return i;
	}
	osip_message_set_expires(message, "120");
	osip_message_set_body(message, buf, strlen(buf));
	osip_message_set_content_type(message, "text/plain");

	eXosip_lock();
	i = eXosip_message_send_request(message);
	eXosip_unlock();
	return i;
}


PPL_DECLARE (int) am_message_answer(int tid, int code)
{
	int i;

	eXosip_lock();

	i = eXosip_message_send_answer(tid, code, NULL);

	eXosip_unlock();
	return i;
}
