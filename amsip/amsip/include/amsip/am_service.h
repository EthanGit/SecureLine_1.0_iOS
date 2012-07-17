/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_SERVICE_H__
#define __AM_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>

/**
 * @file am_service.h
 * @brief amsip service API
 *
 * This file provide the API needed to start a few services.
 *
 * <ul>
 * <li>call pickup</li>
 * </ul>
 *
 */

/**
 * @defgroup amsip_service amsip service interface
 * @ingroup amsip_message
 * @{
 */

/**
 * Intercept a call by using SUBSCRIBE/NOTIFY on dialog followed by INVITE/Replaces.
 *
 * @param identity           Caller SIP identity
 * @param url                Callee SIP identity
 * @param proxy              Set the proxy server
 */
	PPL_DECLARE (int) am_service_pickup(const char *identity,
										const char *url,
										const char *proxy);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
