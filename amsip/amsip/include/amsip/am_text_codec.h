/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_TEXT_CODEC_H__
#define __AM_TEXT_CODEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <amsip/am_version.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>

/**
 * @file am_text_codec.h
 * @brief amsip codec API
 *
 * This file provide the API needed to manage text codec.
 *
 */

/**
 * @defgroup amsip_codec amsip text codec interface
 * @ingroup amsip_internal
 * @{
 */

	/**
    * Structure for text codec configuration.
    * @var am_text_codec_info_t
    */
	typedef struct am_text_codec_info am_text_codec_info_t;

	/**
    * Structure for text codec configuration.
    * @struct am_text_codec_info
    */
	struct am_text_codec_info {
		int enable;
		char name[64];
		int payload;
		int freq;
	};

	/**
    * Structure for text codec attribute configuration.
    * @var am_text_codec_attr_t
    */
	typedef struct am_text_codec_attr am_text_codec_attr_t;

	/**
    * Structure for text codec attribute configuration.
    * @struct am_text_codec_attr
    */
	struct am_text_codec_attr {
		int ptime;
		int maxptime;
		int upload_bandwidth;
		int download_bandwidth;
	};


	/**
    * Modify text codec list at index.
    *
    * @param codec              pointer to text codec information.
    * @param index              index in support list.
    */
	PPL_DECLARE (int) am_text_codec_info_modify(am_text_codec_info_t *
												codec, int index);

	/**
    * Modify text codec attribute configuration.
    *
    * @param codec_attr              pointer to text codec attribute information.
    */
	PPL_DECLARE (int) am_text_codec_attr_modify(am_text_codec_attr_t *
												codec_attr);

	/**
    * Get index of text codec.
    *
    * @param name              name of text codec to search.
    */
	int _am_text_codec_get_definition(char *name);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
