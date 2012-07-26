/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include <stdarg.h>
#include <osipparser2/osip_port.h>
#include "sdptools.h"

/**
 * Get message information (method, target//code, reason + From/To) from message.
 *
 * @param msg                SIP message where to extract message info.
 * @param value              Elements from message info.
 */
PPL_DECLARE (int)
am_message_get_messageinfo(osip_message_t * msg, am_messageinfo_t * value)
{
	int i;
	char *tmp = NULL;

	memset(value, '\0', sizeof(am_messageinfo_t));
	if (msg == NULL)
		return -4;
	if (MSG_IS_RESPONSE(msg)) {
		value->answer_code = msg->status_code;
		if (msg->reason_phrase != NULL) {
			osip_strncpy(value->reason, msg->reason_phrase,
						 sizeof(value->reason) - 1);
		}
	} else {
		if (msg->sip_method != NULL) {
			osip_strncpy(value->method, msg->sip_method,
						 sizeof(value->method) - 1);
		}
		tmp = NULL;
		i = osip_uri_to_str(msg->req_uri, &tmp);
		if (i >= 0 && tmp != NULL) {
			osip_strncpy(value->target, tmp, sizeof(value->target) - 1);
			osip_free(tmp);
		}
	}

	if (msg->call_id != NULL && msg->call_id != NULL
		&& msg->call_id->host != NULL)
		snprintf(value->call_id, sizeof(value->call_id), "%s@%s",
				 msg->call_id->number, msg->call_id->host);
	else if (msg->call_id != NULL && msg->call_id != NULL
			 && msg->call_id->host == NULL)
		snprintf(value->call_id, sizeof(value->call_id), "%s",
				 msg->call_id->number);

	if (msg->from != NULL) {
		osip_from_t *from;
		int i = osip_from_clone(msg->from, &from);

		if (i == 0) {
			int pos = 0;
			osip_uri_header_t *u_header;
			osip_uri_param_t *u_param;

			while (!osip_list_eol(&from->gen_params, 0)) {
				u_param =
					(osip_uri_param_t *) osip_list_get(&from->gen_params,
													   0);
				osip_list_remove(&from->gen_params, 0);
				osip_uri_param_free(u_param);
			}
			while (!osip_list_eol(&from->url->url_params, 0)) {
				u_param =
					(osip_uri_param_t *) osip_list_get(&from->url->
													   url_params, 0);
				osip_list_remove(&from->url->url_params, 0);
				osip_uri_param_free(u_param);
			}
			while (!osip_list_eol(&from->url->url_headers, pos)) {
				u_header =
					(osip_uri_header_t *) osip_list_get(&from->url->
														url_headers, pos);
				osip_list_remove(&from->url->url_headers, pos);
				osip_uri_header_free(u_header);
			}

			tmp = NULL;
			i = osip_from_to_str(from, &tmp);
			if (i >= 0 && tmp != NULL) {
				osip_strncpy(value->from, tmp, sizeof(value->from) - 1);
				osip_free(tmp);
			}
			osip_from_free(from);
		}
	}

	if (msg->to != NULL) {
		osip_from_t *to;
		int i = osip_to_clone(msg->to, &to);

		if (i == 0) {
			int pos = 0;
			osip_uri_header_t *u_header;
			osip_uri_param_t *u_param;

			while (!osip_list_eol(&to->gen_params, 0)) {
				u_param =
					(osip_uri_param_t *) osip_list_get(&to->gen_params, 0);
				osip_list_remove(&to->gen_params, 0);
				osip_uri_param_free(u_param);
			}
			while (!osip_list_eol(&to->url->url_params, 0)) {
				u_param =
					(osip_uri_param_t *) osip_list_get(&to->url->
													   url_params, 0);
				osip_list_remove(&to->url->url_params, 0);
				osip_uri_param_free(u_param);
			}
			while (!osip_list_eol(&to->url->url_headers, pos)) {
				u_header =
					(osip_uri_header_t *) osip_list_get(&to->url->
														url_headers, pos);
				osip_list_remove(&to->url->url_headers, pos);
				osip_uri_header_free(u_header);
			}

			tmp = NULL;
			i = osip_to_to_str(to, &tmp);
			if (i >= 0 && tmp != NULL) {
				osip_strncpy(value->to, tmp, sizeof(value->to) - 1);
				osip_free(tmp);
			}
			osip_to_free(to);
		}
	}

	if (osip_list_size(&msg->contacts) > 0) {
		osip_contact_t *co;

		i = osip_message_get_contact(msg, 0, &co);
		if (i >= 0 && co != NULL) {
			tmp = NULL;
			i = osip_contact_to_str(co, &tmp);
			if (i >= 0 && tmp != NULL) {
				osip_strncpy(value->contact, tmp,
							 sizeof(value->contact) - 1);
				osip_free(tmp);
			}
		}
	}

	return 0;
}

PPL_DECLARE (int)
am_message_add_header_to_uri(char *uri, const char *header_name,
							 const char *header_value, char *dest_uri,
							 int dest_size)
{
	osip_uri_t *uri_new = NULL;
	char *dest = NULL;
	int i;

	i = osip_uri_init(&uri_new);
	if (uri_new == NULL) {
		return -1;
	}
	i = osip_uri_parse(uri_new, uri);
	if (i < 0 || uri_new->host == NULL) {
		osip_uri_free(uri_new);
		return -1;
	}

	osip_uri_uheader_add(uri_new, osip_strdup(header_name),
						 osip_strdup(header_value));

	i = osip_uri_to_str(uri_new, &dest);
	osip_uri_free(uri_new);

	if (i < 0 || dest == NULL) {
		return -1;
	}

	snprintf(dest_uri, dest_size, "%s", dest);
	return 0;
}

PPL_DECLARE (int)
am_message_get_header(osip_message_t * msg, char *header,
					  am_header_t * value)
{
	osip_header_t *hdr;
	char *tmp = NULL;
	int i;

	memset(value->value, '\0', sizeof(value->value));
	if (msg == NULL)
		return -4;

	i = osip_message_header_get_byname(msg, header, value->index, &hdr);
	if (hdr != NULL && hdr->hvalue != NULL) {
		value->index = i;
		osip_strncpy(value->value, hdr->hvalue, sizeof(value->value) - 1);
		return 0;
	}
	if (osip_strcasecmp(header, "from") == 0) {
		if (msg->from == NULL)
			return -3;
		i = osip_from_to_str(msg->from, &tmp);
		if (i >= 0 && tmp != NULL) {
			osip_strncpy(value->value, tmp, sizeof(value->value) - 1);
			osip_free(tmp);
			return 0;
		}
		return -2;
	}
	if (osip_strcasecmp(header, "to") == 0) {
		if (msg->to == NULL)
			return -3;
		i = osip_from_to_str(msg->to, &tmp);
		if (i >= 0 && tmp != NULL) {
			osip_strncpy(value->value, tmp, sizeof(value->value) - 1);
			osip_free(tmp);
			return 0;
		}
		return -2;
	}
	if (osip_strcasecmp(header, "content-type") == 0) {
		if (msg->content_type == NULL)
			return -3;
		i = osip_content_type_to_str(msg->content_type, &tmp);
		if (i >= 0 && tmp != NULL) {
			osip_strncpy(value->value, tmp, sizeof(value->value) - 1);
			osip_free(tmp);
			return 0;
		}
		return -2;
	}

	return -1;
}

PPL_DECLARE (int)
am_message_get_bodyinfo(osip_message_t * msg, int attachemnt_index,
						am_bodyinfo_t * bodyinfo)
{
	osip_body_t *body;
	size_t body_length;
	char *tmp = NULL;
	char *ct = NULL;
	int i;

	memset(bodyinfo, '\0', sizeof(am_bodyinfo_t));
	if (msg == NULL)
		return -4;

	if (attachemnt_index >= osip_list_size(&msg->bodies))
		return -3;				/* no body at this index */

	body = (osip_body_t *) osip_list_get(&msg->bodies, attachemnt_index);
	if (body == NULL)
		return -1;

	i = osip_body_to_str(body, &tmp, &body_length);
	if (i < 0 || body_length <= 0 || tmp == NULL)
		return -1;

	bodyinfo->content_length = (int) body_length;
	bodyinfo->attachment = tmp;

	if (osip_list_size(&msg->bodies) == 1) {
		/* get content-type SIP header */
		i = osip_content_type_to_str(msg->content_type, &ct);
		if (i >= 0 && ct != NULL) {
			snprintf(bodyinfo->content_type,
					 sizeof(bodyinfo->content_type), "%s", ct);
			osip_free(ct);
		}
		return 0;
	}

	/* get content-type from body */
	i = osip_content_type_to_str(body->content_type, &ct);
	if (i >= 0 && ct != NULL) {
		snprintf(bodyinfo->content_type, sizeof(bodyinfo->content_type),
				 "%s", ct);
		osip_free(ct);
	}
	return 0;
}

PPL_DECLARE (int) am_message_release_bodyinfo(am_bodyinfo_t * bodyinfo)
{
	osip_free(bodyinfo->attachment);
	return 0;
}

PPL_DECLARE (int) am_message_get_audio_rtpdirection(osip_message_t * msg)
{
	sdp_message_t *msg_sdp;
	sdp_media_t *msg_med;
	int direction;


	msg_sdp = eXosip_get_sdp_info(msg);
	if (msg_sdp == NULL)
		return -1;
	msg_med = eXosip_get_audio_media(msg_sdp);
	if (msg_med == NULL) {
		sdp_message_free(msg_sdp);
		return -1;
	}

	direction = _sdp_analyse_attribute(msg_sdp, msg_med);
	sdp_message_free(msg_sdp);

	return direction;
}

PPL_DECLARE (int) am_message_get_video_rtpdirection(osip_message_t * msg)
{
	sdp_message_t *msg_sdp;
	sdp_media_t *msg_med;
	int direction;


	msg_sdp = eXosip_get_sdp_info(msg);
	if (msg_sdp == NULL)
		return -1;
	msg_med = eXosip_get_video_media(msg_sdp);
	if (msg_med == NULL) {
		sdp_message_free(msg_sdp);
		return -1;
	}

	direction = _sdp_analyse_attribute(msg_sdp, msg_med);
	sdp_message_free(msg_sdp);

	return direction;
}

/**
 * Set body in SIP message
 *
 * @param msg                SIP message where to add info.
 * @param ctt                Content-Type.
 * @param body               Body To add.
 */
PPL_DECLARE (int)
am_message_set_body(osip_message_t * msg, const char *ctt,
					const char *body, int body_size)
{
	if (msg == NULL)
		return -1;
	if (ctt != NULL)
		osip_message_set_content_type(msg, ctt);
	if (body != NULL)
		osip_message_set_body(msg, body, body_size);
	return 0;
}

/**
 * Allocate and Add an "unknown" header (not defined in oSIP).
 * @param sip The element to work on.
 * @param hname The token name.
 * @param hvalue The token value.
 */
PPL_DECLARE (int)
am_message_set_header(osip_message_t * sip, const char *hname,
							const char *hvalue)
{
	return osip_message_set_header(sip, hname, hvalue);
}
