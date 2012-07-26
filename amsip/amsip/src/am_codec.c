/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"
#include "amsip/am_codec.h"

#ifndef WIN32
#include <unistd.h>
#endif

#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>

PPL_DECLARE (int) am_codec_info_modify(am_codec_info_t * codec, int i)
{
	if (codec == NULL)
		return AMSIP_BADPARAMETER;
	if (i > 4 || i < 0) {
		return AMSIP_BADPARAMETER;
	}
	if (codec->name[0] == '\0') {
		codec->enable = 0;
	}
	if (codec->enable > 0) {
		/* verify codec availability */
		if (!ms_filter_codec_supported(codec->name)) {
			codec->enable = 0;	/* disable */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
								  "No decoder/encoder plugin available for %s\n",
								  codec->name));
			memcpy(&_antisipc.codecs[i], codec, sizeof(am_codec_info_t));
			return AMSIP_NOTFOUND;
		}
	}
	memcpy(&_antisipc.codecs[i], codec, sizeof(am_codec_info_t));
	return AMSIP_SUCCESS;
}

PPL_DECLARE (int) am_codec_attr_modify(am_codec_attr_t * codec_attr)
{
	if (codec_attr == NULL)
		return AMSIP_BADPARAMETER;
	memcpy(&_antisipc.codec_attr, codec_attr, sizeof(am_codec_attr_t));
	return AMSIP_SUCCESS;
}


int _am_codec_get_definition(char *name)
{
	int i;

	for (i = 0; i < 5; i++) {
		if (osip_strcasecmp(_antisipc.codecs[i].name, name) == 0) {
			if (!ms_filter_codec_supported(name)) {
				/* in case we don't support it, we disable it */
				_antisipc.codecs[i].enable = 0;
			}
			return i;
		}
	}
	return AMSIP_NOTFOUND;
}
