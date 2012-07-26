/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003-2012 Aymeric MOIZARD - <amoizard@gmail.com>
*/

#include <math.h>

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/rfc3984.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msticker.h>

#if defined(__cplusplus)
#define B64_NO_NAMESPACE
#endif
#include <ortp/b64.h>

//UMC includes
#include <umc_video_encoder.h>
#include <umc_video_decoder.h>

#include <umc_h264_video_encoder.h>
#include <umc_h264_dec.h>
#include <umc_color_space_conversion.h>

#include <ippi.h>

#define REMOVE_PREVENTING_BYTES 1
#define FORCE_MODE -1

static int level_idc = 32;

using namespace::UMC;

/* #define USE_H264_UMC_DEC */
#ifdef USE_H264_UMC_DEC

class MSUMCDec{
	public:
		MSUMCDec(){
			_dec=new H264VideoDecoder();
			_csv=new ColorSpaceConversion();
			_omsg=NULL;
			_dec_ready=false;
			rfc3984_init(&_unpacker);
		}
		mblk_t * decode(mblk_t *im);
		~MSUMCDec(){
			if (_omsg) freemsg(_omsg);
			rfc3984_uninit(&_unpacker);
			delete _dec;
			delete _csv;
		}
	private:
		bool convNalusToFrame(MSQueue *naluq,MediaData *md);
		mblk_t *getYuvBuf();
		VideoDecoder *_dec;
		ColorSpaceConversion *_csv;
		Rfc3984Context _unpacker;
		MSPicture _opict;
		mblk_t *_omsg;
		bool _dec_ready;
		bool _pad[3];
		uint8_t _bitstream[64000];
};


bool MSUMCDec::convNalusToFrame(MSQueue *naluq, MediaData *md){
	mblk_t *im;
	uint8_t *dst=_bitstream,*src;
	bool start_picture=true;
	uint8_t nalu_type;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nalu_type=(*src) & ((1<<5)-1);
		if (start_picture || nalu_type==7/*SPS*/ || nalu_type==8/*PPS*/ ){
			*dst++=0;
			start_picture=false;
		}
		/*prepend nal marker*/
		*dst++=0;
		*dst++=0;
		*dst++=1;
		*dst++=*src++;
		while(src<(im->b_wptr-3)){
#ifdef REMOVE_PREVENTING_BYTES
			if (src[0]==0 && src[1]==0 && src[2]<=3){
				*dst++=0;
				*dst++=0;
				*dst++=3;
				src+=2;
			}
#endif
			*dst++=*src++;
		}
		*dst++=*src++;
		*dst++=*src++;
		*dst++=*src++;
		freemsg(im);
	}
	md->SetBufferPointer(_bitstream,sizeof(_bitstream));
	md->SetDataSize(dst-_bitstream);
	return md->GetDataSize()>0;
}

mblk_t * MSUMCDec::getYuvBuf(){
	if (_omsg && _omsg->b_datap->db_ref>1){
		freemsg(_omsg);
		_omsg=0;
	}
	if (_omsg==0){
		_omsg=yuv_buf_alloc(&_opict,_opict.w,_opict.h);
	}
	return dupb(_omsg);
}

mblk_t * MSUMCDec::decode(mblk_t *im){
	MSQueue naluq;
	mblk_t *om=NULL;
	Status s;

	ms_queue_init(&naluq);
	rfc3984_unpack(&_unpacker,im,&naluq);
	MediaData md;
	if (convNalusToFrame(&naluq,&md)){
		if (!_dec_ready){
			VideoDecoderParams par;
			par.m_pData=&md;
			par.pPostProcessing=_csv;
			s=_dec->Init(&par);
			if (s==UMC_OK){
				VideoDecoderParams params;
				s=_dec->GetInfo(&params);
				if (s==UMC_OK){
					_opict.w=params.info.clip_info.width;
					_opict.h=params.info.clip_info.height;
					_dec_ready=true;
					ms_message("Decoder ready, incoming video is %ix%i",_opict.w,_opict.h);
					md.MoveDataPointer(0);
				}else{
					ms_error("GetInfo failed: %s",GetErrString(s));
				}
			}else{
				ms_error("decoder Init() failed: %s",
					GetErrString(s));
			}
		}
		if (_dec_ready){
			VideoData vd;
			om=getYuvBuf();
			vd.Init(_opict.w,_opict.h,YUV420);
			vd.SetPlanePointer(_opict.planes[0],0);
			vd.SetPlanePointer(_opict.planes[1],1);
			vd.SetPlanePointer(_opict.planes[2],2);
			vd.SetPlanePitch(_opict.strides[0],0);
			vd.SetPlanePitch(_opict.strides[1],1);
			vd.SetPlanePitch(_opict.strides[2],2);
			s=_dec->GetFrame(&md,&vd);
			if (s!=UMC_OK){
				ms_error("decoder GetFrame failed: %s",GetErrString(s));
				freemsg(om);
				om=NULL;
			}			
		}
	}
	return om;
}


static void dec_init(MSFilter *f){
  //ippStaticInit();
	f->data=(void*)new MSUMCDec();
}

static void dec_uninit(MSFilter *f){
	delete (MSUMCDec*)f->data;
}

static void dec_process(MSFilter *f){
	MSUMCDec *dec=(MSUMCDec*)f->data;
	mblk_t *im,*om;
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		om=dec->decode(im);
		if (om) ms_queue_put(f->outputs[0],om);
	}
}

MSFilterDesc ms_umc_h264_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSIppH264Dec",
	"H264 decoder",
	MS_FILTER_DECODER,
	"h264",
	1,
	1,
	dec_init,
	NULL,
	dec_process,
	NULL,
	dec_uninit,
	NULL
};

#else

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51,13,0)
#include <libavutil/opt.h>
#endif

#ifdef __cplusplus
}
#endif

typedef struct _DecData{
	mblk_t *yuv_msg;
	mblk_t *sps,*pps;
	Rfc3984Context unpacker;
	MSPicture outbuf;
	struct MSScalerContext *sws_ctx;
	AVCodecContext av_context;
	unsigned int packet_num;
	uint8_t *bitstream;
	int bitstream_size;
	int mStat_incoming_image;
}DecData;

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

static void dec_open(DecData *d){
	AVCodec *codec;
	int error;
	codec=avcodec_find_decoder(CODEC_ID_H264);
	if (codec==NULL) ms_fatal("Could not find H264 decoder in ffmpeg.");
	avcodec_get_context_defaults(&d->av_context);
	d->av_context.flags2 |= CODEC_FLAG2_CHUNKS;
	error=avcodec_open(&d->av_context,codec);
	if (error!=0){
		ms_fatal("avcodec_open() failed.");
	}
}

static void dec_init(MSFilter *f){
	DecData *d=(DecData*)ms_new(DecData,1);
	ffmpeg_init();
	d->yuv_msg=NULL;
	d->sps=NULL;
	d->pps=NULL;

#if 0
	{
		char value[256];
		snprintf(value, sizeof(value), "Z0IAHqtAoPyA,aM44gA==");
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
#endif

	d->sws_ctx=NULL;
	rfc3984_init(&d->unpacker);
	d->packet_num=0;
	dec_open(d);
	d->outbuf.w=0;
	d->outbuf.h=0;
	d->bitstream_size=65536;
	d->bitstream=(uint8_t*)ms_malloc0(d->bitstream_size);
	d->mStat_incoming_image=0;
	f->data=d;
}

static void dec_reinit(DecData *d){
	avcodec_close(&d->av_context);
	dec_open(d);
}

static void dec_uninit(MSFilter *f){
	DecData *d=(DecData*)f->data;
	rfc3984_uninit(&d->unpacker);
	avcodec_close(&d->av_context);
	if (d->sws_ctx!=NULL)	ms_video_scalercontext_free(d->sws_ctx);
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

static mblk_t *get_as_yuvmsg(MSFilter *f, DecData *s, AVFrame *orig){
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

		// CHECK pix_fmt -> convert to MS_xxx
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

static void update_sps(DecData *d, mblk_t *sps){
	if (d->sps)
		freemsg(d->sps);
	d->sps=NULL;
	if (sps)
		d->sps=dupb(sps);
}

static void update_pps(DecData *d, mblk_t *pps){
	if (d->pps)
		freemsg(d->pps);
	d->pps=NULL;
	if (pps)
		d->pps=dupb(pps);
}

static bool_t check_sps_pps_change(DecData *d, mblk_t *sps, mblk_t *pps){
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

static void enlarge_bitstream(DecData *d, int new_size){
	d->bitstream_size=new_size;
	d->bitstream=(uint8_t*)ms_realloc(d->bitstream,d->bitstream_size);
}

static int nalusToFrame(DecData *d, MSQueue *naluq, bool_t *new_sps_pps){
	mblk_t *im;
	uint8_t *dst=d->bitstream,*src,*end;
	int nal_len;
	bool_t start_picture=TRUE;
	uint8_t nalu_type;
	*new_sps_pps=FALSE;
	end=d->bitstream+d->bitstream_size;
	while((im=ms_queue_get(naluq))!=NULL){
		src=im->b_rptr;
		nal_len=(int)(im->b_wptr-src);
		if (dst+nal_len+100>end){
			int pos=(int)(dst-d->bitstream);
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
	return (int)(dst-d->bitstream);
}

static void dec_process(MSFilter *f){
	DecData *d=(DecData*)f->data;
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
				pkt.size = (int)(end-p);
				len=avcodec_decode_video2(&d->av_context,&orig,&got_picture,&pkt);
				if (len<=0) {
					ms_warning("ms_AVdecoder_process: error %i.",len);
					break;
				}
				if (got_picture) {
					d->mStat_incoming_image++;
					if (d->mStat_incoming_image%100==0)
						ms_message("dec_process: stat: incoming_image %i.",d->mStat_incoming_image);
						
					ms_queue_put(f->outputs[0],get_as_yuvmsg(f,d,&orig));
				}
				p+=len;
			}
		}
		d->packet_num++;
	}
}


static int dec_add_fmtp(MSFilter *f, void *arg){
	DecData *d=(DecData*)f->data;
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
	"MSH264Dec",
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


#endif

class MSUMCEnc{
	public:
		MSUMCEnc(){
			_enc=new H264VideoEncoder();
			rfc3984_init(&_packer);
			rfc3984_set_mode(&_packer,0); /* default mode */
			mode=0;
			if (FORCE_MODE>=0)
			{
				rfc3984_set_mode(&_packer,FORCE_MODE);
				mode=FORCE_MODE;
			}

			_bitrate=512000;
			_fps=15;
			_vsize.width=MS_VIDEO_SIZE_CIF_W;
			_vsize.height=MS_VIDEO_SIZE_CIF_H;
			_send_idr=false;
			_bitstream=NULL;
			initialized=false;
			framenum=0;
			sps=NULL;
			pps=NULL;
		}
		int getBitrate(){
			return (int)((float)_bitrate/0.9);
		}
		void setBitrate(int br){
			//remove a few to take account of RTP/UDP/IP + encoder peaks
			_bitrate=(int)((float)br*0.9);
			if (br>=8192000){
				_vsize.width=MS_VIDEO_SIZE_720P_W;
				_vsize.height=MS_VIDEO_SIZE_720P_H;
				_fps=30;
			}else if (br>=4096000){
				_vsize.width=MS_VIDEO_SIZE_VGA_W;
				_vsize.height=MS_VIDEO_SIZE_VGA_H;
				_fps=30;
			}else if (br>=2048000){
				_vsize.width=MS_VIDEO_SIZE_VGA_W;
				_vsize.height=MS_VIDEO_SIZE_VGA_H;
				_fps=30;
			}else if (br>=1024000){
				_vsize.width=MS_VIDEO_SIZE_VGA_W;
				_vsize.height=MS_VIDEO_SIZE_VGA_H;
				_fps=30;
			}else if (br>=512000){
				_vsize.width=MS_VIDEO_SIZE_CIF_W;
				_vsize.height=MS_VIDEO_SIZE_CIF_H;
				_fps=30;
			}else if (br>=256000){
				_vsize.width=MS_VIDEO_SIZE_CIF_W;
				_vsize.height=MS_VIDEO_SIZE_CIF_H;
				_fps=25;
			}else if (br>=128000){
				_vsize.width=MS_VIDEO_SIZE_CIF_W;
				_vsize.height=MS_VIDEO_SIZE_CIF_H;
				_fps=20;
			}else if (br>=64000){
				_vsize.width=MS_VIDEO_SIZE_QCIF_W;
				_vsize.height=MS_VIDEO_SIZE_QCIF_H;
				_fps=10;
			}else if (br>=20000){
				/* for static images */
				_vsize.width=MS_VIDEO_SIZE_CIF_W;
				_vsize.height=MS_VIDEO_SIZE_CIF_H;
				_fps=1;
				_bitrate=_bitrate*2;
			}
		}
		void generateIDR(){
			_send_idr=true;
		}
		void setFps(float fps){
			_fps=fps;
		}
		void setSize(MSVideoSize sz){
			_vsize=sz;
		}
		void addFmtp(const char *fmtp){
			char strmode[10];
			if (fmtp_get_value(fmtp,"packetization-mode",strmode,sizeof(strmode))){
				if (FORCE_MODE<0)
				{
					mode=atoi(strmode);
					rfc3984_set_mode(&_packer,mode);
				}
			}
		}
		MSVideoSize getSize()const{
			return _vsize;
		}
		float getFps()const{
			return _fps;
		}
		void init(){
			Status s;
			params.info.bitrate=_bitrate;
			params.info.framerate=_fps;
			params.info.clip_info.width=_vsize.width;
			params.info.clip_info.height=_vsize.height;
			params.profile_idc=H264_PROFILE_BASELINE; //H264_BASE_PROFILE;
			params.transform_8x8_mode_flag = false;

			//send I frame every 5 seconds
			params.key_frame_controls.method=H264_KFCM_INTERVAL;
			params.key_frame_controls.interval=(Ipp32s)(5*_fps);
			params.key_frame_controls.idr_interval=1;

			params.entropy_coding_mode=0;
			//level_1   = 10, /*!< 0x0A */
			//level_1_1 = 11, /*!< 0x0B */
			//level_1_2 = 12, /*!< 0x0C */
			//level_1_3 = 13, /*!< 0x0D */
			//level_2   = 20, /*!< 0x14 */
			//level_2_1 = 21, /*!< 0x15 */
			//level_2_2 = 22, /*!< 0x16 */
			//level_3   = 30, /*!< 0x1E */
			//level_3_1 = 31, /*!< 0x1F */
			//level_3_2 = 32, /*!< 0x20 */
			//level_4   = 40, /*!< 0x28 */
			//level_4_1 = 41, /*!< 0x29 */
			//level_4_2 = 42, /*!< 0x2A */
			//level_5   = 50, /*!< 0x32 */
			//level_5_1 = 51, /*!< 0x33 */
			params.level_idc=level_idc;

			params.rate_controls.method=H264_RCM_CBR;

			rfc3984_set_mode(&_packer,mode);

#ifdef __APPLE__
            /* not sure wether I need this or not, do more test */
			params.num_slices = 1;
#endif
			if (_packer.mode==0){
				params.num_slices = -_packer.maxsz;
			}
			s=_enc->Init(&params);
			if (s!=UMC_OK){
				ms_error("Fail to initialize encoder !");
			}
			initialized=true;
			framenum=0;

			sps=NULL;
			pps=NULL;
		}
		void reset(){
			Status s;
			s=_enc->Reset();
			if (s!=UMC_OK){
				ms_error("Fail to reset encoder !");
			}
			rfc3984_uninit(&_packer);
			rfc3984_init(&_packer);
			initialized=false;
			framenum=0;
			if (sps!=NULL)
				freeb(sps);
			if (pps!=NULL)
				freeb(pps);
			sps=NULL;
			pps=NULL;
		}
		void encode(mblk_t *im, MSQueue *output, uint32_t timestamp);
		~MSUMCEnc(){
			rfc3984_uninit(&_packer);
			delete _enc;
			if (_bitstream!=NULL)
				freeb(_bitstream);
			if (sps!=NULL)
				freeb(sps);
			if (pps!=NULL)
				freeb(pps);
			sps=NULL;
			pps=NULL;
		}
		void push_nalu(MSQueue *nalus, uint8_t *begin, uint8_t *end);
		void get_nalus(uint8_t *frame, int frame_size, MSQueue *nalus);
		VideoEncoder *_enc;
		H264EncoderParams params;
		Rfc3984Context _packer;
		int mode; /* save mode after detach/attach operation */
		MSVideoSize _vsize;
		int _bitrate;
		float _fps;
		//uint8_t _bitstream_old[65000];
		mblk_t *_bitstream;
		bool _send_idr;
		bool initialized;
		uint64_t framenum;
		mblk_t *sps;
		mblk_t *pps;
};

void  MSUMCEnc::push_nalu(MSQueue *nalus, uint8_t *begin, uint8_t *end){
	mblk_t *m;
	uint8_t *src=begin;
	m=allocb((int)(end-begin),0);
	uint8_t nalu_type=(*begin) & ((1<<5)-1);
#ifdef REMOVE_PREVENTING_BYTES
	*m->b_wptr++=*src++;
	while(src<end-3){
		if (src[0]==0 && src[1]==0 && src[2]==3){
			*m->b_wptr++=0;
			*m->b_wptr++=0;
			src+=3;
			continue;
		}
		*m->b_wptr++=*src++;
	}
	*m->b_wptr++=*src++;
	*m->b_wptr++=*src++;
	*m->b_wptr++=*src++;
#else
	memcpy(m->b_wptr,begin,end-begin);
	m->b_wptr+=(end-begin);
#endif
	if (nalu_type==5) {
		ms_message("A IDR is being sent.");
	}else if (nalu_type==7) {
		ms_message("A SPS is being sent.");
		if (sps!=NULL)
			freeb(sps);
		sps=dupb(m);
	}
	else if (nalu_type==8) {
		ms_message("A PPS is being sent.");
		if (pps!=NULL)
			freeb(pps);
		pps=dupb(m);
	}
	ms_queue_put(nalus,m);	
}

void  MSUMCEnc::get_nalus(uint8_t *frame, int frame_size, MSQueue *nalus){
	int i;
	uint8_t *p,*begin=NULL;
	int zeroes=0;
	
	for(i=0,p=frame;i<frame_size;++i){
		if (*p==0){
			++zeroes;
		}else if (zeroes>=2 && *p==1 ){
			if (begin){
				push_nalu(nalus,begin,p-zeroes);
			}
			begin=p+1;
		}else zeroes=0;
		++p;
	}
	if (begin) push_nalu(nalus,begin,p);
}

void MSUMCEnc::encode(mblk_t *im, MSQueue *output, uint32_t ts){
	MSPicture in_frame;
	if (yuv_buf_init_from_mblk(&in_frame,im)!=0){
		freemsg(im);
		return;
	}
	if (in_frame.w!=params.info.clip_info.width
		|| in_frame.h!=params.info.clip_info.height)
	{
		ms_message("h264 encoder: wrong size for incoming image (%ix%i expected=%ix%i)",
			in_frame.w, in_frame.h, _vsize.width, _vsize.height);
		freemsg(im);
		return;
	}

	framenum++;
	if (framenum==(int)(_fps*2))
	{
		ms_message("h264 encoder: force a SPS/PPS retransmission afer 2 sec");
		_send_idr=true;
		if (sps!=NULL && pps!=NULL)
		{
			MSQueue nalus;
			ms_queue_init(&nalus);
			ms_queue_put(&nalus,sps);
			ms_queue_put(&nalus,pps);
			rfc3984_pack(&_packer,&nalus,output,ts);
			sps=NULL;
			pps=NULL;
		}
	}

	VideoData vd;
	MediaData md;
	Status s;
	vd.Init(in_frame.w,in_frame.h,YV12);
	vd.SetPlanePointer(in_frame.planes[0],0);
	vd.SetPlanePointer(in_frame.planes[2],1);
	vd.SetPlanePointer(in_frame.planes[1],2);
	vd.SetPlanePitch(in_frame.strides[0],0);
	vd.SetPlanePitch(in_frame.strides[2],1);
	vd.SetPlanePitch(in_frame.strides[1],2);
	//md.SetBufferPointer(_bitstream_old,sizeof(_bitstream_old));
	if (_bitstream==NULL)
	{
		_bitstream = allocb(8*_bitrate/((int)_fps), 0);
		if (_bitstream==NULL)
		{
			freemsg(im);
			return;
		}
		_bitstream->b_wptr = _bitstream->b_rptr + 8*_bitrate/((int)_fps);
		ms_message("h264 encoder: allocating buffer (%i)", msgdsize(_bitstream));
	}
	md.SetBufferPointer(_bitstream->b_rptr,msgdsize(_bitstream));
	if (_send_idr){
		_send_idr=false;
		vd.SetFrameType(I_PICTURE);
	}
	s=_enc->GetFrame(&vd,&md);
	while (s==UMC::UMC_ERR_NOT_ENOUGH_BUFFER)
	{
		int size = msgdsize(_bitstream);
		if (size>1000000)
		{
			ms_error("h264 encoder: stop re-allocating buffer (%i>1000000)");
			break;
		}
		freeb(_bitstream);
		_bitstream = allocb(size*2, 0);
		if (_bitstream==NULL)
		{
			ms_error("h264 encoder: error re-allocating buffer (%i)", size*2);
			break;
		}
		_bitstream->b_wptr = _bitstream->b_rptr + size*2;
		ms_message("h264 encoder: allocating buffer (%i)", size*2);		

		md.SetBufferPointer(_bitstream->b_rptr,msgdsize(_bitstream));
		s=_enc->GetFrame(&vd,&md);
	}
	if (s==UMC_OK){
		MSQueue nalus;
		ms_queue_init(&nalus);
		get_nalus((uint8_t*)md.GetBufferPointer(),(int)md.GetDataSize(),&nalus);
		rfc3984_pack(&_packer,&nalus,output,ts);
	}else{
		ms_error("Fail to encode frame: %s",GetErrString(s));
	}
	freemsg(im);
}

static void h264_enc_init(MSFilter *f){
  //ippStaticInit();
	f->data=new MSUMCEnc();
}

static void enc_uninit(MSFilter *f){
	delete (MSUMCEnc*)f->data;
}

static void enc_preprocess(MSFilter *f){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	enc->init();
}

static void enc_postprocess(MSFilter *f){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	enc->reset();
}

static void enc_process(MSFilter *f){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	uint32_t ts=(uint32_t)(f->ticker->time*90LL);
	mblk_t *im;
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		enc->encode(im,f->outputs[0],ts);
	}
}

static int enc_set_vsize(MSFilter *f, void *arg){
	MSVideoSize *sz=(MSVideoSize*)arg;
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	enc->setSize(*sz);
	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	enc->addFmtp((char*)arg);
	return 0;
}

static int enc_set_br(MSFilter *f, void *arg){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	enc->setBitrate(*(int*)arg);
	if (enc->initialized==true){
		/*apply new settings dynamically*/
		ms_filter_lock(f);
		enc_postprocess(f);
		enc_preprocess(f);
		ms_filter_unlock(f);
	}
	return 0;
}

static int enc_get_br(MSFilter *f, void *arg){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	*(int*)arg=enc->getBitrate();
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	MSVideoSize *sz=(MSVideoSize*)arg;
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	*sz=enc->getSize();
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	float *fps=(float*)arg;
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	*fps=enc->getFps();
	return 0;
}

static int enc_req_vfu(MSFilter *f, void *arg){
	MSUMCEnc *enc=(MSUMCEnc*)f->data;
	enc->generateIDR();
	return 0;
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_SET_VIDEO_SIZE,	enc_set_vsize},
	{	MS_FILTER_GET_VIDEO_SIZE,	enc_get_vsize},
	{	MS_FILTER_GET_FPS,		enc_get_fps },
	{	MS_FILTER_ADD_FMTP,		enc_add_fmtp },
	{	MS_FILTER_GET_BITRATE	,	enc_get_br	},
	{	MS_FILTER_SET_BITRATE	,	enc_set_br	},
	{	MS_FILTER_REQ_VFU	,	enc_req_vfu	},
	{	0	,		NULL}
};

MSFilterDesc ms_umc_h264_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSIppH264Enc",
	"H264 encoder",
	MS_FILTER_ENCODER,
	"h264",
	1,
	1,
	h264_enc_init,
	enc_preprocess,
	enc_process,
	enc_postprocess,
	enc_uninit,
	enc_methods
};

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

extern "C" GLOBAL_LINKAGE void libmsipph264_init(void){
	const IppLibraryVersion* ippj;
	ippStaticInit();

	ippj = ippiGetLibVersion();

	ms_message("msipph264: Intel(R) Integrated Performance Primitives ");
	ms_message("  version: %s, [%d.%d.%d.%d] ",
		ippj->Version, ippj->major, ippj->minor, ippj->build, ippj->majorBuild);
	ms_message("  name:    %s ", ippj->Name);
	ms_message("  date:    %s ", ippj->BuildDate);
	ms_message("msipph264:    starting with level_idc %i ", level_idc);

#ifdef USE_H264_UMC_DEC
	ms_filter_register(&ms_umc_h264_dec_desc);
#else
	ms_filter_register(&h264_dec_desc);
#endif
	ms_filter_register(&ms_umc_h264_enc_desc);
}

extern "C" GLOBAL_LINKAGE void libmsipph264_idc12_init(void){
	level_idc=12;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc13_init(void){
	level_idc=13;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc20_init(void){
	level_idc=20;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc21_init(void){
	level_idc=21;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc22_init(void){
	level_idc=22;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc30_init(void){
	level_idc=30;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc31_init(void){
	level_idc=31;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc32_init(void){
	level_idc=32;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc40_init(void){
	level_idc=40;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc41_init(void){
	level_idc=41;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc42_init(void){
	level_idc=42;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc50_init(void){
	level_idc=50;
	libmsipph264_init();
}
extern "C" GLOBAL_LINKAGE void libmsipph264_idc51_init(void){
	level_idc=51;
	libmsipph264_init();
}
