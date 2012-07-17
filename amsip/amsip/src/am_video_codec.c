/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
    Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>

*/

#include "amsip/am_options.h"

#ifdef ENABLE_VIDEO

#include "amsip/am_video_codec.h"

#ifndef WIN32
#include <unistd.h>
#endif

#include <osipparser2/osip_port.h>
#include <osipparser2/sdp_message.h>

PPL_DECLARE (int)
am_video_codec_info_modify(am_video_codec_info_t * codec, int i)
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
			memcpy(&_antisipc.video_codecs[i], codec,
				   sizeof(am_video_codec_info_t));
			return AMSIP_NOTFOUND;
		}
	}
	memcpy(&_antisipc.video_codecs[i], codec,
		   sizeof(am_video_codec_info_t));
	return AMSIP_SUCCESS;
}

PPL_DECLARE (int)
am_video_codec_attr_modify(am_video_codec_attr_t * codec_attr)
{
	if (codec_attr == NULL)
		return AMSIP_BADPARAMETER;
	memcpy(&_antisipc.video_codec_attr, codec_attr,
		   sizeof(am_video_codec_attr_t));
	if (_antisipc.video_codec_attr.download_bandwidth <= 0)
		_antisipc.video_codec_attr.download_bandwidth = 256;
	return 0;
}


int _am_video_codec_get_definition(char *name)
{
	int i;

	for (i = 0; i < 5; i++) {
		if (osip_strcasecmp(_antisipc.video_codecs[i].name, name) == 0)
			return i;
	}
	return AMSIP_NOTFOUND;
}

#endif
