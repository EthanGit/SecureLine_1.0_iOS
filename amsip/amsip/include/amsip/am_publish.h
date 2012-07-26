/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_PUBLISH_H__
#define __AM_PUBLISH_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>
#include <osipparser2/osip_port.h>

/**
 * @file am_publish.h
 * @brief amsip publication API
 *
 * This file provide the API needed to publish SIP events to a SIP proxy.
 *
 * <ul>
 * <li>send PUBLISH message.</li>
 * <li>send PUBLISH with Expires=0 message (delete publication).</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_publish amsip publication interface
 * @ingroup amsip_message
 * @{
 */

/**
 * Configure amsip to publish to a SIP server.
 *
 * @param to        Set the target SIP identity (often the same than "from" header)
 * @param from      Set your SIP identity
 * @param route     Set the proxy server
 * @param event     Set the event package extension.
 * @param expires   Set the Expires header
 * @param ctype     Set the Content-Type of body
 * @param body      Set the body data.
 */
	PPL_DECLARE (int) am_publish_send(const char *to,
									  const char *from,
									  const char *route,
									  const char *event,
									  const char *expires,
									  const char *ctype, const char *body);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
