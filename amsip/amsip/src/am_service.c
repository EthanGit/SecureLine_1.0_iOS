/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include "amsip/am_options.h"

#include "am_calls.h"

extern struct antisipc _antisipc;	/* main handle */

struct callpickup_service {
	int callpickup_sid;
	char identity[128];
	char proxy[128];
};

static struct callpickup_service cps;

static char *_am_service_get_body_info(osip_message_t * message, const char *type,
								const char *subtype)
{
	osip_content_type_t *ctt;
	osip_body_t *oldbody;
	int pos;

	if (message == NULL)
		return NULL;

	/* get content-type info */
	ctt = osip_message_get_content_type(message);
	if (ctt == NULL)
		return NULL;			/* previous message was not correct or empty */

	if (ctt->type == NULL || ctt->subtype == NULL)
		return NULL;
	if (osip_strcasecmp(ctt->type, "multipart") == 0) {
		/* probably within the multipart attachement */
	} else if (osip_strcasecmp(ctt->type, type) == 0 &&
			   osip_strcasecmp(ctt->subtype, subtype) == 0) {
		oldbody = (osip_body_t *) osip_list_get(&message->bodies, 0);
		return osip_strdup(oldbody->body);
	}

	pos = 0;
	while (!osip_list_eol(&message->bodies, pos)) {
		oldbody = (osip_body_t *) osip_list_get(&message->bodies, pos);
		pos++;
		if (oldbody->content_type != NULL
			&& oldbody->content_type->type != NULL
			&& oldbody->content_type->subtype != NULL
			&& osip_strcasecmp(oldbody->content_type->type, type) == 0
			&& osip_strcasecmp(oldbody->content_type->subtype,
							   subtype) == 0)
			return osip_strdup(oldbody->body);
	}
	return NULL;
}

int _am_service_automatic_pickup(eXosip_event_t * evt_notify)
{
	char *xml = NULL;

	if (cps.callpickup_sid == 0 || evt_notify->sid != cps.callpickup_sid)
		return -2;				/* not related to call pickup */

	if (evt_notify->type != EXOSIP_SUBSCRIPTION_NOTIFY)
		return -1;

	cps.callpickup_sid = 0;

	xml =
		_am_service_get_body_info(evt_notify->request, "application",
								  "dialog-info+xml");
	if (xml != NULL) {
		char *remote_entity;
		char *remote_entity_end;
		char *call_id;
		char *local_tag;
		char *remote_tag;
		char *early;

		char *ptr;				/* add \0 between "<dialog" and ">" */
		char *end;				/* add \0 between "<dialog" and ">" */

		ptr = osip_strcasestr(xml, "<dialog ");
		if (ptr == NULL) {
			osip_free(xml);
			return -1;
		}
		end = osip_strcasestr(ptr, "/dialog");
		if (end == NULL) {
			osip_free(xml);
			return -1;
		}
		*ptr = '\0';
		*end = '\0';

		early = osip_strcasestr(ptr + 1, "early");
		if (early == NULL) {
			/* too late... */
			osip_free(xml);
			return -1;
		}
		remote_entity = osip_strcasestr(ptr + 1, "<remote");
		if (remote_entity == NULL || remote_entity > end) {
			osip_free(xml);
			return -1;
		}
		remote_entity_end = osip_strcasestr(remote_entity, "/remote");
		if (remote_entity_end == NULL || remote_entity_end > end) {
			osip_free(xml);
			return -1;
		}
		remote_entity = osip_strcasestr(remote_entity, "<identity");
		if (remote_entity == NULL || remote_entity > remote_entity_end) {
			osip_free(xml);
			return -1;
		}
		remote_entity = strstr(remote_entity, ">");
		if (remote_entity == NULL || remote_entity > remote_entity_end) {
			osip_free(xml);
			return -1;
		}

		call_id = osip_strcasestr(ptr + 1, "call-id");
		local_tag = osip_strcasestr(ptr + 1, "local-tag");
		remote_tag = osip_strcasestr(ptr + 1, "remote-tag");

		if (call_id == NULL || local_tag == NULL || remote_tag == NULL) {
			osip_free(xml);
			return -1;
		}

		remote_entity = osip_strcasestr(remote_entity, "sip:");
		call_id = strchr(call_id, '"');
		local_tag = strchr(local_tag, '"');
		remote_tag = strchr(remote_tag, '"');

		if (remote_entity == NULL || call_id == NULL || local_tag == NULL
			|| remote_tag == NULL) {
			osip_free(xml);
			return -1;
		}

		remote_entity--;
		*remote_entity = '\0';
		*call_id = '\0';
		*local_tag = '\0';
		*remote_tag = '\0';

		remote_entity++;
		call_id++;
		local_tag++;
		remote_tag++;

		/* end dialog info */
		ptr = strchr(remote_entity, '<');
		if (ptr == NULL) {
			osip_free(xml);
			return -1;
		}
		*ptr = '\0';

		ptr = strchr(call_id, '"');
		if (ptr == NULL) {
			osip_free(xml);
			return -1;
		}
		*ptr = '\0';
		ptr = strchr(local_tag, '"');
		if (ptr == NULL) {
			osip_free(xml);
			return -1;
		}
		*ptr = '\0';
		ptr = strchr(remote_tag, '"');
		if (ptr == NULL) {
			osip_free(xml);
			return -1;
		}
		*ptr = '\0';



		/* Ready to send INVITE with Replaces header to caller! */
		{
			int i;
			char atmp[256];
			char *final_uri;
			osip_uri_t *uri;

			osip_uri_init(&uri);
			i = osip_uri_parse(uri, remote_entity);
			if (i != 0) {
				osip_free(xml);
				osip_uri_free(uri);
				return -1;
			}
			snprintf(atmp, sizeof(atmp),
					 "%s;to-tag=%s;from-tag=%s;early-only", call_id,
					 local_tag, remote_tag);
			osip_uri_uheader_add(uri, osip_strdup("Replaces"),
								 osip_strdup(atmp));

			i = osip_uri_to_str(uri, &final_uri);
			osip_uri_free(uri);
			if (i != 0) {
				osip_free(xml);
				return -1;
			}

			am_session_start(cps.identity, final_uri, cps.proxy, NULL);
			osip_free(final_uri);
		}

		osip_free(xml);
		return 0;
	}
	return -1;
}

PPL_DECLARE (int)
am_service_pickup(const char *identity, const char *url, const char *proxy)
{
	if (identity == NULL)
		return -1;

	/* old pickup is silently discarded */
	memset(&cps, 0, sizeof(struct callpickup_service));
	snprintf(cps.identity, sizeof(cps.identity), "%s", identity);
	if (proxy != NULL)
		snprintf(cps.proxy, sizeof(cps.proxy), "%s", proxy);

	/* start a subscription on a dialog, and send an INVITE with Replaces header on it. */
	cps.callpickup_sid =
		am_subscription_start(identity, url, proxy, "dialog",
							  "application/dialog-info+xml", 0);

	if (cps.callpickup_sid <= 0) {
		return -1;
	}

	return 0;
}
