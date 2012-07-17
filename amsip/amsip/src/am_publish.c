/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include <stdarg.h>
#include <eXosip2/eXosip.h>
#include <osipparser2/osip_port.h>


#include "amsip/am_publish.h"

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

PPL_DECLARE (int)
am_publish_send(const char *to,
				const char *from,
				const char *route,
				const char *event,
				const char *expires, const char *ctype, const char *body)
{
	osip_message_t *publish_message;
	int i;


	eXosip_lock();
	/* build a default SIP PUBLISH message */
	i = eXosip_build_publish(&publish_message,
							 to, from, route, event, expires, ctype, body);
	if (i != 0) {
		/* error */
		eXosip_unlock();
		return i;
	}

	/* send the PUBLISH message */
	i = eXosip_publish(publish_message, to);
	if (i != 0) {
		/* error */
		eXosip_unlock();
		return i;
	}

	eXosip_unlock();
	return AMSIP_SUCCESS;
}
