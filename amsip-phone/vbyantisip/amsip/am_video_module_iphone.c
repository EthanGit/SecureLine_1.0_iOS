/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Aymeric MOIZARD - <jack@atosc.org>
*/

#ifdef ENABLE_VIDEO

#include "../../amsip/amsip/src/am_calls.h"

#include <ortp/ortp.h>
#include <ortp/telephonyevents.h>


#ifndef DISABLE_SRTP
#include <ortp/srtp.h>
#endif

#define DISABLE_STITCHER

#define DISABLE_MSDRAW

#include "mediastreamer2/msrtp.h"

#ifndef DISABLE_MSDRAW
#include "mediastreamer2/msdraw.h"
#endif

#include <osip2/osip_time.h>
#include <osip2/osip_mt.h>

#include "../../amsip/amsip/src/sdptools.h"

#define KEEP_RTP_SESSION

struct am_video_info {
	RtpSession *video_rtp_session;
	OrtpEvQueue *video_rtp_queue;

#ifndef DISABLE_SRTP
	srtp_t video_srtp;
#endif
	RtpProfile *video_rtp_profile;
	MSFilter *source_pixconv;
	MSFilter *source_sizeconv;
	MSFilter *video_source;
	MSFilter *video_encoder;
	int video_encoder_bitrate;
	MSFilter *video_decoder;
	MSFilter *video_display;

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

	struct am_video_info ctx[MAX_NUMBER_OF_CALLS];
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
};

#ifndef DISABLE_SRTP
int
am_get_security_descriptions(srtp_t * srtp, RtpSession * rtp,
							 sdp_message_t * sdp_answer,
							 sdp_message_t * sdp_offer,
							 sdp_message_t * sdp_local,
							 sdp_message_t * sdp_remote, char *media_type);
#endif

int iphone_add_selfview_image(int did, int width, int height, int format, mblk_t *data)
{
	struct am_video_info *ainfo = NULL;
	am_call_t *ca;
	
	eXosip_lock();
	ca = _am_calls_find_audio_connection(-2, did);
	
	if (ca == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_ERROR, NULL,
							  "no call with did (%i)\n",
							  did));
		return AMSIP_NOTFOUND;
	}
	
	ainfo = ca->video_ctx;
	if (ainfo == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_NOTFOUND;
	}
	
	if (ainfo->video_source == NULL) {
		eXosip_unlock();
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video source\n"));
		return AMSIP_NOTFOUND;
	}

	MSVideoSize vsize;
	vsize.width=width;
	vsize.height=height;
	ms_filter_call_method(ainfo->source_pixconv, MS_FILTER_SET_PIX_FMT, &format);
	ms_filter_call_method(ainfo->source_pixconv, MS_FILTER_SET_VIDEO_SIZE, &vsize);
	ms_filter_call_method(ainfo->video_source, MS_FILTER_SET_DATA, data);
	eXosip_unlock();
	return 0;
}

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

	if (video_ctx.video_ticker == NULL)
	  video_ctx.video_ticker = ms_ticker_new();
	ms_ticker_set_name(video_ctx.video_ticker, "amsip-video");

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

	if (video_ctx.video_ticker == NULL)
	  video_ctx.video_ticker = ms_ticker_new();
	ms_ticker_set_name(video_ctx.video_ticker, "amsip-video");
	return 0;
}

static int video_module_quit()
{
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
	const MSList *elem =
		ms_web_cam_manager_get_list(ms_web_cam_manager_get());
	int k = 0;

	if (card > 20)
		return -1;
	if (card <= 0)
		card = 0;

	for (; elem != NULL; elem = elem->next) {
		MSWebCam *adevice = (MSWebCam *) elem->data;

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
		|| (MS_VIDEO_SIZE_QCIF_W == width
			&& MS_VIDEO_SIZE_QCIF_H == height)
		|| (MS_VIDEO_SIZE_VGA_W == width && MS_VIDEO_SIZE_VGA_H == height)
		|| (MS_VIDEO_SIZE_4CIF_W == width
			&& MS_VIDEO_SIZE_4CIF_H == height)) {
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
	video_ctx.prefered_screen_size.width = width;
	video_ctx.prefered_screen_size.height = height;

	return 0;
}

static int video_module_set_window_handle(long handle, int width, int height)
{

	video_ctx.prefered_screen_size.width = width;
	video_ctx.prefered_screen_size.height = height;

	return 0;
}

static int video_module_set_window_preview_handle(long handle, int width, int height)
{
  return -1;
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

static int video_module_enable_preview(int enable)
{
  return 0;
}

static int video_module_set_selfview_mode(int mode)
{
	return -1;
}

static int video_module_set_selfview_position(float posx, float posy, float size)
{
	return -1;
}

static int video_module_get_selfview_position(float *posx, float *posy, float *size)
{
	return -1;
}

static int video_module_set_selfview_scalefactor(float scalefactor)
{
	return -1;
}

static int video_module_set_background_color(int red, int green, int blue)
{
	return -1;
}

static int video_module_set_video_option(int opt, void *arg)
{
	if(opt==AMSIP_OPTION_WEBCAM_FPS)
	{
		video_ctx.option_webcam_fps = *((float*)arg);
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

					if (tmpinfo!=NULL && tmpinfo->video_decoder!=NULL && tmpinfo->video_display!=NULL)
					{
						ms_filter_unlink(tmpinfo->video_rtprecv, 0,
										 tmpinfo->video_decoder, 0);
						ms_filter_unlink(tmpinfo->video_decoder, 0,
										 tmpinfo->video_display, 0);

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

						ms_filter_link(tmpinfo->video_rtprecv, 0, tmpinfo->video_decoder, 0);
						ms_filter_link(tmpinfo->video_decoder, 0, tmpinfo->video_display, 0);
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
#ifndef DISABLE_SRTP
	if (ainfo->video_srtp != NULL)
	{
		if (ainfo->video_rtp_session != NULL)
		{
			RtpTransport *rtptr = ainfo->video_rtp_session->rtp.tr;
			RtpTransport *rtcptr = ainfo->video_rtp_session->rtcp.tr;
			if (rtptr!=NULL)
				ortp_free(rtptr);
			if (rtcptr!=NULL)
				ortp_free(rtcptr);
			rtp_session_set_transports(ainfo->video_rtp_session, NULL, NULL);
		}
	}
#endif
#if !defined(KEEP_RTP_SESSION)
	if (ainfo->video_rtp_session != NULL)
		rtp_session_destroy(ainfo->video_rtp_session);
#endif
	if (ainfo->video_rtp_queue != NULL)
		ortp_ev_queue_destroy(ainfo->video_rtp_queue);
	if (ainfo->video_encoder != NULL)
		ms_filter_destroy(ainfo->video_encoder);
	if (ainfo->video_source != NULL)
		ms_filter_destroy(ainfo->video_source);
	if (ainfo->source_pixconv != NULL)
		ms_filter_destroy(ainfo->source_pixconv);
	if (ainfo->source_sizeconv != NULL)
		ms_filter_destroy(ainfo->source_sizeconv);
	if (ainfo->video_decoder != NULL)
		ms_filter_destroy(ainfo->video_decoder);
	if (ainfo->video_display != NULL)
		ms_filter_destroy(ainfo->video_display);
	if (ainfo->video_rtpsend != NULL)
		ms_filter_destroy(ainfo->video_rtpsend);
	if (ainfo->video_rtprecv != NULL)
		ms_filter_destroy(ainfo->video_rtprecv);
	if (ainfo->video_ice != NULL)
		ms_filter_destroy(ainfo->video_ice);

#ifndef DISABLE_SRTP
	if (ainfo->video_srtp != NULL)
		ortp_srtp_dealloc(ainfo->video_srtp);
#endif

#if !defined(KEEP_RTP_SESSION)
	ainfo->video_rtp_session = NULL;
#endif
	ainfo->video_rtp_queue=NULL;
	ainfo->video_encoder = NULL;
	ainfo->video_source = NULL;
	ainfo->source_pixconv = NULL;
	ainfo->source_sizeconv = NULL;
	ainfo->video_decoder = NULL;
	ainfo->video_display = NULL;
	ainfo->video_rtpsend = NULL;
	ainfo->video_rtprecv = NULL;
	ainfo->video_ice = NULL;
#ifndef DISABLE_SRTP
	ainfo->video_srtp = NULL;
#endif
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

	ainfo->video_display = ms_filter_new(MS_VIDEO_OUT_ID);

	ainfo->video_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ainfo->video_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

	ainfo->source_pixconv =  ms_filter_new(MS_PIX_CONV_ID);
	ainfo->source_sizeconv =  ms_filter_new(MS_SIZE_CONV_ID);
	format = MS_NV21;
	ms_filter_call_method(ainfo->source_pixconv, MS_FILTER_SET_PIX_FMT,
			      &format);
	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->source_pixconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);
	
	ainfo->video_source = ms_filter_new(MS_V4L_ID);
	
	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
		ainfo->video_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->video_ice != NULL) {
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->video_checklist);
		}
	}

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


	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
						  &vsize);
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_FPS, &fps);

	ms_filter_call_method(ainfo->source_sizeconv, MS_FILTER_SET_FPS,
			      &fps);
	ms_filter_call_method(ainfo->source_sizeconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);
	ms_filter_call_method(ainfo->source_pixconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);

	if (pt == NULL)
		ms_message
			("am_video_module.c: (NULL) fps: %f video encoder/sizeconverter size:%i/%i",
			 fps, vsize.height, vsize.width);
	else
		ms_message
			("am_video_module.c: %s fps: %f video encoder/sizeconverter size:%i/%i",
			 pt->mime_type, fps, vsize.height, vsize.width);

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
	MSPixFmt format;
	MSVideoSize vsize;
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

	ainfo->video_display = ms_filter_new(MS_VIDEO_OUT_ID);

	ainfo->video_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ainfo->video_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

	ainfo->source_pixconv =  ms_filter_new(MS_PIX_CONV_ID);
	ainfo->source_sizeconv =  ms_filter_new(MS_SIZE_CONV_ID);
	format = MS_NV21;
	ms_filter_call_method(ainfo->source_pixconv, MS_FILTER_SET_PIX_FMT,
			      &format);
	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->source_pixconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);

	ainfo->video_source = ms_filter_new_from_name("socket_reader");

	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
		ainfo->video_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->video_ice != NULL) {
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->video_checklist);
		}
	}

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

	ainfo->video_rtpsend = ms_filter_new(MS_RTP_SEND_ID);
	ms_filter_call_method(ainfo->video_rtpsend, MS_RTP_SEND_SET_SESSION,
						  rtps);
	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

	ainfo->source_pixconv =  ms_filter_new(MS_PIX_CONV_ID);
	ainfo->source_sizeconv =  ms_filter_new(MS_SIZE_CONV_ID);
	format = MS_NV21;
	ms_filter_call_method(ainfo->source_pixconv, MS_FILTER_SET_PIX_FMT,
			      &format);
	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->source_pixconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);

	ainfo->video_source = ms_filter_new_from_name("socket_reader");

	if (ca->video_checklist.cand_pairs[0].remote_candidate.conn_addr[0] !=
		'\0') {
		ainfo->video_ice = ms_filter_new(MS_ICE_ID);
		if (ainfo->video_ice != NULL) {
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_SESSION, rtps);
			ms_filter_call_method(ainfo->video_ice, MS_ICE_SET_CANDIDATEPAIRS,
								  &ca->video_checklist);
		}
	}

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

	vsize.height = MS_VIDEO_SIZE_CIF_H;
	vsize.width = MS_VIDEO_SIZE_CIF_W;
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_VIDEO_SIZE,
						  &vsize);
	ms_filter_call_method(ainfo->video_encoder, MS_FILTER_GET_FPS, &fps);

	ms_filter_call_method(ainfo->source_sizeconv, MS_FILTER_SET_FPS,
			      &fps);
	ms_filter_call_method(ainfo->source_sizeconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);
	ms_filter_call_method(ainfo->source_pixconv,
			      MS_FILTER_SET_VIDEO_SIZE, &vsize);

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
	if (video_ctx.video_ticker != NULL) {
	  /* detach current calls */
	  if (ainfo->video_ice != NULL)
	    ms_ticker_detach(video_ctx.video_ticker,
			     ainfo->video_ice);
	  
	  
	  if (ainfo->video_decoder != NULL && ainfo->video_display!=NULL) {
	    
	    ms_ticker_detach(video_ctx.video_ticker,
			     ainfo->video_rtprecv);
	    
	    ms_filter_unlink(ainfo->video_rtprecv, 0,
			     ainfo->video_decoder, 0);
	    ms_filter_unlink(ainfo->video_decoder, 0,
			     ainfo->video_display, 0);
	  }
	  
	  if (ainfo->video_encoder != NULL) {

	    ms_ticker_detach(video_ctx.video_ticker,
			     ainfo->video_source);
	    
	    ms_filter_unlink(ainfo->video_source, 0,
			   ainfo->source_pixconv, 0);
	    ms_filter_unlink(ainfo->source_pixconv, 0,
			   ainfo->source_sizeconv, 0);
	    ms_filter_unlink(ainfo->source_sizeconv, 0,
			   ainfo->video_encoder, 0);	  
	    ms_filter_unlink(ainfo->video_encoder, 0,
			     ainfo->video_rtpsend, 0);
	  }
	}

	return 0;
}

static int am_ms2_video_attach(am_call_t * call_to_attach)
{
	struct am_video_info *ainfo = call_to_attach->video_ctx;
	if (ainfo == NULL) {
		OSIP_TRACE(osip_trace(__FILE__, __LINE__, OSIP_WARNING, NULL,
							  "No video ctx\n"));
		return AMSIP_WRONG_STATE;
	}

	/* detach preview */
	if (video_ctx.video_ticker == NULL) {
	  return -1;
	}

	if (ainfo->video_decoder != NULL && ainfo->video_display!=NULL) {
	  ms_filter_link(ainfo->video_rtprecv, 0,
			 ainfo->video_decoder, 0);
	  ms_filter_link(ainfo->video_decoder, 0,
			 ainfo->video_display, 0);
	}
	  
	if (ainfo->video_encoder != NULL) {
	  ms_filter_link(ainfo->video_source, 0,
			 ainfo->source_pixconv, 0);
	  ms_filter_link(ainfo->source_pixconv, 0,
			   ainfo->source_sizeconv, 0);
	  ms_filter_link(ainfo->source_sizeconv, 0,
			 ainfo->video_encoder, 0);	  
	  ms_filter_link(ainfo->video_encoder, 0,
			 ainfo->video_rtpsend, 0);
	}

	if (ainfo->video_decoder != NULL) {
	  ms_ticker_attach(video_ctx.video_ticker,
			   ainfo->video_rtprecv);
	}
	  
	if (ainfo->video_encoder != NULL) {
	  ms_ticker_attach(video_ctx.video_ticker,
			   ainfo->video_source);
	}

	if (ainfo->video_ice != NULL)
	  ms_ticker_attach(video_ctx.video_ticker,
			   ainfo->video_ice);

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
						if (bitrate >= 1024)
							bitrate = 1024;
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

			if (bd <= 1024000)
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
	am_get_security_descriptions(&ainfo->video_srtp, ainfo->video_rtp_session,
								 sdp_answer, sdp_offer,
								 sdp_local, sdp_remote, "video");
#endif

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
	if (osip_strncasecmp(ca->remote_useragent, "antisip/", 8) != 0
		&& NULL == strstr(ca->remote_useragent, "amsip")) {
		if (_antisipc.do_symmetric_rtp == 1) {
			rtp_session_set_symmetric_rtp(ainfo->video_rtp_session, 1);
		}
	} else if (_antisipc.use_turn_server == 0) {
		if (_antisipc.do_symmetric_rtp == 1) {
			rtp_session_set_symmetric_rtp(ainfo->video_rtp_session, 1);
		}
	}

	ca->enable_video = 1;
	/* should start here the mediastreamer thread */

	am_ms2_video_attach(ca);

	//ms_mutex_lock (&video_ctx.video_ticker->lock);
	//ms_ticker_print_graphs(video_ctx.video_ticker);
	//ms_mutex_unlock (&video_ctx.video_ticker->lock);
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

				ms_filter_call_method(ainfo->source_sizeconv, MS_FILTER_SET_FPS,
									  &fps);
				ms_filter_call_method(ainfo->source_sizeconv,
									  MS_FILTER_SET_VIDEO_SIZE, &vsize);
				ms_filter_call_method(ainfo->source_pixconv,
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
	NULL, //video_module_set_image_callback,

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
	video_module_session_send_vfu,
	video_module_session_adapt_video_bitrate,
};

#endif
