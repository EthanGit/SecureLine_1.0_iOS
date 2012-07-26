/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)
Copyright (C) 2010-2012 Aymeric MOIZARD - <amoizard@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msvideo.h"

#define VPX_CODEC_DISABLE_COMPAT 1
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#define VP8_PAYLOAD_DESC_X_MASK      0x80
#define VP8_PAYLOAD_DESC_RSV_MASK    0x40
#define VP8_PAYLOAD_DESC_N_MASK      0x20
#define VP8_PAYLOAD_DESC_S_MASK      0x10
#define VP8_PAYLOAD_DESC_PARTID_MASK 0x0F

static int get_cpu() {
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );

	return sysinfo.dwNumberOfProcessors;
#elif ((TARGET_OS_IPHONE == 1) || defined(ANDROID))
	return 1;
#elif defined(__APPLE__) && defined(__OBJC__)
	return (int)[[NSProcessInfo processInfo] activeProcessorCount];
#elif defined(__linux)
	return sysconf( _SC_NPROCESSORS_ONLN );
#else
	return 0;
#endif
}

typedef struct Vp8EncState {
	vpx_codec_ctx_t codec;
	int threshold;
	int autoaltref;
	vp8e_token_partitions token_partitions;

	MSVideoSize vsize;
	int bitrate;
	float fps;
	uint64_t framenum;
	int mtu;	/* network maximum transmission unit in bytes */
	bool_t req_vfu;
	bool_t ready;
    int reset;
} Vp8EncState;

static void enc_init(MSFilter *f) {
	Vp8EncState *d=ms_new(Vp8EncState,1);
	memset(&d->codec, 0, sizeof(d->codec));
	
	d->threshold=0;
	d->autoaltref=1;
	d->token_partitions=VP8_FOUR_TOKENPARTITION;

	d->bitrate=384000;
	d->vsize.width=MS_VIDEO_SIZE_CIF_W;
	d->vsize.height=MS_VIDEO_SIZE_CIF_H;
	d->fps=30;
	d->framenum=0;
	d->mtu=MIN(1408-5,ms_get_payload_max_size()-5);/*-5 for the vp8 payload header*/
	d->req_vfu=TRUE;
	d->ready=FALSE;
    d->reset=0;
	f->data=d;
}

static void enc_uninit(MSFilter *f) {
	Vp8EncState *d=(Vp8EncState*)f->data;
	ms_free(d);
}

static void enc_setup_param(MSFilter *f, vpx_codec_enc_cfg_t *cfg)
{
	Vp8EncState *d=(Vp8EncState*)f->data;
	vpx_codec_err_t err;
	err = vpx_codec_enc_config_default(vpx_codec_vp8_cx(), cfg, 0);
	if (err != VPX_CODEC_OK) {
		ms_error("vp8: vpx_codec_enc_config_default: %s", vpx_codec_err_to_string(err));
		return;
	}

	cfg->g_w = d->vsize.width;
	cfg->g_h = d->vsize.height;
	cfg->g_timebase.den = (int)d->fps;
	cfg->rc_target_bitrate = (int)(((float)d->bitrate)*0.9/1024);

	cfg->rc_end_usage = VPX_CBR;
	cfg->kf_mode = VPX_KF_AUTO;
	cfg->kf_max_dist = (unsigned int)(d->fps)*5;
	cfg->kf_min_dist = 0;

	cfg->g_error_resilient = 1;
	cfg->g_threads = get_cpu();
}

static void enc_preprocess(MSFilter *f) {
	Vp8EncState *d=(Vp8EncState*)f->data;
	vpx_codec_enc_cfg_t cfg;
	vpx_codec_err_t err;

	d->framenum=0;
	d->req_vfu=TRUE;
	d->ready=FALSE;
	d->reset=0;
    enc_setup_param(f, &cfg);

	err =  vpx_codec_enc_init(&d->codec, vpx_codec_vp8_cx(), &cfg, 0);
	if (err != VPX_CODEC_OK) {
		ms_error("vp8: vpx_codec_enc_init: %s", vpx_codec_err_to_string(err));
		ms_error("vp8: codec details: %s", vpx_codec_error_detail(&d->codec));
		return;
	}
    /*cpu/quality tradeoff: positive values decrease CPU usage at the expense of quality*/
#if ((TARGET_OS_IPHONE == 1) || defined(ANDROID))
	vpx_codec_control(&d->codec, VP8E_SET_CPUUSED, 16); 
#else
	if (cfg.g_threads > 2)
		vpx_codec_control(&d->codec, VP8E_SET_CPUUSED, -8); 
	else
		vpx_codec_control(&d->codec, VP8E_SET_CPUUSED, 4);
#endif
	vpx_codec_control(&d->codec, VP8E_SET_STATIC_THRESHOLD, d->threshold);
	vpx_codec_control(&d->codec, VP8E_SET_ENABLEAUTOALTREF, d->autoaltref);
	if (cfg.g_threads > 1) {
		err = vpx_codec_control(&d->codec, VP8E_SET_TOKEN_PARTITIONS, d->token_partitions);
		if (err != VPX_CODEC_OK) {
			ms_error("vp8: vpx_codec_control(VP8E_SET_TOKEN_PARTITIONS, %i): %s", d->token_partitions, vpx_codec_err_to_string(err));
			ms_error("vp8: codec details: %s", vpx_codec_error_detail(&d->codec));
		}
	}

	d->ready=TRUE;
}

static void enc_postprocess(MSFilter *f) {
	Vp8EncState *d=(Vp8EncState*)f->data;
	if (d->ready) vpx_codec_destroy(&d->codec);
	d->ready=FALSE;
}

static void vp8_add_header(mblk_t **packet, const vpx_codec_cx_pkt_t *vpx_data, unsigned char start_bit){
	mblk_t *header;
	header = allocb(1, 0);
	(*header->b_rptr) = 0;

	/*
		draft-ietf-payload-vp8-03:
	   The first octets after the RTP header are the VP8 payload descriptor,
	   with the following structure.

			 0 1 2 3 4 5 6 7
			+-+-+-+-+-+-+-+-+
			|X|R|N|S|PartID | (REQUIRED)
			+-+-+-+-+-+-+-+-+
	   X:   |I|L|T|K| RSV   | (OPTIONAL)
			+-+-+-+-+-+-+-+-+
	   I:   |   PictureID   | (OPTIONAL)
			+-+-+-+-+-+-+-+-+
	   L:   |   TL0PICIDX   | (OPTIONAL)
			+-+-+-+-+-+-+-+-+
	   T/K: |TID|Y| KEYIDX  | (OPTIONAL)
			+-+-+-+-+-+-+-+-+
	   X: Extended control bits present.  When set to one, the extension
		  octet MUST be provided immediately after the mandatory first
		  octet.  If the bit is zero, all optional fields MUST be omitted.
	   R: Bit reserved for future use.  MUST be set to zero and MUST be
		  ignored by the receiver.
	   N: Non-reference frame.  When set to one, the frame can be discarded
		  without affecting any other future or past frames.  If the
		  reference status of the frame is unknown, this bit SHOULD be set
		  to zero to avoid discarding frames needed for reference.
	   S: Start of VP8 partition.  SHOULD be set to 1 when the first payload
		  octet of the RTP packet is the beginning of a new VP8 partition,
		  and MUST NOT be 1 otherwise.  The S bit MUST be set to 1 for the
		  first packet of each encoded frame.
	  PartID:  Partition index.  Denotes which VP8 partition the first
		  payload octet of the packet belongs to.  The first VP8 partition
		  (containing modes and motion vectors) MUST be labeled with PartID
		  = 0.  PartID SHOULD be incremented for each subsequent partition,
		  but MAY be kept at 0 for all packets.  PartID MUST NOT be larger
		  than 8.  If more than one packet in an encoded frame contains the
		  same PartID, the S bit MUST NOT be set for any other packet than
		  the first packet with that PartID.
	*/
#define VP8_X_BIT 0x80
#define VP8_R_BIT 0x40
#define VP8_N_BIT 0x20
#define VP8_S_BIT 0x10
#define VP8_PARTID_BITS 0x0F

	(*header->b_rptr) = 0;
	//(*header->b_rptr) &= ~VP8_X_BIT;
	//(*header->b_rptr) &= ~VP8_R_BIT;
	//(*header->b_rptr) &= ~VP8_N_BIT;
	if (vpx_data->data.frame.flags & VPX_FRAME_IS_DROPPABLE)
		(*header->b_rptr) |= VP8_N_BIT;
	(*header->b_rptr) |= start_bit;

	header->b_wptr++;
	header->b_cont = *packet;
	*packet = header;
}

static void vp8_fragment_and_send(MSFilter *f,Vp8EncState *d,mblk_t *frame, uint32_t timestamp, const vpx_codec_cx_pkt_t *vpx_data){
	uint8_t *rptr;
	mblk_t *packet=NULL;
	int len;
	for (rptr=frame->b_rptr;rptr<frame->b_wptr;){
		len=MIN(d->mtu,(int)(frame->b_wptr-rptr));
		packet=dupb(frame);
		packet->b_rptr=rptr;
		packet->b_wptr=rptr+len;
		if (rptr==frame->b_rptr)
			vp8_add_header(&packet, vpx_data, VP8_S_BIT);
		else
			vp8_add_header(&packet, vpx_data, 0);
		mblk_set_timestamp_info(packet,timestamp);
		ms_queue_put(f->outputs[0],packet);
		rptr+=len;
	}
	/*set marker bit on last packet*/
	mblk_set_marker_info(packet,TRUE);
	freeb(frame);
}

static void enc_process(MSFilter *f) {
	Vp8EncState *d=(Vp8EncState*)f->data;
	uint32_t ts=(uint32_t)(f->ticker->time*90LL);
	mblk_t *im;
	MSPicture pic;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (yuv_buf_init_from_mblk(&pic,im)==0){
			vpx_codec_err_t err;
			unsigned int flags = 0;
			vpx_image_t vpx_image;

			if (d->vsize.width!=pic.w || d->vsize.height!=pic.h || d->reset==1)
			{
				d->vsize.width=pic.w;
				d->vsize.height=pic.h;
				enc_postprocess(f);
				enc_preprocess(f);
				ms_message("vp8: vp8_encoder_reconfig applied (%ix%i)", pic.w, pic.h);
				d->reset=0;
			}
 
			vpx_img_wrap(&vpx_image, VPX_IMG_FMT_I420, d->vsize.width, d->vsize.height, 1, pic.planes[0]);
		
			if (d->framenum%((int)(d->fps*5))==0
				|| (d->framenum==(int)(d->fps*2))) {
				flags = VPX_EFLAG_FORCE_KF;
			}
			if (d->req_vfu){
				flags = VPX_EFLAG_FORCE_KF;
				d->req_vfu=FALSE;
			}

			err = vpx_codec_encode(&d->codec, &vpx_image, d->framenum, 1, flags, VPX_DL_REALTIME);

			if (err != VPX_CODEC_OK) {
				ms_error("vp8: vpx_codec_encode: %s", vpx_codec_err_to_string(err));
				ms_error("vp8: codec details: %s", vpx_codec_error_detail(&d->codec));
			} else {
				const vpx_codec_cx_pkt_t *vpx_data;
				vpx_codec_iter_t vpx_iter = NULL;

				d->framenum++;

				while( (vpx_data = vpx_codec_get_cx_data(&d->codec, &vpx_iter)) ) {
					if (vpx_data->kind == VPX_CODEC_CX_FRAME_PKT) {
						if (vpx_data->data.frame.sz > 0) {
							mblk_t *om;
							om = allocb((int)vpx_data->data.frame.sz,0);
							memcpy(om->b_wptr, vpx_data->data.frame.buf, vpx_data->data.frame.sz);
							om->b_wptr += vpx_data->data.frame.sz;

							//if ((!!(vpx_data->data.frame.flags & VPX_FRAME_IS_KEY))) {
							//	ms_debug("vp8: sending key-FRAME: %u/%i/%i", vpx_data->data.frame.sz, vpx_data->data.frame.flags, vpx_data->data.frame.partition_id);
							//}
							vp8_fragment_and_send(f, d, om, ts, vpx_data);
						}
					}
				}
			}
		}
		freemsg(im);
	}
}

static int enc_set_vsize(MSFilter *f, void*data){
	MSVideoSize *vs=(MSVideoSize*)data;
	Vp8EncState *d=(Vp8EncState*)f->data;
	d->vsize.width=vs->width;
	d->vsize.height=vs->height;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *data){
	Vp8EncState *d=(Vp8EncState*)f->data;
	MSVideoSize *vs=(MSVideoSize*)data;
	vs->width=d->vsize.width;
	vs->height=d->vsize.height;
	return 0;
}

static int enc_add_attr(MSFilter *f, void*data){
	return 0;
}

static int enc_set_fps(MSFilter *f, void *data){
	float *fps=(float*)data;
	Vp8EncState *d=(Vp8EncState*)f->data;
	d->fps=*fps;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *data){
	Vp8EncState *d=(Vp8EncState*)f->data;
	float *fps=(float*)data;
	*fps=d->fps;
	return 0;
}

static int enc_get_br(MSFilter *f, void*data){
	Vp8EncState *d=(Vp8EncState*)f->data;
	*(int*)data=d->bitrate;
	return 0;
}

static int enc_set_br(MSFilter *f, void *arg){
	Vp8EncState *d=(Vp8EncState*)f->data;
	d->bitrate=*(int*)arg;
	if (d->bitrate>=8192000){
		d->vsize.width=MS_VIDEO_SIZE_720P_W;
		d->vsize.height=MS_VIDEO_SIZE_720P_H;
		d->fps=30;
	}else if (d->bitrate>=4096000){
		d->vsize.width=MS_VIDEO_SIZE_4CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_4CIF_H;
		d->fps=30;
	}else if (d->bitrate>=2048000){
		d->vsize.width=MS_VIDEO_SIZE_4CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_4CIF_H;
		d->fps=30;
	}else if (d->bitrate>=1024000){
		d->vsize.width=MS_VIDEO_SIZE_4CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_4CIF_H;
		d->fps=30;
	}else if (d->bitrate>=512000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=30;
	} else if (d->bitrate>=384000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=25;
	}else if (d->bitrate>=256000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=15;
	}else if (d->bitrate>=128000){
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=15;
	}else if (d->bitrate>=64000){
		d->vsize.width=MS_VIDEO_SIZE_QCIF_W;
		d->vsize.height=MS_VIDEO_SIZE_QCIF_H;
		d->fps=10;
	}else if (d->bitrate>=20000){
		/* for static images */
		d->vsize.width=MS_VIDEO_SIZE_CIF_W;
		d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=1;
		d->bitrate=d->bitrate*2;
	}
#if TARGET_OS_IPHONE
	if (d->bitrate>=512000){
        d->vsize.width=MS_VIDEO_SIZE_IOS2_W;
        d->vsize.height=MS_VIDEO_SIZE_IOS2_H;
		d->fps=10;
    }else if (d->bitrate>=64000){
        d->vsize.width=MS_VIDEO_SIZE_IOS1_W;
        d->vsize.height=MS_VIDEO_SIZE_IOS1_H;
		d->fps=10;
    }else if (d->bitrate>=20000){
        /* for static images */
        d->vsize.width=MS_VIDEO_SIZE_CIF_W;
        d->vsize.height=MS_VIDEO_SIZE_CIF_H;
		d->fps=10;
    }
#endif
    if (d->ready==TRUE)
    {
        d->reset=1;
    }
	return 0;
}

static int enc_set_mtu(MSFilter *f, void*data){
	Vp8EncState *d=(Vp8EncState*)f->data;
	d->mtu=*(int*)data;
	return 0;
}

static int enc_add_fmtp(MSFilter *f,void *arg){
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *unused){
	Vp8EncState *d=(Vp8EncState*)f->data;
	d->req_vfu=TRUE;
	return 0;
}

static int get_statistics(MSFilter *f, void *arg){
	int i;
	ms_warning("filter: %s[%i->%i]", f->desc->name,
		f->desc->ninputs, f->desc->noutputs);
	for (i=0;i<f->desc->ninputs;i++) {
		if (f->inputs[i]!=NULL) ms_warning("filter: %s in[%i]=%i", f->desc->name,
			i, f->inputs[i]->q.q_mcount);
	}
	for (i=0;i<f->desc->noutputs;i++) {
		if (f->outputs[i]!=NULL) ms_warning("filter: %s out[%i]=%i", f->desc->name,
			i, f->outputs[i]->q.q_mcount);
	}
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_SET_VIDEO_SIZE ,	enc_set_vsize },
	{	MS_FILTER_GET_VIDEO_SIZE ,	enc_get_vsize },
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp },
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_GET_BITRATE	,	enc_get_br	},
	{	MS_FILTER_SET_MTU	,	enc_set_mtu	},
	{	MS_FILTER_REQ_VFU	,	enc_req_vfu	},
	{	MS_FILTER_GET_STATISTICS, get_statistics },
	{	0			,	NULL	}
};

MSFilterDesc vp8_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSVp8Enc",
	N_("A VP8 encoder based on libvpx"),
	MS_FILTER_ENCODER,
	"VP8",
	1,
	1,
	enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};

MS_FILTER_DESC_EXPORT(vp8_enc_desc)

typedef struct vp8_depacketizer{
	//MSQueue q;
	mblk_t *m;
	uint32_t last_ts;
} vp8_depacketizer;

static void vp8_depacketizer_init(vp8_depacketizer *ctx)
{
	//ms_queue_init(&ctx->q);
	ctx->m=NULL;
	ctx->last_ts=0x943FEA43;/*some random value*/
}

static void vp8_depacketizer_uninit(vp8_depacketizer *ctx){
	//ms_queue_flush(&ctx->q);
	if (ctx->m) freemsg(ctx->m);
	ctx->m=NULL;
}

#define vp8_header_get_x(h) ((*h) >> 7)
#define vp8_header_get_r(h) (((*h) >> 6) & 0x01)
#define vp8_header_get_n(h) (((*h) >> 5) & 0x01)
#define vp8_header_get_s(h) (((*h) >> 4) & 0x01)
#define vp8_header_get_partid(h) ((*h) & VP8_PARTID_BITS)

#define vp8_header_get_i(h) ((*(h)) >> 7)
#define vp8_header_get_l(h) (((*h) >> 6) & 0x01)
#define vp8_header_get_t(h) (((*h) >> 5) & 0x01)
#define vp8_header_get_k(h) (((*h) >> 4) & 0x01)

static mblk_t * vp8_depacketizer_aggregate(vp8_depacketizer *ctx, mblk_t *im){
	mblk_t *om=NULL;
	int marker = mblk_get_marker_info(im);
	uint8_t start_bit = vp8_header_get_s(im->b_rptr);
	uint8_t x_bit = vp8_header_get_x(im->b_rptr);
	int header_size=1;
	if (x_bit==1) {
		unsigned char *octet2 = im->b_rptr+1;
		int ibit = vp8_header_get_i(octet2);
		int lbit = vp8_header_get_l(octet2);
		int tbit = vp8_header_get_t(octet2);
		int kbit = vp8_header_get_k(octet2);
		int tkbit = tbit|kbit;
		if (ibit==1)
			ibit += vp8_header_get_i(octet2+1); //may be 16b size if first bit == 1
		header_size=header_size+lbit+tkbit;
	}

	if (start_bit){
		if (ctx->m!=NULL){
			ms_error("vp8: receiving VP8PACKET with start_bit while previous VP8FRAME is not finished");
			freemsg(ctx->m);
			ctx->m=NULL;
		}
		im->b_rptr += header_size;
		ctx->m=im;
	}else{
		if (ctx->m!=NULL){
			im->b_rptr += header_size;
			concatb(ctx->m,im);
		}else{
			ms_error("vp8: Receiving continuation VP8FRAME packet but no start_bit.");
			freemsg(im);
		}
	}
	if (marker && ctx->m){
		//ms_debug("vp8: Receiving end of VP8FRAME");
		msgpullup(ctx->m,-1);
		om=ctx->m;
		ctx->m=NULL;
	}
	return om;
}

/*process incoming rtp data and output NALUs, whenever possible*/
static void vp8_depacketizer_unpack(vp8_depacketizer *ctx, mblk_t *im, MSQueue *out){
	uint8_t start_bit=vp8_header_get_s(im->b_rptr);
	int marker = mblk_get_marker_info(im);
	uint32_t ts=mblk_get_timestamp_info(im);
	mblk_t *o;

	if (ctx->last_ts!=ts){
		/* a new frame is arriving */
		ctx->last_ts=ts;
		if (ctx->m!=NULL && start_bit==1){
			/* marker was lost? or last packet was lost? drop or no drop? */
			ms_queue_put(out,ctx->m);
			ctx->m=NULL;
		}
	}

	if (im->b_cont) msgpullup(im,-1);

	o=vp8_depacketizer_aggregate(ctx,im);
	if (o) ms_queue_put(out,o);

	if (marker){
		ctx->last_ts=ts;
	}
}

typedef struct Vp8DecState {
	mblk_t *yuv_msg;
	MSPicture outbuf;
	vpx_codec_ctx_t codec;
	vp8_depacketizer unpacker;
} Vp8DecState;

static void dec_init(MSFilter *f) {
	Vp8DecState *d=(Vp8DecState *)ms_new(Vp8DecState,1);
	vpx_codec_err_t err;

	d->yuv_msg=NULL;
	vp8_depacketizer_init(&d->unpacker);
	err = vpx_codec_dec_init(&d->codec, vpx_codec_vp8_dx(), NULL, 0);
	if (err != VPX_CODEC_OK) {
		ms_error("vp8: vpx_codec_dec_init: %s", vpx_codec_err_to_string(err));
	}
	d->outbuf.w=0;
	d->outbuf.h=0;

	f->data = d;
}

static void dec_uninit(MSFilter *f) {
	Vp8DecState *d=(Vp8DecState*)f->data;
	vp8_depacketizer_uninit(&d->unpacker);
	vpx_codec_destroy(&d->codec);

	if (d->yuv_msg)
		freemsg(d->yuv_msg);

	ms_free(d);
}

static mblk_t *get_as_yuvmsg(MSFilter *f, Vp8DecState *d, vpx_image_t *vpx_image){
	MSVideoSize roi;
	if (d->outbuf.w!=vpx_image->d_w || d->outbuf.h!=vpx_image->d_h){
		if (d->yuv_msg!=NULL){
			freemsg(d->yuv_msg);
			d->yuv_msg=NULL;
		}
		ms_message("vp8: Getting yuv picture of %ix%i",vpx_image->d_w,vpx_image->d_h);
		d->yuv_msg=yuv_buf_alloc(&d->outbuf, vpx_image->d_w, vpx_image->d_h);
		d->outbuf.w=vpx_image->d_w;
		d->outbuf.h=vpx_image->d_h;
	}
	roi.width=vpx_image->d_w;
	roi.height=vpx_image->d_h;
	ms_yuv_buf_copy(vpx_image->planes, vpx_image->stride, d->outbuf.planes, d->outbuf.strides, roi);
	return dupmsg(d->yuv_msg);
}

static void dec_process(MSFilter *f) {
	Vp8DecState *d=(Vp8DecState*)f->data;
	mblk_t *im;
	MSQueue nalus;
	ms_queue_init(&nalus);

	while( (im=ms_queue_get(f->inputs[0]))!=0) {
		mblk_t *m;

		vp8_depacketizer_unpack(&d->unpacker, im, &nalus);

		while((m=ms_queue_get(&nalus))!=NULL){
			vpx_codec_err_t err;

			err = vpx_codec_decode(&d->codec, m->b_rptr, (unsigned int)(m->b_wptr - m->b_rptr), NULL, 0);
			if (err != VPX_CODEC_OK) {
				ms_error("vp8: vpx_codec_decode: %s", vpx_codec_err_to_string(err));
				ms_error("vp8: codec details: %s", vpx_codec_error_detail(&d->codec));
			}

			freemsg(m);
		}
	}

	{
		vpx_codec_iter_t  vpx_iter = NULL;
		vpx_image_t      *vpx_image;
		while((vpx_image = vpx_codec_get_frame(&d->codec, &vpx_iter))) {
			ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,vpx_image));
		}
	}
}

MSFilterDesc vp8_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSVp8Dec",
	N_("A VP8 decoder based on libvpx"),
	MS_FILTER_DECODER,
	"VP8",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	NULL
};

MS_FILTER_DESC_EXPORT(vp8_dec_desc)



#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsvp8_init(void);

GLOBAL_LINKAGE void libmsvp8_init(void){
	ms_filter_register(&vp8_enc_desc);
	ms_filter_register(&vp8_dec_desc);
}
