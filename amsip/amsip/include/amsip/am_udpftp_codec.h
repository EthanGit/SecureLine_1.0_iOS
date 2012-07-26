/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#ifndef __AM_UDPFTP_CODEC_H__
#define __AM_UDPFTP_CODEC_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include <amsip/am_version.h>
#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>

/**
 * @file am_udpftp_codec.h
 * @brief amsip codec API
 *
 * This file provide the API needed to manage udpftp codec.
 *
 */

/**
 * @defgroup amsip_codec amsip udpftp codec interface
 * @ingroup amsip_internal
 * @{
 */

	/**
    * Structure for udpftp codec configuration.
    * @var am_udpftp_codec_info_t
    */
	typedef struct am_udpftp_codec_info am_udpftp_codec_info_t;

	/**
    * Structure for udpftp codec configuration.
    * @struct am_udpftp_codec_info
    */
	struct am_udpftp_codec_info {
		int enable;
		char name[64];
		int payload;
		int freq;
	};

	/**
    * Structure for udpftp codec attribute configuration.
    * @var am_udpftp_codec_attr_t
    */
	typedef struct am_udpftp_codec_attr am_udpftp_codec_attr_t;

	/**
    * Structure for udpftp codec attribute configuration.
    * @struct am_udpftp_codec_attr
    */
	struct am_udpftp_codec_attr {
		int ptime;
		int maxptime;
		int upload_bandwidth;
		int download_bandwidth;
	};


	/**
    * Modify udpftp codec list at index.
    *
    * @param codec              pointer to udpftp codec information.
    * @param index              index in support list.
    */
	PPL_DECLARE (int) am_udpftp_codec_info_modify(am_udpftp_codec_info_t *
												  codec, int index);

	/**
    * Modify udpftp codec attribute configuration.
    *
    * @param codec_attr              pointer to udpftp codec attribute information.
    */
	PPL_DECLARE (int) am_udpftp_codec_attr_modify(am_udpftp_codec_attr_t *
												  codec_attr);

	/**
    * Get index of udpftp codec.
    *
    * @param name              name of udpftp codec to search.
    */
	int _am_udpftp_codec_get_definition(char *name);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
