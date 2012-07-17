/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2010-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/rfc3984.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <ortp/b64.h>

#ifdef _MSC_VER
#include <stdint.h>
#endif
#include <x264.h>

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

typedef struct _X264EncState{
	x264_t *enc;
	MSVideoSize vsize;
	int bitrate;
	float fps;
	int mode;
	int profile_idc;
	uint64_t framenum;
	Rfc3984Context packer;
	mblk_t *sav_sps;
	mblk_t *sav_pps;
    int reset;
}X264EncState;

static void enc_init(MSFilter *f){
	X264EncState *d=ms_new(X264EncState,1);
	d->enc=NULL;
	d->bitrate=384000;
	d->vsize.width=MS_VIDEO_SIZE_CIF_W;
	d->vsize.height=MS_VIDEO_SIZE_CIF_H;
	d->fps=30;
	d->mode=0;
	d->profile_idc=13;
	d->framenum=0;
	d->sav_sps=NULL;
	d->sav_pps=NULL;
    d->reset=0;
	f->data=d;
}

static void enc_uninit(MSFilter *f){
	X264EncState *d=(X264EncState*)f->data;
	ms_free(d);
}

#ifndef TUNE_PARAMETER
#if defined(ANDROID)
#define TUNE_PARAMETER "ultrafast"
#define USE_ABR
#elif TARGET_OS_IPHONE
#define USE_ABR
#define TUNE_PARAMETER "ultrafast"
#else
#define USE_ABR
#define TUNE_PARAMETER "veryfast"
#endif
#endif

static void enc_setup_param(MSFilter *f, x264_param_t *params)
{
	X264EncState *d=(X264EncState*)f->data;
    x264_param_default_preset(params, TUNE_PARAMETER, "zerolatency");
    
    //params.i_level_idc=d->profile_idc;
    params->i_level_idc=12;
    params->b_repeat_headers=1; /* put SPS/PPS before each keyframe */
    params->i_slice_max_size=d->packer.maxsz; /* lower than MTU */
    params->i_width=d->vsize.width;
    params->i_height=d->vsize.height;
    params->i_fps_num=(int)d->fps;
    params->i_fps_den=1;
    
    params->rc.i_bitrate=(int)(d->bitrate/1000);
#ifdef USE_CQP
    params->rc.i_rc_method = X264_RC_CQP;
#elif defined(USE_ABR)
    params->rc.i_rc_method = X264_RC_ABR;
    params->rc.f_rate_tolerance=0.01f;
    params->rc.i_vbv_max_bitrate=(int) ((params->rc.i_bitrate*1.1f));
    params->rc.i_vbv_buffer_size=params->rc.i_vbv_max_bitrate/2; /* delay buffering */
    //params->rc.f_vbv_buffer_init=0.5;
#else
    params->rc.i_rc_method = X264_RC_CRF;
    params->rc.f_rf_constant=28;
#endif
    
    params->b_repeat_headers=1;
    params->b_annexb=0;
    
    params->i_keyint_max = (int)d->fps*30;
    params->i_keyint_min = (int)d->fps*5;
    /* params.b_intra_refresh = 1; */
    
    x264_param_apply_profile(params, "baseline");
}

static void enc_preprocess(MSFilter *f){
	X264EncState *d=(X264EncState*)f->data;
	x264_param_t params;
	rfc3984_init(&d->packer);
	rfc3984_set_mode(&d->packer,d->mode);

    enc_setup_param(f, &params);

	d->enc=x264_encoder_open(&params);
	if (d->enc==NULL) ms_error("Fail to create x264 encoder.");
	d->framenum=0;
	d->sav_pps=NULL;
	d->sav_sps=NULL;
}

static void x264_nals_to_msgb(MSFilter *f, x264_nal_t *xnals, int num_nals, MSQueue * nalus){
	X264EncState *d=(X264EncState*)f->data;
	int i;
	mblk_t *m;
	for (i=0;i<num_nals;++i){
		m=allocb(xnals[i].i_payload+10,0);
		memcpy(m->b_wptr,xnals[i].p_payload+4,xnals[i].i_payload-4);
        m->b_wptr+=xnals[i].i_payload-4;
		if (xnals[i].i_type==7) {
			ms_message("A SPS is being sent.");
			if (d->sav_sps!=NULL)
				freeb(d->sav_sps);
			d->sav_sps=dupb(m);
		}else if (xnals[i].i_type==8) {
			ms_message("A PPS is being sent.");
			if (d->sav_pps!=NULL)
				freeb(d->sav_pps);
			d->sav_pps=dupb(m);
		}
		ms_queue_put(nalus,m);
	}
}

static void enc_postprocess(MSFilter *f){
	X264EncState *d=(X264EncState*)f->data;
	rfc3984_uninit(&d->packer);
	if (d->enc!=NULL){
		x264_encoder_close(d->enc);
		d->enc=NULL;
	}
	if (d->sav_sps!=NULL)
		freeb(d->sav_sps);
	d->sav_sps=NULL;
	if (d->sav_pps!=NULL)
		freeb(d->sav_pps);
	d->sav_pps=NULL;
}

static void enc_process(MSFilter *f){
	X264EncState *d=(X264EncState*)f->data;
	uint32_t ts=(uint32_t)(f->ticker->time*90LL);
	mblk_t *im;
	MSPicture pic;
	MSQueue nalus;
	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		if (yuv_buf_init_from_mblk(&pic,im)==0){
			x264_picture_t xpic;
			x264_picture_t oxpic;
			x264_nal_t *xnals=NULL;
			int num_nals=0;

            if (d->vsize.width!=pic.w || d->vsize.height!=pic.h || d->reset==1)
            {
                d->vsize.width=pic.w;
                d->vsize.height=pic.h;
                enc_postprocess(f);
                enc_preprocess(f);
                ms_message("x264_encoder_reconfig applied (%ix%i)", pic.w, pic.h);
                d->reset=0;
            }
            
			memset(&xpic, 0, sizeof(xpic));
            memset(&oxpic, 0, sizeof(oxpic));

			xpic.i_type=X264_TYPE_AUTO;
			if (d->framenum==0)
				xpic.i_type=X264_TYPE_IDR;
			else if (d->framenum==(int)(d->fps*2))
				xpic.i_type=X264_TYPE_IDR;
			else if (d->framenum==(int)(d->fps*5))
				xpic.i_type=X264_TYPE_IDR;

			xpic.i_qpplus1=0;
			xpic.i_pts=d->framenum;
			xpic.img.i_csp=X264_CSP_I420;
			xpic.img.i_plane=3;
			xpic.img.i_stride[0]=pic.strides[0];
			xpic.img.i_stride[1]=pic.strides[1];
			xpic.img.i_stride[2]=pic.strides[2];
			xpic.img.i_stride[3]=0;
			xpic.img.plane[0]=pic.planes[0];
			xpic.img.plane[1]=pic.planes[1];
			xpic.img.plane[2]=pic.planes[2];
			xpic.img.plane[3]=0;
			if (x264_encoder_encode(d->enc,&xnals,&num_nals,&xpic,&oxpic)>=0){
				x264_nals_to_msgb(f, xnals,num_nals,&nalus);
				rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
				d->framenum++;

				if (d->framenum%((int)(d->fps*5))==0)
				{
					/* resend PPS/SPS */
					if (d->sav_sps!=NULL && d->sav_pps!=NULL)
					{
						ms_queue_put(&nalus,dupb(d->sav_sps));
						ms_queue_put(&nalus,dupb(d->sav_pps));
						rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
					}
				}

			}else{
				ms_error("x264_encoder_encode() error.");
			}
		}
		freemsg(im);
	}
}

static int enc_set_br(MSFilter *f, void *arg){
	X264EncState *d=(X264EncState*)f->data;
	d->bitrate=*(int*)arg;
	if (d->bitrate>=8192000){
		d->vsize.width=MS_VIDEO_SIZE_720P_W;
		d->vsize.height=MS_VIDEO_SIZE_720P_H;
		d->fps=30;
	}else if (d->bitrate>=4096000){
		d->vsize.width=MS_VIDEO_SIZE_VGA_W;
		d->vsize.height=MS_VIDEO_SIZE_VGA_H;
		d->fps=30;
	}else if (d->bitrate>=2048000){
		d->vsize.width=MS_VIDEO_SIZE_VGA_W;
		d->vsize.height=MS_VIDEO_SIZE_VGA_H;
		d->fps=30;
	}else if (d->bitrate>=1024000){
		d->vsize.width=MS_VIDEO_SIZE_VGA_W;
		d->vsize.height=MS_VIDEO_SIZE_VGA_H;
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
    if (d->enc!=NULL)
    {
        d->reset=1;
    }
	return 0;
}

static int enc_set_fps(MSFilter *f, void *arg){
	X264EncState *d=(X264EncState*)f->data;
	d->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	X264EncState *d=(X264EncState*)f->data;
	*(float*)arg=d->fps;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	X264EncState *d=(X264EncState*)f->data;
	*(MSVideoSize*)arg=d->vsize;
	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	X264EncState *d=(X264EncState*)f->data;
	const char *fmtp=(const char *)arg;
	char value[12];
	if (fmtp_get_value(fmtp,"packetization-mode",value,sizeof(value))){
		d->mode=atoi(value);
		ms_message("packetization-mode set to %i",d->mode);
	}
	if (fmtp_get_value(fmtp,"profile-level-id",value,sizeof(value))){
		if (value[0]!='\0' && strlen(value)==6)
		{
			d->profile_idc=0;
			if (value[4] >= '0' && value[4] <= '9')
	            d->profile_idc = (d->profile_idc << 4) + (value[4] - '0');
			else if (value[4] >= 'A' && value[4] <= 'F')
	            d->profile_idc = (d->profile_idc << 4) + (value[4] - 'A' + 10);
			else if (value[4] >= 'a' && value[4] <= 'f')
	            d->profile_idc = (d->profile_idc << 4) + (value[4] - 'a' + 10);

			if (value[5] >= '0' && value[5] <= '9')
	            d->profile_idc = (d->profile_idc << 4) + (value[5] - '0');
			else if (value[5] >= 'A' && value[5] <= 'F')
	            d->profile_idc = (d->profile_idc << 4) + (value[5] - 'A' + 10);
			else if (value[5] >= 'a' && value[5] <= 'f')
	            d->profile_idc = (d->profile_idc << 4) + (value[5] - 'a' + 10);
		}
	}
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_FPS	,	enc_set_fps	},
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_GET_FPS	,	enc_get_fps	},
	{	MS_FILTER_GET_VIDEO_SIZE,	enc_get_vsize	},
	{	MS_FILTER_ADD_FMTP	,	enc_add_fmtp	},
	{	0	,			NULL		}
};

MSFilterDesc x264_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSX264Enc",
	"A H264 encoder based on x264 project.",
	MS_FILTER_ENCODER,
	"H264",
	1,
	1,
	enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};


typedef struct _X264DecState{
	mblk_t *yuv_msg;
	mblk_t *sps,*pps;
	Rfc3984Context unpacker;
	MSPicture outbuf;
	struct MSScalerContext *sws_ctx;
	AVCodecContext av_context;
	unsigned int packet_num;
	uint8_t *bitstream;
	int bitstream_size;
}X264DecState;

static void ffmpeg_init(){
	static bool_t done=FALSE;
	if (!done){
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,13,0)
#else
		avcodec_init();
#endif
		avcodec_register_all();
		done=TRUE;
	}
}

static void dec_open(X264DecState *d){
	AVCodec *codec;
	int error;
	codec=avcodec_find_decoder(CODEC_ID_H264);
	if (codec==NULL) ms_fatal("Could not find H264 decoder in ffmpeg.");
	avcodec_get_context_defaults(&d->av_context);
#if defined(ANDROID)
	d->av_context.flags2 |= CODEC_FLAG2_FAST;
	d->av_context.flags2 |= CODEC_FLAG2_CHUNKS;
#elif TARGET_OS_IPHONE
	d->av_context.flags2 |= CODEC_FLAG2_FAST;
	d->av_context.flags2 |= CODEC_FLAG2_CHUNKS;
#else
	d->av_context.flags2 |= CODEC_FLAG2_CHUNKS;
#endif
	d->av_context.thread_count = 1;
	error=avcodec_open(&d->av_context,codec);
	if (error!=0){
		ms_fatal("avcodec_open() failed.");
	}
}

static void dec_init(MSFilter *f){
	X264DecState *d=(X264DecState*)ms_new(X264DecState,1);
	ffmpeg_init();
	d->yuv_msg=NULL;
	d->sps=NULL;
	d->pps=NULL;

	d->sws_ctx=NULL;
	rfc3984_init(&d->unpacker);
	d->packet_num=0;
	dec_open(d);
	d->outbuf.w=0;
	d->outbuf.h=0;
	d->bitstream_size=65536;
	d->bitstream=(uint8_t*)ms_malloc0(d->bitstream_size);
	f->data=d;
}

static void dec_reinit(X264DecState *d){
	avcodec_close(&d->av_context);
	dec_open(d);
}

static void dec_uninit(MSFilter *f){
	X264DecState *d=(X264DecState*)f->data;
	rfc3984_uninit(&d->unpacker);
	avcodec_close(&d->av_context);
	if (d->yuv_msg) freemsg(d->yuv_msg);
	if (d->sps) freemsg(d->sps);
	if (d->pps) freemsg(d->pps);
	ms_free(d->bitstream);
	ms_free(d);
}

static MSPixFmt ffmpeg_pix_fmt_to_ms(int fmt){
	switch(fmt){
		case PIX_FMT_YUV420P:
			return MS_YUV420P;
		case PIX_FMT_YUYV422:
			return MS_YUYV;     /* same as MS_YUY2 */
		case PIX_FMT_RGB24:
			return MS_RGB24;
		case PIX_FMT_BGR24:
			return MS_RGB24_REV;
		case PIX_FMT_UYVY422:
			return MS_UYVY;
		case PIX_FMT_RGBA:
			return MS_RGBA;
		case PIX_FMT_NV21:
			return MS_NV21;
		case PIX_FMT_NV12:
			return MS_NV12;
		case PIX_FMT_ABGR:
			return MS_ABGR;
		case PIX_FMT_ARGB:
			return MS_ARGB;
		case PIX_FMT_RGB565:
			return MS_RGB565;
		default:
			ms_fatal("format not supported.");
			return MS_YUV420P; /* default */
	}
	return MS_YUV420P; /* default */
}

static mblk_t *get_as_yuvmsg(MSFilter *f, X264DecState *s, AVFrame *orig){
	AVCodecContext *ctx=&s->av_context;

	if (s->outbuf.w!=ctx->width || s->outbuf.h!=ctx->height){
		if (s->sws_ctx!=NULL){
			ms_video_scalercontext_free(s->sws_ctx);
			s->sws_ctx=NULL;
			freemsg(s->yuv_msg);
			s->yuv_msg=NULL;
		}
		ms_message("Getting yuv picture of %ix%i",ctx->width,ctx->height);
		s->yuv_msg=yuv_buf_alloc(&s->outbuf,ctx->width,ctx->height);
		s->outbuf.w=ctx->width;
		s->outbuf.h=ctx->height;
		s->sws_ctx=ms_video_scalercontext_init(ctx->width,ctx->height,ffmpeg_pix_fmt_to_ms(ctx->pix_fmt),
			ctx->width,ctx->height,MS_YUV420P,MS_YUVFAST,
                	NULL, NULL, NULL);
	}
	if (ms_video_scalercontext_convert(s->sws_ctx,orig->data,orig->linesize, 0,
					ctx->height, s->outbuf.planes, s->outbuf.strides)<0){
		ms_error("%s: error in ms_video_scalercontext_convert().",f->desc->name);
	}
	return dupmsg(s->yuv_msg);
}

static void update_sps(X264DecState *d, mblk_t *sps){
	if (d->sps)
		freemsg(d->sps);
	d->sps=NULL;
	if (sps)
		d->sps=dupb(sps);
}

static void update_pps(X264DecState *d, mblk_t *pps){
	if (d->pps)
		freemsg(d->pps);
	d->pps=NULL;
	if (pps)
		d->pps=dupb(pps);
}

static bool_t check_sps_pps_change(X264DecState *d, mblk_t *sps, mblk_t *pps){
	bool_t ret1=FALSE,ret2=FALSE;
	if (d->sps){
		if (sps){
			ret1=(msgdsize(sps)!=msgdsize(d->sps)) || (memcmp(d->sps->b_rptr,sps->b_rptr,msgdsize(sps))!=0);
			if (ret1) {
				update_sps(d,sps);
				ms_message("SPS changed !");
				update_pps(d,NULL);
			}
		}
	}else if (sps) {
		ms_message("Receiving first SPS");
		update_sps(d,sps);
	}
	if (d->pps){
		if (pps){
			ret2=(msgdsize(pps)!=msgdsize(d->pps)) || (memcmp(d->pps->b_rptr,pps->b_rptr,msgdsize(pps))!=0);
			if (ret2) {
				update_pps(d,pps);
				ms_message("PPS changed ! %i,%i",msgdsize(pps),msgdsize(d->pps));
			}
		}
	}else if (pps) {
		ms_message("Receiving first PPS");
		update_pps(d,pps);
	}
	return ret1 || ret2;
}

static void enlarge_bitstream(X264DecState *d, int new_size){
	d->bitstream_size=new_size;
	d->bitstream=(uint8_t*)ms_realloc(d->bitstream,d->bitstream_size);
}

static int nalusToFrame(X264DecState *d, MSQueue *naluq, bool_t *new_sps_pps){
	mblk_t *im;
	uint8_t *dst=d->bitstream,*src,*end;
	int nal_len;
	bool_t start_picture=TRUE;
	uint8_t nalu_type;
	*new_sps_pps=FALSE;
	end=d->bitstream+d->bitstream_size;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nal_len=im->b_wptr-src;
		if (dst+nal_len+100>end){
			int pos=dst-d->bitstream;
			enlarge_bitstream(d, d->bitstream_size+nal_len+100);
			dst=d->bitstream+pos;
			end=d->bitstream+d->bitstream_size;
		}
		nalu_type=(*src) & ((1<<5)-1);
		if (nalu_type==7)
			*new_sps_pps=check_sps_pps_change(d,im,NULL) || *new_sps_pps;
		if (nalu_type==8)
			*new_sps_pps=check_sps_pps_change(d,NULL,im) || *new_sps_pps;
		if (start_picture || nalu_type==7/*SPS*/ || nalu_type==8/*PPS*/ ){
			*dst++=0;
			start_picture=FALSE;
		}
		/*prepend nal marker*/
		*dst++=0;
		*dst++=0;
		*dst++=1;
		*dst++=*src++;
		while(src<(im->b_wptr-3)){
			if (src[0]==0 && src[1]==0 && src[2]<3){
				*dst++=0;
				*dst++=0;
				*dst++=3;
				src+=2;
			}
			*dst++=*src++;
		}
		*dst++=*src++;
		*dst++=*src++;
		*dst++=*src++;
		freemsg(im);
	}
	return dst-d->bitstream;
}

static void dec_process(MSFilter *f){
	X264DecState *d=(X264DecState*)f->data;
	mblk_t *im;
	MSQueue nalus;
	AVFrame orig;
	ms_queue_init(&nalus);
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		/*push the sps/pps given in sprop-parameter-sets if any*/
		if (d->packet_num==0 && d->sps && d->pps){
			mblk_set_timestamp_info(d->sps,mblk_get_timestamp_info(im));
			mblk_set_timestamp_info(d->pps,mblk_get_timestamp_info(im));
			rfc3984_unpack(&d->unpacker,d->sps,&nalus);
			rfc3984_unpack(&d->unpacker,d->pps,&nalus);
			d->sps=NULL;
			d->pps=NULL;
		}
		rfc3984_unpack(&d->unpacker,im,&nalus);
		if (!ms_queue_empty(&nalus)){
			int size;
			uint8_t *p,*end;
			bool_t need_reinit=FALSE;

			size=nalusToFrame(d,&nalus,&need_reinit);
			if (need_reinit)
				dec_reinit(d);
			p=d->bitstream;
			end=d->bitstream+size;
			while (end-p>0) {
				int len;
				int got_picture=0;
				AVPacket pkt;
				avcodec_get_frame_defaults(&orig);
				av_init_packet(&pkt);
				pkt.data = p;
				pkt.size = end-p;
				len=avcodec_decode_video2(&d->av_context,&orig,&got_picture,&pkt);
				if (len<=0) {
					ms_warning("ms_AVdecoder_process: error %i.",len);
					break;
				}
				if (got_picture) {
					ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,&orig));
				}
				p+=len;
			}
		}
		d->packet_num++;
	}
}


static int dec_add_fmtp(MSFilter *f, void *arg){
	X264DecState *d=(X264DecState*)f->data;
	const char *fmtp=(const char *)arg;
	char value[256];
	if (fmtp_get_value(fmtp,"sprop-parameter-sets",value,sizeof(value))){
		char * b64_sps=value;
		char * b64_pps=strchr(value,',');
		if (b64_pps){
			*b64_pps='\0';
			++b64_pps;
			ms_message("Got sprop-parameter-sets : sps=%s , pps=%s",b64_sps,b64_pps);
			d->sps=allocb(sizeof(value),0);
			d->sps->b_wptr+=b64_decode((const char *)b64_sps,(size_t)strlen(b64_sps),(void *)d->sps->b_wptr,(size_t)sizeof(value));
			d->pps=allocb(sizeof(value),0);
			d->pps->b_wptr+=b64_decode(b64_pps,strlen(b64_pps),d->pps->b_wptr,sizeof(value));
		}
	}
	return 0;
}

static MSFilterMethod  h264_dec_methods[]={
	{	MS_FILTER_ADD_FMTP	,	dec_add_fmtp	},
	{	0			,	NULL	}
};

static MSFilterDesc h264_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSH264FFMPEGDec",
	"A H264 decoder based on ffmpeg project.",
	MS_FILTER_DECODER,
	"H264",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	h264_dec_methods
};

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

GLOBAL_LINKAGE void libmsantisipx264_init(void);

GLOBAL_LINKAGE void libmsantisipx264_init(void){
	ms_filter_register(&x264_enc_desc);
	ms_filter_register(&h264_dec_desc);
}
