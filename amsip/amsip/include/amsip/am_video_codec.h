/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_VIDEO_CODEC_H__
#define __AM_VIDEO_CODEC_H__


#ifdef ENABLE_VIDEO

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>

/**
 * @file am_video_codec.h
 * @brief amsip codec API
 *
 * This file provide the API needed to manage video codec.
 *
 */

/**
 * @defgroup amsip_codec amsip video codec interface
 * @ingroup amsip_internal
 * @{
 */

	/**
    * Structure for video codec configuration.
    * @var am_video_codec_info_t
    */
	typedef struct am_video_codec_info am_video_codec_info_t;

	/**
    * Structure for video codec configuration.
    * @struct am_video_codec_info
    */
	struct am_video_codec_info {
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
    * Structure for video codec attribute configuration.
    * @var am_video_codec_attr_t
    */
	typedef struct am_video_codec_attr am_video_codec_attr_t;

	/**
    * Structure for video codec attribute configuration.
    * @struct am_video_codec_attr
    */
	struct am_video_codec_attr {
		int ptime;
		int maxptime;
		int upload_bandwidth;
		int download_bandwidth;
	};

	/**
    * Modify video codec list at index.
    *
    * @param codec              pointer to video codec information.
    * @param index              index in support list.
    */
	PPL_DECLARE (int) am_video_codec_info_modify(am_video_codec_info_t *
												 codec, int index);

	/**
    * Modify video codec attribute configuration.
    *
    * @param codec_attr              pointer to video codec attribute information.
    */
	PPL_DECLARE (int) am_video_codec_attr_modify(am_video_codec_attr_t *
												 codec_attr);

	/**
    * Get index of video codec.
    *
    * @param name              name of video codec to search.
    */
	int _am_video_codec_get_definition(char *name);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
#endif
