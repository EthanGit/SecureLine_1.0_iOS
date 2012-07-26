/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include <osipparser2/osip_port.h>

#include <eXosip2/eXosip.h>
//#include "am_sysdep.h"
//#include "am_uuid.h"

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

PPL_DECLARE (int) am_register_stop(int rid)
{
	osip_message_t *reg;
	int i;

	eXosip_lock();
	i = eXosip_register_build_register(rid, 0, &reg);
	if (i == 0) {
		i = eXosip_register_send_register(rid, reg);
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_register_start_with_parameter(const char *identity, const char *proxy,
				  int refresh_interval, int expires_interval,
				  char *contact_param_name, char *contact_param_value)
{
	int rid = 0;
	osip_message_t *reg;

	eXosip_lock();
	reg = NULL;
	rid = eXosip_register_build_initial_register(identity,
												 proxy,
												 NULL, expires_interval,
												 &reg);
	if (rid < 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "unable to register this line (on %s as %s)", proxy,
			   identity);
	} else {
		int i;

		if (_antisipc.supported_extensions[0] != '\0')
			osip_message_set_supported(reg, _antisipc.supported_extensions);
		if (_antisipc.allowed_events[0] != '\0')
			osip_message_set_header(reg, "Allow-Events", _antisipc.allowed_events);

		if (_antisipc.supported_gruu[0] != '\0') {
			/*
			   ;+sip.instance="<urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6>"
			 */
			osip_contact_t *co = NULL;

			i = osip_message_get_contact(reg, 0, &co);
			if (i >= 0 && co != NULL && co->url != NULL) {
				/* replace username with conference name */
				osip_contact_param_add(co, osip_strdup("+sip.instance"),
									   osip_strdup(_antisipc.
												   supported_gruu));
				//osip_message_set_supported(reg, "gruu");
			}
		}

		if (_antisipc.outbound_proxy != NULL
			&& _antisipc.outbound_proxy[0] != '\0') {
			osip_message_set_route(reg, _antisipc.outbound_proxy);
		}

		if (contact_param_name!=NULL)
		{
			osip_contact_t *co = NULL;

			i = osip_message_get_contact(reg, 0, &co);
			if (i >= 0 && co != NULL && co->url != NULL) {
				if (contact_param_value==NULL)
					osip_contact_param_add(co, osip_strdup(contact_param_name),
											   NULL);
				else
					osip_contact_param_add(co, osip_strdup(contact_param_name),
											   osip_strdup(contact_param_value));
			}
		}

		i = eXosip_register_send_register(rid, reg);
		if (i == 0)
			am_trace(__FILE__, __LINE__, -OSIP_INFO1, "registration started on %s as %s", proxy,
				   identity);
		else
			am_trace(__FILE__, __LINE__, -OSIP_ERROR,
				   "unable to build/send register this line (on %s as %s)",
				   proxy, identity);
		if (i != 0) {
			eXosip_register_remove(rid);
			rid = i;
		}
	}
	eXosip_unlock();

	return rid;
}

PPL_DECLARE (int)
am_register_start(const char *identity, const char *proxy,
				  int refresh_interval, int expires_interval)
{
	return am_register_start_with_parameter(identity, proxy,
				  refresh_interval, expires_interval, NULL, NULL);
}

PPL_DECLARE (int) am_register_refresh(int rid, int expires)
{
	osip_message_t *reg;
	int i;

	eXosip_lock();
	reg = NULL;
	i = eXosip_register_build_register(rid, expires, &reg);
	if (i < 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "unable to refresh register (rid=%i)", rid);
		eXosip_unlock();
		return i;
	}
	i = eXosip_register_send_register(rid, reg);
	if (i < 0) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "unable to send refresh register (rid=%i)", rid);
		eXosip_unlock();
		return i;
	}
	eXosip_unlock();
	return i;
}

PPL_DECLARE (int)
am_register_send_star(const char *identity, const char *proxy)
{
	int rid = 0;
	osip_message_t *reg;
	int pos;
	int i;


	eXosip_lock();
	reg = NULL;
	rid =
		eXosip_register_build_initial_register(identity, proxy, "*", 0,
											   &reg);
	if (rid < 0 || reg == NULL) {
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "unable to register this line (on %s as %s)", proxy,
			   identity);
		eXosip_unlock();
		return rid;
	}

	/* if registration already exist, old contacts are still used */
	pos = 0;
	while (!osip_list_eol(&reg->contacts, pos)) {
		osip_contact_t *co;

		co = (osip_contact_t *) osip_list_get(&reg->contacts, pos);
		osip_list_remove(&reg->contacts, pos);
		osip_contact_free(co);
	}

	osip_message_set_contact(reg, "*");

	if (_antisipc.supported_extensions[0] != '\0')
		osip_message_set_supported(reg, _antisipc.supported_extensions);
	if (_antisipc.allowed_events[0] != '\0')
		osip_message_set_header(reg, "Allow-Events", _antisipc.allowed_events);

	if (_antisipc.supported_gruu[0] != '\0') {
		/*
		   ;+sip.instance="<urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6>"
		 */
		osip_contact_t *co = NULL;

		i = osip_message_get_contact(reg, 0, &co);
		if (i >= 0 && co != NULL && co->url != NULL) {
			/* replace username with conference name */
			osip_contact_param_add(co, osip_strdup("+sip.instance"),
								   osip_strdup(_antisipc.supported_gruu));
			//osip_message_set_supported(reg, "gruu");
		}
	}

	if (_antisipc.outbound_proxy != NULL
		&& _antisipc.outbound_proxy[0] != '\0') {
		osip_message_set_route(reg, _antisipc.outbound_proxy);
	}

	i = eXosip_register_send_register(rid, reg);
	if (i == 0)
		am_trace(__FILE__, __LINE__, -OSIP_INFO1, "registration started (contact=*) on %s as %s", proxy,
			   identity);
	else
		am_trace(__FILE__, __LINE__, -OSIP_ERROR, "unable to build/send register (contact=*) on %s as %s",
			   proxy, identity);
	if (i != 0) {
		eXosip_register_remove(rid);
		rid = i;
	}

	eXosip_unlock();

	return rid;
}

PPL_DECLARE (int) am_register_remove(int rid)
{
	int i;

	eXosip_lock();
	i = eXosip_register_remove(rid);
	if (i == 0) {
	}
	eXosip_unlock();
	return i;
}
