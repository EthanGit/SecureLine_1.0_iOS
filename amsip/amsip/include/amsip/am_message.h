/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_MESSAGE_H__
#define __AM_MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>
#include <osipparser2/osip_port.h>

/**
 * @file am_message.h
 * @brief amsip messaging API
 *
 * This file provide the API needed to send SIP messages.
 *
 * <ul>
 * <li>send SIP messages.</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_instantmessage amsip publication interface
 * @ingroup amsip_message
 * @{
 */


/**
* This method build and send a SIP message.
*
* The url argument can be used to specify the method: default
* will be MESSAGE. The url can contains "method" parameter to
* specify a different SIP method as well as SIP headers.
*
* This message must then be sent using "ams_message_send".
*
* @param message The message to build.
* @param identity
* @param url
* @param proxy
* @param outbound_proxy DO NOT USE (use proxy instead)
* @param buf
*/
	PPL_DECLARE (int)
	 am_message_execute_uri(osip_message_t ** message,
							const char *identity, const char *url,
							const char *proxy, const char *outbound_proxy,
							const char *buf);

/**
 * Configure amsip to send a SIP MESSAGE (SIMPLE rfc)
 * with text data.
 *
 * The body must be text/plain.
 * If you want to set other content, put NULL
 * and then use am_message_set_body to specify
 * your own content.
 *
 * This message must then be sent using "ams_message_send".
 *
 * @param message            Message pointer to build.
 * @param identity           Set your SIP identity.
 * @param url                Set the target SIP identity.
 * @param proxy              Set the proxy server.
 * @param outbound_proxy     DO NOT USE (use proxy instead)
 * @param buf                Set the body data.
 */
	PPL_DECLARE (int)
	 ams_message_build(osip_message_t ** message,
					   const char *identity, const char *url,
					   const char *proxy, const char *outbound_proxy,
					   const char *buf);

/**
 * This method send a any previously built SIP message.
 *
 * @param message The message to send.
 */
	PPL_DECLARE (int) ams_message_send(osip_message_t * message);

/**
 * Configure amsip to send a SIP message with text data.
 *
 * This method buil and send "MESSAGE" (SIMPLE rfc) with
 * text/plain content-type.
 *
 * @param identity           Set your SIP identity
 * @param url                Set the target SIP identity
 * @param proxy              Set the proxy server
 * @param outbound_proxy     DO NOT USE (use proxy instead)
 * @param buf                Set the body data.
 */
	PPL_DECLARE (int) am_message_send(const char *identity,
									  const char *url, const char *proxy,
									  const char *outbound_proxy,
									  const char *buf);

/**
 * Configure amsip to answer any SIP request.
 *
 * @param tid                id of transaction.
 * @param code               Code to answer.
 */
	PPL_DECLARE (int) am_message_answer(int tid, int code);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
