/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"

#include "am_calls.h"

#if !defined(WIN32)
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>				/* dirname */
#endif

#include <osip2/osip.h>
#include <eXosip2/eXosip.h>

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

#if defined(HAVE_STDARG_H) || defined(WIN32)
#  include <stdarg.h>
#  define VA_START(a, f)  va_start(a, f)
#else
#  if defined(HAVE_VARARGS_H)
#    include <varargs.h>
#    define VA_START(a, f) va_start(a)
#  else
#    include <stdarg.h>
#    define VA_START(a, f)  va_start(a, f)
#  endif
#endif

static void log_event(eXosip_event_t * je);

PPL_DECLARE (void) am_log(int lev, char *chfr, ...)
{
	va_list ap;
	char buf1[256];

	VA_START(ap, chfr);
	vsnprintf(buf1, 256, chfr, ap);

	if (_antisipc.syslog_name[0] == '\0')
		snprintf(_antisipc.syslog_name, sizeof(_antisipc.syslog_name),
				 "amsip");

	if (lev >= 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, lev, NULL, "%s: %s\n",
					_antisipc.syslog_name, buf1));
	} else {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, -lev, NULL, "%s: %s\n",
					_antisipc.syslog_name, buf1));
	}
	va_end(ap);
}

PPL_DECLARE (void) am_trace(char *fi, int li, int lev, char *chfr, ...)
{
	va_list ap;
	char buf1[256];

	VA_START(ap, chfr);
	vsnprintf(buf1, 256, chfr, ap);

	if (_antisipc.syslog_name[0] == '\0')
		snprintf(_antisipc.syslog_name, sizeof(_antisipc.syslog_name),
				 "amsip");

	if (lev >= 0) {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, lev, NULL, "%s: %s\n",
					_antisipc.syslog_name, buf1));
	} else {
		OSIP_TRACE(osip_trace
				   (__FILE__, __LINE__, -lev, NULL, "%s: %s\n",
					_antisipc.syslog_name, buf1));
	}
	va_end(ap);
}

static void log_event(eXosip_event_t * je)
{
	char buf[200];

	memset(buf, '\0', sizeof(buf));
	if (je->type == EXOSIP_CALL_NOANSWER || je->type == EXOSIP_CALL_CLOSED
		|| je->type == EXOSIP_CALL_RELEASED) {
		snprintf(buf, 199, "<- (%i %i) Call Closed", je->cid, je->did);
	} else if (je->type == EXOSIP_CALL_CANCELLED
			   && je->request != NULL) {
		snprintf(buf, 199, "<- (%i %i) Call Cancelled", je->cid, je->did);
	} else if (je->type == EXOSIP_MESSAGE_NEW
			   && je->request != NULL && MSG_IS_MESSAGE(je->request)) {
		char *tmp = NULL;

		if (je->request != NULL) {
			osip_body_t *body;

			osip_from_to_str(je->request->from, &tmp);

			osip_message_get_body(je->request, 0, &body);
			if (body != NULL && body->body != NULL) {
				snprintf(buf, 199, "<- (%i) from: %s TEXT: %s",
						 je->tid, tmp, body->body);
			}
			osip_free(tmp);
		} else {
			snprintf(buf, 199, "<- (%i) New event for unknown request?",
					 je->tid);
		}
	} else if (je->type == EXOSIP_MESSAGE_NEW
			   && je->request != NULL && MSG_IS_OPTIONS(je->request)) {
		char *tmp = NULL;

		osip_from_to_str(je->request->from, &tmp);
		snprintf(buf, 199, "<- (%i) %s from: %s",
				 je->tid, je->request->cseq->method, tmp);
		osip_free(tmp);
	} else if (je->type == EXOSIP_MESSAGE_NEW) {
		char *tmp = NULL;

		osip_from_to_str(je->request->from, &tmp);
		snprintf(buf, 199, "<- (%i) %s from: %s",
				 je->tid, je->request->sip_method, tmp);
		osip_free(tmp);
	} else if (je->type == EXOSIP_MESSAGE_PROCEEDING
			   || je->type == EXOSIP_MESSAGE_ANSWERED
			   || je->type == EXOSIP_MESSAGE_REDIRECTED
			   || je->type == EXOSIP_MESSAGE_REQUESTFAILURE
			   || je->type == EXOSIP_MESSAGE_SERVERFAILURE
			   || je->type == EXOSIP_MESSAGE_GLOBALFAILURE) {
		if (je->response != NULL && je->request != NULL) {
			char *tmp = NULL;

			osip_to_to_str(je->request->to, &tmp);
			snprintf(buf, 199, "<- (%i) [%i %s for %s] to: %s",
					 je->tid, je->response->status_code,
					 je->response->reason_phrase, je->request->sip_method,
					 tmp);
			osip_free(tmp);
		} else if (je->request != NULL) {
			snprintf(buf, 199, "<- (%i) Error for %s request",
					 je->tid, je->request->sip_method);
		} else {
			snprintf(buf, 199, "<- (%i) Error for unknown request",
					 je->tid);
		}
	} else if (je->response == NULL && je->request != NULL && je->cid > 0) {
		char *tmp = NULL;

		osip_from_to_str(je->request->from, &tmp);
		snprintf(buf, 199, "<- (%i %i) %s from: %s",
				 je->cid, je->did, je->request->cseq->method, tmp);
		osip_free(tmp);
	} else if (je->response != NULL && je->cid > 0) {
		char *tmp = NULL;

		osip_to_to_str(je->request->to, &tmp);
		snprintf(buf, 199, "<- (%i %i) [%i %s] to: %s",
				 je->cid, je->did, je->response->status_code,
				 je->response->reason_phrase, tmp);
		osip_free(tmp);
	} else if (je->response == NULL && je->request != NULL && je->rid > 0) {
		char *tmp = NULL;

		osip_from_to_str(je->request->from, &tmp);
		snprintf(buf, 199, "<- (%i) %s from: %s",
				 je->rid, je->request->cseq->method, tmp);
		osip_free(tmp);

	} else if (je->response != NULL && je->rid > 0) {
		char *tmp = NULL;

		osip_from_to_str(je->request->from, &tmp);
		snprintf(buf, 199, "<- (%i) [%i %s] from: %s",
				 je->rid, je->response->status_code,
				 je->response->reason_phrase, tmp);
		osip_free(tmp);

	}

	am_trace(__FILE__, __LINE__, -OSIP_INFO1, buf);
}

PPL_DECLARE (void) am_event_release(eXosip_event_t * evt)
{
	if (evt == NULL)
		return;
	if (evt->request != NULL)
		osip_message_free(evt->request);
	if (evt->response != NULL)
		osip_message_free(evt->response);
	if (evt->ack != NULL)
		osip_message_free(evt->ack);
}

PPL_DECLARE (int) am_event_get(eXosip_event_t * evt)
{
	return am_event_wait(evt, 0, 0);
}

PPL_DECLARE (int) am_event_wait(eXosip_event_t * evt, int tv_s, int tv_ms)
{
	eXosip_event_t *je;
	int err = AMSIP_SUCCESS;

	if (evt != NULL)
		memset(evt, '\0', sizeof(eXosip_event_t));

	je = eXosip_event_wait(tv_s, tv_ms);
	eXosip_lock();
	eXosip_automatic_action();
	eXosip_unlock();
	if (je == NULL)
		return AMSIP_TIMEOUT;
	log_event(je);

#if 0
	{
		int err = _am_service_automatic_pickup(je);

		if (err != -2) {
			/* event for call pickup service */
			am_event_release(je);
			return err;
		}
	}
#endif

	eXosip_lock();

#if 0
	if (je->nid > 0) {
		int i;
		osip_header_t *event_header = NULL;

		if (je->request != NULL) {
			osip_message_header_get_byname(je->request, "event", 0,
										   &event_header);
			if (event_header != NULL && event_header->hvalue != NULL
				&& osip_strcasecmp(event_header->hvalue, "dialog") == 0) {
				i = eXosip_insubscription_automatic(je);
				if (i == 0) {
					am_event_release(je);
					eXosip_unlock();
					return i;
				}
			}
		}
	}
#endif

	if (je->type == EXOSIP_CALL_MESSAGE_REQUESTFAILURE && je->did < 0) {
		if (je != NULL && je->response != NULL
			&& (je->response->status_code == 407
				|| je->response->status_code == 401))
			eXosip_default_action(je);
	}

	if (je->type == EXOSIP_CALL_INVITE) {
		int answer_code = 0;

		err = call_new(je, &answer_code);
	} else if (je->type == EXOSIP_CALL_ACK) {
		err = call_ack(je);
	} else if (je->type == EXOSIP_CALL_ANSWERED) {
		err = call_answered(je);
	} else if (je->type == EXOSIP_CALL_PROCEEDING) {
		err = call_proceeding(je);
	} else if (je->type == EXOSIP_CALL_RINGING) {
		err = call_ringing(je);
	} else if (je->type == EXOSIP_CALL_REDIRECTED) {
		err = call_redirected(je);
	} else if (je->type == EXOSIP_CALL_REQUESTFAILURE) {
		err = call_requestfailure(je);
	} else if (je->type == EXOSIP_CALL_SERVERFAILURE) {
		err = call_serverfailure(je);
	} else if (je->type == EXOSIP_CALL_GLOBALFAILURE) {
		err = call_globalfailure(je);
	} else if (je->type == EXOSIP_CALL_NOANSWER) {
	} else if (je->type == EXOSIP_CALL_CLOSED) {
		err = call_closed(je);
	} else if (je->type == EXOSIP_CALL_RELEASED) {
		err = call_closed(je);
	} else if (je->type == EXOSIP_CALL_REINVITE) {
		int answer_code = 0;

		err = call_modified(je, &answer_code);
	} else if (je->type == EXOSIP_REGISTRATION_SUCCESS) {
		_antisipc.gruu_contact[0] = '\0';
		if (_antisipc.supported_gruu[0] != '\0' && je->response != NULL) {
			/*
			   Contact: <sip:callee@192.0.2.1>
			   ;gruu="sip:callee@example.com;
			   opaque=urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6"
			   ;+sip.instance="<urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6>"
			   ;expires=3600    
			 */
			osip_contact_t *co = NULL;
			int i = osip_message_get_contact(je->response, 0, &co);

			if (i >= 0 && co != NULL && co->url != NULL) {
				osip_generic_param_t *gruu;

				/* replace username with conference name */
				i = osip_contact_param_get_byname(co, "gruu", &gruu);
				if (i == 0 && gruu != NULL && gruu->gvalue != NULL) {
					size_t len_gruu = strlen(gruu->gvalue);

					if (len_gruu > 5
						&& len_gruu < sizeof(_antisipc.gruu_contact)) {
						memset(_antisipc.gruu_contact, 0,
							   sizeof(_antisipc.gruu_contact));
						snprintf(_antisipc.gruu_contact, len_gruu - 2,
								 "%s", gruu->gvalue + 1);
					}
				} else {
					/* disable gruu support */
					_antisipc.supported_gruu[0] = '\0';
#if 0
					/* test GRUU in invite */
					snprintf(_antisipc.gruu_contact,
							 sizeof(_antisipc.gruu_contact), "%s",
							 "sip:jack@sip.antisip.com;opaque=urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6");
#endif
				}
			}
		}
	} else if (je->type == EXOSIP_CALL_MESSAGE_NEW
			   && je->request != NULL && MSG_IS_UPDATE(je->request)) {
		int answer_code = 0;

		err = call_update(je, &answer_code);
	} else if (je->type == EXOSIP_MESSAGE_NEW
			   && je->request != NULL && MSG_IS_OPTIONS(je->request)) {
		err = calls_options_outside_calls(je);
	} else if (je->type == EXOSIP_CALL_MESSAGE_NEW
			   && je->request != NULL && MSG_IS_OPTIONS(je->request)) {
	} else if (je->type == EXOSIP_MESSAGE_REQUESTFAILURE
			   && je->request != NULL) {
		if (je->response != NULL) {
			if (MSG_IS_PUBLISH(je->request)) {
			} else if (je->response->status_code == 407
					   || je->response->status_code == 401)
				eXosip_default_action(je);
		}
	}

	if (evt != NULL)
		memcpy(evt, je, sizeof(eXosip_event_t));
	else {
		am_event_release(je);
	}
	eXosip_unlock();
	osip_free(je);
	return err;
}
