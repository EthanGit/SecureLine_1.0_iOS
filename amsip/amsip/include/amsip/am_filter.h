/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#ifndef __AM_FILTER_H__
#define __AM_FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>
#include <mediastreamer2/msfilter.h>

/**
 * @file am_filter.h
 * @brief amsip internal filters API
 *
 * This file provide the API for internal amsip filters.
 *
 */

/**
 * @defgroup amsip_filter amsip filter interface
 * @ingroup amsip_internal
 * @{
 */

	typedef struct MST140Block {
		int size;
		char *utf8_string;
	} MST140Block;

#define MS_FILTER_TEXT_GET  MS_FILTER_BASE_METHOD(150,MST140Block*)
#define MS_FILTER_TEXT_SEND MS_FILTER_BASE_METHOD(151,MST140Block*)

/**
 * Replace the MSRTP filters.
 *
 * @param rtprecv        rtpfilter description.
 * @param rtpsend        rtpfilter description.
 */
	PPL_DECLARE (void) am_filter_set_rtpfilter(MSFilterDesc * rtprecv,
											   MSFilterDesc * rtpsend);

/**
 * Replace the Sound Card Drivers.
 *
 * @param snd_driver     snd driver filter.
 */
	PPL_DECLARE (void) am_filter_set_sounddriver(MSSndCardDesc *
												 snd_driver);

/**
 * build a RTP_RECV filter.
 *
 */
	MSFilter *am_filter_new_rtprecv(void);

/**
 * build a RTP_SEND filter.
 *
 */
	MSFilter *am_filter_new_rtpsend(void);

/**
 * Register internal filters.
 *
 */
	void am_filter_register(void);

/**
 * build a dispatcher filter.
 *
 */
	MSFilter *am_filter_new_dispatcher(void);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
