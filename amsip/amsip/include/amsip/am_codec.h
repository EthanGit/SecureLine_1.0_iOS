/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_CODEC_H__
#define __AM_CODEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>
	
#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>

/**
 * @file am_codec.h
 * @brief amsip codec API
 *
 * This file provide the API needed to manage codec.
 *
 */

/**
 * @defgroup amsip_codec amsip codec interface
 * @ingroup amsip_internal
 * @{
 */

	/**
    * Structure for audio codec configuration.
    * @var am_codec_info_t
    */
	typedef struct am_codec_info am_codec_info_t;

	/**
    * Structure for audio codec configuration.
    * @struct am_codec_info
    */
	struct am_codec_info {
		int enable;
		char name[64];
		int payload;
		int freq;
		int mode;
		int vbr;
		int cng;
		int vad;
		int dtx;
	};

	/**
    * Structure for audio codec attribute configuration.
    * @var am_codec_attr_t
    */
	typedef struct am_codec_attr am_codec_attr_t;

	/**
    * Structure for audio codec attribute configuration.
    * @struct am_codec_attr
    */
	struct am_codec_attr {
		int ptime;
		int maxptime;
		int bandwidth;
	};

	/**
    * Modify codec list at index.
    *
    * @param codec              pointer to codec information.
    * @param index              index in support list.
    */
	PPL_DECLARE (int) am_codec_info_modify(am_codec_info_t * codec,
										   int index);

	/**
    * Modify codec attribute configuration.
    *
    * @param codec_attr              pointer to codec attribute information.
    */
	PPL_DECLARE (int) am_codec_attr_modify(am_codec_attr_t * codec_attr);

	/**
    * Get index of codec.
    *
    * @param name              name of codec to search.
    */
	int _am_codec_get_definition(char *name);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
