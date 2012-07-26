/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_SUBSCRIBE_H__
#define __AM_SUBSCRIBE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>

/**
 * @file am_subscribe.h
 * @brief amsip outgoing subscription API
 *
 * This file provide the API needed to control outgoing subscription.
 *
 * <ul>
 * <li>send SUBSCRIBE</li>
 * <li>send SUBSCRIBE refresh</li>
 * <li>answer NOTIFY</li>
 * <li>send SUBSCRIBE to close subscription</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_subscription amsip subscription interface
 * @ingroup amsip_message
 * @{
 */

/**
 * Configure amsip to start a SIP subscription.
 *
 * @param identity  SIP url for caller.
 * @param url       SIP url for callee.
 * @param proxy     proxy header for SUBSCRIBE.
 * @param event     Event header for SUBSCRIBE.
 * @param accept    Accept header.
 * @param expires   Expires header for SUBSCRIBE.
 */
	PPL_DECLARE (int) am_subscription_start(const char *identity,
											const char *url,
											const char *proxy,
											const char *event,
											const char *accept,
											int expires);
/**
 * Build and send a SUBSCRIBE to refresh a subscription.
 *
 * @param did                Session identifier.
 * @param event              Event header for SUBSCRIBE.
 * @param accept             Accept header.
 * @param expires            expires time (0 to stop subscription).
*/
	PPL_DECLARE (int) am_subscription_refresh(int did, const char *event,
											  const char *accept,
											  int expires);

/**
 * Build a SUBSCRIBE to refresh a subscription.
 *
 * @param did                Session identifier.
 * @param event              Event header for SUBSCRIBE.
 * @param accept             Accept header.
 * @param expires            expires time (0 to stop subscription).
 * @param subscribe          pointer will contains a prepared SUBSCRIBE request.
*/
	PPL_DECLARE (int) am_subscription_build_refresh(int did, const char *event,
													const char *accept,
													int expires, osip_message_t **subscribe);

/**
 * Send a SUBSCRIBE to refresh a subscription.
 * 
 * @param did          identifier of the subscription.
 * @param subscribe    SUBSCRIBE request to be sent.
 */
	PPL_DECLARE (int) am_subscription_send_refresh(int did, osip_message_t *subscribe);

/**
 * Remove outgoing subscription context.
 * 
 * @param did          identifier of the subscription.
 */
	PPL_DECLARE (int) am_subscription_remove(int did);

/** @} */

/**
 * @defgroup amsip_insubscription amsip incoming subscription interface
 * @ingroup amsip_message
 * @{
 */

/**
 * Configure amsip to answer a SIP SUBSCRIBE.
 *
 * @param tid                Transaction identifier.
 * @param did                Session identifier.
 * @param code               Code to use.
 */
	PPL_DECLARE (int) am_insubscription_answer(int tid, int did, int code);

/**
 * Configure amsip to build a SIP NOTIFY answer.
 *
 * @param tid        transaction identifier.
 * @param code       answer code for NOTIFY answer.
 * @param answer     answer pointer.
 */
	PPL_DECLARE (int) am_insubscription_build_answer(int tid, int code,
													 osip_message_t **
													 answer);
/**
 * Configure amsip to send a SIP NOTIFY answer.
 *
 * @param tid        transaction identifier.
 * @param code       answer code for NOTIFY answer.
 * @param answer     answer pointer.
 */
	PPL_DECLARE (int) am_insubscription_send_answer(int tid, int code,
													osip_message_t *
													answer);

/**
 * Configure amsip to send a SIP NOTIFY request.
 *
 * @param did                 Session identifier.
 * @param subscription_status subscription status value.
 * @param subscription_reason subscription reason value.
 * @param notify              request pointer.
 */
	PPL_DECLARE (int) am_insubscription_build_notify(int did,
													 int
													 subscription_status,
													 int
													 subscription_reason,
													 osip_message_t **
													 notify);

/**
 * Configure amsip to send a SIP NOTIFY request.
 *
 * @param did        Session identifier.
 * @param request    request pointer.
 */
	PPL_DECLARE (int) am_insubscription_send_request(int did,
													 osip_message_t *
													 request);

/**
 * Remove outgoing subscription context.
 * 
 * @param did          identifier of the subscription.
 */
	PPL_DECLARE (int) am_insubscription_remove(int did);

/** @} */


#ifdef __cplusplus
}
#endif
#endif
