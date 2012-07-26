/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#ifdef ENABLE_VIDEO

#include "am_calls.h"

#include <ortp/ortp.h>
#include <ortp/telephonyevents.h>


#ifndef DISABLE_SRTP
#include <ortp/srtp.h>
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if defined (_WIN32_WCE)
#define DISABLE_STITCHER
#endif

#if TARGET_OS_IPHONE
#define DISABLE_CAMERA_IN_OUTPUT
#ifndef DISABLE_STITCHER
#define DISABLE_STITCHER
#endif
#define DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
#endif

#define DISABLE_MSDRAW

#include "mediastreamer2/dtmfgen.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilerec.h"

#ifndef DISABLE_VOLUME
#include "mediastreamer2/msvolume.h"
#endif
#ifndef DISABLE_EQUALIZER
#include "mediastreamer2/msequalizer.h"
#endif
#ifndef DISABLE_MSDRAW
#include "mediastreamer2/msdraw.h"
#endif

#include <osip2/osip_time.h>
#include <osip2/osip_mt.h>

#include "sdptools.h"

#ifdef EXOSIP4
#include "amsip-internal.h"
#endif

#define KEEP_RTP_SESSION

struct am_video_info {
	RtpSession *video_rtp_session;
	OrtpEvQueue *video_rtp_queue;

	RtpProfile *video_rtp_profile;
	MSFilter *video_sourcesizeconv;
	MSFilter *video_encoder;
	int video_encoder_bitrate;
	MSFilter *video_decoder;
	MSFilter *video_rtprecv;
	MSFilter *video_rtpsend;
	MSFilter *video_ice;
};

struct video_module_ctx {

	int video_device;
	char video_source_name[256];
	char nowebcam_image[256];
	MSVideoSize prefered_input_size;
	MSVideoSize prefered_screen_size;

	float option_webcam_fps;
	float option_force_encoder_fps;

	MSTicker *video_ticker;
	MSFilter *video_tee;
	MSFilter *video_void;
	MSFilter *video_source;
	MSFilter *video_sourcepixconv;
	MSFilter *video_stitcher;
	MSFilter *video_output;
	MSFilter *video_selfview_tee;
	MSDisplay *video_display;
	on_video_module_new_image_cb on_new_image_cb;
	struct am_video_info ctx[MAX_NUMBER_OF_CALLS];
#ifndef DISABLE_MSDRAW
	MSFilter *video_msdraw;
#endif
};

struct video_module_ctx video_ctx = {

	0,
	"",
	"",
	{ MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H },
	{ MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H },

	15.0f,
	-1.0f,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

#ifndef DISABLE_SRTP
int
am_get_security_descriptions(RtpSession * rtp,
							 sdp_message_t * sdp_answer,
							 sdp_message_t * sdp_offer,
							 sdp_message_t * sdp_local,
							 sdp_message_t * sdp_remote, char *media_type);
#endif

static int video_module_init(const char *name, int debug_level);
static int video_module_reset(const char *name, int debug_level);
static int video_module_quit();
static int video_module_find_camera(struct am_camera *camera);
static int video_module_select_camera(int card);
static int video_module_set_input_video_size(int width, int height);
static int video_module_set_window_display(MSDisplayDesc *desc, long handle, int width, int height);
static int video_module_set_window_handle(long handle, int width, int height);
static int video_module_set_window_preview_handle(long handle, int width, int height);
static int video_module_set_nowebcam(const char *nowebcam_image);
static int _video_module_open_camera();
static int video_module_enable_preview(int enable);
static void _on_video_module_new_image(void *userdata , unsigned int id, void *arg);
static int video_module_set_image_callback(on_video_module_new_image_cb func);

static int video_module_set_selfview_mode(int mode);
static int video_module_set_selfview_position(float posx, float posy, float size);
static int video_module_get_selfview_position(float *posx, float *posy, float *size);
static int video_module_set_selfview_scalefactor(float scalefactor);
static int video_module_set_background_color(int red, int green, int blue);
static int video_module_set_video_option(int opt, void *arg);
static int video_module_set_callback(unsigned int id,
						  MSFilterNotifyFunc speex_pp_process,
						  void *userdata);
static int video_module_session_release(am_call_t * ca);
static int video_module_session_close(am_call_t * ca);
static int video_module_session_start(am_call_t * ca, sdp_message_t * sdp_answer,
						 sdp_message_t * sdp_offer,
						 sdp_message_t * sdp_local,
						 sdp_message_t * sdp_remote, int local_port,
						 char *remote_ip, int remote_port,
						 int setup_passive);
static int video_module_session_get_bandwidth_statistics(am_call_t * ca,
											  struct am_bandwidth_stats
											  *band_stats);
static OrtpEvent *video_module_session_get_video_rtp_events(am_call_t * ca);
static int video_module_session_send_vfu(am_call_t * ca);
static int video_module_session_adapt_video_bitrate(am_call_t * ca, float lost);

#if TARGET_OS_IPHONE
int iphone_add_selfview_image(int width, int height, int format, mblk_t *data);

int iphone_add_selfview_image(int width, int height, int format, mblk_t *data)
{
	eXosip_lock();
	
	if (video_ctx.video_source == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video source\n"));
		return AMSIP_NOTFOUND;
	}
    

	MSVideoSize vsize;
	vsize.width=width;
	vsize.height=height;
	ms_filter_call_method(video_ctx.video_source, MS_FILTER_SET_PIX_FMT, &format);
	ms_filter_call_method(video_ctx.video_source, MS_FILTER_SET_VIDEO_SIZE, &vsize);
	ms_filter_call_method(video_ctx.video_source, MS_FILTER_SET_DATA, data);
	ms_filter_call_method(video_ctx.video_sourcepixconv, MS_FILTER_SET_PIX_FMT, &format);
	ms_filter_call_method(video_ctx.video_sourcepixconv, MS_FILTER_SET_VIDEO_SIZE, &vsize);
    
	eXosip_unlock();
	return 0;
}
#endif

static int video_module_init(const char *name, int debug_level)
{
	memset(&video_ctx, 0, sizeof(video_ctx));
	_antisipc.video_dscp = 0x28;

	video_ctx.video_device = 0;
	memset(video_ctx.video_source_name, 0,
		   sizeof(video_ctx.video_source_name));
	video_ctx.prefered_screen_size.width = MS_VIDEO_SIZE_CIF_W;
	video_ctx.prefered_screen_size.height = MS_VIDEO_SIZE_CIF_H;
	video_ctx.prefered_input_size.width = MS_VIDEO_SIZE_CIF_W;
	video_ctx.prefered_input_size.height = MS_VIDEO_SIZE_CIF_H;
	video_ctx.option_webcam_fps = 15.0f;
	video_ctx.option_force_encoder_fps = -1.0f;

	return 0;
}

static int video_module_reset(const char *name, int debug_level)
{
	MSWebCamManager *wcm;
	memset(&video_ctx, 0, sizeof(video_ctx));
	_antisipc.video_dscp = 0x28;

	wcm = ms_web_cam_manager_get();
	if (wcm != NULL)
		ms_web_cam_manager_reload(wcm);

	video_ctx.video_device = 0;
	memset(video_ctx.video_source_name, 0,
		   sizeof(video_ctx.video_source_name));
	video_ctx.prefered_screen_size.width = MS_VIDEO_SIZE_CIF_W;
	video_ctx.prefered_screen_size.height = MS_VIDEO_SIZE_CIF_H;
	video_ctx.prefered_input_size.width = MS_VIDEO_SIZE_CIF_W;
	video_ctx.prefered_input_size.height = MS_VIDEO_SIZE_CIF_H;
	video_ctx.option_webcam_fps = 15.0f;
	video_ctx.option_force_encoder_fps = -1.0f;

	return 0;
}

static int video_module_quit()
{
	video_module_enable_preview(0);
	if (video_ctx.video_ticker != NULL)
		ms_ticker_destroy(video_ctx.video_ticker);
	video_ctx.video_ticker=NULL;

	video_module_set_window_display(NULL, 0, 0, 0);
	memset(&video_ctx, 0, sizeof(video_ctx));
	return 0;
}

static int video_module_find_camera(struct am_camera *camera)
{
	MSWebCam *device = NULL;
	int card = camera->card;
	const MSList *elem;
	int k = 0;

	if (card==0)
		ms_web_cam_manager_update(ms_web_cam_manager_get());
	elem = ms_web_cam_manager_get_list(ms_web_cam_manager_get());

	if (card > 20)
		return -1;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSWebCam *adevice = (MSWebCam *) elem->data;

		if (adevice->disconnected==1)
			continue;
		if (k == card) {
			device = adevice;
			ms_message("am_option_find_camera: found device = %s %s.",
					   adevice->desc->driver_type, adevice->name);
			break;
		}
		k++;
	}

	if (device == NULL)
		return -1;
	snprintf(camera->name, sizeof(camera->name), "%s", device->name);
	return 0;
}

static int video_module_select_camera(int card)
{
	MSWebCam *device = NULL;
	const MSList *elem =
		ms_web_cam_manager_get_list(ms_web_cam_manager_get());
	int k = 0;

	if (card < 0) {
		snprintf(video_ctx.video_source_name,
				 sizeof(video_ctx.video_source_name),
				 "StaticImage: Static picture");
		return 0;
	}

	for (; elem != NULL; elem = elem->next) {
		device = (MSWebCam *) elem->data;

		if (device->disconnected==1)
		{
			device = NULL;
			continue;
		}
		if (k == card) {
			ms_message("am_option_select_camera: found device = %s %s.",
					   device->desc->driver_type, device->name);
			break;
		}
		device = NULL;
		k++;
	}

	if (device != NULL)
		snprintf(video_ctx.video_source_name,
				 sizeof(video_ctx.video_source_name), "%s: %s",
				 device->desc->driver_type, device->name);
	else
		snprintf(video_ctx.video_source_name,
				 sizeof(video_ctx.video_source_name),
				 "StaticImage: Static picture");

	return 0;
}

static int video_module_set_input_video_size(int width, int height)
{
	if (width <= 0 || height <= 0) {
		video_ctx.prefered_input_size.width = MS_VIDEO_SIZE_CIF_W;
		video_ctx.prefered_input_size.height = MS_VIDEO_SIZE_CIF_H;
		return 0;
	}

	if ((MS_VIDEO_SIZE_CIF_W == width && MS_VIDEO_SIZE_CIF_H == height)
		|| (MS_VIDEO_SIZE_QCIF_W == width && MS_VIDEO_SIZE_QCIF_H == height)
		|| (MS_VIDEO_SIZE_VGA_W == width && MS_VIDEO_SIZE_VGA_H == height)
		|| (MS_VIDEO_SIZE_4CIF_W == width && MS_VIDEO_SIZE_4CIF_H == height)
		|| (MS_VIDEO_SIZE_720P_W == width && MS_VIDEO_SIZE_720P_H == height)) {
	} else {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "video_module_set_input_video_size: only QCIF/CIF/VGA/4CIF size are supported\r\n"));
		return -1;
	}
	video_ctx.prefered_input_size.width = width;
	video_ctx.prefered_input_size.height = height;
	return 0;
}

static int video_module_set_window_display(MSDisplayDesc *desc, long handle, int width, int height)
{
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_stitcher != NULL
		&& video_ctx.video_source != NULL
		&& video_ctx.video_sourcepixconv != NULL
		&& video_ctx.video_tee != NULL
		&& video_ctx.video_void != NULL
		&& video_ctx.video_output != NULL
		&& video_ctx.video_selfview_tee != NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Cannot set handle: please disable preview first\n"));
		return AMSIP_WRONG_STATE;
	}

	if (width == 0) {
		width = MS_VIDEO_SIZE_QCIF_W;
		height = MS_VIDEO_SIZE_QCIF_H;
	}

	if (video_ctx.video_display != NULL) {
		ms_display_destroy(video_ctx.video_display);
		video_ctx.video_display = NULL;
	}

	video_ctx.prefered_screen_size.width = width;
	video_ctx.prefered_screen_size.height = height;

	if (handle == 0)
		return 0;

	if (desc!=NULL)
	{
		video_ctx.video_display = ms_display_new(desc);
		if (video_ctx.video_display == NULL)
			return -1;
		ms_display_set_window_id(video_ctx.video_display, (long) handle);
	}
	else
	{
#if defined(WIN32) || defined(__APPLE__)
		MSDisplayDesc *desc = ms_display_desc_get_default();
		video_ctx.video_display = ms_display_new(desc);
		if (video_ctx.video_display == NULL)
			return -1;
		ms_display_set_window_id(video_ctx.video_display, (long) handle);
#endif
	}

	return 0;
}

static int video_module_set_window_handle(long handle, int width, int height)
{
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_stitcher != NULL
		&& video_ctx.video_source != NULL
		&& video_ctx.video_sourcepixconv != NULL
		&& video_ctx.video_tee != NULL
		&& video_ctx.video_void != NULL
		&& video_ctx.video_output != NULL
		&& video_ctx.video_selfview_tee != NULL)
	{
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "Cannot set handle: please disable preview first\n"));
		return AMSIP_WRONG_STATE;
	}

	if (width == 0) {
		width = MS_VIDEO_SIZE_QCIF_W;
		height = MS_VIDEO_SIZE_QCIF_H;
	}

	if (video_ctx.video_display != NULL) {
		ms_display_destroy(video_ctx.video_display);
		video_ctx.video_display = NULL;
	}
	video_ctx.prefered_screen_size.width = width;
	video_ctx.prefered_screen_size.height = height;

	if (handle == 0)
		return 0;

	if (video_ctx.video_display == NULL) {
#if defined(WIN32) || defined(__APPLE__)
		MSDisplayDesc *desc = ms_display_desc_get_default();
		video_ctx.video_display = ms_display_new(desc);
		if (video_ctx.video_display == NULL)
			return -1;
		ms_display_set_window_id(video_ctx.video_display, (long) handle);
#endif
		return 0;
	}
	return -1;
}

static int video_module_set_window_preview_handle(long handle, int width, int height)
{
	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							"video_module_set_window_preview_handle: not implemented\n"));
	return AMSIP_UNDEFINED_ERROR;
}

static int video_module_set_nowebcam(const char *nowebcam_image)
{
	if (nowebcam_image == NULL) {
		memset(video_ctx.nowebcam_image, 0,
			   sizeof(video_ctx.nowebcam_image));
		return 0;
	}

	snprintf(video_ctx.nowebcam_image, sizeof(video_ctx.nowebcam_image),
			 "%s", nowebcam_image);
	return 0;
}

static int _video_module_open_camera()
{
	MSWebCam *device = NULL;
	const MSList *elem =
		ms_web_cam_manager_get_list(ms_web_cam_manager_get());

	if (video_ctx.video_source != NULL)
		return -1;				/* already started */

	if (video_ctx.video_source_name[0] == '\0') {
		/* open defaut device */
		device = (MSWebCam *) elem->data;
		if (device == NULL)
			return -1;
		video_ctx.video_source = ms_web_cam_create_reader(device);
		if (video_ctx.video_source == NULL)
			return -1;

		return 0;
	}

	for (; elem != NULL; elem = elem->next) {
		char video_source_name[256];
		memset(video_source_name, 0, sizeof(video_source_name));
		device = (MSWebCam *) elem->data;

		snprintf(video_source_name, sizeof(video_source_name), "%s: %s",
				 device->desc->driver_type, device->name);
		if (osip_strcasecmp(video_source_name, video_ctx.video_source_name)
			== 0) {
			ms_message("am_option_open_camera: using device = %s %s.",
					   device->desc->driver_type, device->name);
			break;
		}
		device = NULL;
	}

	if (device == NULL) {
		char temp_video_source_name[256];
		snprintf(temp_video_source_name, sizeof(temp_video_source_name),
				 "StaticImage: Static picture");
		elem = ms_web_cam_manager_get_list(ms_web_cam_manager_get());
		for (; elem != NULL; elem = elem->next) {
			char video_source_name[256];
			memset(video_source_name, 0, sizeof(video_source_name));
			device = (MSWebCam *) elem->data;

			snprintf(video_source_name, sizeof(video_source_name),
					 "%s: %s", device->desc->driver_type, device->name);
			if (osip_strcasecmp(video_source_name, temp_video_source_name)
				== 0) {
				ms_message
					("am_option_open_camera: using static image device = %s %s.",
					 device->desc->driver_type, device->name);
				break;
			}
			device = NULL;
		}
	}

	if (device == NULL)
		return -1;

	video_ctx.video_source = ms_web_cam_create_reader(device);
	if (video_ctx.video_source == NULL)
		return -1;
	return 0;
}

static int video_module_enable_preview(int enable)
{
	MSPixFmt format;
	MSVideoSize vsize;

	/* close preview */
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_stitcher != NULL
		&& video_ctx.video_source != NULL
		&& video_ctx.video_sourcepixconv != NULL
		&& video_ctx.video_tee != NULL
		&& video_ctx.video_void != NULL
		&& video_ctx.video_output != NULL
		&& video_ctx.video_selfview_tee != NULL) {

		ms_ticker_detach(video_ctx.video_ticker,
						 video_ctx.video_source);
		/* detach all pending calls */
		{
			int k;
            int max = STITCHER_MAX_INPUTS;
            if (max > MAX_NUMBER_OF_CALLS)
                max = MAX_NUMBER_OF_CALLS;

			/* search index of elements */
			for (k = 0; k < max - 1; k++) {
				struct am_video_info *tmpinfo = _antisipc.calls[k].video_ctx;
				if (tmpinfo==NULL)
				{}
				else if (tmpinfo->video_rtprecv != NULL && tmpinfo->video_rtpsend != NULL
					&& tmpinfo->video_encoder != NULL
					&& tmpinfo->video_decoder != NULL) {
					//AMD fix 22/02/2012
					if (tmpinfo->video_ice != NULL)
						ms_ticker_detach(video_ctx.video_ticker,
														 tmpinfo->video_ice);
#ifdef DISABLE_STITCHER
                    ms_ticker_detach(video_ctx.video_ticker, tmpinfo->video_rtprecv);
#endif
					ms_filter_unlink(tmpinfo->video_rtprecv, 0,
									 tmpinfo->video_decoder, 0);
					ms_filter_unlink(tmpinfo->video_decoder, 0,
									 video_ctx.video_stitcher, k + 1);

#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
					ms_filter_unlink(video_ctx.video_tee, k,
									 tmpinfo->video_encoder, 0);
#else
					ms_filter_unlink(video_ctx.video_tee, k,
									 tmpinfo->video_sourcesizeconv, 0);
					ms_filter_unlink(tmpinfo->video_sourcesizeconv, 0,
									 tmpinfo->video_encoder, 0);
#endif
					ms_filter_unlink(tmpinfo->video_encoder, 0,
									 tmpinfo->video_rtpsend, 0);
				} else if (tmpinfo->video_rtprecv != NULL
						   && tmpinfo->video_decoder != NULL) {
					//AMD fix 22/02/2012
					if (tmpinfo->video_ice != NULL)
						ms_ticker_detach(video_ctx.video_ticker,
														 tmpinfo->video_ice);
#ifdef DISABLE_STITCHER
                    ms_ticker_detach(video_ctx.video_ticker, tmpinfo->video_rtprecv);
#endif
					ms_filter_unlink(tmpinfo->video_rtprecv, 0,
									 tmpinfo->video_decoder, 0);
					ms_filter_unlink(tmpinfo->video_decoder, 0,
									 video_ctx.video_stitcher, k + 1);
				} else if (tmpinfo->video_rtpsend != NULL
						   && tmpinfo->video_encoder != NULL) {
					//AMD fix 22/02/2012
					if (tmpinfo->video_ice != NULL)
						ms_ticker_detach(video_ctx.video_ticker,
														 tmpinfo->video_ice);
#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
					ms_filter_unlink(video_ctx.video_tee, k,
									 tmpinfo->video_encoder, 0);
#else
					ms_filter_unlink(video_ctx.video_tee, k,
									 tmpinfo->video_sourcesizeconv, 0);
					ms_filter_unlink(tmpinfo->video_sourcesizeconv, 0,
									 tmpinfo->video_encoder, 0);
#endif
					ms_filter_unlink(tmpinfo->video_encoder, 0,
									 tmpinfo->video_rtpsend, 0);
				}
			}
		}


		ms_filter_unlink(video_ctx.video_source, 0,
						 video_ctx.video_sourcepixconv, 0);
#ifdef DISABLE_STITCHER
		ms_filter_unlink(video_ctx.video_sourcepixconv, 0,
						 video_ctx.video_tee, 0);
#else
		ms_filter_unlink(video_ctx.video_sourcepixconv, 0,
						 video_ctx.video_selfview_tee, 0);
		ms_filter_unlink(video_ctx.video_selfview_tee, 0,
						 video_ctx.video_stitcher, 0);
#endif
#ifndef DISABLE_CAMERA_IN_OUTPUT
		ms_filter_unlink(video_ctx.video_selfview_tee, 1,
						 video_ctx.video_output, 1);
#endif
#ifndef DISABLE_MSDRAW
		if (video_ctx.video_msdraw==NULL)
			ms_filter_unlink(video_ctx.video_stitcher, 0,
							 video_ctx.video_output, 0);
		else
		{
			ms_filter_unlink(video_ctx.video_stitcher, 0,
							 video_ctx.video_msdraw, 0);
			ms_filter_unlink(video_ctx.video_msdraw, 0,
							 video_ctx.video_output, 0);
		}
#else
		ms_filter_unlink(video_ctx.video_stitcher, 0,
						 video_ctx.video_output, 0);
#endif
#ifdef DISABLE_STITCHER
#else
		ms_filter_unlink(video_ctx.video_stitcher, 1,
						 video_ctx.video_tee, 0);
#endif
		ms_filter_unlink(video_ctx.video_tee, 9,
						 video_ctx.video_void, 0);

		if (enable <= 0) {
			if (video_ctx.video_stitcher != NULL)
				ms_filter_destroy(video_ctx.video_stitcher);
			if (video_ctx.video_tee != NULL)
				ms_filter_destroy(video_ctx.video_tee);
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);
		}
		if (video_ctx.video_source != NULL)
			ms_filter_destroy(video_ctx.video_source);
		if (video_ctx.video_sourcepixconv != NULL)
			ms_filter_destroy(video_ctx.video_sourcepixconv);
		if (video_ctx.video_output != NULL)
			ms_filter_destroy(video_ctx.video_output);
#ifndef DISABLE_MSDRAW
		if (video_ctx.video_msdraw != NULL)
			ms_filter_destroy(video_ctx.video_msdraw);
#endif
		if (video_ctx.video_selfview_tee != NULL)
			ms_filter_destroy(video_ctx.video_selfview_tee);

		if (enable <= 0) {
			video_ctx.video_stitcher = NULL;
			video_ctx.video_tee = NULL;
			video_ctx.video_void = NULL;
		}
		video_ctx.video_source = NULL;
		video_ctx.video_sourcepixconv = NULL;
		video_ctx.video_output = NULL;
#ifndef DISABLE_MSDRAW
		video_ctx.video_msdraw = NULL;
#endif
		video_ctx.video_selfview_tee = NULL;
	}

	if (enable <= 0) {
		return 0;
	}

	if (video_ctx.video_output == NULL) {
		if (video_ctx.video_stitcher == NULL) {
			video_ctx.video_stitcher =
				ms_filter_new_from_name("MSVideoStitcher");
		}
		if (video_ctx.video_stitcher == NULL) {
			ms_error("am_video_module.c: missing video stitcher plugin.");
			return -1;
		}
		if (video_ctx.video_selfview_tee == NULL)
			video_ctx.video_selfview_tee = ms_filter_new(MS_TEE_ID);

		if (video_ctx.video_tee == NULL)
			video_ctx.video_tee = ms_filter_new(MS_TEE_ID);

		if (video_ctx.video_void == NULL)
			video_ctx.video_void = ms_filter_new(MS_VOID_SINK_ID);

		if (video_ctx.video_ticker == NULL)
			video_ctx.video_ticker = ms_ticker_new_withname("amsip-video");

		_video_module_open_camera();
#if 1
		video_ctx.video_output = ms_filter_new(MS_VIDEO_OUT_ID);
		if(video_ctx.video_output!=NULL)
			ms_filter_set_notify_callback(video_ctx.video_output, _on_video_module_new_image,NULL);
#else
		video_ctx.video_output = ms_filter_new(MS_DRAWDIB_DISPLAY_ID);
		if (video_ctx.video_display != NULL)
		{
			ms_filter_call_method(video_ctx.video_output,
							  MS_VIDEO_DISPLAY_SET_NATIVE_WINDOW_ID, &video_ctx.video_display->window_id);
		}
#endif

#ifndef DISABLE_MSDRAW
		video_ctx.video_msdraw = ms_filter_new_from_name("MSDraw");
		if (video_ctx.video_msdraw == NULL) {
			ms_warning("am_video_module.c: missing msdraw plugin.");
		}
#endif

		if (video_ctx.video_ticker == NULL
			|| video_ctx.video_stitcher == NULL
			|| video_ctx.video_source == NULL
			|| video_ctx.video_tee == NULL
			|| video_ctx.video_void == NULL
			|| video_ctx.video_output == NULL
			|| video_ctx.video_selfview_tee == NULL) {
			if (video_ctx.video_ticker != NULL)
				ms_ticker_destroy(video_ctx.video_ticker);
			if (video_ctx.video_stitcher != NULL)
				ms_filter_destroy(video_ctx.video_stitcher);
			if (video_ctx.video_source != NULL)
				ms_filter_destroy(video_ctx.video_source);
			if (video_ctx.video_tee != NULL)
				ms_filter_destroy(video_ctx.video_tee);
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);
			if (video_ctx.video_output != NULL)
				ms_filter_destroy(video_ctx.video_output);
#ifndef DISABLE_MSDRAW
			if (video_ctx.video_msdraw != NULL)
				ms_filter_destroy(video_ctx.video_msdraw);
#endif
			if (video_ctx.video_selfview_tee != NULL)
				ms_filter_destroy(video_ctx.video_selfview_tee);

			video_ctx.video_ticker = NULL;
			video_ctx.video_stitcher = NULL;
			video_ctx.video_source = NULL;
			video_ctx.video_tee = NULL;
			video_ctx.video_void = NULL;
			video_ctx.video_output = NULL;
#ifndef DISABLE_MSDRAW
			video_ctx.video_msdraw = NULL;
#endif
			video_ctx.video_selfview_tee = NULL;

			return -1;
		}

		if (video_ctx.video_source->desc->id != MS_STATIC_IMAGE_ID)
			ms_filter_call_method(video_ctx.video_source,MS_FILTER_SET_FPS, &video_ctx.option_webcam_fps);

#if 0
		ms_filter_call_method(video_ctx.video_source,
							  MS_V4L_SET_DEVICE, &video_ctx.video_device);
#endif
		{
			int val=4;
			ms_filter_call_method(video_ctx.video_output,
							  MS_VIDEO_DISPLAY_SET_LOCAL_VIEW_MODE, &val);
		}
		if (video_ctx.video_display != NULL && video_ctx.video_output->desc->id==MS_VIDEO_OUT_ID)
			ms_filter_call_method(video_ctx.video_output,
								  MS_VIDEO_OUT_SET_DISPLAY,
								  video_ctx.video_display);

		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_SET_IMAGE,
							  video_ctx.nowebcam_image);

#define TEST_HIGH_QUALITY
#ifdef TEST_HIGH_QUALITY
		/* prefered quality for input: example 640x480 */
		vsize.height = video_ctx.prefered_input_size.height;
		vsize.width = video_ctx.prefered_input_size.width;
		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_SET_VIDEO_SIZE, &vsize);
#endif

#ifdef TEST_HIGH_QUALITY
		/* prefered quality for input: example 640x480 */
		vsize.height = video_ctx.prefered_input_size.height;
		vsize.width = video_ctx.prefered_input_size.width;
		ms_filter_call_method(video_ctx.video_stitcher,
							  MS_FILTER_SET_VIDEO_SIZE, &vsize);
#endif

		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_GET_PIX_FMT, &format);
		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_GET_VIDEO_SIZE, &vsize);
		if (format == MS_MJPEG) {
			video_ctx.video_sourcepixconv =
				ms_filter_new(MS_MJPEG_DEC_ID);
		} else {
			video_ctx.video_sourcepixconv =
				ms_filter_new(MS_PIX_CONV_ID);
			/*set it to the pixconv */
			ms_filter_call_method(video_ctx.video_sourcepixconv,
								  MS_FILTER_SET_PIX_FMT, &format);
			ms_filter_call_method(video_ctx.video_sourcepixconv,
								  MS_FILTER_SET_VIDEO_SIZE, &vsize);
		}

		if (video_ctx.video_sourcepixconv == NULL) {
			if (video_ctx.video_ticker != NULL)
				ms_ticker_destroy(video_ctx.video_ticker);
			if (video_ctx.video_stitcher != NULL)
				ms_filter_destroy(video_ctx.video_stitcher);
			if (video_ctx.video_source != NULL)
				ms_filter_destroy(video_ctx.video_source);
			if (video_ctx.video_sourcepixconv != NULL)
				ms_filter_destroy(video_ctx.video_sourcepixconv);
			if (video_ctx.video_tee != NULL)
				ms_filter_destroy(video_ctx.video_tee);
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);
			if (video_ctx.video_output != NULL)
				ms_filter_destroy(video_ctx.video_output);
#ifndef DISABLE_MSDRAW
			if (video_ctx.video_msdraw != NULL)
				ms_filter_destroy(video_ctx.video_msdraw);
#endif
			if (video_ctx.video_selfview_tee != NULL)
				ms_filter_destroy(video_ctx.video_selfview_tee);

			video_ctx.video_ticker = NULL;
			video_ctx.video_stitcher = NULL;
			video_ctx.video_source = NULL;
			video_ctx.video_sourcepixconv = NULL;
			video_ctx.video_tee = NULL;
			video_ctx.video_void = NULL;
			video_ctx.video_output = NULL;
#ifndef DISABLE_MSDRAW
			video_ctx.video_msdraw = NULL;
#endif
			video_ctx.video_selfview_tee = NULL;
			return -1;
		}

		/*force the decoder to output YUV420P */
		format = MS_YUV420P;
		vsize.height = video_ctx.prefered_screen_size.height;
		vsize.width = video_ctx.prefered_screen_size.width;
		ms_filter_call_method(video_ctx.video_output,
							  MS_FILTER_SET_PIX_FMT, &format);
		ms_filter_call_method(video_ctx.video_output,
							  MS_FILTER_SET_VIDEO_SIZE, &vsize);

		ms_filter_link(video_ctx.video_source, 0,
					   video_ctx.video_sourcepixconv, 0);
#ifdef DISABLE_STITCHER
		ms_filter_link(video_ctx.video_sourcepixconv, 0,
					   video_ctx.video_tee, 0);
#else
		ms_filter_link(video_ctx.video_sourcepixconv, 0,
					   video_ctx.video_selfview_tee, 0);
		ms_filter_link(video_ctx.video_selfview_tee, 0,
					   video_ctx.video_stitcher, 0);
#endif
#ifndef DISABLE_CAMERA_IN_OUTPUT
		ms_filter_link(video_ctx.video_selfview_tee, 1,
					   video_ctx.video_output, 1);
#endif

#if 0
		{
			static MSFilter *enc = NULL;
			static MSFilter *dec = NULL;
			int bd = 64000;
			int fps = 15;

			enc = ms_filter_create_encoder("H264");
			ms_filter_call_method(enc, MS_FILTER_SET_BITRATE,
								  (void *) &bd);
			dec = ms_filter_create_decoder("H264");

			ms_filter_call_method(enc, MS_FILTER_GET_VIDEO_SIZE, &vsize);
			ms_filter_call_method(video_ctx.video_stitcher,
								  MS_FILTER_SET_VIDEO_SIZE, &vsize);
			ms_filter_call_method(enc, MS_FILTER_GET_FPS, &fps);
			ms_filter_call_method(video_ctx.video_source,
								  MS_FILTER_SET_FPS, &fps);

			ms_filter_link(video_ctx.video_stitcher, 0, enc, 0);
			ms_filter_link(enc, 0, dec, 0);
			ms_filter_link(dec, 0, video_ctx.video_output, 0);
		}
#else
#ifndef DISABLE_MSDRAW
		if (video_ctx.video_msdraw==NULL)
			ms_filter_link(video_ctx.video_stitcher, 0,
						   video_ctx.video_output, 0);
		else
		{
			ms_filter_link(video_ctx.video_stitcher, 0,
							 video_ctx.video_msdraw, 0);
			ms_filter_link(video_ctx.video_msdraw, 0,
							 video_ctx.video_output, 0);
		}
#else
		ms_filter_link(video_ctx.video_stitcher, 0,
					   video_ctx.video_output, 0);
#endif
#endif
#ifdef DISABLE_STITCHER
#else
		ms_filter_link(video_ctx.video_stitcher, 1,
					   video_ctx.video_tee, 0);
#endif
		ms_filter_link(video_ctx.video_tee, 9,
					   video_ctx.video_void, 0);

		/* reattach all pending calls */
		{
			int k;
            int max = STITCHER_MAX_INPUTS;
            if (max > MAX_NUMBER_OF_CALLS)
                max = MAX_NUMBER_OF_CALLS;

			/* search index of elements */
			for (k = 0; k < max - 1; k++) {
				struct am_video_info *tmpinfo = _antisipc.calls[k].video_ctx;

				if (tmpinfo==NULL)
				{
				}
				else if (tmpinfo->video_rtprecv != NULL && tmpinfo->video_rtpsend != NULL
					&& tmpinfo->video_encoder != NULL
					&& tmpinfo->video_decoder != NULL) {
					ms_filter_link(tmpinfo->video_rtprecv, 0, tmpinfo->video_decoder,
								   0);
					ms_filter_link(tmpinfo->video_decoder, 0,
								   video_ctx.video_stitcher, k + 1);

#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
					ms_filter_link(video_ctx.video_tee, k,
								   tmpinfo->video_encoder, 0);
#else
					ms_filter_link(video_ctx.video_tee, k,
								   tmpinfo->video_sourcesizeconv, 0);
					ms_filter_link(tmpinfo->video_sourcesizeconv, 0,
								   tmpinfo->video_encoder, 0);
#endif
					ms_filter_link(tmpinfo->video_encoder, 0, tmpinfo->video_rtpsend,
								   0);

					if (video_ctx.video_source->desc->id == MS_STATIC_IMAGE_ID)
					{
						float fps = 15;
						/* if the graph contains the static image
						-> use bitrate=20000 & FPS=1 */
						int bitrate=20000;
						ms_filter_call_method(tmpinfo->video_encoder,
											  MS_FILTER_SET_BITRATE, (void *) &bitrate);

						vsize.height = MS_VIDEO_SIZE_CIF_H;
						vsize.width = MS_VIDEO_SIZE_CIF_W;
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
											  &vsize);
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_FPS, &fps);

						ms_filter_call_method(tmpinfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
											  &fps);
						ms_filter_call_method(tmpinfo->video_sourcesizeconv,
											  MS_FILTER_SET_VIDEO_SIZE, &vsize);
					}
					else
					{
						float fps = 15;
						/* revert bitrate with negotiated value */
						if (tmpinfo->video_encoder_bitrate>0)
							ms_filter_call_method(tmpinfo->video_encoder,
												  MS_FILTER_SET_BITRATE, (void *) &tmpinfo->video_encoder_bitrate);

						vsize.height = MS_VIDEO_SIZE_CIF_H;
						vsize.width = MS_VIDEO_SIZE_CIF_W;
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
											  &vsize);
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_FPS, &fps);

						ms_filter_call_method(tmpinfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
											  &fps);
						ms_filter_call_method(tmpinfo->video_sourcesizeconv,
											  MS_FILTER_SET_VIDEO_SIZE, &vsize);
					}
                    
#ifdef DISABLE_STITCHER
                    ms_ticker_attach(video_ctx.video_ticker, tmpinfo->video_rtprecv);
#endif
					//AMD fix 22/02/2012
					if (tmpinfo->video_ice != NULL)
						ms_ticker_attach(video_ctx.video_ticker,
														 tmpinfo->video_ice);
				} else if (tmpinfo->video_rtprecv != NULL
						   && tmpinfo->video_decoder != NULL) {
					ms_filter_link(tmpinfo->video_rtprecv, 0, tmpinfo->video_decoder,
								   0);
					ms_filter_link(tmpinfo->video_decoder, 0,
								   video_ctx.video_stitcher, k + 1);
#ifdef DISABLE_STITCHER
                    ms_ticker_attach(video_ctx.video_ticker, tmpinfo->video_rtprecv);
#endif
					//AMD fix 22/02/2012
					if (tmpinfo->video_ice != NULL)
						ms_ticker_attach(video_ctx.video_ticker,
														 tmpinfo->video_ice);
				} else if (tmpinfo->video_rtpsend != NULL
						   && tmpinfo->video_encoder != NULL) {
#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
					ms_filter_link(video_ctx.video_tee, k,
								   tmpinfo->video_encoder, 0);
#else
					ms_filter_link(video_ctx.video_tee, k,
								   tmpinfo->video_sourcesizeconv, 0);
					ms_filter_link(tmpinfo->video_sourcesizeconv, 0,
								   tmpinfo->video_encoder, 0);
#endif
					ms_filter_link(tmpinfo->video_encoder, 0, tmpinfo->video_rtpsend,
								   0);

					if (video_ctx.video_source->desc->id == MS_STATIC_IMAGE_ID)
					{
						float fps = 15;
						/* if the graph contains the static image
						-> use bitrate=20000 & FPS=1 */
						int bitrate=20000;
						ms_filter_call_method(tmpinfo->video_encoder,
											  MS_FILTER_SET_BITRATE, (void *) &bitrate);

						vsize.height = MS_VIDEO_SIZE_CIF_H;
						vsize.width = MS_VIDEO_SIZE_CIF_W;
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
											  &vsize);
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_FPS, &fps);

						ms_filter_call_method(tmpinfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
											  &fps);
						ms_filter_call_method(tmpinfo->video_sourcesizeconv,
											  MS_FILTER_SET_VIDEO_SIZE, &vsize);
					}
					else
					{
						float fps = 15;
						/* revert bitrate with negotiated value */
						if (tmpinfo->video_encoder_bitrate>0)
							ms_filter_call_method(tmpinfo->video_encoder,
												  MS_FILTER_SET_BITRATE, (void *) &tmpinfo->video_encoder_bitrate);

						vsize.height = MS_VIDEO_SIZE_CIF_H;
						vsize.width = MS_VIDEO_SIZE_CIF_W;
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
											  &vsize);
						ms_filter_call_method(tmpinfo->video_encoder, MS_FILTER_GET_FPS, &fps);

						ms_filter_call_method(tmpinfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
											  &fps);
						ms_filter_call_method(tmpinfo->video_sourcesizeconv,
											  MS_FILTER_SET_VIDEO_SIZE, &vsize);
					}
					//AMD fix 22/02/2012
					if (tmpinfo->video_ice != NULL)
						ms_ticker_attach(video_ctx.video_ticker,
														 tmpinfo->video_ice);
				}
			}
		}

		ms_ticker_attach(video_ctx.video_ticker,
						 video_ctx.video_source);
	}
	return 0;
}

static void _on_video_module_new_image(void *userdata , unsigned int id, void *arg)
{
	if (id==MS_DISPLAY_NEW_IMAGE || id==MS_DISPLAY_NEW_IMAGE_SELFVIEW)
	{
		struct amsip_picture {
				int width;
				int height;
				int format;
				int size;
				unsigned char *pixels;
			};
		struct amsip_picture *pic = (struct amsip_picture *)arg;
		if (video_ctx.on_new_image_cb!=NULL)
		{
			if (id==MS_DISPLAY_NEW_IMAGE)
				video_ctx.on_new_image_cb(0, pic->width, pic->height, pic->format, pic->size, pic->pixels);
			else if (id==MS_DISPLAY_NEW_IMAGE_SELFVIEW)
				video_ctx.on_new_image_cb(1, pic->width, pic->height, pic->format, pic->size, pic->pixels);
		}
	}
}

static int video_module_set_image_callback(on_video_module_new_image_cb func)
{
	if (video_ctx.video_ticker!=NULL)
	{
		ms_mutex_lock(&video_ctx.video_ticker->lock);
		video_ctx.on_new_image_cb=func;
		if(video_ctx.video_output!=NULL)
			ms_filter_set_notify_callback(video_ctx.video_output, _on_video_module_new_image,NULL);
		ms_mutex_unlock(&video_ctx.video_ticker->lock);
	}
	else
		video_ctx.on_new_image_cb=func;
	return 0;
}

static int video_module_set_selfview_mode(int mode)
{
	if(video_ctx.video_output!=NULL)
		return ms_filter_call_method(video_ctx.video_output,MS_VIDEO_DISPLAY_SET_LOCAL_VIEW_MODE, &mode);
	return -1;
}

static int video_module_set_selfview_position(float posx, float posy, float size)
{
	float data[3];
	data[0]=posx;
	data[1]=posy;
	data[2]=size;
	if(video_ctx.video_output!=NULL)
		return ms_filter_call_method(video_ctx.video_output,MS_VIDEO_DISPLAY_SET_SELFVIEW_POS, &data);
	return -1;
}

static int video_module_get_selfview_position(float *posx, float *posy, float *size)
{
	*posx=0;
	*posy=0;
	*size=0;
	if(video_ctx.video_output!=NULL)
	{
		float data[3];
		if(ms_filter_call_method(video_ctx.video_output,MS_VIDEO_DISPLAY_GET_SELFVIEW_POS, &data)==0)
		{
			*posx=data[0];
			*posy=data[1];
			*size=data[2];
			return 0;
		}
	}
	return -1;
}

static int video_module_set_selfview_scalefactor(float scalefactor)
{
	if(video_ctx.video_output!=NULL)
		return ms_filter_call_method(video_ctx.video_output,MS_VIDEO_DISPLAY_SET_LOCAL_VIEW_SCALEFACTOR, &scalefactor);
	return -1;
}

static int video_module_set_background_color(int red, int green, int blue)
{
	int err1;
	int err2;
	int color[3];
	color[0]=red;
	color[1]=green;
	color[2]=blue;
	if(video_ctx.video_output!=NULL)
		err1 = ms_filter_call_method(video_ctx.video_output,MS_VIDEO_DISPLAY_SET_BACKGROUND_COLOR, &color);
	if(video_ctx.video_stitcher!=NULL)
		err2 = ms_filter_call_method(video_ctx.video_stitcher,MS_VIDEO_DISPLAY_SET_BACKGROUND_COLOR, &color);
	if (err1==0 && err2==0)
		return AMSIP_SUCCESS;
	return AMSIP_UNDEFINED_ERROR;
}

static int video_module_set_video_option(int opt, void *arg)
{
	if(opt==AMSIP_OPTION_WEBCAM_FPS)
	{
		video_ctx.option_webcam_fps = *((float*)arg);
		if(video_ctx.video_source!=NULL && video_ctx.video_source->desc->id != MS_STATIC_IMAGE_ID)
			return ms_filter_call_method(video_ctx.video_source,MS_FILTER_SET_FPS, &video_ctx.option_webcam_fps);
	}else if(opt==AMSIP_OPTION_FORCE_ENCODER_FPS){
		video_ctx.option_force_encoder_fps = *((float*)arg);
	}
	return -1;
}

static int video_module_set_callback(unsigned int id,
						  MSFilterNotifyFunc speex_pp_process,
						  void *userdata)
{
	return 0;
}

static void
video_on_timestamp_jump(RtpSession * s, uint32_t * ts, void *user_data)
{
	ms_warning("Remote phone is sending data with a future timestamp: %u",
			   *ts);
	rtp_session_resync(s);
}

static void video_payload_type_changed(RtpSession * session, void *data)
{
	am_call_t *ca = (am_call_t *) data;
	int payload;
	RtpProfile *prof;
	PayloadType *pt;

	payload = rtp_session_get_recv_payload_type(session);
	prof = rtp_session_get_profile(session);
	pt = rtp_profile_get_payload(prof, payload);

	if (pt != NULL) {
		MSFilter *dec = ms_filter_create_decoder(pt->mime_type);

		if (dec != NULL) {

			int k;

			/* search index of elements */
			for (k = 0; k < MAX_NUMBER_OF_CALLS; k++) {
				if (&_antisipc.calls[k] == ca) {
					MSPixFmt format;
					struct am_video_info *tmpinfo = _antisipc.calls[k].video_ctx;

					if (tmpinfo!=NULL)
					{
						ms_filter_unlink(tmpinfo->video_rtprecv, 0,
										 tmpinfo->video_decoder, 0);
						ms_filter_unlink(tmpinfo->video_decoder, 0,
										 video_ctx.video_stitcher, k + 1);

						ms_filter_postprocess(tmpinfo->video_decoder);
						ms_filter_destroy(tmpinfo->video_decoder);
						tmpinfo->video_decoder = dec;

						format = MS_YUV420P;
						ms_filter_call_method(tmpinfo->video_decoder,
											  MS_FILTER_SET_PIX_FMT, &format);
						if (pt->recv_fmtp != NULL)
							ms_filter_call_method(tmpinfo->video_decoder,
												  MS_FILTER_ADD_FMTP,
												  (void *) pt->recv_fmtp);

						ms_filter_link(tmpinfo->video_rtprecv, 0, tmpinfo->video_decoder,
									   0);
						ms_filter_link(tmpinfo->video_decoder, 0,
									   video_ctx.video_stitcher, k + 1);
						ms_filter_preprocess(tmpinfo->video_decoder,
											 video_ctx.video_ticker);
					}
				}
			}
		} else {
			ms_warning("No video decoder found for %s", pt->mime_type);
		}
	} else {
		ms_warning("No video payload defined with number %i", payload);
	}

}

static RtpSession *am_create_duplex_rtpsession(am_call_t * ca, int locport,
											   const char *remip,
											   int remport, int jitt_comp)
{
	RtpSession *rtpr;
	JBParameters jbp;
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return NULL;
	}

	if (ainfo->video_rtp_session==NULL)
	{
		rtpr = rtp_session_new(RTP_SESSION_SENDRECV);
		rtp_session_set_recv_buf_size(rtpr, UDP_MAX_SIZE * 3);
		rtp_session_set_profile(rtpr, ainfo->video_rtp_profile);
		rtp_session_set_rtp_socket_recv_buffer_size(rtpr,2000000);
		rtp_session_set_rtp_socket_send_buffer_size(rtpr,2000000);
		rtp_session_set_local_addr(rtpr, "0.0.0.0", locport);
		if (remport > 0)
			rtp_session_set_remote_addr(rtpr, remip, remport);
		rtp_session_set_dscp(rtpr, _antisipc.video_dscp);
		rtp_session_set_scheduling_mode(rtpr, 0);
		rtp_session_set_blocking_mode(rtpr, 0);

		jbp.min_size = RTP_DEFAULT_JITTER_TIME;
		jbp.nom_size = RTP_DEFAULT_JITTER_TIME;
		jbp.max_size = -1;
		jbp.max_packets = 500;		/* maximum number of packet allowed to be queued */
		jbp.adaptive = TRUE;
		rtp_session_set_jitter_buffer_params(rtpr, &jbp);

		rtp_session_set_jitter_compensation(rtpr, jitt_comp);
		rtp_session_enable_adaptive_jitter_compensation(rtpr, TRUE);
		rtp_session_enable_jitter_buffer(rtpr, FALSE);
		rtp_session_signal_connect(rtpr, "timestamp_jump",
								   (RtpCallback) video_on_timestamp_jump, 0);
	}
	else
	{
		rtpr = ainfo->video_rtp_session;
		rtp_session_flush_sockets(rtpr);
		rtp_session_resync(rtpr);

		rtp_session_signal_disconnect_by_callback(rtpr, "timestamp_jump",
								   (RtpCallback) video_on_timestamp_jump);
		rtp_session_signal_disconnect_by_callback(rtpr, "payload_type_changed",
								   (RtpCallback) video_payload_type_changed);

		rtp_session_set_recv_buf_size(rtpr, UDP_MAX_SIZE * 3);
		rtp_session_set_profile(rtpr, ainfo->video_rtp_profile);
		rtp_session_set_local_addr(rtpr, "0.0.0.0", locport);
		if (remport > 0)
			rtp_session_set_remote_addr(rtpr, remip, remport);
		rtp_session_set_dscp(rtpr, _antisipc.video_dscp);
		rtp_session_set_scheduling_mode(rtpr, 0);
		rtp_session_set_blocking_mode(rtpr, 0);

		jbp.min_size = RTP_DEFAULT_JITTER_TIME;
		jbp.nom_size = RTP_DEFAULT_JITTER_TIME;
		jbp.max_size = -1;
		jbp.max_packets = 500;		/* maximum number of packet allowed to be queued */
		jbp.adaptive = TRUE;
		rtp_session_set_jitter_buffer_params(rtpr, &jbp);

		rtp_session_set_jitter_compensation(rtpr, jitt_comp);
		rtp_session_enable_adaptive_jitter_compensation(rtpr, TRUE);
		rtp_session_enable_jitter_buffer(rtpr, FALSE);
		rtp_session_signal_connect(rtpr, "timestamp_jump",
								   (RtpCallback) video_on_timestamp_jump, 0);
	}
	return rtpr;
}

static void video_stream_graph_reset(am_call_t * ca)
{
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return;
	}

	if (ainfo->video_rtp_session != NULL
		&& ainfo->video_rtp_queue != NULL)
		rtp_session_unregister_event_queue(ainfo->video_rtp_session,
										   ainfo->video_rtp_queue);
	if (ainfo->video_rtp_session != NULL
		&& ainfo->video_rtp_session->rtp.tr!=NULL
		&& strcmp(ainfo->video_rtp_session->rtp.tr->name, "ZRTP")!=0)
	{
		rtp_session_set_transports(ainfo->video_rtp_session, NULL, NULL);
	}
#if !defined(KEEP_RTP_SESSION)
	if (ainfo->video_rtp_session != NULL)
		rtp_session_destroy(ainfo->video_rtp_session);
#endif
	if (ainfo->video_rtp_queue != NULL)
		ortp_ev_queue_destroy(ainfo->video_rtp_queue);
	if (ainfo->video_sourcesizeconv != NULL)
		ms_filter_destroy(ainfo->video_sourcesizeconv);
	if (ainfo->video_encoder != NULL)
		ms_filter_destroy(ainfo->video_encoder);
	if (ainfo->video_decoder != NULL)
		ms_filter_destroy(ainfo->video_decoder);
	if (ainfo->video_rtpsend != NULL)
		ms_filter_destroy(ainfo->video_rtpsend);
	if (ainfo->video_rtprecv != NULL)
		ms_filter_destroy(ainfo->video_rtprecv);
	if (ainfo->video_ice != NULL)
		ms_filter_destroy(ainfo->video_ice);

#if !defined(KEEP_RTP_SESSION)
	ainfo->video_rtp_session = NULL;
#endif
	ainfo->video_rtp_queue=NULL;
	ainfo->video_sourcesizeconv = NULL;
	ainfo->video_encoder = NULL;
	ainfo->video_decoder = NULL;
	ainfo->video_rtpsend = NULL;
	ainfo->video_rtprecv = NULL;
	ainfo->video_ice = NULL;
}

static int video_stream_start_send_recv(am_call_t * ca, PayloadType * pt)
{
	RtpSession *rtps;
	MSPixFmt format;
	MSVideoSize vsize;
	float fps = 15;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	rtps = ainfo->video_rtp_session;
	if (rtps == NULL)
		return -1;

	if (video_ctx.video_stitcher == NULL) {
		video_module_enable_preview(1);
	}
	if (video_ctx.video_stitcher == NULL) {
		ms_error("am_video_module.c: missing video stitcher plugin.");
		return -1;
	}

	ainfo->video_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ainfo->video_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

#if 0
	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
#endif
		ainfo->video_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->video_ice != NULL) {
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->video_checklist);
		}
#if 0
	}
#endif

	if (ainfo->video_rtp_queue==NULL)
		ainfo->video_rtp_queue = ortp_ev_queue_new();
	if (ainfo->video_rtp_queue != NULL)
		rtp_session_register_event_queue(rtps,
										 ainfo->video_rtp_queue);

	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) video_payload_type_changed,
							   (unsigned long) ca);

	if ((ainfo->video_encoder == NULL) || (ainfo->video_decoder == NULL)) {
		/* big problem: we have not a registered codec for this payload... */
		video_stream_graph_reset(ca);
		if (pt == NULL)
			ms_error
				("am_video_module.c: No decoder/encoder available for payload (NULL).");
		else
			ms_error
				("am_video_module.c: No decoder/encoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}

	if (video_ctx.video_source->desc->id == MS_STATIC_IMAGE_ID)
	{
		/* if the graph contains the static image
		-> use bitrate=20000 & FPS=1 */
		int bitrate=20000;
		ms_filter_call_method(ainfo->video_encoder,
							  MS_FILTER_SET_BITRATE, (void *) &bitrate);
	}

	if (ainfo->video_sourcesizeconv == NULL) {
		ainfo->video_sourcesizeconv = ms_filter_new(MS_SIZE_CONV_ID);
	}

	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
						  &vsize);
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_FPS, &fps);

	ms_filter_call_method(ainfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
						  &fps);
	ms_filter_call_method(ainfo->video_sourcesizeconv,
						  MS_FILTER_SET_VIDEO_SIZE, &vsize);
	if (pt == NULL)
		ms_message
			("am_video_module.c: (NULL) fps: %f video encoder/sizeconverter size:%ix%i",
			 fps, vsize.width, vsize.height);
	else
		ms_message
			("am_video_module.c: %s fps: %f video encoder/sizeconverter size:%ix%i",
			 pt->mime_type, fps, vsize.width, vsize.height);

#if 0
	ms_filter_call_method(video_ctx.video_source, MS_FILTER_SET_FPS,
						  &fps);

	ms_filter_call_method(video_ctx.video_source,
						  MS_FILTER_SET_VIDEO_SIZE, &vsize);
#endif

	/*force the decoder to output YUV420P */
	format = MS_YUV420P;
	ms_filter_call_method(ainfo->video_decoder, MS_FILTER_SET_PIX_FMT,
						  &format);

	if (pt != NULL && pt->send_fmtp != NULL)
		ms_filter_call_method(ainfo->video_encoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->send_fmtp);
	if (pt != NULL && pt->recv_fmtp != NULL)
		ms_filter_call_method(ainfo->video_decoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);
	return 0;
}

static int video_stream_start_recv_only(am_call_t * ca, PayloadType * pt)
{
	RtpSession *rtps;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	rtps = ainfo->video_rtp_session;
	if (rtps == NULL)
		return -1;

	if (video_ctx.video_stitcher == NULL) {
		video_module_enable_preview(1);
	}
	if (video_ctx.video_stitcher == NULL) {
		ms_error("am_video_module.c: missing video stitcher plugin.");
		return -1;
	}

	ainfo->video_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ainfo->video_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

#if 0
	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
#endif
		ainfo->video_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->video_ice != NULL) {
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->video_checklist);
		}
#if 0
	}
#endif

	if (ainfo->video_rtp_queue==NULL)
		ainfo->video_rtp_queue = ortp_ev_queue_new();
	if (ainfo->video_rtp_queue != NULL)
		rtp_session_register_event_queue(rtps,
										 ainfo->video_rtp_queue);
	rtp_session_signal_connect(rtps, "payload_type_changed",
							   (RtpCallback) video_payload_type_changed,
							   (unsigned long) ca);

	if (ainfo->video_decoder == NULL) {
		/* big problem: we have not a registered codec for this payload... */
		video_stream_graph_reset(ca);
		if (pt == NULL)
			ms_error
				("am_video_module.c: No decoder available for payload (NULL).");
		else
			ms_error
				("am_video_module.c: No decoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}

	if (pt != NULL && pt->recv_fmtp != NULL)
		ms_filter_call_method(ainfo->video_decoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);

	return 0;
}

static int video_stream_start_send_only(am_call_t * ca, PayloadType * pt)
{
	RtpSession *rtps;
	MSVideoSize vsize;
	float fps = 15;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	rtps = ainfo->video_rtp_session;
	if (rtps == NULL)
		return -1;

	if (video_ctx.video_stitcher == NULL) {
		video_module_enable_preview(1);
	}
	if (video_ctx.video_stitcher == NULL) {
		ms_error("am_video_module.c: missing video stitcher plugin.");
		return -1;
	}

	ainfo->video_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ainfo->video_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

#if 0
	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
#endif
		ainfo->video_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->video_ice != NULL) {
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->video_checklist);
		}
#if 0
	}
#endif

	if (ainfo->video_rtp_queue==NULL)
		ainfo->video_rtp_queue = ortp_ev_queue_new();
	if (ainfo->video_rtp_queue != NULL)
		rtp_session_register_event_queue(rtps,
										 ainfo->video_rtp_queue);

	if (ainfo->video_encoder == NULL) {
		/* big problem: we have not a registered codec for this payload... */
		video_stream_graph_reset(ca);
		if (pt == NULL)
			ms_error
				("am_video_module.c: No decoder available for payload (NULL).");
		else
			ms_error
				("am_video_module.c: No decoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}

	if (video_ctx.video_source->desc->id == MS_STATIC_IMAGE_ID)
	{
		/* if the graph contains the static image
		-> use bitrate=20000 & FPS=1 */
		int bitrate=20000;
		ms_filter_call_method(ainfo->video_encoder,
							  MS_FILTER_SET_BITRATE, (void *) &bitrate);
	}

	if (ainfo->video_sourcesizeconv == NULL) {
		ainfo->video_sourcesizeconv = ms_filter_new(MS_SIZE_CONV_ID);
	}

	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
						  &vsize);
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_FPS, &fps);

	ms_filter_call_method(ainfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
						  &fps);
	ms_filter_call_method(ainfo->video_sourcesizeconv,
						  MS_FILTER_SET_VIDEO_SIZE, &vsize);

#if 0
	ms_filter_call_method(video_ctx.video_source, MS_FILTER_SET_FPS,
						  &fps);
	ms_filter_call_method(video_ctx.video_source,
						  MS_FILTER_SET_VIDEO_SIZE, &vsize);
#endif

	if (pt != NULL && pt->send_fmtp != NULL)
		ms_filter_call_method(ainfo->video_encoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->send_fmtp);
	return 0;
}

static int am_ms2_video_detach(am_call_t * call_to_detach)
{
	struct am_video_info *ainfo = call_to_detach->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}
	/* detach preview */
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_stitcher != NULL
		&& video_ctx.video_source != NULL
		&& video_ctx.video_sourcepixconv != NULL
		&& video_ctx.video_tee != NULL
		&& video_ctx.video_void != NULL
		&& video_ctx.video_output != NULL
		&& video_ctx.video_selfview_tee != NULL) {
		/* detach current calls */
		{
			int k;
            int max = STITCHER_MAX_INPUTS;
            if (max > MAX_NUMBER_OF_CALLS)
                max = MAX_NUMBER_OF_CALLS;

			/* search index of elements */
			for (k = 0; k < max - 1; k++) {
				am_call_t *ca = &_antisipc.calls[k];

				if (ca == call_to_detach) {
					if (ainfo->video_ice != NULL)
						ms_ticker_detach(video_ctx.video_ticker,
										 ainfo->video_ice);
					if (ainfo->video_encoder != NULL
						&& ainfo->video_decoder != NULL) {
						ms_ticker_detach(video_ctx.video_ticker,
										 video_ctx.video_source);
#ifdef DISABLE_STITCHER
                        ms_ticker_detach(video_ctx.video_ticker, ainfo->video_rtprecv);
#endif

						ms_filter_unlink(ainfo->video_rtprecv, 0,
										 ainfo->video_decoder, 0);
						ms_filter_unlink(ainfo->video_decoder, 0,
										 video_ctx.video_stitcher,
										 k + 1);

#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
						ms_filter_unlink(video_ctx.video_tee, k,
										 ainfo->video_encoder, 0);
#else
						ms_filter_unlink(video_ctx.video_tee, k,
										 ainfo->video_sourcesizeconv, 0);
						ms_filter_unlink(ainfo->video_sourcesizeconv, 0,
										 ainfo->video_encoder, 0);
#endif
						ms_filter_unlink(ainfo->video_encoder, 0,
										 ainfo->video_rtpsend, 0);

						ms_ticker_attach(video_ctx.video_ticker,
										 video_ctx.video_source);
					} else if (ainfo->video_decoder != NULL) {
						ms_ticker_detach(video_ctx.video_ticker,
										 video_ctx.video_source);
#ifdef DISABLE_STITCHER
                        ms_ticker_detach(video_ctx.video_ticker, ainfo->video_rtprecv);
#endif

						ms_filter_unlink(ainfo->video_rtprecv, 0,
										 ainfo->video_decoder, 0);
						ms_filter_unlink(ainfo->video_decoder, 0,
										 video_ctx.video_stitcher,
										 k + 1);

						ms_ticker_attach(video_ctx.video_ticker,
										 video_ctx.video_source);
					} else if (ainfo->video_encoder != NULL) {
						ms_ticker_detach(video_ctx.video_ticker,
										 video_ctx.video_source);

#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
						ms_filter_unlink(video_ctx.video_tee, k,
										 ainfo->video_encoder, 0);
#else
						ms_filter_unlink(video_ctx.video_tee, k,
										 ainfo->video_sourcesizeconv, 0);
						ms_filter_unlink(ainfo->video_sourcesizeconv, 0,
										 ainfo->video_encoder, 0);
#endif
						ms_filter_unlink(ainfo->video_encoder, 0,
										 ainfo->video_rtpsend, 0);

						ms_ticker_attach(video_ctx.video_ticker,
										 video_ctx.video_source);
					}
				}
			}
		}
	}
	return 0;
}

static int am_ms2_video_attach(am_call_t * call_to_attach)
{
	MSPixFmt format;
	MSVideoSize vsize;
	struct am_video_info *ainfo = call_to_attach->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	/* detach preview */
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_stitcher != NULL
		&& video_ctx.video_source != NULL
		&& video_ctx.video_sourcepixconv != NULL
		&& video_ctx.video_tee != NULL
		&& video_ctx.video_void != NULL
		&& video_ctx.video_output != NULL
		&& video_ctx.video_selfview_tee != NULL) {
		ms_ticker_detach(video_ctx.video_ticker,
						 video_ctx.video_source);
	} else if (
			   video_ctx.video_stitcher == NULL
			   && video_ctx.video_source == NULL
			   && video_ctx.video_sourcepixconv == NULL
			   && video_ctx.video_tee == NULL
			   && video_ctx.video_void == NULL
			   && video_ctx.video_output == NULL
			   && video_ctx.video_selfview_tee == NULL) {
		video_ctx.video_stitcher =
			ms_filter_new_from_name("MSVideoStitcher");
		if (video_ctx.video_stitcher == NULL) {
			ms_error("am_video_module.c: missing video stitcher plugin.");
			return -1;
		}

		if (video_ctx.video_ticker==NULL)
			video_ctx.video_ticker = ms_ticker_new_withname("amsip-video");
		_video_module_open_camera();
		video_ctx.video_tee = ms_filter_new(MS_TEE_ID);
		video_ctx.video_void = ms_filter_new(MS_VOID_SINK_ID);
		video_ctx.video_selfview_tee = ms_filter_new(MS_TEE_ID);
#if 1
		video_ctx.video_output = ms_filter_new(MS_VIDEO_OUT_ID);
		if(video_ctx.video_output!=NULL)
			ms_filter_set_notify_callback(video_ctx.video_output, _on_video_module_new_image,NULL);
#else
		video_ctx.video_output = ms_filter_new(MS_DRAWDIB_DISPLAY_ID);
		if (video_ctx.video_display != NULL)
		{
			ms_filter_call_method(video_ctx.video_output,
							  MS_VIDEO_DISPLAY_SET_NATIVE_WINDOW_ID, &video_ctx.video_display->window_id);
		}
#endif
#ifndef DISABLE_MSDRAW
		video_ctx.video_msdraw = ms_filter_new_from_name("MSDraw");
		if (video_ctx.video_msdraw == NULL) {
			ms_warning("am_video_module.c: missing msdraw plugin.");
		}
#endif

		if (video_ctx.video_ticker == NULL
			|| video_ctx.video_stitcher == NULL
			|| video_ctx.video_source == NULL
			|| video_ctx.video_tee == NULL
			|| video_ctx.video_void == NULL
			|| video_ctx.video_output == NULL
			|| video_ctx.video_selfview_tee == NULL) {
			if (video_ctx.video_ticker != NULL)
				ms_ticker_destroy(video_ctx.video_ticker);
			if (video_ctx.video_stitcher != NULL)
				ms_filter_destroy(video_ctx.video_stitcher);
			if (video_ctx.video_source != NULL)
				ms_filter_destroy(video_ctx.video_source);
			if (video_ctx.video_tee != NULL)
				ms_filter_destroy(video_ctx.video_tee);
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);
			if (video_ctx.video_output != NULL)
				ms_filter_destroy(video_ctx.video_output);
#ifndef DISABLE_MSDRAW
			if (video_ctx.video_msdraw != NULL)
				ms_filter_destroy(video_ctx.video_msdraw);
#endif
			if (video_ctx.video_selfview_tee != NULL)
				ms_filter_destroy(video_ctx.video_selfview_tee);

			video_ctx.video_ticker = NULL;
			video_ctx.video_stitcher = NULL;
			video_ctx.video_source = NULL;
			video_ctx.video_tee = NULL;
			video_ctx.video_void = NULL;
			video_ctx.video_output = NULL;
#ifndef DISABLE_MSDRAW
			video_ctx.video_msdraw = NULL;
#endif
			video_ctx.video_selfview_tee = NULL;
			return -1;
		}

		if (video_ctx.video_source->desc->id != MS_STATIC_IMAGE_ID)
			ms_filter_call_method(video_ctx.video_source,MS_FILTER_SET_FPS, &video_ctx.option_webcam_fps);

#if 0
		ms_filter_call_method(video_ctx.video_source,
							  MS_V4L_SET_DEVICE, &video_ctx.video_device);
#endif
		{
			int val=4;
			ms_filter_call_method(video_ctx.video_output,
							  MS_VIDEO_DISPLAY_SET_LOCAL_VIEW_MODE, &val);
		}
		if (video_ctx.video_display != NULL && video_ctx.video_output->desc->id==MS_VIDEO_OUT_ID)
			ms_filter_call_method(video_ctx.video_output,
								  MS_VIDEO_OUT_SET_DISPLAY,
								  video_ctx.video_display);

		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_SET_IMAGE,
							  video_ctx.nowebcam_image);

#define TEST_HIGH_QUALITY
#ifdef TEST_HIGH_QUALITY
		/* prefered quality for input: example 640x480 */
		vsize.height = video_ctx.prefered_input_size.height;
		vsize.width = video_ctx.prefered_input_size.width;
		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_SET_VIDEO_SIZE, &vsize);
#endif

#ifdef TEST_HIGH_QUALITY
		/* prefered quality for input: example 640x480 */
		vsize.height = video_ctx.prefered_input_size.height;
		vsize.width = video_ctx.prefered_input_size.width;
		ms_filter_call_method(video_ctx.video_stitcher,
							  MS_FILTER_SET_VIDEO_SIZE, &vsize);
#endif

		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_GET_PIX_FMT, &format);
		ms_filter_call_method(video_ctx.video_source,
							  MS_FILTER_GET_VIDEO_SIZE, &vsize);
		if (format == MS_MJPEG) {
			video_ctx.video_sourcepixconv =
				ms_filter_new(MS_MJPEG_DEC_ID);
		} else {
			video_ctx.video_sourcepixconv =
				ms_filter_new(MS_PIX_CONV_ID);
			/*set it to the pixconv */
			ms_filter_call_method(video_ctx.video_sourcepixconv,
								  MS_FILTER_SET_PIX_FMT, &format);
			ms_filter_call_method(video_ctx.video_sourcepixconv,
								  MS_FILTER_SET_VIDEO_SIZE, &vsize);
		}

		if (video_ctx.video_sourcepixconv == NULL) {
			if (video_ctx.video_ticker != NULL)
				ms_ticker_destroy(video_ctx.video_ticker);
			if (video_ctx.video_stitcher != NULL)
				ms_filter_destroy(video_ctx.video_stitcher);
			if (video_ctx.video_source != NULL)
				ms_filter_destroy(video_ctx.video_source);
			if (video_ctx.video_sourcepixconv != NULL)
				ms_filter_destroy(video_ctx.video_sourcepixconv);
			if (video_ctx.video_tee != NULL)
				ms_filter_destroy(video_ctx.video_tee);
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);
			if (video_ctx.video_output != NULL)
				ms_filter_destroy(video_ctx.video_output);
#ifndef DISABLE_MSDRAW
			if (video_ctx.video_msdraw != NULL)
				ms_filter_destroy(video_ctx.video_msdraw);
#endif
			if (video_ctx.video_selfview_tee != NULL)
				ms_filter_destroy(video_ctx.video_selfview_tee);

			video_ctx.video_ticker = NULL;
			video_ctx.video_stitcher = NULL;
			video_ctx.video_source = NULL;
			video_ctx.video_sourcepixconv = NULL;
			video_ctx.video_tee = NULL;
			video_ctx.video_void = NULL;
			video_ctx.video_output = NULL;
#ifndef DISABLE_MSDRAW
			video_ctx.video_msdraw = NULL;
#endif
			video_ctx.video_selfview_tee = NULL;
			return -1;
		}

		/*force the decoder to output YUV420P */
		format = MS_YUV420P;
		vsize.height = video_ctx.prefered_screen_size.height;
		vsize.width = video_ctx.prefered_screen_size.width;
		ms_filter_call_method(video_ctx.video_output,
							  MS_FILTER_SET_PIX_FMT, &format);
		ms_filter_call_method(video_ctx.video_output,
							  MS_FILTER_SET_VIDEO_SIZE, &vsize);

		ms_filter_link(video_ctx.video_source, 0,
					   video_ctx.video_sourcepixconv, 0);
#ifdef DISABLE_STITCHER
		ms_filter_link(video_ctx.video_sourcepixconv, 0,
					   video_ctx.video_tee, 0);
#else
		ms_filter_link(video_ctx.video_sourcepixconv, 0,
					   video_ctx.video_selfview_tee, 0);
		ms_filter_link(video_ctx.video_selfview_tee, 0,
					   video_ctx.video_stitcher, 0);
#endif
#ifndef DISABLE_CAMERA_IN_OUTPUT
		ms_filter_link(video_ctx.video_selfview_tee, 1,
					   video_ctx.video_output, 1);
#endif
#ifndef DISABLE_MSDRAW
		if (video_ctx.video_msdraw==NULL)
			ms_filter_link(video_ctx.video_stitcher, 0,
						   video_ctx.video_output, 0);
		else
		{
			ms_filter_link(video_ctx.video_stitcher, 0,
						   video_ctx.video_msdraw, 0);
			ms_filter_link(video_ctx.video_msdraw, 0,
						   video_ctx.video_output, 0);
		}
#else
		ms_filter_link(video_ctx.video_stitcher, 0,
					   video_ctx.video_output, 0);
#endif
#ifdef DISABLE_STITCHER
#else
		ms_filter_link(video_ctx.video_stitcher, 1,
					   video_ctx.video_tee, 0);
#endif
		ms_filter_link(video_ctx.video_tee, 9,
					   video_ctx.video_void, 0);
	} else {
		/* wrong state for video */
		return -1;
	}

	/* reattach all pending calls */
	{
		int k;
        int max = STITCHER_MAX_INPUTS;
        if (max > MAX_NUMBER_OF_CALLS)
            max = MAX_NUMBER_OF_CALLS;

		/* search index of elements */
		for (k = 0; k < max - 1; k++) {
			am_call_t *ca = &_antisipc.calls[k];

			if (call_to_attach == ca) {
				if (ainfo->video_encoder != NULL && ainfo->video_decoder != NULL) {
					ms_filter_link(ainfo->video_rtprecv, 0, ainfo->video_decoder,
								   0);
					ms_filter_link(ainfo->video_decoder, 0,
								   video_ctx.video_stitcher, k + 1);

#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
					ms_filter_link(video_ctx.video_tee, k,
								   ainfo->video_encoder, 0);
#else
					ms_filter_link(video_ctx.video_tee, k,
								   ainfo->video_sourcesizeconv, 0);
					ms_filter_link(ainfo->video_sourcesizeconv, 0,
								   ainfo->video_encoder, 0);
#endif
					ms_filter_link(ainfo->video_encoder, 0, ainfo->video_rtpsend,
								   0);
                    
#ifdef DISABLE_STITCHER
                    ms_ticker_attach(video_ctx.video_ticker, ainfo->video_rtprecv);
#endif
                    
				} else if (ainfo->video_decoder != NULL) {
					ms_filter_link(ainfo->video_rtprecv, 0, ainfo->video_decoder,
								   0);
					ms_filter_link(ainfo->video_decoder, 0,
								   video_ctx.video_stitcher, k + 1);
                    
#ifdef DISABLE_STITCHER
                    ms_ticker_attach(video_ctx.video_ticker, ainfo->video_rtprecv);
#endif
                    
				} else if (ainfo->video_encoder != NULL) {
#ifdef DISABLE_STITCHER_DONOTRESIZEOUTGOINGIMAGE
					ms_filter_link(video_ctx.video_tee, k,
								   ainfo->video_encoder, 0);
#else
					ms_filter_link(video_ctx.video_tee, k,
								   ainfo->video_sourcesizeconv, 0);
					ms_filter_link(ainfo->video_sourcesizeconv, 0,
								   ainfo->video_encoder, 0);
#endif
					ms_filter_link(ainfo->video_encoder, 0, ainfo->video_rtpsend,
								   0);
				}
				if (ainfo->video_ice != NULL)
					ms_ticker_attach(video_ctx.video_ticker,
									 ainfo->video_ice);
			}
		}

		ms_ticker_attach(video_ctx.video_ticker,
						 video_ctx.video_source);
	}

	return 0;
}

static int
_am_add_fmtp_parameter_encoders(am_call_t * ca, sdp_message_t * sdp_answer,
								PayloadType * pt)
{
	int pos;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	if (sdp_answer == NULL)
		return -1;

	if (ainfo->video_encoder == NULL)
		return -1;

	pos = 0;
	while (!osip_list_eol(&sdp_answer->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&sdp_answer->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "video")) {
			int p_number = -1;
			char payload_text[10];
			int pos3 = 0;

			/* find rtpmap number for this payloadtype */
			while (!osip_list_eol(&med->a_attributes, pos3)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&med->a_attributes,
													  pos3);
				if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
					/* search for each supported payload */
					int n;
					char codec[64];
					char subtype[64];
					char freq[64];

					if (attr->a_att_value == NULL
						|| strlen(attr->a_att_value) > 63) {
						pos3++;
						continue;	/* avoid buffer overflow! */
					}
					n = rtpmap_sscanf(attr->a_att_value, codec, subtype,
									  freq);
					if (n == 3) {
						if (0 == osip_strcasecmp(subtype, pt->mime_type)
							&& pt->clock_rate == atoi(freq)) {
							/* found payload! */
							p_number = atoi(codec);
							break;
						}

					}
				}
				pos3++;
			}

			if (p_number >= 0) {
				snprintf(payload_text, sizeof(payload_text), "%i ",
						 p_number);

				pos3 = 0;
				while (!osip_list_eol(&med->a_attributes, pos3)) {
					sdp_attribute_t *attr;

					attr =
						(sdp_attribute_t *) osip_list_get(&med->
														  a_attributes,
														  pos3);
					if (osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
						if (attr->a_att_value != NULL
							&& strlen(attr->a_att_value) >
							strlen(payload_text)
							&& osip_strncasecmp(attr->a_att_value,
												payload_text,
												strlen(payload_text)) ==
							0) {
							ms_filter_call_method(ainfo->video_encoder,
												  MS_FILTER_ADD_FMTP,
												  (void *) attr->
												  a_att_value);
						}
					} else if (osip_strcasecmp(attr->a_att_field, "ptime")
							   == 0) {
						char ptattr[256];

						memset(ptattr, 0, sizeof(ptattr));
						snprintf(ptattr, 256, "%s:%s", attr->a_att_field,
								 attr->a_att_value);
						ms_filter_call_method(ainfo->video_encoder,
											  MS_FILTER_ADD_ATTR,
											  (void *) ptattr);
					}
					pos3++;
				}
			}

			return p_number;
		} else {
			/* skip non video */
		}

		pos++;
	}

	return -1;
}


static int
_am_add_bandwidth_parameter_encoders(am_call_t * ca,
									 sdp_message_t * sdp_remote,
									 PayloadType * pt)
{
	int pos;
	int bd;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	if (sdp_remote == NULL)
		return -1;

	if (ainfo->video_encoder == NULL)
		return -1;

	bd = _antisipc.video_codec_attr.upload_bandwidth * 1000;
	ms_message("am_video_module.c: set default bandwidth for %s (%i).",
			   pt->mime_type, bd);

	pos = 0;
	while (!osip_list_eol(&sdp_remote->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&sdp_remote->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "video")) {
			int p_number = -1;
			int pos3 = 0;

			/* find rtpmap number for this payloadtype */
			while (!osip_list_eol(&med->a_attributes, pos3)) {
				sdp_attribute_t *attr;

				attr =
					(sdp_attribute_t *) osip_list_get(&med->a_attributes,
													  pos3);
				if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
					/* search for each supported payload */
					int n;
					char codec[64];
					char subtype[64];
					char freq[64];

					if (attr->a_att_value == NULL
						|| strlen(attr->a_att_value) > 63) {
						pos3++;
						continue;	/* avoid buffer overflow! */
					}
					n = rtpmap_sscanf(attr->a_att_value, codec, subtype,
									  freq);
					if (n == 3) {
						if (0 == osip_strcasecmp(subtype, pt->mime_type)
							&& pt->clock_rate == atoi(freq)) {
							/* found payload! */
							p_number = atoi(codec);
							break;
						}

					}
				}
				pos3++;
			}

			if (p_number >= 0) {
				pos3 = 0;
				while (!osip_list_eol(&med->b_bandwidths, pos3)) {
					sdp_bandwidth_t *bandwidth;

					bandwidth =
						(sdp_bandwidth_t *) osip_list_get(&med->
														  b_bandwidths,
														  pos3);
					if (osip_strcasecmp(bandwidth->b_bwtype, "AS") == 0) {
						int bitrate = atoi(bandwidth->b_bandwidth);

						if (bitrate <= 64)
							bitrate = 64;
						if (bitrate >= 8192)
							bitrate = 8192;
						bitrate = bitrate * 1000;
						if (bitrate < bd)
							bd = bitrate;
						ms_message
							("am_video_module.c: %s bandwidth limited to: %i.",
							 pt->mime_type, bd);
					}

					pos3++;
				}
			}

			if (bd > 8192000) //if (bd <= 1024000)
				bd = 8192000; // hard coded limitation?

			ms_filter_call_method(ainfo->video_encoder,
									MS_FILTER_SET_BITRATE, (void *) &bd);
			ainfo->video_encoder_bitrate=bd;
			return p_number;
		} else {
			/* skip non video */
		}

		pos++;
	}

	return -1;
}

static int
_am_prepare_video_decoders(am_call_t * ca, sdp_message_t * sdp,
						   sdp_media_t * med, int *decoder_payload)
{
	int pos2 = 0;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	*decoder_payload = -1;
	while (!osip_list_eol(&med->m_payloads, pos2)) {
		char *p_char = (char *) osip_list_get(&med->m_payloads, pos2);
		int p_number = atoi(p_char);

		int pos3 = 0;

		while (!osip_list_eol(&med->a_attributes, pos3)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&med->a_attributes,
												  pos3);
			if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
				/* search for each supported payload */
				int n;
				int payload = 0;
				char codec[64];
				char subtype[64];
				char freq[64];

				if (attr->a_att_value == NULL
					|| strlen(attr->a_att_value) > 63) {
					pos3++;
					continue;	/* avoid buffer overflow! */
				}
				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3)
					payload = atoi(codec);
				if (n == 3 && p_number == payload) {
					/* step 1: add profile */
					if (0 == osip_strcasecmp(subtype, "theora")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_theora));
					} else if (0 == osip_strcasecmp(subtype, "MP4V-ES")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_mp4v));
					} else if (0 == osip_strcasecmp(subtype, "h263-1998")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_h263_1998));
					} else if (0 == osip_strcasecmp(subtype, "h263")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_h263));
					} else if (0 == osip_strcasecmp(subtype, "JPEG")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_jpeg));
					} else if (0 == osip_strcasecmp(subtype, "h264")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_h264));
					} else if (0 == osip_strcasecmp(subtype, "vp8")) {
						rtp_profile_set_payload(ainfo->video_rtp_profile,
												payload,
												payload_type_clone
												(&payload_type_vp8));
					}

					/* step 2: create decoders: */
					if (ainfo->video_decoder == NULL) {
						PayloadType *pt;

						pt = rtp_profile_get_payload(ainfo->video_rtp_profile,
													 p_number);
						if (pt != NULL && pt->mime_type != NULL) {
							ainfo->video_decoder =
								ms_filter_create_decoder(pt->mime_type);
							if (ainfo->video_decoder != NULL) {
								/* all fine! */
								*decoder_payload = p_number;
								OSIP_TRACE(osip_trace
										   (__FILE__, __LINE__,
											OSIP_WARNING, NULL,
											"Decoder created for payload=%i %s\n",
											p_number, pt->mime_type));
							}
						}
					}
				}
			}
			pos3++;
		}
		pos2++;
	}

	return 0;
}

static int _am_complete_decoders_configuration(am_call_t * ca,
											   sdp_message_t * sdp_remote,
											   sdp_media_t * med)
{
	int payload = -1;
	int pos3;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return -1;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return -1;
	}

	if (ainfo->video_decoder == NULL)
		return 0;
		
	if (osip_strncasecmp(ainfo->video_decoder->desc->enc_fmt, "h264", 4)!=0)
		return 0;

	pos3=0;
	while (!osip_list_eol(&med->a_attributes, pos3)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&med->a_attributes,
											  pos3);
		if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
			/* search for each supported payload */
			int n;
			char codec[64];
			char subtype[64];
			char freq[64];

			if (attr->a_att_value == NULL
				|| strlen(attr->a_att_value) > 63) {
				pos3++;
				continue;	/* avoid buffer overflow! */
			}
			n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
			if (n == 3)
				payload = atoi(codec);
			if (n == 3) {
				if (0 == osip_strcasecmp(subtype, "h264")) {
					/* found! */
					OSIP_TRACE(osip_trace
							   (__FILE__, __LINE__,
								OSIP_WARNING, NULL,
								"H264 encoder rtpmap found payload=%i\n", payload));
					break;
				}
			}
		}
		payload=-1;
		pos3++;
	}

	if (payload==-1)
		return 0;

	pos3=0;
	while (!osip_list_eol(&med->a_attributes, pos3)) {
		sdp_attribute_t *attr;

		attr =
			(sdp_attribute_t *) osip_list_get(&med->a_attributes,
											  pos3);
		if (osip_strcasecmp(attr->a_att_field, "fmtp") == 0) {
			if (attr->a_att_value != NULL
				&& payload == atoi(attr->a_att_value)) {
				ms_filter_call_method(ainfo->video_decoder,
									  MS_FILTER_ADD_FMTP,
									  (void *) attr->
									  a_att_value);
			}
		}
		pos3++;
	}
	return 0;
}

static PayloadType *_am_prepare_video_encoder(am_call_t * ca,
											  sdp_message_t * sdp_remote,
											  sdp_media_t * med,
											  int *encoder_payload)
{
	int pos2 = 0;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return NULL;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return NULL;
	}

	*encoder_payload = -1;
	/* use first supported remote payload */
	while (!osip_list_eol(&med->m_payloads, pos2)) {
		char *p_char = (char *) osip_list_get(&med->m_payloads, pos2);
		int p_number = atoi(p_char);
		PayloadType *pt = NULL;
		int found_rtpmap = 0;

		int pos3 = 0;

		while (!osip_list_eol(&med->a_attributes, pos3)) {
			sdp_attribute_t *attr;

			attr =
				(sdp_attribute_t *) osip_list_get(&med->a_attributes,
												  pos3);
			if (osip_strcasecmp(attr->a_att_field, "rtpmap") == 0) {
				/* search for each supported payload */
				int n;
				int payload = 0;
				char codec[64];
				char subtype[64];
				char freq[64];

				if (attr->a_att_value == NULL
					|| strlen(attr->a_att_value) > 63) {
					pos3++;
					continue;	/* avoid buffer overflow! */
				}
				n = rtpmap_sscanf(attr->a_att_value, codec, subtype, freq);
				if (n == 3)
					payload = atoi(codec);
				if (n == 3 && p_number == payload) {
					PayloadType *pt_index;
					char rtpmap_tmp[128];

					found_rtpmap = 1;
					memset(rtpmap_tmp, '\0', sizeof(rtpmap_tmp));
					snprintf(rtpmap_tmp, sizeof(rtpmap_tmp), "%s/%s",
							 subtype, freq);

					/* rtp_profile are added with numbers that comes from the
					   local_sdp: the current p_number could be different!! */

					/* create an encoder: pt will exist if a decoder was found
					   or if it's H263 */
					pt_index =
						rtp_profile_get_payload(ainfo->video_rtp_profile,
												p_number);
					pt = rtp_profile_get_payload_from_rtpmap(ainfo->
															 video_rtp_profile,
															 rtpmap_tmp);
					if (pt_index != pt && pt != NULL) {
						OSIP_TRACE(osip_trace
								   (__FILE__, __LINE__, OSIP_WARNING, NULL,
									"Payload for %s is different in offer(%i) and answer(%i)\n",
									rtpmap_tmp, -1, p_number));
					}

					if (pt != NULL && pt->mime_type != NULL) {
						if (ca->local_sendrecv != _RECVONLY) {
							ainfo->video_encoder =
								ms_filter_create_encoder(pt->mime_type);
							if (ainfo->video_encoder != NULL) {
								*encoder_payload = p_number;
								return pt;
							}
						} else {
							*encoder_payload = p_number;
							return pt;
						}
					}
					break;
				}
			}
			pos3++;
		}

		if (found_rtpmap == 0) {
			/* missing rtpmap attribute in remote_sdp!!! */
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
								  "Missing rtpmap attribute in remote SDP for payload=%i\n",
								  p_number));
			pt = NULL;
			if (p_number == 34) {
				pt = rtp_profile_get_payload(ainfo->video_rtp_profile,
											 p_number);
			}

			if (pt != NULL && pt->mime_type != NULL) {
				if (ca->local_sendrecv != _RECVONLY) {
					ainfo->video_encoder =
						ms_filter_create_encoder(pt->mime_type);
					if (ainfo->video_encoder != NULL) {
						*encoder_payload = p_number;
						return pt;
					}
				} else {
					*encoder_payload = p_number;
					return pt;
				}
			}
		}

		pos2++;
	}

	return NULL;
}

static PayloadType *_am_prepare_coders(am_call_t * ca,
									   sdp_message_t * sdp_answer,
									   sdp_message_t * sdp_offer,
									   sdp_message_t * local_sdp,
									   sdp_message_t * remote_sdp,
									   char *remote_ip,
									   int *decoder_payload,
									   int *encoder_payload)
{
	int pos;
	struct am_video_info *ainfo = NULL;
	if (ca == NULL)
		return NULL;
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return NULL;
	}

	*decoder_payload = -1;
	*encoder_payload = -1;
	if (sdp_answer == NULL)
		return NULL;
	if (sdp_offer == NULL)
		return NULL;
	if (remote_ip == NULL)
		return NULL;

	pos = 0;
	while (!osip_list_eol(&local_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&local_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "video")) {
			_am_prepare_video_decoders(ca, local_sdp, med,
									   decoder_payload);
		} else {
			/* skip non video */
		}

		pos++;
	}

	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "video")) {
			_am_complete_decoders_configuration(ca, remote_sdp, med);
		} else {
			/* skip non video */
		}

		pos++;
	}

	/* find encoder that we do support AND offer */
	pos = 0;
	while (!osip_list_eol(&remote_sdp->m_medias, pos)) {
		sdp_media_t *med;

		med = (sdp_media_t *) osip_list_get(&remote_sdp->m_medias, pos);

		if (0 == osip_strcasecmp(med->m_media, "video")) {
			PayloadType *pt =
				_am_prepare_video_encoder(ca, remote_sdp, med,
										  encoder_payload);
			if (pt != NULL && ainfo->video_encoder != NULL) {
				/* step 3: set fmtp parameters */
				_am_add_fmtp_parameter_encoders(ca, sdp_answer, pt);
				/* when payload is not supported in sdp_answer, we might
				   want to set encoder parameter from sdp_offer... */
				_am_add_bandwidth_parameter_encoders(ca, remote_sdp, pt);
			}
			return pt;
		} else {
			/* skip non video */
		}
		pos++;
	}

	return NULL;
}


static int video_module_session_init(am_call_t * ca, int idx)
{
	struct am_video_info *ainfo;
    
    ca->video_ctx=NULL;
    if (idx>=STITCHER_MAX_INPUTS || idx>=MAX_NUMBER_OF_CALLS)
        return -1;
	ca->video_ctx = &video_ctx.ctx[idx];

	ainfo = ca->video_ctx;
	ainfo->video_encoder_bitrate=_antisipc.video_codec_attr.upload_bandwidth * 1000;
	return 0;
}

static int video_module_session_release(am_call_t * ca)
{
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	ainfo->video_encoder_bitrate=0;
	if (ainfo->video_rtp_session != NULL)
		rtp_session_destroy(ainfo->video_rtp_session);
	ainfo->video_rtp_session=NULL;

	ca->video_ctx = NULL;
	return 0;
}

static int video_module_session_close(am_call_t * ca)
{
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	OSIP_TRACE(osip_trace
			   (__FILE__, __LINE__, OSIP_INFO2, NULL,
				"video_module_session_close - RTP stream cid=%i did=%i\n",
				ca->cid, ca->did));

	ca->enable_video = -1;
	/* stop mediastreamer thread */

	/* stop mediastreamer session */

	/* print statistics */
	if (ca != NULL && ainfo->video_rtp_session != NULL)
		rtp_stats_display(&ainfo->video_rtp_session->rtp.stats,
						  "end of video session");

	if (ca != NULL) {
		am_ms2_video_detach(ca);
		video_stream_graph_reset(ca);
	}

	if (ainfo->video_rtp_profile != NULL) {
		rtp_profile_destroy(ainfo->video_rtp_profile);
		ainfo->video_rtp_profile = NULL;
	}
	return 0;
}

static int video_module_session_start(am_call_t * ca, sdp_message_t * sdp_answer,
						 sdp_message_t * sdp_offer,
						 sdp_message_t * sdp_local,
						 sdp_message_t * sdp_remote, int local_port,
						 char *remote_ip, int remote_port,
						 int setup_passive)
{
	PayloadType *pt;
	int encoder_payload = -1;
	int decoder_payload = -1;
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ca->p_am_sessiontype[0]!='\0' && strstr(ca->p_am_sessiontype, "video")==NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "video disabled using P-AM-ST private header\n"));
		return AMSIP_WRONG_STATE;
	}

	OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
						  "video_module_session_start - RTP stream cid=%i did=%i\n",
						  ca->cid, ca->did));

	if (ca->video_local_sendrecv == _INACTIVE) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO2, NULL,
							  "video_module_session_start - RTP stream is inactive cid=%i did=%i\n",
							  ca->cid, ca->did));
		return -1;
	}

#if !defined(KEEP_RTP_SESSION)
	ainfo->video_rtp_session = NULL;
#endif

	if (ca->video_candidate.stun_candidates[0].conn_port > 0)
		local_port = ca->video_candidate.stun_candidates[0].rel_port;
	if (ca->video_candidate.relay_candidates[0].conn_port > 0)
		local_port = ca->video_candidate.relay_candidates[0].rel_port;


	/* open mediastreamer session */
	ainfo->video_rtp_profile =
		(RtpProfile *) rtp_profile_new("RtpProfileVideo");
	pt = _am_prepare_coders(ca, sdp_answer, sdp_offer, sdp_local,
							sdp_remote, remote_ip, &decoder_payload,
							&encoder_payload);

	ainfo->video_rtp_session =
		am_create_duplex_rtpsession(ca, local_port, remote_ip, remote_port,
									50);

	if (!ainfo->video_rtp_session) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "Could not initialize video session for %s port %d\n",
							  remote_ip, remote_port));
		if (ca->video_local_sendrecv == _SENDONLY) {
		}
		video_module_session_close(ca);
		return -1;
	}
#ifndef DISABLE_SRTP
	am_get_security_descriptions(ainfo->video_rtp_session,
								 sdp_answer, sdp_offer,
								 sdp_local, sdp_remote, "video");
#endif
	if (ainfo->video_rtp_session->rtp.tr==NULL
		&& _antisipc.enable_zrtp > 0) {
		/* ZRTP */
		RtpTransport *rtpt;
		RtpTransport *rtcpt;
		rtpt = ortp_transport_new("ZRTP");
		if (rtpt == NULL) {
			OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
				"oRTP compiled without ZRTP support\n"));
		} else {
      rtcpt = ortp_transport_new("ZRTP");
      rtp_session_set_transports(ainfo->video_rtp_session, rtpt, rtcpt );
      ortp_transport_set_option(rtpt, 1, &ca->call_direction);
      ortp_transport_set_option(rtcpt, 1, &ca->call_direction);
    }
	}

	/* RTP */
	if (encoder_payload >= 0)
		rtp_session_set_send_payload_type(ainfo->video_rtp_session,
										  encoder_payload);
	if (encoder_payload < 0 && decoder_payload >= 0)
		rtp_session_set_send_payload_type(ainfo->video_rtp_session,
										  decoder_payload);
	if (decoder_payload >= 0)
		rtp_session_set_recv_payload_type(ainfo->video_rtp_session,
										  decoder_payload);
	if (encoder_payload >= 0 && decoder_payload < 0)
		rtp_session_set_send_payload_type(ainfo->video_rtp_session,
										  encoder_payload);

	if (ca->local_sendrecv == _SENDONLY) {
		/* send a wav file!  ca->wav_file */
		video_stream_start_send_only(ca, pt);
	} else if (ca->local_sendrecv == _RECVONLY) {
		video_stream_start_recv_only(ca, pt);
	} else {
		video_stream_start_send_recv(ca, pt);
	}

	if (ainfo->video_rtp_session == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "Not able to start video session\n"));
		video_module_session_close(ca);
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_INFO1, NULL,
							  "Video session stopped\n"));
		return -1;
	}
	/* set sdes (local user) info */

	/* check if symmetric RTP is required
	   with antisip to antisip softphone, we use STUN connectivity check
	 */
	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0]=='\0') {
		if (_antisipc.do_symmetric_rtp == 1) {
			rtp_session_set_symmetric_rtp(ainfo->video_rtp_session, 1);
		}
	}

	ca->enable_video = 1;
	/* should start here the mediastreamer thread */

	am_ms2_video_attach(ca);

	return 0;
}

static int video_module_session_get_bandwidth_statistics(am_call_t * ca,
											  struct am_bandwidth_stats
											  *band_stats)
{
	struct am_video_info *ainfo = ca->video_ctx;
	memset(band_stats, 0, sizeof(struct am_bandwidth_stats));
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ca->enable_video <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "video not started for this call (%i:val=%i) : no available statistics\n",
							  ca->did, ca->enable_video));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->video_rtp_session != NULL) {
		RtpStream *rtpstream;
		rtp_stats_t *stats;

		ms_mutex_lock(&video_ctx.video_ticker->lock);
		rtpstream = &ainfo->video_rtp_session->rtp;
		stats = &rtpstream->stats;
		rtp_stats_display(stats, "amsip video statistics");

		band_stats->incoming_received = (int) stats->packet_recv;
		band_stats->incoming_expected = (int) stats->packet_recv;
		band_stats->incoming_packetloss = (int) stats->cum_packet_loss;
		band_stats->incoming_outoftime = (int) stats->outoftime;
		band_stats->incoming_notplayed = 0;
		band_stats->incoming_discarded = (int) stats->discarded;

		band_stats->outgoing_sent = (int) stats->packet_sent;

		band_stats->download_rate =
			rtp_session_compute_recv_bandwidth(ainfo->video_rtp_session);
		band_stats->upload_rate =
			rtp_session_compute_send_bandwidth(ainfo->video_rtp_session);


#ifndef DISABLE_MSDRAW
		if (video_ctx.video_msdraw!=NULL) {
			static MSDrawObject *text=NULL;
			char text_data[256];
			snprintf(text_data, sizeof(text_data),
				"down: %0.2fKb/s/up: %0.2fKb/s",
				band_stats->download_rate*1e-3,
				band_stats->upload_rate*1e-3);
			if (text!=NULL)
			{
				ms_filter_call_method(video_ctx.video_msdraw,MS_DRAW_UNSET_OBJECT,&text->common.id);
				text=NULL;
			}
			text=ms_draw_text_new(text_data,15,95);
			ms_draw_text_set_color(text,255,0,0);
			ms_draw_text_set_font_size(text, 16);
			ms_filter_call_method(video_ctx.video_msdraw,MS_DRAW_SET_OBJECT,text);
		}
#endif

		ms_mutex_unlock(&video_ctx.video_ticker->lock);
	}

	return AMSIP_SUCCESS;
}

static OrtpEvent *video_module_session_get_video_rtp_events(am_call_t * ca)
{
	OrtpEvent *evt;
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return NULL;
	}

	if (ca->enable_video <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"video not started for this call (%i:val=%i) : no available statistics\n",
			ca->did, ca->enable_video));
		return NULL;
	}

	if (ainfo->video_rtp_session == NULL) {
		return NULL;	/* no RTP session? */
	}
	if (ainfo->video_rtp_queue == NULL) {
		return NULL;	/* no video queue for RTCP? */
	}
	ms_mutex_lock(&video_ctx.video_ticker->lock);

	evt = ortp_ev_queue_get(ainfo->video_rtp_queue);

	ms_mutex_unlock(&video_ctx.video_ticker->lock);

	return evt;
}

static int
video_module_session_set_video_zrtp_sas_verified (am_call_t *ca)
{
	int verified=1;
	struct am_video_info *ainfo = (struct am_video_info *)ca->audio_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ca->enable_video <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "video not started for this call (%i:val=%i) : no available statistics\n",
							  ca->did, ca->enable_video));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->video_rtp_session == NULL) {
		return AMSIP_WRONG_STATE;	/* no RTP session? */
	}
	if (ainfo->video_rtp_queue == NULL) {
		return AMSIP_WRONG_STATE;	/* no video queue for RTCP? */
	}
	if (ainfo->video_rtp_session->rtp.tr == NULL) {
		return AMSIP_WRONG_STATE;	/* no transport? */
	}
	ms_mutex_lock(&video_ctx.video_ticker->lock);

	if (strcmp(ainfo->video_rtp_session->rtp.tr->name, "ZRTP")==0) {
		ortp_transport_set_option(ainfo->video_rtp_session->rtp.tr, 2, &verified);
	}

	ms_mutex_unlock(&video_ctx.video_ticker->lock);

	return AMSIP_SUCCESS;
}

static int video_module_session_send_vfu(am_call_t * ca)
{
	int i = AMSIP_UNDEFINED_ERROR;
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ca->enable_video <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"video not started for this call (%i:val=%i) : no available statistics\n",
			ca->did, ca->enable_video));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->video_encoder != NULL) {
		ms_mutex_lock(&video_ctx.video_ticker->lock);
		i = ms_filter_call_method_noarg(ainfo->video_encoder,
										MS_FILTER_REQ_VFU);
		ms_mutex_unlock(&video_ctx.video_ticker->lock);
	}

	return i;
}

static int video_module_session_adapt_video_bitrate(am_call_t * ca, float lost)
{
	struct am_video_info *ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	if (ca->enable_video <= 0) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
			"video not started for this call (%i:val=%i) : cannot adapt video bitrate\n",
			ca->did, ca->enable_video));
		return AMSIP_WRONG_STATE;
	}

	if (ainfo->video_encoder != NULL) {
		ms_mutex_lock(&video_ctx.video_ticker->lock);

		if (lost>10){
			int bitrate=0;
			int new_bitrate;
			
			ms_filter_call_method(ainfo->video_encoder,MS_FILTER_GET_BITRATE,&bitrate);
			if (bitrate==0){
				ms_error("Video encoder does not implement MS_FILTER_GET_BITRATE.");
				ms_mutex_unlock(&video_ctx.video_ticker->lock);
				return AMSIP_UNDEFINED_ERROR;
			}
			if (bitrate>=20000){
				MSVideoSize vsize;
				float fps = 15;
				new_bitrate=(int)(bitrate*90/100);
				ms_warning("Encoder bitrate reduced from %i to %i b/s.",bitrate,new_bitrate);
				ms_filter_call_method(ainfo->video_encoder,MS_FILTER_SET_BITRATE,&new_bitrate);

				vsize.height = MS_VIDEO_SIZE_CIF_H;
				vsize.width = MS_VIDEO_SIZE_CIF_W;
				ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
									  &vsize);
				ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_FPS, &fps);

				ms_filter_call_method(ainfo->video_sourcesizeconv, MS_FILTER_SET_FPS,
									  &fps);
				ms_filter_call_method(ainfo->video_sourcesizeconv,
									  MS_FILTER_SET_VIDEO_SIZE, &vsize);
			}else{
				ms_warning("Video encoder bitrate already at minimum.");
			}
			
		}
		ms_mutex_unlock(&video_ctx.video_ticker->lock);
	}

	return AMSIP_SUCCESS;
}


struct video_module ms2_video_module = {
	"MS2VIDEO",					/* module name */
	"Mediastreamer2 video default module",	/* descriptive text */
	"video",					/* media type */

	video_module_init,
	video_module_reset,
	video_module_quit,

	video_module_find_camera,
	video_module_select_camera,
	video_module_set_input_video_size,
	video_module_set_window_display,
	video_module_set_window_handle,
	video_module_set_window_preview_handle,
	video_module_set_nowebcam,
	video_module_enable_preview,
	video_module_set_image_callback,

	video_module_set_selfview_mode,
	video_module_set_selfview_position,
	video_module_get_selfview_position,
	video_module_set_selfview_scalefactor,
	video_module_set_background_color,
	video_module_set_video_option,

	video_module_set_callback,

	video_module_session_init,
	video_module_session_release,
	video_module_session_start,
	video_module_session_close,

	video_module_session_get_bandwidth_statistics,
	video_module_session_get_video_rtp_events,
	video_module_session_set_video_zrtp_sas_verified,
	video_module_session_send_vfu,
	video_module_session_adapt_video_bitrate,
};

#endif
