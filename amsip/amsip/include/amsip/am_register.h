/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_REGISTER_H__
#define __AM_REGISTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>

/**
 * @file am_register.h
 * @brief amsip registration API
 *
 * This file provide the API needed to register and unregister to a SIP proxy.
 *
 * <ul>
 * <li>send REGISTER message.</li>
 * <li>send REGISTER with Expires=0 message (unregister).</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_register amsip register interface
 * @ingroup amsip_message
 * @{
 */

/**
 * Configure amsip to register to a SIP server.
 *
 * @param identity           Provide your SIP identity
 * @param proxy              Set the proxy server
 * @param refresh_interval   Give the refresh rate
 * @param expires_interval   Provide the Expires header
 */
	PPL_DECLARE (int) am_register_start(const char *identity,
										const char *proxy,
										int refresh_interval,
										int expires_interval);

/**
 * Configure amsip to register to a SIP server.
 *
 * @param identity            Provide your SIP identity
 * @param proxy               Set the proxy server
 * @param refresh_interval    Give the refresh rate
 * @param expires_interval    Provide the Expires header
 * @param contact_param_name  Name of an additionnal contact parameter
 * @param contact_param_value value of an additionnal contact parameter
 */
	PPL_DECLARE (int) am_register_start_with_parameter(const char *identity, const char *proxy,
		int refresh_interval, int expires_interval,
		char *contact_param_name, char *contact_param_value);

/**
 * Configure amsip to refresh register on a SIP server.
 *
 * @param rid                Previous registration id.
 * @param expires            New expiration value for contact.
 */
	PPL_DECLARE (int) am_register_refresh(int rid, int expires);

/**
 * Configure amsip to unregister to a SIP server.
 *
 * @param rid           identifier for registration
 */
	PPL_DECLARE (int) am_register_stop(int rid);

/**
 * Ask amsip to delete current registration context.
 *
 * @param rid           identifier for registration
 */
	PPL_DECLARE (int) am_register_remove(int rid);

/**
 * Configure amsip to unregister all previous IP from server.
 *
 * @param identity           Provide your SIP identity
 * @param proxy              Set the proxy server
 */
	PPL_DECLARE (int) am_register_send_star(const char *identity,
											const char *proxy);
/** @} */

#ifdef __cplusplus
}
#endif
#endif
