/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2011  Aymeric MOIZARD - <amoizard@gmail.com>
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

#if defined (_WIN32_WCE) || defined(TARGET_OS_IPHONE)
#define DISABLE_STITCHER
#endif

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

#include <osip2/osip_time.h>
#include <osip2/osip_mt.h>

#include "sdptools.h"

#define KEEP_RTP_SESSION

struct am_video_info {
	RtpSession *video_rtp_session;
	OrtpEvQueue *video_rtp_queue;

#ifndef DISABLE_SRTP
	srtp_t video_srtp;
#endif
	RtpProfile *video_rtp_profile;
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

	MSTicker *video_ticker;
	MSFilter *video_void;

	struct am_video_info ctx[MAX_NUMBER_OF_CALLS];
};

struct video_module_ctx video_ctx = {

	0,
	"",
	"",
	{ MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H },
	{ MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H },

	NULL,
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

void am_filter_register_h264(void);
static int video_module_enable_preview(int enable);
static int video_module_set_selfview_mode(int mode);
static int video_module_set_selfview_position(float posx, float posy, float size);
static int video_module_get_selfview_position(float *posx, float *posy, float *size);
static int video_module_set_selfview_scalefactor(float scalefactor);
static int video_module_set_background_color(int red, int green, int blue);
static int video_module_set_video_option(int opt, void *arg);

int video_module_init(const char *name, int debug_level)
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

	am_filter_register_h264();

	return 0;
}

int video_module_reset(const char *name, int debug_level)
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

	return 0;
}

int video_module_quit()
{
	video_module_enable_preview(0);
	if (video_ctx.video_ticker != NULL)
		ms_ticker_destroy(video_ctx.video_ticker);
	video_ctx.video_ticker=NULL;

	memset(&video_ctx, 0, sizeof(video_ctx));
	return 0;
}

int video_module_find_camera(struct am_camera *camera)
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

int video_module_select_camera(int card)
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

int video_module_set_input_video_size(int width, int height)
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

int video_module_set_window_display(MSDisplayDesc *desc, long handle, int width, int height)
{
	if (width == 0) {
		width = MS_VIDEO_SIZE_QCIF_W;
		height = MS_VIDEO_SIZE_QCIF_H;
	}

	video_ctx.prefered_screen_size.width = width;
	video_ctx.prefered_screen_size.height = height;

	if (handle == 0)
		return 0;

	return 0;
}

int video_module_set_window_handle(long handle, int width, int height)
{
	if (width == 0) {
		width = MS_VIDEO_SIZE_QCIF_W;
		height = MS_VIDEO_SIZE_QCIF_H;
	}

	video_ctx.prefered_screen_size.width = width;
	video_ctx.prefered_screen_size.height = height;

	if (handle == 0)
		return 0;

	return 0;
}

int video_module_set_nowebcam(const char *nowebcam_image)
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

int _video_module_open_camera()
{
	return 0;
}

static int video_module_enable_preview(int enable)
{

	/* close preview */
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_void != NULL) {

		/* detach all pending calls */
		{
			int k;

			/* search index of elements */
			for (k = 0; k < STITCHER_MAX_INPUTS - 1; k++) {
				am_call_t *ca = &_antisipc.calls[k];
				struct am_video_info *tmpinfo = _antisipc.calls[k].video_ctx;
				if (tmpinfo==NULL)
				{
				} else if (tmpinfo->video_rtprecv != NULL
						   && tmpinfo->video_decoder != NULL) {
					ms_ticker_detach(video_ctx.video_ticker,
									 tmpinfo->video_rtprecv);
					ms_filter_unlink(tmpinfo->video_rtprecv, 0,
									 tmpinfo->video_decoder, 0);
				}
			}
		}


		if (enable <= 0) {
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);
		}
		if (enable <= 0) {
			video_ctx.video_void = NULL;
		}
	}

	if (enable <= 0) {
		return 0;
	}

	if (1) {

		if (video_ctx.video_void == NULL)
			video_ctx.video_void = ms_filter_new(MS_VOID_SINK_ID);

		if (video_ctx.video_ticker == NULL)
			video_ctx.video_ticker = ms_ticker_new();
		ms_ticker_set_name(video_ctx.video_ticker, "amsip-video");

		_video_module_open_camera();

		if (video_ctx.video_ticker == NULL
			|| video_ctx.video_void == NULL) {
			if (video_ctx.video_ticker != NULL)
				ms_ticker_destroy(video_ctx.video_ticker);
			if (video_ctx.video_void != NULL)
				ms_filter_destroy(video_ctx.video_void);

			video_ctx.video_ticker = NULL;
			video_ctx.video_void = NULL;
			return -1;
		}

		/* reattach all pending calls */
		{
			int k;

			/* search index of elements */
			for (k = 0; k < STITCHER_MAX_INPUTS - 1; k++) {
				am_call_t *ca = &_antisipc.calls[k];
				struct am_video_info *tmpinfo = _antisipc.calls[k].video_ctx;

				if (tmpinfo==NULL)
				{
				} else if (tmpinfo->video_rtprecv != NULL
						   && tmpinfo->video_decoder != NULL) {
					ms_filter_link(tmpinfo->video_rtprecv, 0, tmpinfo->video_decoder,
								   0);
					ms_ticker_attach(video_ctx.video_ticker,
									 tmpinfo->video_rtprecv);
				}
			}
		}
	}
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
	return -1;
}

int
video_module_set_callback(unsigned int id,
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
#if !defined(KEEP_RTP_SESSION)
	if (ainfo->video_rtp_session != NULL)
		rtp_session_destroy(ainfo->video_rtp_session);
#endif
	if (ainfo->video_rtp_queue != NULL)
		ortp_ev_queue_destroy(ainfo->video_rtp_queue);
	if (ainfo->video_decoder != NULL)
		ms_filter_destroy(ainfo->video_decoder);
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
	ainfo->video_decoder = NULL;
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

	if (video_ctx.video_ticker == NULL) {
		video_module_enable_preview(1);
	}
	if (video_ctx.video_ticker == NULL) {
		ms_error("am_ms2_video.c: missing video ticker.");
		return -1;
	}

	ainfo->video_rtprecv = ms_filter_new(MS_RTP_RECV_ID);
	ms_filter_call_method(ainfo->video_rtprecv, MS_RTP_RECV_SET_SESSION,
						  rtps);

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
				("am_ms2_video.c: No decoder/encoder available for payload (NULL).");
		else
			ms_error
				("am_ms2_video.c: No decoder/encoder available for payload %s.",
				 pt->mime_type);
		return -1;
	}

	/*force the decoder to output YUV420P */
	format = MS_YUV420P;
	ms_filter_call_method(ainfo->video_decoder, MS_FILTER_SET_PIX_FMT,
						  &format);

	if (pt != NULL && pt->recv_fmtp != NULL)
		ms_filter_call_method(ainfo->video_decoder, MS_FILTER_ADD_FMTP,
							  (void *) pt->recv_fmtp);
	return 0;
}

static int video_stream_start_recv_only(am_call_t * ca, PayloadType * pt)
{
	return video_stream_start_send_recv(ca, pt);
}

static int video_stream_start_send_only(am_call_t * ca, PayloadType * pt)
{
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
		&& video_ctx.video_void != NULL) {
		/* detach current calls */
		{
			if (ainfo->video_ice != NULL)
				ms_ticker_detach(video_ctx.video_ticker,
								 ainfo->video_ice);
			if (ainfo->video_decoder != NULL) {
				ms_ticker_detach(video_ctx.video_ticker,
								 ainfo->video_rtprecv);

				ms_filter_unlink(ainfo->video_rtprecv, 0,
								 ainfo->video_decoder, 0);
			}
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
	if (video_ctx.video_ticker != NULL
		&& video_ctx.video_void != NULL) {
	} else if (//video_ctx.video_ticker == NULL &&
			   video_ctx.video_void == NULL) {
		video_module_enable_preview(1);
	} else {
		/* wrong state for video */
		return -1;
	}

	/* reattach all pending calls */
	if (ainfo->video_decoder != NULL) {
		ms_filter_link(ainfo->video_rtprecv, 0, ainfo->video_decoder,
					   0);
		ms_ticker_attach(video_ctx.video_ticker,
						 ainfo->video_rtprecv);
	}
	if (ainfo->video_ice != NULL)
		ms_ticker_attach(video_ctx.video_ticker,
						 ainfo->video_ice);

	return 0;
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

	return NULL;
}


static int video_module_session_init(am_call_t * ca, int idx)
{
	struct am_video_info *ainfo;
	ca->video_ctx = &video_ctx.ctx[idx];

	ainfo = ca->video_ctx;
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

static int
video_module_session_start(am_call_t * ca, sdp_message_t * sdp_answer,
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

int video_module_session_get_bandwidth_statistics(am_call_t * ca,
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

		ms_mutex_unlock(&video_ctx.video_ticker->lock);
	}

	return AMSIP_SUCCESS;
}

OrtpEvent *video_module_session_get_video_rtp_events(am_call_t * ca)
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

int video_module_session_send_vfu(am_call_t * ca)
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

	return AMSIP_WRONG_STATE;
}

static int video_module_session_adapt_video_bitrate(am_call_t * ca, float lost)
{
	return AMSIP_UNDEFINED_ERROR;
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
	video_module_set_nowebcam,
	video_module_enable_preview,

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
