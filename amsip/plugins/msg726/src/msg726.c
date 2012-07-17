/*
	NO LICENSE
*/
#include "g72x.h"

#include <mediastreamer2/msfilter.h>

typedef struct G726EncState{
	int	 (*enc_routine)();
	int enc_bits;
	int nsamples;
	int nbytes;
	int ms_per_frame;
	int ptime;
	uint32_t ts;
	MSBufferizer bufferizer;
	struct g72x_state g72x_state;
}G726EncState;

static void enc40_init(MSFilter *f){
	G726EncState *s=ms_new(G726EncState,1);
	s->enc_routine=g723_40_encoder;
	s->enc_bits=5;
	s->ms_per_frame=10;
	s->nsamples=80; /* size of encoded data?? */
	s->nbytes=50;
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);
	memset(&s->g72x_state, 0, sizeof(s->g72x_state));
	f->data=s;
}
static void enc32_init(MSFilter *f){
	G726EncState *s=ms_new(G726EncState,1);
	s->enc_routine=g721_encoder;
	s->enc_bits=4;
	s->ms_per_frame=10;
	s->nsamples=80; /* size of encoded data?? */
	s->nbytes=40;
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);
	memset(&s->g72x_state, 0, sizeof(s->g72x_state));
	f->data=s;
}
static void enc24_init(MSFilter *f){
	G726EncState *s=ms_new(G726EncState,1);
	s->enc_routine=g723_24_encoder;
	s->enc_bits=3;
	s->ms_per_frame=10;
	s->nsamples=80; /* size of encoded data?? */
	s->nbytes=30;
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);
	memset(&s->g72x_state, 0, sizeof(s->g72x_state));
	f->data=s;
}
static void enc16_init(MSFilter *f){
	G726EncState *s=ms_new(G726EncState,1);
	s->enc_routine=g723_16_encoder;
	s->enc_bits=2;
	s->ms_per_frame=10;
	s->nsamples=80; /* size of encoded data?? */
	s->nbytes=20;
	s->ptime=20;
	s->ts=0;
	ms_bufferizer_init(&s->bufferizer);
	memset(&s->g72x_state, 0, sizeof(s->g72x_state));
	f->data=s;
}


static void enc_uninit(MSFilter *f){
	G726EncState *s=(G726EncState*)f->data;
	ms_bufferizer_uninit(&s->bufferizer);
	ms_free(f->data);
}

static void enc_preprocess(MSFilter *f){
	G726EncState *s=(G726EncState*)f->data;
	g72x_init_state(&s->g72x_state);
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	char buf[64];
	const char *fmtp=(const char *)arg;
	G726EncState *s=(G726EncState*)f->data;

	memset(buf, '\0', sizeof(buf));
	//?? fmtp_get_value(fmtp, "mode", buf, sizeof(buf));
	if (buf[0]=='\0'){
		ms_warning("unsupported fmtp parameter (%s)!", fmtp);
	}
	return 0;
}

static int enc_add_attr(MSFilter *f, void *arg){
	const char *fmtp=(const char *)arg;
	G726EncState *s=(G726EncState*)f->data;
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

#define G726_PACK_RFC3551 1
#define PCM16_B2S(b) ((b) >> 1)

static void enc40_process(MSFilter *f){
	G726EncState *s=(G726EncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* TODO: 160 * 14 is the largest size ?? */
	int frame_per_packet=1;

	memset(samples, 0, sizeof(samples));
	if (s->ptime>=10 && s->ptime%s->ms_per_frame==0)
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
		short* sample_buf;

		om=allocb(s->nbytes*frame_per_packet,0);


		for (k=0;k<frame_per_packet;k++)
		{
			int i,j;
			unsigned char out_buf[480];
			sample_buf = (short *)samples+(k*s->nsamples);


			for (i = 0; i < PCM16_B2S(size); i += 8) {
				uint64_t v = 0;
				for (j = 0; j < 8; j++) {
#ifdef G726_PACK_RFC3551
					v |= ((uint64_t)(*s->enc_routine)(sample_buf[i+j],			
						AUDIO_ENCODING_LINEAR, &s->g72x_state)) << (j * 5);
#else
					v |= ((uint64_t)(*s->enc_routine)(sample_buf[i+j],
						AUDIO_ENCODING_LINEAR, &s->g72x_state)) << ((7-j) * 5);
#endif
				}

				out_buf[(i >> 3) * 5] = (v & 0xff);
				out_buf[(i >> 3) * 5 + 1] = ((v >> 8) & 0xff);
				out_buf[(i >> 3) * 5 + 2] = ((v >> 16) & 0xff);
				out_buf[(i >> 3) * 5 + 3] = ((v >> 24) & 0xff);
				out_buf[(i >> 3) * 5 + 4] = ((v >> 32) & 0xff);
			}

			i = (PCM16_B2S(size) >> 3) * 5;
        
			s->ts+=s->nsamples;
			memcpy(om->b_wptr, out_buf, s->nbytes);
			om->b_wptr+=s->nbytes;

		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static void enc32_process(MSFilter *f){
	G726EncState *s=(G726EncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* TODO: 160 * 14 is the largest size ?? */
	int frame_per_packet=1;

	memset(samples, 0, sizeof(samples));
	if (s->ptime>=10 && s->ptime%s->ms_per_frame==0)
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
		short* sample_buf;

		om=allocb(s->nbytes*frame_per_packet,0);

		for (k=0;k<frame_per_packet;k++)
		{
			int i,j;
			unsigned char out_buf[480];
			sample_buf = (short *)samples+(k*s->nsamples);


			for (i = 0; i < PCM16_B2S(size); i += 2) {
				out_buf[i >> 1] = 0;
				for (j = 0; j < 2; j++) {
					char v = (*s->enc_routine)(sample_buf[i+j],			
						AUDIO_ENCODING_LINEAR, &s->g72x_state);
#ifdef G726_PACK_RFC3551
			        out_buf[i >> 1] |= v << (j * 4);
#else 
					out_buf[i >> 1] |= v << ((1-j) * 4);
#endif
				}
			}

			i = (PCM16_B2S(size) >> 1);
        
			s->ts+=s->nsamples;
			memcpy(om->b_wptr, out_buf, s->nbytes);
			om->b_wptr+=s->nbytes;

		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static void enc24_process(MSFilter *f){
	G726EncState *s=(G726EncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* TODO: 160 * 14 is the largest size ?? */
	int frame_per_packet=1;

	memset(samples, 0, sizeof(samples));
	if (s->ptime>=10 && s->ptime%s->ms_per_frame==0)
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
		short* sample_buf;

		om=allocb(s->nbytes*frame_per_packet,0);

		for (k=0;k<frame_per_packet;k++)
		{
			int i,j;
			unsigned char out_buf[480];
			sample_buf = (short *)samples+(k*s->nsamples);


			for (i = 0; i < PCM16_B2S(size); i += 8) {
				uint32_t v = 0;
				for (j = 0; j < 8; j++) {
#ifdef G726_PACK_RFC3551
					v |= ((*s->enc_routine)(sample_buf[i+j],
                                    AUDIO_ENCODING_LINEAR, &s->g72x_state)) << (j * 3);
#else
			        v |= ((*s->enc_routine)(sample_buf[i+j],
                                    AUDIO_ENCODING_LINEAR, &s->g72x_state)) << ((7-j) * 3);
#endif
				}

				out_buf[(i >> 3) * 3] = (v & 0xff);
				out_buf[(i >> 3) * 3 + 1] = ((v >> 8) & 0xff);
				out_buf[(i >> 3) * 3 + 2] = ((v >> 16) & 0xff);
			}

			i = (PCM16_B2S(size) >> 3) * 3;
        
			s->ts+=s->nsamples;
			memcpy(om->b_wptr, out_buf, s->nbytes);
			om->b_wptr+=s->nbytes;

		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static void enc16_process(MSFilter *f){
	G726EncState *s=(G726EncState*)f->data;
	mblk_t *im,*om;
	int size=s->nsamples*2;
	int16_t samples[2240]; /* TODO: 160 * 14 is the largest size ?? */
	int frame_per_packet=1;

	memset(samples, 0, sizeof(samples));
	if (s->ptime>=10 && s->ptime%s->ms_per_frame==0)
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
		short* sample_buf;

		om=allocb(s->nbytes*frame_per_packet,0);

		for (k=0;k<frame_per_packet;k++)
		{
			int i,j;
			unsigned char out_buf[480];
			sample_buf = (short *)samples+(k*s->nsamples);


			for (i = 0; i < PCM16_B2S(size); i += 4) {
				out_buf[i >> 2] = 0;
				for (j = 0; j < 4; j++) {
					char v = 0;
					v = (*s->enc_routine)(sample_buf[i+j],
                                    AUDIO_ENCODING_LINEAR, &s->g72x_state);

#ifdef G726_PACK_RFC3551
					out_buf[i >> 2] |= v << (j * 2);
#else
					out_buf[i >> 2] |= v << ((3-j) * 2);
#endif
				}

			}

			i = PCM16_B2S(size) >> 2;
        
			s->ts+=s->nsamples;
			memcpy(om->b_wptr, out_buf, s->nbytes);
			om->b_wptr+=s->nbytes;

		}
		mblk_set_timestamp_info(om,s->ts-s->nsamples);
		ms_queue_put(f->outputs[0],om);
	}
}

static MSFilterMethod enc_methods[]={
	{	MS_FILTER_ADD_FMTP,		enc_add_fmtp },
	{	MS_FILTER_ADD_ATTR,		enc_add_attr},
	{	0								,		NULL			}
};


MSFilterDesc ms_g726_40_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-40Enc",
	"G726-40 encoder",
	MS_FILTER_ENCODER,
	"g726-40",
	1,
	1,
	enc40_init,
    enc_preprocess,
	enc40_process,
    NULL,
	enc_uninit,
	enc_methods
};
MSFilterDesc ms_g721_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG721Enc",
	"G721 encoder",
	MS_FILTER_ENCODER,
	"g721",
	1,
	1,
	enc32_init,
    enc_preprocess,
	enc32_process,
    NULL,
	enc_uninit,
	enc_methods
};
MSFilterDesc ms_g726_32_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-32Enc",
	"G726-32 encoder",
	MS_FILTER_ENCODER,
	"g726-32",
	1,
	1,
	enc32_init,
    enc_preprocess,
	enc32_process,
    NULL,
	enc_uninit,
	enc_methods
};
MSFilterDesc ms_g726_24_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-24Enc",
	"G726-24 encoder",
	MS_FILTER_ENCODER,
	"g726-24",
	1,
	1,
	enc24_init,
    enc_preprocess,
	enc24_process,
    NULL,
	enc_uninit,
	enc_methods
};

MSFilterDesc ms_g726_16_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-16Enc",
	"G726-16 encoder",
	MS_FILTER_ENCODER,
	"g726-16",
	1,
	1,
	enc16_init,
    enc_preprocess,
	enc16_process,
    NULL,
	enc_uninit,
	enc_methods
};

typedef struct G726DecState{
	int nsamples;
	int nbytes;
	int ms_per_frame;
	struct g72x_state g723_dec;
}G726DecState;

static void dec_init(MSFilter *f){
	G726DecState *s=ms_new(G726DecState,1);
	s->nsamples=0;
	s->nbytes=0;
	s->ms_per_frame=0;
	f->data=s;
	g72x_init_state(&s->g723_dec);
}

static void dec_uninit(MSFilter *f){
	ms_free(f->data);
}

#define PCM16_S2B(s) ((s) << 1)

static void dec40_process(MSFilter *f){
	G726DecState *s=(G726DecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			return;
		}

		if (nbytes%50!=0)
		{
			freemsg(im);
			return;
		}
		/* not yet configured, or misconfigured */
		s->nsamples=80;
		s->nbytes=50;
		s->ms_per_frame=10;

		if (s->nbytes>0 && nbytes>=s->nbytes){
			int frame_per_packet = nbytes/s->nbytes;
			int k;

			for (k=0;k<frame_per_packet;k++)
			{
				int i, j;
				uint64_t v = 0;
				unsigned char *in_buf;
				short* pcm_buf;
				om=allocb(s->nsamples*2,0);

				in_buf = im->b_rptr + (k*s->nbytes);
				pcm_buf = (short*)om->b_wptr;

				for (i = 0; i < s->nbytes; i += 5) {
					v = ((uint64_t)in_buf[i+4]) << 32  | 
					  (uint64_t)(in_buf[i+3]) << 24 | 
					  (uint64_t)(in_buf[i+2]) << 16 | 
					  (uint64_t)(in_buf[i+1]) << 8 | 
					  (uint64_t)(in_buf[i]); 


					for (j = 0; j < 8; j++) {
						char w;
#ifdef G726_PACK_RFC3551
						w = (v >> (j*5)) & 0x1f;
#else
						w = (v >> ((7-j)*5)) & 0x1f;
#endif
						pcm_buf[8*(i/5)+j] = g723_40_decoder(w, AUDIO_ENCODING_LINEAR, &s->g723_dec);
					}
				 }


				i = PCM16_S2B(s->nbytes * 8 / 5);				
				om->b_wptr = om->b_wptr + s->nsamples*2;
				ms_queue_put(f->outputs[0],om);

			}
		}else{
			ms_warning("bad g726 frame!");
		}
		freemsg(im);
	}
}
static void dec32_process(MSFilter *f){
	G726DecState *s=(G726DecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			return;
		}

		if (nbytes%40!=0)
		{
			freemsg(im);
			return;
		}

		s->nsamples=80;
		s->nbytes=40;
		s->ms_per_frame=10;

		if (s->nbytes>0 && nbytes>=s->nbytes){
			int frame_per_packet = nbytes/s->nbytes;
			int k;

			for (k=0;k<frame_per_packet;k++)
			{
				int i, j;
				uint64_t v = 0;
				unsigned char *in_buf;
				short* pcm_buf;
				om=allocb(s->nsamples*2,0);

				in_buf = im->b_rptr + (k*s->nbytes);
				pcm_buf = (short*)om->b_wptr;

				for (i = 0; i < s->nbytes; i++) {
					for (j = 0; j < 2; j++) {
						char w;
#ifdef G726_PACK_RFC3551					
						w = (in_buf[i] >> (j*4)) & 0xf;
#else
						w = (in_buf[i] >> ((1-j)*4)) & 0xf;
#endif

						pcm_buf[2*i+j] = g721_decoder(w, AUDIO_ENCODING_LINEAR, &s->g723_dec);
					}
				 }


				i = PCM16_S2B(s->nbytes * 2);				
				om->b_wptr = om->b_wptr+s->nsamples*2;
				ms_queue_put(f->outputs[0],om);

			}
		}else{
			ms_warning("bad g726 frame!");
		}
		freemsg(im);
	}
}
static void dec24_process(MSFilter *f){
	G726DecState *s=(G726DecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			return;
		}

		if (nbytes%30!=0)
		{
			freemsg(im);
			return;
		}

		s->nsamples=80;
		s->nbytes=30;
		s->ms_per_frame=10;

		if (s->nbytes>0 && nbytes>=s->nbytes){
			int frame_per_packet = nbytes/s->nbytes;
			int k;

			for (k=0;k<frame_per_packet;k++)
			{
				int i, j;
				uint64_t v = 0;
				unsigned char *in_buf;
				short* pcm_buf;
				om=allocb(s->nsamples*2,0);

				in_buf = im->b_rptr + (k*s->nbytes);
				pcm_buf = (short*)om->b_wptr;

				for (i = 0; i < s->nbytes; i += 3) {
					
					uint32_t v = ((in_buf[i+2]) << 16) |
						((in_buf[i+1]) << 8) |
						(in_buf[i]);

					for (j = 0; j < 8; j++) {
						char w;
#ifdef G726_PACK_RFC3551
				        w = (v >> (j*3)) & 0x7;
#else
				        w = (v >> ((7-j)*3)) & 0x7;
#endif
						pcm_buf[8*(i/3)+j] = g723_24_decoder(w, AUDIO_ENCODING_LINEAR, &s->g723_dec);
					}
				 }


				i = PCM16_S2B(s->nbytes * 8 / 3);				
				om->b_wptr = om->b_wptr+s->nsamples*2;
				ms_queue_put(f->outputs[0],om);

			}
		}else{
			ms_warning("bad g726 frame!");
		}
		freemsg(im);
	}
}
static void dec16_process(MSFilter *f){
	G726DecState *s=(G726DecState*)f->data;
	mblk_t *im,*om;
	int nbytes;

	while ((im=ms_queue_get(f->inputs[0]))!=NULL){
		nbytes=msgdsize(im);
		if (nbytes<=0)
		{
			freemsg(im);
			return;
		}

		if (nbytes%20!=0)
		{
			freemsg(im);
			return;
		}

		/* not yet configured, or misconfigured */
		s->nsamples=80;
		s->nbytes=20;
		s->ms_per_frame=10;

		if (s->nbytes>0 && nbytes>=s->nbytes){
			int frame_per_packet = nbytes/s->nbytes;
			int k;

			for (k=0;k<frame_per_packet;k++)
			{
				int i, j;
				uint64_t v = 0;
				unsigned char *in_buf;
				short* pcm_buf;
				om=allocb(s->nsamples*2,0);

				in_buf = im->b_rptr + (k*s->nbytes);
				pcm_buf = (short*)om->b_wptr;

				for (i = 0; i < s->nbytes; i++) {
					for (j = 0; j < 4; j++) {
						char w;
#ifdef G726_PACK_RFC3551
					    w = (in_buf[i] >> (j*2)) & 0x3;
#else 
					    w = (in_buf[i] >> ((3-j)*2)) & 0x3;
#endif
						pcm_buf[4*i+j] = g723_16_decoder(w, AUDIO_ENCODING_LINEAR, &s->g723_dec);
					}
				 }


				i = PCM16_S2B(s->nbytes * 4);				
				om->b_wptr = om->b_wptr+s->nsamples*2;
				ms_queue_put(f->outputs[0],om);

			}
		}else{
			ms_warning("bad g726 frame!");
		}
		freemsg(im);
	}
}

MSFilterDesc ms_g726_40_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-40Dec",
	"G726-40 decoder",
	MS_FILTER_DECODER,
	"g726-40",
	1,
	1,
	dec_init,
    NULL,
	dec40_process,
    NULL,
	dec_uninit,
    NULL
};
MSFilterDesc ms_g721_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG721Dec",
	"G721 decoder",
	MS_FILTER_DECODER,
	"g721",
	1,
	1,
	dec_init,
    NULL,
	dec32_process,
    NULL,
	dec_uninit,
    NULL
};
MSFilterDesc ms_g726_32_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-32Dec",
	"G726-32 decoder",
	MS_FILTER_DECODER,
	"g726-32",
	1,
	1,
	dec_init,
    NULL,
	dec32_process,
    NULL,
	dec_uninit,
    NULL
};
MSFilterDesc ms_g726_24_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-24Dec",
	"G726-24 decoder",
	MS_FILTER_DECODER,
	"g726-24",
	1,
	1,
	dec_init,
    NULL,
	dec24_process,
    NULL,
	dec_uninit,
    NULL
};
MSFilterDesc ms_g726_16_dec_desc={
	MS_FILTER_PLUGIN_ID,
	"MSG726-16Dec",
	"G726-16 decoder",
	MS_FILTER_DECODER,
	"g726-16",
	1,
	1,
	dec_init,
    NULL,
	dec16_process,
    NULL,
	dec_uninit,
    NULL
};

#ifdef _MSC_VER
#define MS_PLUGIN_DECLARE(type) __declspec(dllexport) type
#else
#define MS_PLUGIN_DECLARE(type) type
#endif

MS_PLUGIN_DECLARE(void) libmsg726_init(){
	ms_filter_register(&ms_g726_40_enc_desc);
	ms_filter_register(&ms_g721_enc_desc);
	ms_filter_register(&ms_g726_32_enc_desc);
	ms_filter_register(&ms_g726_24_enc_desc);
	ms_filter_register(&ms_g726_16_enc_desc);

	ms_filter_register(&ms_g726_40_dec_desc);
	ms_filter_register(&ms_g721_dec_desc);
	ms_filter_register(&ms_g726_32_dec_desc);
	ms_filter_register(&ms_g726_24_dec_desc);
	ms_filter_register(&ms_g726_16_dec_desc);
}
