#ifdef WIN32
#include <winsock2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <ippcore.h>
#include <ipps.h>
#include <ippsc.h>
#include <ippi.h>

#include "usc.h"

#include "util.h"
#include "loadcodec.h"
#include "wavfile.h"
#include "usccodec.h"

#include "g711api.h"

#include "mediastreamer2/msfilter.h"

Ipp32s ProcessOneFrameOneChannel(USCParams *uscPrms, Ipp8s *inputBuffer,
                              Ipp8s *outputBuffer, Ipp32s *pSrcLenUsed, Ipp32s *pDstLenUsed, FILE *f_log);

#ifdef __cplusplus
}
#endif

#if defined(_USC_GSMAMR) || defined(_USC_AMRWB)
#include "rtp_amr_payload.h"
#include "base_rtp_cnvrt.h"
#endif

#if defined(_USC_GSMAMR) || defined(_USC_AMRWB)
using namespace UMC;
#endif

typedef struct IPPEncState{
	int nsamples;
	int nbytes;
	int ms_per_frame;
	int ptime;
	uint32_t ts;
	MSBufferizer bufferizer;
	
	LoadedCodec codec;
#if defined(_USC_GSMAMR) || defined(_USC_AMRWB)
	RTPBasePacketizerParams *packetizerParams;
	RTPBasePacketizer *packetizer;
	RTPBaseControlParams *controlParams;
#endif

	//for G729.1
	int maxbitrate;
	int mbs;
	int dtx;
}IPPEncState;


typedef struct IPPDecState{
	LoadedCodec codec;
#if defined(_USC_GSMAMR) || defined(_USC_AMRWB)
	RTPBaseDePacketizerParams *depacketizerParams;
	RTPBaseDepacketizer *depacketizer;
#endif
}IPPDecState;


static int enc_loadcodec(LoadedCodec *codec, int bitrate, int vad)
{
	int lCallResult;
	lCallResult = LoadUSCCodecByName(codec,NULL);
	/*Get USC codec params*/
	lCallResult = USCCodecAllocInfo(&codec->uscParams, NULL);
	if(lCallResult<0) return lCallResult;
	lCallResult = USCCodecGetInfo(&codec->uscParams, NULL);
	if(lCallResult<0) return lCallResult;
	// Get its supported format details
   lCallResult = GetUSCCodecParamsByFormat(codec, BY_NAME, NULL);
   if (lCallResult < 0) { return lCallResult; }

   // Set params for encode
   USC_PCMType streamType;
   streamType.bitPerSample = codec->uscParams.pInfo->params.pcmType.bitPerSample;
   streamType.nChannels = 1;
   streamType.sample_frequency = codec->uscParams.pInfo->params.pcmType.sample_frequency;

   lCallResult = SetUSCEncoderPCMType(&codec->uscParams, LINEAR_PCM, &streamType, NULL);
   if (lCallResult < 0) { return lCallResult; }

   codec->uscParams.pInfo->params.direction = USC_ENCODE;
   codec->uscParams.pInfo->params.law = 0;
   codec->uscParams.pInfo->params.modes.vad = vad;
   if (bitrate>0)
	   codec->uscParams.pInfo->params.modes.bitrate = bitrate;

   lCallResult = USCCodecAlloc(&codec->uscParams, NULL);
   if(lCallResult<0) return lCallResult;
   /*Init encoder*/
   lCallResult = USCEncoderInit(&codec->uscParams, NULL, NULL);
   if(lCallResult<0) return lCallResult;
   return 0;
}

static int dec_loadcodec(LoadedCodec *codec)
{
	int lCallResult;
	lCallResult = LoadUSCCodecByName(codec,NULL);
	/*Get USC codec params*/
	lCallResult = USCCodecAllocInfo(&codec->uscParams, NULL);
	if(lCallResult<0) return lCallResult;
	lCallResult = USCCodecGetInfo(&codec->uscParams, NULL);
	if(lCallResult<0) return lCallResult;
	// Get its supported format details
   lCallResult = GetUSCCodecParamsByFormat(codec, BY_NAME, NULL);
   if (lCallResult < 0) { return lCallResult; }

   // Set params for decode
   USC_PCMType streamType;
   streamType.bitPerSample = codec->uscParams.pInfo->params.pcmType.bitPerSample;
   streamType.nChannels = 1;
   streamType.sample_frequency = codec->uscParams.pInfo->params.pcmType.sample_frequency;

   lCallResult = SetUSCDecoderPCMType(&codec->uscParams, LINEAR_PCM, &streamType, NULL);
   if (lCallResult < 0) { return lCallResult; }

   codec->uscParams.pInfo->params.direction = USC_DECODE;
   codec->uscParams.pInfo->params.law = 0;

   lCallResult = USCCodecAlloc(&codec->uscParams, NULL);
   if(lCallResult<0) return lCallResult;
   /*Init encoder*/
   lCallResult = USCDecoderInit(&codec->uscParams, NULL, NULL);
   if(lCallResult<0) return lCallResult;
   return 0;
}

/***************************** G729 *******************************/

static void enc_g729_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=10;
	s->nsamples=80; /* size of pcm data */
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_G729A");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 0, 0);

	f->data=s;
}

static void enc_g729_uninit(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void enc_g729_preprocess(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
}

static void enc_g729_process(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* 160 * 14 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=10 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 14*10 == 140 ms max */
		frame_per_packet=14;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		int inLen;
		int outLenData=0;
		USCCodecGetTerminationCondition(&s->codec.uscParams, &inLen);
		inLen=inLen+1;
		USCCodecGetSize(&s->codec.uscParams, inLen, &outLenData, NULL);

		om=allocb(outLenData*frame_per_packet,0);

		memset(om->b_wptr, 0, outLenData*frame_per_packet);
		for (k=0;k<frame_per_packet;k++)
		{
			char bit_stream[1024];
			Ipp32s frmlen, infrmLen, FrmDataLen;
			USC_PCMStream PCMStream;
			USC_Bitstream Bitstream;

			/*Do the pre-procession of the frame*/
			infrmLen = USCEncoderPreProcessFrame(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
				(Ipp8s*)bit_stream,&PCMStream,&Bitstream);
			/*Encode one frame*/
			FrmDataLen = USCCodecEncode(&s->codec.uscParams, &PCMStream,&Bitstream,NULL);
			if(FrmDataLen < 0) {
				freeb(om);
				om=NULL;
				break;
			}
			infrmLen += FrmDataLen;
			/*Do the post-procession of the frame*/
			frmlen = USCEncoderPostProcessFrame(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
				(Ipp8s*)bit_stream,&PCMStream,&Bitstream);

			if (Bitstream.nbytes == 10 || // 8000 bps
				Bitstream.nbytes == 8 || // 6400 bps
				Bitstream.nbytes == 14 || // 11800 bps
				Bitstream.nbytes == 15 || // 11800 bps
				Bitstream.nbytes == 2) // SID
			{
				memcpy(om->b_wptr, bit_stream+(frmlen-Bitstream.nbytes), Bitstream.nbytes);
				om->b_wptr+=Bitstream.nbytes;

				if (Bitstream.nbytes == 2)
				{
					ms_message("g729 b // SID sent");
					break;
				}
			} else {
				if (Bitstream.nbytes == 0)
				{
					ms_message("g729 b // cont");
				}
				freeb(om);
				om=NULL;
				break;
			}
		}

		//move time ANYWAY
		s->ts+=(s->nsamples*frame_per_packet);
		if (om!=NULL)
		{
			mblk_set_timestamp_info(om,s->ts-s->nsamples);
			ms_queue_put(f->outputs[0],om);
		}
	}
}

static int enc_g729_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
    
	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "annexb", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"no")!=NULL){
	}
	else if (strstr(buf,"yes")!=NULL){
        USCFree(&s->codec.uscParams);
        FreeUSCSharedObjects(&s->codec);
        memset(&s->codec, 0, sizeof(LoadedCodec));
        strcpy((char*)s->codec.codecName, "IPP_G729A");
        s->codec.lIsVad = 1;
        enc_loadcodec(&s->codec, 0, 1);
	}
	return 0;
}

static int enc_g729_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=10;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static MSFilterMethod enc_g729_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g729_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g729_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g729_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG729Enc",
	"G729 encoder",
	MS_FILTER_ENCODER,
	"G729",
	1,
	1,
	enc_g729_init,
    enc_g729_preprocess,
	enc_g729_process,
    NULL,
	enc_g729_uninit,
	enc_g729_methods
};

#else

MSFilterDesc ms_g729_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG729Enc",
	.text="G729 encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G729",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g729_init,
	.preprocess=enc_g729_preprocess,
	.process=enc_g729_process,
	.uninit=enc_g729_uninit,
	.methods=enc_g729_methods
};

#endif

static void dec_g729_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_G729A");
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g729_uninit(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void dec_g729_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;
		int FT;
		int BITRATE;
		int FRAMESIZE;
		int SID=0;

		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}

		if (nbytes%10==0 || nbytes%10==2) {
			// 8000 bps, G.729
			FT=3;
			BITRATE=8000;
			FRAMESIZE=10;
			frame_per_packet = nbytes/10;
		} else {
			freemsg(im);
			continue;
		}

		if (nbytes%FRAMESIZE==2)
			SID=1; // a SID frame exist after packet

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(80*2+2,0);
			in.bitrate = BITRATE;
			in.frametype = FT;
			in.nbytes = FRAMESIZE;
			in.pBuffer = (char*)im->b_rptr+(k*FRAMESIZE);
			out.pBuffer = (char*)om->b_wptr;

			if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
				ms_warning("Non Aligned pointer");

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			if (outLen>=0)
			{
				if (outLen!=FRAMESIZE)
					ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
				om->b_wptr+=80*2;
				ms_queue_put(f->outputs[0],om);
			}
			else
			{
				ms_error("Error decoding G729D");
				freeb(om);
				break;
			}
		}

		if (SID==1)
		{
			// a SID frame exist after packet
			ms_message("SID frame received!");
			
			/* should make a loop */
			{
				USC_PCMStream out;
				USC_Bitstream in;
				int outLen;
				om=allocb(80*2+2,0);
				in.bitrate = BITRATE;
				in.frametype = 1;
				in.nbytes = 2;
				in.pBuffer = (char*)im->b_wptr-2;
				out.pBuffer = (char*)om->b_wptr;

				if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
					ms_warning("Non Aligned pointer");

				outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
				if (outLen>=0)
				{
					if (outLen!=2)
						ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
					om->b_wptr+=80*2;
					ms_queue_put(f->outputs[0],om);
				}
				else
				{
					ms_error("Error decoding G729D");
					freeb(om);
					break;
				}
			}
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g729_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG729Dec",
	"G729 decoder",
	MS_FILTER_DECODER,
	"G729",
	1,
	1,
	dec_g729_init,
    NULL,
	dec_g729_process,
    NULL,
	dec_g729_uninit,
    NULL
};

#else

MSFilterDesc ms_g729_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG729Dec",
	.text="G729 decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G729",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g729_init,
	.process=dec_g729_process,
	.uninit=dec_g729_uninit
};

#endif

/***************************** G729D *******************************/

static void enc_g729d_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=10;
	s->nsamples=80; /* size of pcm data */
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_G729I");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 6400, 0);   
    
	f->data=s;
}

static int enc_g729d_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
    
	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "annexb", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"no")!=NULL){
	}
	else if (strstr(buf,"yes")!=NULL){
        USCFree(&s->codec.uscParams);
        FreeUSCSharedObjects(&s->codec);
        memset(&s->codec, 0, sizeof(LoadedCodec));
        strcpy((char*)s->codec.codecName, "IPP_G729I");
        s->codec.lIsVad = 1;
        enc_loadcodec(&s->codec, 6400, 1);
	}
	return 0;
}

static MSFilterMethod enc_g729d_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g729d_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g729_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g729d_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG729DEnc",
	"G729D encoder",
	MS_FILTER_ENCODER,
	"G729D",
	1,
	1,
	enc_g729d_init,
    enc_g729_preprocess,
	enc_g729_process,
    NULL,
	enc_g729_uninit,
	enc_g729d_methods
};

#else

MSFilterDesc ms_g729d_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG729DEnc",
	.text="G729D encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G729D",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g729d_init,
	.preprocess=enc_g729_preprocess,
	.process=enc_g729_process,
	.uninit=enc_g729_uninit,
	.methods=enc_g729d_methods
};

#endif

static void dec_g729d_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_G729I");
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g729d_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;
		int FT;
		int BITRATE;
		int FRAMESIZE;
		int SID=0;

		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}

		if (nbytes%8==0 || nbytes%8==2) {
			// 6400 bps, G.729/D
			FT=2;
			BITRATE=6400;
			FRAMESIZE=8;
			frame_per_packet = nbytes/8;
		} else {
			freemsg(im);
			continue;
		}
		if (nbytes%FRAMESIZE==2)
			SID=1; // a SID frame exist after packet

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(80*2+2,0);
			in.bitrate = BITRATE;
			in.frametype = FT;
			in.nbytes = FRAMESIZE;
			in.pBuffer = (char*)im->b_rptr+(k*FRAMESIZE);
			out.pBuffer = (char*)om->b_wptr;

			if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
				ms_warning("Non Aligned pointer");

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			if (outLen>=0)
			{
				if (outLen!=FRAMESIZE)
					ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
				om->b_wptr+=80*2;
				ms_queue_put(f->outputs[0],om);
			}
			else
			{
				ms_error("Error decoding G729D");
				freeb(om);
				break;
			}
		}

		if (SID==1)
		{
			// a SID frame exist after packet
			ms_message("SID frame received!");
			
			/* should make a loop */
			{
				USC_PCMStream out;
				USC_Bitstream in;
				int outLen;
				om=allocb(80*2+2,0);
				in.bitrate = BITRATE;
				in.frametype = 1;
				in.nbytes = 2;
				in.pBuffer = (char*)im->b_wptr-2;
				out.pBuffer = (char*)om->b_wptr;

				if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
					ms_warning("Non Aligned pointer");

				outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
				if (outLen>=0)
				{
					if (outLen!=2)
						ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
					om->b_wptr+=80*2;
					ms_queue_put(f->outputs[0],om);
				}
				else
				{
					ms_error("Error decoding G729D");
					freeb(om);
					break;
				}
			}
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g729d_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG729DDec",
	"G729D decoder",
	MS_FILTER_DECODER,
	"G729D",
	1,
	1,
	dec_g729d_init,
    NULL,
	dec_g729d_process,
    NULL,
	dec_g729_uninit,
    NULL
};

#else

MSFilterDesc ms_g729d_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG729DDec",
	.text="G729D decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G729D",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g729d_init,
	.process=dec_g729d_process,
	.uninit=dec_g729_uninit
};

#endif

/***************************** G729E *******************************/

static void enc_g729e_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=10;
	s->nsamples=80; /* size of pcm data */
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_G729I");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 11800, 0);

	f->data=s;
}

static int enc_g729e_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
    
	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "annexb", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"no")!=NULL){
	}
	else if (strstr(buf,"yes")!=NULL){
        USCFree(&s->codec.uscParams);
        FreeUSCSharedObjects(&s->codec);
        memset(&s->codec, 0, sizeof(LoadedCodec));
        strcpy((char*)s->codec.codecName, "IPP_G729I");
        s->codec.lIsVad = 1;
        enc_loadcodec(&s->codec, 11800, 1);
	}
	return 0;
}

static MSFilterMethod enc_g729e_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g729e_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g729_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g729e_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG729EEnc",
	"G729E encoder",
	MS_FILTER_ENCODER,
	"G729E",
	1,
	1,
	enc_g729e_init,
    enc_g729_preprocess,
	enc_g729_process,
    NULL,
	enc_g729_uninit,
	enc_g729e_methods
};

#else

MSFilterDesc ms_g729e_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG729EEnc",
	.text="G729E encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G729E",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g729e_init,
	.preprocess=enc_g729_preprocess,
	.process=enc_g729_process,
	.uninit=enc_g729_uninit,
	.methods=enc_g729e_methods
};

#endif

static void dec_g729e_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_G729A");
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g729e_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;
		int FT;
		int BITRATE;
		int FRAMESIZE;
		int SID=0;

		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}

		if (nbytes%15==0 || nbytes%15==2) {
			// 11800 bps, G.729/E
			FT=4;
			BITRATE=11800;
			FRAMESIZE=15;
			frame_per_packet = nbytes/15;
		} else {
			freemsg(im);
			continue;
		}

		if (nbytes%FRAMESIZE==2)
			SID=1; // a SID frame exist after packet

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(80*2+2,0);
			in.bitrate = BITRATE;
			in.frametype = FT;
			in.nbytes = FRAMESIZE;
			in.pBuffer = (char*)im->b_rptr+(k*FRAMESIZE);
			out.pBuffer = (char*)om->b_wptr;

			if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
				ms_warning("Non Aligned pointer");

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			if (outLen>=0)
			{
				if (outLen!=FRAMESIZE)
					ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
				om->b_wptr+=80*2;
				ms_queue_put(f->outputs[0],om);
			}
			else
			{
				ms_error("Error decoding G729E");
				freeb(om);
				break;
			}
		}

		if (SID==1)
		{
			// a SID frame exist after packet
			ms_message("SID frame received!");
			
			/* should make a loop */
			{
				USC_PCMStream out;
				USC_Bitstream in;
				int outLen;
				om=allocb(80*2+2,0);
				in.bitrate = BITRATE;
				in.frametype = 1;
				in.nbytes = 2;
				in.pBuffer = (char*)im->b_wptr-2;
				out.pBuffer = (char*)om->b_wptr;

				if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
					ms_warning("Non Aligned pointer");

				outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
				if (outLen>=0)
				{
					if (outLen!=2)
						ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
					om->b_wptr+=80*2;
					ms_queue_put(f->outputs[0],om);
				}
				else
				{
					ms_error("Error decoding G729E");
					freeb(om);
					break;
				}
			}
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g729e_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG729EDec",
	"G729E decoder",
	MS_FILTER_DECODER,
	"G729E",
	1,
	1,
	dec_g729e_init,
    NULL,
	dec_g729e_process,
    NULL,
	dec_g729_uninit,
    NULL
};

#else

MSFilterDesc ms_g729e_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG729EDec",
	.text="G729E decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G729E",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g729e_init,
	.process=dec_g729e_process,
	.uninit=dec_g729_uninit
};

#endif

/***************************** G7291 *******************************/

#if 0
                  +-------+---------------+------------+
                  |   FT  | encoding rate | frame size |
                  +-------+---------------+------------+
                  |   0   |     8 kbps    |  20 octets |
                  |   1   |    12 kbps    |  30 octets |
                  |   2   |    14 kbps    |  35 octets |
                  |   3   |    16 kbps    |  40 octets |
                  |   4   |    18 kbps    |  45 octets |
                  |   5   |    20 kbps    |  50 octets |
                  |   6   |    22 kbps    |  55 octets |
                  |   7   |    24 kbps    |  60 octets |
                  |   8   |    26 kbps    |  65 octets |
                  |   9   |    28 kbps    |  70 octets |
                  |   10  |    30 kbps    |  75 octets |
                  |   11  |    32 kbps    |  80 octets |
                  | 12-14 |   (reserved)  |            |
                  |   15  |    NO_DATA    |      0     |
                  +-------+---------------+------------+
#endif
static int encoding_rate[] = {8000, 12000, 14000, 16000, 18000, 20000, 22000, 24000, 26000, 28000, 30000, 32000, 0, 0, 0, 0};
static int frame_size[] = {20, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 0, 0, 0, 0};

static void enc_g7291_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=20;
	s->nsamples=320; /* size of pcm data */
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_G729.1");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 32000, 0);

	s->maxbitrate = 32000;
	s->mbs = 32000;
	s->dtx = 0;
	f->data=s;
}

static void enc_g7291_uninit(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void enc_g7291_preprocess(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
}

static int enc_g7291_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;

	/* Permissible values are 8000, 12000, 14000, 16000, 18000, 20000,
	22000, 24000, 26000, 28000, 30000, and 32000. 32000 is implied if 
	this parameter is omitted  */

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "maxbitrate", buf, sizeof(buf));
	if (buf[0]=='\0'){
		//ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	} else {
		char *val = strstr(buf, "=");
		if (val!=NULL)
		{
			int k;
			s->maxbitrate = atoi(val+1);

			for (k=0;k<12;k++)
			{
				if (s->maxbitrate<=encoding_rate[k]) {
					s->maxbitrate = encoding_rate[k];
					break;
				}
			}
			if (s->maxbitrate>32000)
				s->maxbitrate=32000;
		}
	}
	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "mbs", buf, sizeof(buf));
	if (buf[0]=='\0'){
		//ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	} else {
		char *val = strstr(buf, "=");
		if (val!=NULL)
		{
			int k;
			s->mbs = atoi(val+1);

			for (k=0;k<12;k++)
			{
				if (s->mbs<=encoding_rate[k]) {
					s->mbs = encoding_rate[k];
					break;
				}
			}
			if (s->mbs>32000)
				s->mbs=32000;

			ms_message("mbs fmtp parameter (%i)!", s->mbs);
		}
	}
	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "dtx", buf, sizeof(buf));
	if (buf[0]=='\0'){
		//ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	} else {
		char *val = strstr(buf, "=");
		if (val!=NULL)
		{
			s->dtx = atoi(val+1);
			ms_message("dtx fmtp parameter (%i)!", s->dtx);
		}

		//IPP DOES NOT IMPLEMENT DTX/VAD YET for G7291...
		s->dtx = 0;
	}

	/* restart anyway */
	USCFree(&s->codec.uscParams);
	FreeUSCSharedObjects(&s->codec);
	strcpy((char*)s->codec.codecName, "IPP_G729.1");
	s->codec.lIsVad = s->dtx;
	enc_loadcodec(&s->codec, s->maxbitrate, s->dtx);

	return 0;
}

static int enc_g7291_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static int g7291_get_ft(int size) {
	int i;
	for (i=0;i<15;i++)
	{
		if (frame_size[i]==size)
			return i;
	}
	return -1;
}

static void enc_g7291_process(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[4480]; /* 640 * 7 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=10 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 7*20 == 140 ms max */
		frame_per_packet=14;

	if (s->maxbitrate<s->codec.uscParams.pInfo->params.modes.bitrate)
		s->codec.uscParams.pInfo->params.modes.bitrate = s->maxbitrate;
	if (s->mbs<s->codec.uscParams.pInfo->params.modes.bitrate)
		s->codec.uscParams.pInfo->params.modes.bitrate = 32000;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		int inLen;
		int outLenData=0;
		int FT;
		USCCodecGetTerminationCondition(&s->codec.uscParams, &inLen);
		inLen=inLen+1;
		USCCodecGetSize(&s->codec.uscParams, inLen, &outLenData, NULL);

		FT = g7291_get_ft(outLenData);
		if (FT<0)
			break;

		om=allocb(outLenData*frame_per_packet+1,0);
		memset(om->b_wptr, 0, outLenData*frame_per_packet+1);
		om->b_wptr[0]= 0xF0 | FT;
		om->b_wptr++;
		for (k=0;k<frame_per_packet;k++)
		{
			char bit_stream[1024];
			Ipp32s frmlen, infrmLen, FrmDataLen;
			USC_PCMStream PCMStream;
			USC_Bitstream Bitstream;

			/*Do the pre-procession of the frame*/
			infrmLen = USCEncoderPreProcessFrame(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
				(Ipp8s*)bit_stream,&PCMStream,&Bitstream);
			/*Encode one frame*/
			FrmDataLen = USCCodecEncode(&s->codec.uscParams, &PCMStream,&Bitstream,NULL);
			if(FrmDataLen < 0) {
				freeb(om);
				om=NULL;
				break;
			}
			infrmLen += FrmDataLen;
			/*Do the post-procession of the frame*/
			frmlen = USCEncoderPostProcessFrame(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
				(Ipp8s*)bit_stream,&PCMStream,&Bitstream);

			if (Bitstream.nbytes >= 2) // SID
			{
				memcpy(om->b_wptr, bit_stream+(frmlen-Bitstream.nbytes), Bitstream.nbytes);
				om->b_wptr+=Bitstream.nbytes;

				if (Bitstream.nbytes == 2 || Bitstream.nbytes == 3 || Bitstream.nbytes == 6)
				{
					if (k==0) {// only ONE frame -> require FT=14
						FT = 14;
						om->b_rptr[0]= 0xF0 | FT;
					}
					ms_message("g729.1 b // SID sent");
					break;
				}
			} else {
				if (Bitstream.nbytes == 0)
				{
					ms_message("g729.1 b // cont");
				}
				freeb(om);
				om=NULL;
				break;
			}
		}

		//move time ANYWAY
		s->ts+=(s->nsamples*frame_per_packet);
		if (om!=NULL)
		{
			mblk_set_timestamp_info(om,s->ts-s->nsamples);
			ms_queue_put(f->outputs[0],om);
		}
	}
}

static MSFilterMethod enc_g7291_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g7291_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g7291_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g7291_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG7291Enc",
	"G7291 encoder",
	MS_FILTER_ENCODER,
	"G7291",
	1,
	1,
	enc_g7291_init,
    enc_g7291_preprocess,
	enc_g7291_process,
    NULL,
	enc_g7291_uninit,
	enc_g7291_methods
};

#else

MSFilterDesc ms_g7291_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG7291Enc",
	.text="G7291 encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G7291",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g7291_init,
	.preprocess=enc_g7291_preprocess,
	.process=enc_g7291_process,
	.uninit=enc_g7291_uninit,
	.methods=enc_g7291_methods
};

#endif

static void dec_g7291_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_G729.1");
	s->codec.lIsVad = 0;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g7291_uninit(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void dec_g7291_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;
		int FT;
		int BITRATE;
		int FRAMESIZE;
		int SID=0;

		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}

		FT = im->b_rptr[0] & 0x0F;
		if (FT<0 || FT>15)
		{
			freemsg(im);
			continue;
		}
		im->b_rptr++; //skip FT

		BITRATE=8000;
		FRAMESIZE=frame_size[FT];
		if (nbytes%FRAMESIZE==1
			|| nbytes%FRAMESIZE==3
			|| nbytes%FRAMESIZE==4
			|| nbytes%FRAMESIZE==7) {
			frame_per_packet = nbytes/FRAMESIZE;
		} else {
			freemsg(im);
			continue;
		}

		if (nbytes%FRAMESIZE==3
			|| nbytes%FRAMESIZE==4
			|| nbytes%FRAMESIZE==7)
			SID=1; // a SID frame exist after packet

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(320*2+2,0);
			in.bitrate = BITRATE;
			in.frametype = 0; /* FT; */
			in.nbytes = FRAMESIZE;
			in.pBuffer = (char*)im->b_rptr+(k*FRAMESIZE);
			out.pBuffer = (char*)om->b_wptr;

			if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
				ms_warning("Non Aligned pointer");

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			if (outLen>=0)
			{
				if (outLen*4!=FRAMESIZE)
					ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
				om->b_wptr+=320*2;
				ms_queue_put(f->outputs[0],om);
			}
			else
			{
				ms_error("Error decoding G7291");
				freeb(om);
				break;
			}
		}

		if (SID==1)
		{
			// a SID frame exist after packet
			ms_message("SID frame received!");
			
			/* should make a loop */
			{
				USC_PCMStream out;
				USC_Bitstream in;
				int outLen;
				om=allocb(320*2+2,0);
				in.bitrate = BITRATE;
				in.frametype = 1;
				in.nbytes = nbytes%FRAMESIZE-1; // 2, 3 or 6
				in.pBuffer = (char*)im->b_wptr-in.nbytes;
				out.pBuffer = (char*)om->b_wptr;

				if (ippAlignPtr(om->b_wptr, 4)!=om->b_wptr)
					ms_warning("Non Aligned pointer");

				outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
				if (outLen>=0)
				{
					if (outLen!=nbytes%FRAMESIZE-1)
						ms_warning("USCCodecDecode!=%i -> =%i", FRAMESIZE, outLen);
					om->b_wptr+=320*2;
					ms_queue_put(f->outputs[0],om);
				}
				else
				{
					ms_error("Error decoding G729D");
					freeb(om);
					break;
				}
			}
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g7291_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG7291Dec",
	"G7291 decoder",
	MS_FILTER_DECODER,
	"G7291",
	1,
	1,
	dec_g7291_init,
    NULL,
	dec_g7291_process,
    NULL,
	dec_g7291_uninit,
    NULL
};

#else

MSFilterDesc ms_g7291_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG7291Dec",
	.text="G7291 decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G7291",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g7291_init,
	.process=dec_g7291_process,
	.uninit=dec_g7291_uninit
};

#endif

/***************************** G723 *******************************/

static void enc_g723_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=30;
	s->nsamples=240; /* size of pcm data */
	s->nbytes=24; /* default 6.3 */
	s->ptime=30;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_G723.1");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 0, 0);

	f->data=s;
}

static void enc_g723_uninit(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void enc_g723_preprocess(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
}

static int enc_g723_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "annexa", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"no")!=NULL){
	}
	else if (strstr(buf,"yes")!=NULL){
	}

	fmtp_get_value(fmtp, "bitrate", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"6.3")!=NULL){
		s->nbytes=24; /* 0-> 5.3       1-> 6.3 */
	}
	else if (strstr(buf,"5.3")!=NULL){
		s->nbytes=20;
	}
	return 0;
}

static int enc_g723_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=120;
	}
	return 0;
}

static void enc_g723_process(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* 160 * 14 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=30 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 14*10 == 140 ms max */
		frame_per_packet=14;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		int inLen;
		int outLenData=0;
		USCCodecGetTerminationCondition(&s->codec.uscParams, &inLen);
		inLen=inLen+1;
		USCCodecGetSize(&s->codec.uscParams, inLen, &outLenData, NULL);

		om=allocb(outLenData*frame_per_packet,0);

		memset(om->b_wptr, 0, outLenData*frame_per_packet);
		for (k=0;k<frame_per_packet;k++)
		{
			char bit_stream[1024];
			int outLen=0;
		    ProcessOneFrameOneChannel(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
                              (Ipp8s*)bit_stream, &inLen, &outLen, NULL);

			memcpy(om->b_wptr, bit_stream+(outLen-outLenData), outLenData);
			om->b_wptr+=outLenData;
			s->ts+=s->nsamples;
		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static MSFilterMethod enc_g723_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g723_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g723_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g723_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG723Enc",
	"G723 encoder",
	MS_FILTER_ENCODER,
	"G723",
	1,
	1,
	enc_g723_init,
    enc_g723_preprocess,
	enc_g723_process,
    NULL,
	enc_g723_uninit,
	enc_g723_methods
};

#else

MSFilterDesc ms_g723_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG723Enc",
	.text="G723 encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G723",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g723_init,
	.preprocess=enc_g723_preprocess,
	.process=enc_g723_process,
	.uninit=enc_g723_uninit,
	.methods=enc_g723_methods
};

#endif

static void dec_g723_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_G723.1");
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g723_uninit(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static int dec_g723_getBitstreamSize(char *pBitstream)
{
    short  Info,size ;

    Info = (short)(pBitstream[0] & (char)0x3) ;
    /* Check frame type and rate informations */
     size=24;
    switch (Info) {
        case 0x0002 : {   /* SID frame */
            size=4;
            break;
        }
        case 0x0003 : {  /* untransmitted silence frame */
            size=1;
            break;
        }
        case 0x0001 : {   /* active frame, low rate */
            size=20;
            break;
        }
        default : {  /* active frame, high rate */
            break;
        }
    }

    return size;
}

static void dec_g723_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;
	int bit_stream_size;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;

		nbytes=msgdsize(im);
		bit_stream_size = dec_g723_getBitstreamSize((char*)im->b_rptr);
		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}
		if (nbytes%bit_stream_size!=0) /* WE SHOULD SUPPORT DIFFERENT FRAME TYPE/FRAME SIZE IN SAME PACKET */
		{
			/* mix between several frame type in same packet -> not support right now */
			freemsg(im);
			continue;
		}

		frame_per_packet = nbytes/bit_stream_size;

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(240*2+2,0);
			if (bit_stream_size==4) /* SID frame */
			{
				memset(om->b_wptr, 0, 240*2); /* should be confort noise... */
			}
			else
			{
				if (bit_stream_size==24) /* 6300 rate */
					in.bitrate = 6300;
				else
					in.bitrate = 5300;
				in.frametype = 0;
				in.nbytes = bit_stream_size;
				in.pBuffer = (char*)im->b_rptr+(k*bit_stream_size);
				out.pBuffer = (char*)om->b_wptr;

				outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			}
			om->b_wptr+=240*2;
			ms_queue_put(f->outputs[0],om);
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g723_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG723Dec",
	"G723 decoder",
	MS_FILTER_DECODER,
	"G723",
	1,
	1,
	dec_g723_init,
    NULL,
	dec_g723_process,
    NULL,
	dec_g723_uninit,
    NULL
};

#else

MSFilterDesc ms_g723_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG723Dec",
	.text="G723 decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G723",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g723_init,
	.process=dec_g723_process,
	.uninit=dec_g723_uninit
};

#endif

/***************************** G7221 *******************************/

static void enc_g7221_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=20;
	s->nsamples=320; /* size of pcm data */
	s->nbytes=60; /* default 24000 */
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_G722.1");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 24000, 0);

	f->data=s;
}

static void enc_g7221_uninit(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void enc_g7221_preprocess(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
}

static int enc_g7221_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;

	memset(buf, '\0', sizeof(buf));

	fmtp_get_value(fmtp, "bitrate", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	else if (strstr(buf,"16000")!=NULL){
		s->nbytes=40; /* 16000; */
	}
	else if (strstr(buf,"24000")!=NULL){
		s->nbytes=60; /* 24000; */
	}
	else if (strstr(buf,"32000")!=NULL){
		s->nbytes=80; /* 32000; */
	}
	return 0;
}

static int enc_g7221_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=140;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static void enc_g7221_process(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[4480]; /* 320 * 14 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=20 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 14*10 == 140 ms max */
		frame_per_packet=14;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		int inLen;
		int outLenData=0;
		USCCodecGetTerminationCondition(&s->codec.uscParams, &inLen);
		inLen=inLen+1;
		USCCodecGetSize(&s->codec.uscParams, inLen, &outLenData, NULL);

		om=allocb(outLenData*frame_per_packet,0);

		memset(om->b_wptr, 0, outLenData*frame_per_packet);
		for (k=0;k<frame_per_packet;k++)
		{
			char bit_stream[1024];
			int outLen=0;
			ProcessOneFrameOneChannel(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
                              (Ipp8s*)bit_stream, &inLen, &outLen, NULL);

			memcpy(om->b_wptr, bit_stream+(outLen-outLenData), outLenData);
			om->b_wptr+=outLenData;
			s->ts+=s->nsamples;
		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static MSFilterMethod enc_g7221_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g7221_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g7221_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g7221_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG7221Enc",
	"G7221 encoder",
	MS_FILTER_ENCODER,
	"G7221",
	1,
	1,
	enc_g7221_init,
    enc_g7221_preprocess,
	enc_g7221_process,
    NULL,
	enc_g7221_uninit,
	enc_g7221_methods
};

#else

MSFilterDesc ms_g7221_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG7221Enc",
	.text="G7221 encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="G7221",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g7221_init,
	.preprocess=enc_g7221_preprocess,
	.process=enc_g7221_process,
	.uninit=enc_g7221_uninit,
	.methods=enc_g7221_methods
};

#endif

static void dec_g7221_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_G722.1");
	s->codec.lIsVad = 0;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g7221_uninit(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void dec_g7221_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;
	int bit_stream_size;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;
		int bitrate;

		nbytes=msgdsize(im);

		/* TODO: detection is not allowed by the specification... */
		bitrate=16000;
		if (nbytes%80==0)
			bitrate=32000;
		else if (nbytes%60==0)
			bitrate = 24000;
		else if (nbytes%40==0)
			bitrate = 16000;

		if (nbytes%80==0)
			bit_stream_size = 80;
		else if (nbytes%60==0)
			bit_stream_size = 60;
		else if (nbytes%40==0)
			bit_stream_size = 40;

		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}
		if (nbytes%bit_stream_size!=0) /* WE SHOULD SUPPORT DIFFERENT FRAME TYPE/FRAME SIZE IN SAME PACKET */
		{
			/* mix between several frame type in same packet -> not support right now */
			freemsg(im);
			continue;
		}

		frame_per_packet = nbytes/bit_stream_size;

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(320*2+2,0);
			in.bitrate = bitrate;
			in.frametype = 0;
			in.nbytes = bit_stream_size;
			in.pBuffer = (char*)im->b_rptr+(k*bit_stream_size);
			out.pBuffer = (char*)om->b_wptr;

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			om->b_wptr+=320*2;
			ms_queue_put(f->outputs[0],om);
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g7221_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG7221Dec",
	"G7221 decoder",
	MS_FILTER_DECODER,
	"G7221",
	1,
	1,
	dec_g7221_init,
    NULL,
	dec_g7221_process,
    NULL,
	dec_g7221_uninit,
    NULL
};

#else

MSFilterDesc ms_g7221_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG7221Dec",
	.text="G7221 decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="G7221",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g7221_init,
	.process=dec_g7221_process,
	.uninit=dec_g7221_uninit
};

#endif


/***************************** G711 *******************************/

static void enc_g711_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=10;
	s->nsamples=80; /* size of pcm data */
	s->nbytes=80;
	s->ptime=20; /* default ptime to 20ms (2 frame) */
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);


	if (strcmp(f->desc->enc_fmt, "PCMA")==0)
	{
		strcpy((char*)s->codec.codecName, "IPP_G711A");
	}
	else
	{
		strcpy((char*)s->codec.codecName, "IPP_G711U");
	}
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 0, 0);

	f->data=s;
}

static void enc_g711_uninit(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void enc_g711_preprocess(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
}

static int enc_g711_add_fmtp(MSFilter *f, void *arg){

	return 0;
}

static int enc_g711_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=10;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static void enc_g711_process(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* 160 * 14 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=10 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 14*10 == 140 ms max */
		frame_per_packet=14;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		int inLen;
		int outLenData=0;
		USCCodecGetTerminationCondition(&s->codec.uscParams, &inLen);
		inLen=inLen+1;
		USCCodecGetSize(&s->codec.uscParams, inLen, &outLenData, NULL);

		om=allocb(outLenData*frame_per_packet,0);

		memset(om->b_wptr, 0, outLenData*frame_per_packet);
		for (k=0;k<frame_per_packet;k++)
		{
			char bit_stream[1024];
			int outLen=0;
		    ProcessOneFrameOneChannel(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
                              (Ipp8s*)bit_stream, &inLen, &outLen, NULL);

			memcpy(om->b_wptr, bit_stream+(outLen-outLenData), outLenData);
			om->b_wptr+=outLenData;
			s->ts+=s->nsamples;
		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static MSFilterMethod enc_g711_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_g711_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_g711_add_attr},
	{	0								,		NULL			}
};

#ifdef _MSC_VER

MSFilterDesc ms_g711a_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG711AEnc",
	"G711A encoder",
	MS_FILTER_ENCODER,
	"PCMA",
	1,
	1,
	enc_g711_init,
    enc_g711_preprocess,
	enc_g711_process,
    NULL,
	enc_g711_uninit,
	enc_g711_methods
};

#else

MSFilterDesc ms_g711a_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG711AEnc",
	.text="G711A encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="PCMA",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g711_init,
	.preprocess=enc_g711_preprocess,
	.process=enc_g711_process,
	.uninit=enc_g711_uninit,
	.methods=enc_g711_methods
};

#endif

#ifdef _MSC_VER

MSFilterDesc ms_g711u_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG711UEnc",
	"G711U encoder",
	MS_FILTER_ENCODER,
	"PCMU",
	1,
	1,
	enc_g711_init,
    enc_g711_preprocess,
	enc_g711_process,
    NULL,
	enc_g711_uninit,
	enc_g711_methods
};

#else

MSFilterDesc ms_g711u_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG711UEnc",
	.text="G711U encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="PCMU",
	.ninputs=1,
	.noutputs=1,
	.init=enc_g711_init,
	.preprocess=enc_g711_preprocess,
	.process=enc_g711_process,
	.uninit=enc_g711_uninit,
	.methods=enc_g711_methods
};

#endif

static void dec_g711_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	if (strcmp(f->desc->enc_fmt, "PCMA")==0)
	{
		strcpy((char*)s->codec.codecName, "IPP_G711A");
	}
	else
	{
		strcpy((char*)s->codec.codecName, "IPP_G711U");
	}
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;
}

static void dec_g711_uninit(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void dec_g711_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;
	int bit_stream_size;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		int k;

		nbytes=msgdsize(im);
		bit_stream_size = 80;
		if (nbytes<=0)
		{
			freemsg(im);
			continue;
		}
		if (nbytes%bit_stream_size!=0) /* WE SHOULD SUPPORT DIFFERENT FRAME TYPE/FRAME SIZE IN SAME PACKET */
		{
			/* mix between several frame type in same packet -> not support right now */
			freemsg(im);
			continue;
		}

		frame_per_packet = nbytes/bit_stream_size;

		for (k=0;k<frame_per_packet;k++)
		{
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(80*2+2,0);
			in.bitrate = 64000;
			in.frametype = G711_Voice_Frame;
			in.nbytes = bit_stream_size;
			in.pBuffer = (char*)im->b_rptr+(k*bit_stream_size);
			out.pBuffer = (char*)om->b_wptr;

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			om->b_wptr+=80*2;
			ms_queue_put(f->outputs[0],om);
		}
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_g711a_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG711ADec",
	"G711A decoder",
	MS_FILTER_DECODER,
	"PCMA",
	1,
	1,
	dec_g711_init,
    NULL,
	dec_g711_process,
    NULL,
	dec_g711_uninit,
    NULL
};

#else

MSFilterDesc ms_g711a_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG711ADec",
	.text="G711A decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="PCMA",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g711_init,
	.process=dec_g711_process,
	.uninit=dec_g711_uninit
};

#endif

#ifdef _MSC_VER

MSFilterDesc ms_g711u_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG711UDec",
	"G711U decoder",
	MS_FILTER_DECODER,
	"PCMU",
	1,
	1,
	dec_g711_init,
    NULL,
	dec_g711_process,
    NULL,
	dec_g711_uninit,
    NULL
};

#else

MSFilterDesc ms_g711u_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSG711UDec",
	.text="G711U decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="PCMU",
	.ninputs=1,
	.noutputs=1,
	.init=dec_g711_init,
	.process=dec_g711_process,
	.uninit=dec_g711_uninit
};

#endif


/***************************** GSMAMR *******************************/
#if defined(_USC_GSMAMR) || defined(_USC_AMRWB)

#if 0
static const value_string amr_nb_codec_mode_vals[] = {
	{0,		"AMR 4,75 kbit/s"}, 
	{1,		"AMR 5,15 kbit/s"},
	{2,		"AMR 5,90 kbit/s"},
	{3,		"AMR 6,70 kbit/s (PDC-EFR)"},
	{4,		"AMR 7,40 kbit/s (TDMA-EFR)"},
	{5,		"AMR 7,95 kbit/s"},
	{6,		"AMR 10,2 kbit/s"},
	{7,		"AMR 12,2 kbit/s (GSM-EFR)"},
	{AMR_NB_SID,	"AMR SID (Comfort Noise Frame)"},
	{9,				"GSM-EFR SID"},
	{10,			"TDMA-EFR SID "},
	{11,			"PDC-EFR SID"},
	{12,			"Illegal Frametype - for future use"},
	{13,			"Illegal Frametype - for future use"},
	{14,			"Illegal Frametype - for future use"},
	{AMR_NO_TRANS,	"No Data (No transmission/No reception)"},
	{ 0,	NULL }
};
#endif
#if 0
static const value_string amr_wb_codec_mode_vals[] = {
	{0, 			"AMR-WB 6.60 kbit/s"},
	{1, 			"AMR-WB 8.85 kbit/s"},
	{2, 			"AMR-WB 12.65 kbit/s"},
	{3, 			"AMR-WB 14.25 kbit/s"},
	{4, 			"AMR-WB 15.85 kbit/s"},
	{5, 			"AMR-WB 18.25 kbit/s"},
	{6, 			"AMR-WB 19.85 kbit/s"},
	{7, 			"AMR-WB 23.05 kbit/s"},
	{8, 			"AMR-WB 23.85 kbit/s"},
	{AMR_WB_SID,	"AMR-WB SID (Comfort Noise Frame)"},
	{10,			"Illegal Frametype"},
	{11,			"Illegal Frametype"},
	{12,			"Illegal Frametype"},
	{13,			"Illegal Frametype"},
	{14,			"Speech lost"},
	{AMR_NO_TRANS,	"No Data (No transmission/No reception)"},
	{ 0,	NULL }
};
#endif

static void enc_gsmamr_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=20;
	s->nsamples=160; /* size of pcm data */
	s->nbytes=0;
	s->ptime=20; /* default ptime to 20ms */
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_GSMAMR");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 7950, 0);

	s->packetizerParams = new AMRPacketizerParams();
    ((AMRPacketizerParams*)s->packetizerParams)->m_ptType = BandEfficient;
	s->packetizer = new AMRPacketizer();
    s->packetizer->Init(s->packetizerParams);

	s->controlParams = new AMRControlParams();
    s->packetizer->SetControls(s->controlParams);

	f->data=s;
}

static void enc_gsmamr_uninit(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	delete s->packetizerParams;
	delete s->controlParams;
	ms_free(f->data);
}

static void enc_gsmamr_preprocess(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
}

static int enc_gsmamr_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "octet-align", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
	    ((AMRPacketizerParams*)s->packetizerParams)->m_ptType = OctetAlign;
		s->packetizer->Init(s->packetizerParams);
	}
	else if (buf[0]!='\0' && strstr(buf,"0")!=NULL){
	    ((AMRPacketizerParams*)s->packetizerParams)->m_ptType = BandEfficient;
		s->packetizer->Init(s->packetizerParams);
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "crc", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		ms_warning("parameter CRC not implemented for AMR");
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "robust-sorting", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		ms_warning("parameter robust-sorting not implemented for AMR");
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "interleaving", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		((AMRPacketizerParams*)s->packetizerParams)->m_InterleavingFlag = 1;
		s->packetizer->Init(s->packetizerParams);
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "channels", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
	}
	else if (buf[0]!='\0') {
		ms_warning("Several Channels not implemented for AMR");
	}
	return 0;
}

static int enc_gsmamr_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	IPPEncState *s=(IPPEncState*)f->data;
	if (strstr(fmtp,"ptime:10")!=NULL){
		s->ptime=10;
	}else if (strstr(fmtp,"ptime:20")!=NULL){
		s->ptime=20;
	}else if (strstr(fmtp,"ptime:30")!=NULL){
		s->ptime=30;
	}else if (strstr(fmtp,"ptime:40")!=NULL){
		s->ptime=40;
	}else if (strstr(fmtp,"ptime:50")!=NULL){
		s->ptime=50;
	}else if (strstr(fmtp,"ptime:60")!=NULL){
		s->ptime=60;
	}else if (strstr(fmtp,"ptime:70")!=NULL){
		s->ptime=70;
	}else if (strstr(fmtp,"ptime:80")!=NULL){
		s->ptime=80;
	}else if (strstr(fmtp,"ptime:90")!=NULL){
		s->ptime=90;
	}else if (strstr(fmtp,"ptime:100")!=NULL){
		s->ptime=100;
	}else if (strstr(fmtp,"ptime:110")!=NULL){
		s->ptime=110;
	}else if (strstr(fmtp,"ptime:120")!=NULL){
		s->ptime=120;
	}else if (strstr(fmtp,"ptime:130")!=NULL){
		s->ptime=130;
	}else if (strstr(fmtp,"ptime:140")!=NULL){
		s->ptime=140;
	}
	return 0;
}

static void enc_gsmamr_process(MSFilter *f){
	IPPEncState *s=(IPPEncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* 160 * 14 is the largest size for ptime == 140 */
	int frame_per_packet=1;

	if (s->ptime>=10 && s->ms_per_frame>0 && s->ptime%s->ms_per_frame==0)
	{
		frame_per_packet = s->ptime/s->ms_per_frame;
	}

	if (frame_per_packet<=0)
		frame_per_packet=1;
	if (frame_per_packet>14) /* 14*10 == 140 ms max */
		frame_per_packet=14;

	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		ms_bufferizer_put(&s->bufferizer,im);
	}

	
	while(ms_bufferizer_read(&s->bufferizer,(uint8_t*)samples,size*frame_per_packet)==(size*frame_per_packet)){
		int k;
		int inLen;
		int outLenData=0;
		USCCodecGetTerminationCondition(&s->codec.uscParams, &inLen);
		inLen=inLen+1;
		USCCodecGetSize(&s->codec.uscParams, inLen, &outLenData, NULL);
		outLenData=outLenData;

		for (k=0;k<frame_per_packet;k++)
		{
			Ipp32s frmlen, infrmLen, FrmDataLen;
			char bit_stream[1024];
			int outLen=0;

			USC_PCMStream PCMStream;
			USC_Bitstream Bitstream;

			infrmLen = USCEncoderPreProcessFrame(&s->codec.uscParams,
				(Ipp8s*)(samples+(k*size/2)),
				(Ipp8s *)bit_stream,
				&PCMStream,
				&Bitstream);

			/*Encode one frame*/
			FrmDataLen = USCCodecEncode(&s->codec.uscParams, &PCMStream,&Bitstream,NULL);
			if(FrmDataLen < 0)
			{
				ms_error("Fail to encode AMR nb.");
				return;
			}

			infrmLen += FrmDataLen;
			/*Do the post-procession of the frame*/
			frmlen = USCEncoderPostProcessFrame(&s->codec.uscParams,
				(Ipp8s*)(samples+(k*size/2)),
				(Ipp8s *)bit_stream,
				&PCMStream,
				&Bitstream);
			inLen = infrmLen;
			outLen = frmlen;

			//ProcessOneFrameOneChannel(&s->codec.uscParams, (Ipp8s*)(samples+(k*size/2)),
			//	(Ipp8s*)bit_stream, &inLen, &outLen, NULL);

			SpeechData from;
			from.SetBufferPointer((Ipp8u*)Bitstream.pBuffer, Bitstream.nbytes);
			from.SetDataSize(Bitstream.nbytes);
			from.SetBitrate(Bitstream.bitrate);
			from.SetFrameType(Bitstream.frametype);
			from.SetNBytes(Bitstream.nbytes);

			s->packetizer->AddFrame(&from);

			SpeechData to(1500);
			memset(to.GetBufferPointer(), 0, 1500);
			if (UMC_OK!=s->packetizer->GetPacket(&to))
			{
				ms_error("Fail to encode AMR nb.");
				return;
			}

			om=allocb(to.GetDataSize(),0);
			memset(om->b_wptr, 0, to.GetDataSize());
			memcpy(om->b_wptr, to.GetDataPointer(), to.GetDataSize());
			om->b_wptr+=to.GetDataSize();
			s->ts+=s->nsamples;

			mblk_set_timestamp_info(om,s->ts-s->nsamples);
			ms_queue_put(f->outputs[0],om);
		}

		 /* The frame type index may be 0-7 for AMR, as defined in Table 1a in [2],
		 or 0-8 for AMR-WB, as defined in Table 1a in [4].  CMR value 15 indicates
		 that no mode request is present, and other values are for future use. */
		//((AMRControlParams*)s->controlParams)->m_CMR = 5;
	    //s->packetizer->SetControls(s->controlParams);
	}
}

static MSFilterMethod enc_gsmamr_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_gsmamr_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_gsmamr_add_attr},
	{	0								,		NULL			}
};


#ifdef _MSC_VER

MSFilterDesc ms_gsmamr_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSGSMAMREnc",
	"GSM-AMR encoder",
	MS_FILTER_ENCODER,
	"AMR",
	1,
	1,
	enc_gsmamr_init,
    enc_gsmamr_preprocess,
	enc_gsmamr_process,
    NULL,
	enc_gsmamr_uninit,
	enc_gsmamr_methods
};

#else

MSFilterDesc ms_gsmamr_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSGSMAMREnc",
	.text="GSM-AMR encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="AMR",
	.ninputs=1,
	.noutputs=1,
	.init=enc_gsmamr_init,
	.preprocess=enc_gsmamr_preprocess,
	.process=enc_gsmamr_process,
	.uninit=enc_gsmamr_uninit,
	.methods=enc_gsmamr_methods
};

#endif

static void dec_gsmamr_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_GSMAMR");
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;

	s->depacketizerParams = new AMRDePacketizerParams();
    ((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
	s->depacketizer = new AMRDePacketizer();;
    s->depacketizer->Init(s->depacketizerParams);
}

static void dec_gsmamr_uninit(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	USCFree(&s->codec.uscParams);
	/* Close plug-ins*/
	FreeUSCSharedObjects(&s->codec);
	ms_free(f->data);
}

static void dec_gsmamr_process(MSFilter *f){
	IPPDecState *s=(IPPDecState*)f->data;
	mblk_t *im,*om;
	int nbytes;
	int samples = 160*2;
	if (strcmp(f->desc->enc_fmt, "AMR-WB")==0)
		samples = 320*2;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		int frame_per_packet = 1;
		unsigned char *frame = im->b_rptr;

		nbytes=msgdsize(im);

		Ipp8u Toc;
		Toc = (*im->b_rptr) & 0x0f;
		Ipp8u Toc2;
		Toc2 = (im->b_rptr[1]) & 0xf8;

		//set all know use-case
		if (Toc!=0) {
			/* non octet-align mode */
			((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
			s->depacketizer->Init(s->depacketizerParams);
		} else { //Toc==0
			//check the only case where in bandwidth efficent mode, we have RRRR=0000
			if (nbytes==14 && Toc2!=0 /* FT=0 AMR 4,75 kbit/s */)
			{
				/* non octet-align mode */
				((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
				s->depacketizer->Init(s->depacketizerParams);
			} else if (nbytes==14  /* FT=0 AMR 4,75 kbit/s */) {
				/* octet align mode */
				((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = OctetAlign;
				s->depacketizer->Init(s->depacketizerParams);
			} else if (nbytes==15 &&  Toc2!=8 /* FT=1 AMR 5,15 kbit/s */)
			{
				/* non octet-align mode */
				((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
				s->depacketizer->Init(s->depacketizerParams);
			} else if (nbytes==15 /* FT=1 AMR 5,15 kbit/s */)
			{
				/* octet align mode */
				((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = OctetAlign;
				s->depacketizer->Init(s->depacketizerParams);
			} else if (strcmp(f->desc->enc_fmt, "AMR-WB")==0
				&&(nbytes==18 /* FT=0 AMR-WB 6.60 kbit/s instead of 19*/
				|| nbytes==24 /* FT=1 AMR-WB 8.85 kbit/s instead of 25 */))
			{
				/* non octet-align mode */
				((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
				s->depacketizer->Init(s->depacketizerParams);
			} else {
				/* other bandwdith efficient codec will never have RRRR=0000
				/* octet align mode */
				((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = OctetAlign;
				s->depacketizer->Init(s->depacketizerParams);
			}
		}

		SpeechData sin;
		if (nbytes > 0) {
		  sin.SetBufferPointer((Ipp8u *)im->b_rptr, nbytes);
		  sin.SetDataSize(nbytes);
		  sin.SetNBytes(nbytes);
		}
		sin.SetBitrate(0); //set maximum bitrate

		s->depacketizer->SetPacket(&sin);

		SpeechData tmpFrom(1500);
		Status err = s->depacketizer->GetNextFrame(&tmpFrom);
		if (err != UMC_ERR_NOT_ENOUGH_DATA && err != UMC_OK)
		{
			freemsg(im);
			continue;
		}
		while (err == UMC_OK) {
			/* retreive the depacketized frames */
			USC_PCMStream out;
			USC_Bitstream in;
			int outLen;
			om=allocb(samples,0);
			//case 4750:  rate = 0; break;
			//case 5150:  rate = 1; break;
			//case 5900:  rate = 2; break;
			//case 6700:  rate = 3; break;
			//case 7400:  rate = 4; break;
			//case 7950:  rate = 5; break;
			//case 10200: rate = 6; break;
			//case 12200: rate = 7; break;
			in.bitrate = tmpFrom.GetBitrate();
			in.frametype = tmpFrom.GetFrameType();
			in.nbytes = tmpFrom.GetNBytes(); //bit_stream_size;
			in.pBuffer = (char*)tmpFrom.GetDataPointer();
			out.pBuffer = (char*)om->b_wptr;

			outLen = USCCodecDecode(&s->codec.uscParams, &in,&out, NULL);
			if (outLen>0)
			{
				om->b_wptr+=samples;
				ms_queue_put(f->outputs[0],om);
			}
			else
			{
				freeb(om);
			}
			tmpFrom.Reset();

			err = s->depacketizer->GetNextFrame(&tmpFrom);
			if (err != UMC_ERR_NOT_ENOUGH_DATA && err != UMC_OK)
			{
				freemsg(im);
				return;
			}
		}
		freemsg(im);
	}
}

static int dec_gsmamr_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	IPPDecState *s=(IPPDecState*)f->data;

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "octet-align", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = OctetAlign;
		s->depacketizer->Init(s->depacketizerParams);
	}
	else if (buf[0]!='\0' && strstr(buf,"0")!=NULL){
	    ((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
	    s->depacketizer->Init(s->depacketizerParams);
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "crc", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		ms_warning("parameter CRC not implemented for AMR");
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "robust-sorting", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		ms_warning("parameter CRC not implemented for AMR");
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "interleaving", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
		((AMRDePacketizerParams*)s->depacketizerParams)->m_InterleavingFlag = 1;
	    s->depacketizer->Init(s->depacketizerParams);
	}

	memset(buf, '\0', sizeof(buf));
	fmtp_get_value(fmtp, "channels", buf, sizeof(buf));
	if (buf[0]!='\0' && strstr(buf,"1")!=NULL){
	}
	else if (buf[0]!='\0') {
		ms_warning("Several Channels not implemented for AMR");
	}
	return 0;
}

static MSFilterMethod dec_gsmamr_methods[]={
	{	MS_FILTER_ADD_FMTP,		dec_gsmamr_add_fmtp },
	{	0, NULL }
};

#ifdef _MSC_VER

MSFilterDesc ms_gsmamr_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSGSMAMRDec",
	"GSM-AMR decoder",
	MS_FILTER_DECODER,
	"AMR",
	1,
	1,
	dec_gsmamr_init,
    NULL,
	dec_gsmamr_process,
    NULL,
	dec_gsmamr_uninit,
    dec_gsmamr_methods
};

#else

MSFilterDesc ms_gsmamr_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSGSMAMRDec",
	.text="GSM-AMR decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="AMR",
	.ninputs=1,
	.noutputs=1,
	.init=dec_gsmamr_init,
	.process=dec_gsmamr_process,
	.uninit=dec_gsmamr_uninit,
	.methods=dec_gsmamr_methods
};

#endif


/***************************** AMR-WB *******************************/

static void enc_amrwb_init(MSFilter *f){
	IPPEncState *s=(IPPEncState *)ms_new(IPPEncState,1);

	s->ms_per_frame=20;
	s->nsamples=320; /* size of pcm data */
	s->nbytes=0;
	s->ptime=20; /* default ptime to 20ms */
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);

	strcpy((char*)s->codec.codecName, "IPP_AMRWB");
	s->codec.lIsVad = 0;
	enc_loadcodec(&s->codec, 23850, 0);

	s->packetizerParams = new AMRPacketizerParams();
    ((AMRPacketizerParams*)s->packetizerParams)->m_ptType = BandEfficient;
	((AMRPacketizerParams*)s->packetizerParams)->m_CodecType = WB; //WideBand;
	s->packetizer = new AMRPacketizer();
    s->packetizer->Init(s->packetizerParams);

	s->controlParams = new AMRControlParams();
    s->packetizer->SetControls(s->controlParams);

	f->data=s;
}


#ifdef _MSC_VER

MSFilterDesc ms_amrwb_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSAMRWBEnc",
	"AMR-WB encoder",
	MS_FILTER_ENCODER,
	"AMR-WB",
	1,
	1,
	enc_amrwb_init,
    enc_gsmamr_preprocess,
	enc_gsmamr_process,
    NULL,
	enc_gsmamr_uninit,
	enc_gsmamr_methods
};

#else

MSFilterDesc ms_amrwb_enc_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSAMRWBEnc",
	.text="AMR-WB encoder",
	.category=MS_FILTER_ENCODER,
	.enc_fmt="AMR-WB",
	.ninputs=1,
	.noutputs=1,
	.init=enc_amrwb_init,
	.preprocess=enc_gsmamr_preprocess,
	.process=enc_gsmamr_process,
	.uninit=enc_gsmamr_uninit,
	.methods=enc_gsmamr_methods
};

#endif

static void dec_amrwb_init(MSFilter *f){
	IPPDecState *s=(IPPDecState *)ms_new(IPPDecState,1);

	strcpy((char*)s->codec.codecName, "IPP_AMRWB");
	s->codec.lIsVad = 1;
	dec_loadcodec(&s->codec);

	f->data=s;

	s->depacketizerParams = new AMRDePacketizerParams();
    ((AMRDePacketizerParams*)s->depacketizerParams)->m_ptType = BandEfficient;
	((AMRDePacketizerParams*)s->depacketizerParams)->m_CodecType = WB; //WideBand;
	s->depacketizer = new AMRDePacketizer();
    s->depacketizer->Init(s->depacketizerParams);
}

#ifdef _MSC_VER

MSFilterDesc ms_amrwb_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSAMRWBDec",
	"AMR-WB decoder",
	MS_FILTER_DECODER,
	"AMR-WB",
	1,
	1,
	dec_amrwb_init,
    NULL,
	dec_gsmamr_process,
    NULL,
	dec_gsmamr_uninit,
	dec_gsmamr_methods
};

#else

MSFilterDesc ms_amrwb_dec_desc={
	.id=MS_FILTER_PLUGIN_ID,
	.name="MSAMRWBDec",
	.text="AMR-WB decoder",
	.category=MS_FILTER_DECODER,
	.enc_fmt="AMR-WB",
	.ninputs=1,
	.noutputs=1,
	.init=dec_amrwb_init,
	.process=dec_gsmamr_process,
	.uninit=dec_gsmamr_uninit,
	.methods=dec_gsmamr_methods
};

#endif

#endif

#ifdef WIN32
#define GLOBAL_LINKAGE __declspec(dllexport)
#else
#define GLOBAL_LINKAGE
#endif

#ifdef __cplusplus
extern "C"
#endif
GLOBAL_LINKAGE void libmsippg7xx_init(){
	const IppLibraryVersion* ippj;
#ifdef WIN32
   ippStaticInit();
   //ippStaticInitCpu(ippCpuUnknown);
#endif

	ippj = ippiGetLibVersion();

	ms_message("msippg7xx: Intel(R) Integrated Performance Primitives ");
	ms_message("  version: %s, [%d.%d.%d.%d] ",
		ippj->Version, ippj->major, ippj->minor, ippj->build, ippj->majorBuild);
	ms_message("  name:    %s ", ippj->Name);
	ms_message("  date:    %s ", ippj->BuildDate);

#ifdef _USC_G729
	ms_filter_register(&ms_g729_enc_desc);
	ms_filter_register(&ms_g729_dec_desc);
	ms_filter_register(&ms_g729d_enc_desc);
	ms_filter_register(&ms_g729d_dec_desc);
	ms_filter_register(&ms_g729e_enc_desc);
	ms_filter_register(&ms_g729e_dec_desc);
#endif
	
#ifdef _USC_G7291
	ms_filter_register(&ms_g7291_enc_desc);
	ms_filter_register(&ms_g7291_dec_desc);
#endif
	

#ifdef _USC_G723
	ms_filter_register(&ms_g723_enc_desc);
	ms_filter_register(&ms_g723_dec_desc);
#endif

#ifdef _USC_G711
	ms_filter_register(&ms_g711a_enc_desc);
	ms_filter_register(&ms_g711a_dec_desc);
	ms_filter_register(&ms_g711u_enc_desc);
	ms_filter_register(&ms_g711u_dec_desc);
#endif

#ifdef _USC_G722
	ms_filter_register(&ms_g7221_enc_desc);
	ms_filter_register(&ms_g7221_dec_desc);
#endif

#ifdef _USC_GSMAMR
	ms_filter_register(&ms_gsmamr_enc_desc);
	ms_filter_register(&ms_gsmamr_dec_desc);
#endif

#ifdef _USC_AMRWB
	ms_filter_register(&ms_amrwb_enc_desc);
	ms_filter_register(&ms_amrwb_dec_desc);
#endif
}
