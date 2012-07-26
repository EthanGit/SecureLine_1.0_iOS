/*
  amsip is a SIP library for softphone (SIP -rfc3261-)
  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Aymeric MOIZARD - <amoizard@gmail.com>
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

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif


#import <CoreVideo/CoreVideo.h>
#import <AVFoundation/AVFoundation.h>

@class IOSH264Enc;

typedef struct _EncData{
    IOSH264Enc *enc;
    CFMutableDictionaryRef options;
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
}EncData;

/*
 ALBUM     => 'albm',    # album title and track number for the media
 ARTIST    => 'perf',    # performer or artist
 COM       => 'auth',    # author/composer of the media
 COMMENT   => 'dscp',    # caption or description for the media
 COPYRIGHT => 'cprt',    # notice about organisation holding copyright
 GENRE     => 'gnre',    # genre (category and style) of the media
 RTNG      => 'rtng',    # media rating
 TITLE     => 'titl',    # title for the media
 YEAR      => 'yrrc',    # recording year for the media
 
 # these exist in 3GP but not really in iTMS meta data
 CLASS    => 'clsf',     # classification of the media
 KEYWORDS => 'kywd',     # media keywords
 LOCATION => 'loci',     # location information
*/

struct tgp_atoms {
    char atom[4];
};

struct tgp_atoms atoms[] = {

    "ftyp",
    
    "albm",
    "perf",
    "auth",
    "dscp",
    "cprt",
    "cprt",
    "gnre",
    "rtng",
    "titl",
    "yrrc",
    "clsf",
    "kywd",
    "loci",
    '\0'
};

@interface IOSH264Enc : NSObject {
    NSURL *readyUrl;
    FILE *fd;
    int data_size;
}

-(void) process:(MSFilter *)f withTs:(uint32_t)ts;

@end

@implementation IOSH264Enc

uint16_t UInt16FromBigEndian(const char *string);
uint32_t UInt32FromBigEndian(const char *string);

uint16_t UInt16FromBigEndian(const char *string) {
#if defined (__ppc__) || defined (__ppc64__)
    uint16_t test;
    memcpy(&test,string,2);
    return test;
#else
    return ((string[0] & 0xff) << 8 | (string[1] & 0xff) << 0);
#endif
}

uint32_t UInt32FromBigEndian(const char *string) {
#if defined (__ppc__) || defined (__ppc64__)
    uint32_t test;
    memcpy(&test,string,4);
    return test;
#else
    return ((string[0] & 0xff) << 24 | (string[1] & 0xff) << 16 | (string[2] & 0xff) << 8 | (string[3] & 0xff) << 0);
#endif
}

-(uint32_t) find_atom:(const char*)atom {
    char char_size[78];
    int len;
    
#if 0
    if (strncasecmp(atom, "avc1", 4)==0)
        len = fread(char_size, 1, 8, fd); //don't yet understand this: read spec...
    if (strncasecmp(atom, "avcC", 4)==0)
        len = fread(char_size, 1, 78, fd); //don't yet understand this: read spec...
#else
    if (strncasecmp(atom, "avc1", 4)==0)
        len = fseek(fd, 8, SEEK_CUR); //don't yet understand this: read spec...
    if (strncasecmp(atom, "avcC", 4)==0)
        len = fseek(fd, 78, SEEK_CUR); //don't yet understand this: read spec...
#endif
    
    len = fread(char_size, 1, 4, fd);
    if (len!=4)
    {
        return -1;
    }        
    uint32_t len_atom = UInt32FromBigEndian(char_size);
    while (len==4)
    {
        
        len = fread(char_size, 1, 4, fd);
        if (len!=4)
        {
            return -1;
        }        
        
        //ms_message("3GPP: atom %c%c%c%c", char_size[0], char_size[1],char_size[2],char_size[3]);
        if (strncasecmp(char_size, atom, 4)==0)
            return len_atom;
#if 0
        mblk_t *atom2 = allocb(len_atom-8, 0);
        len = fread(atom2->b_rptr, 1, len_atom-8, fd);
        if (len!=len_atom-8)
        {
            return -1;
        }
        atom2->b_wptr = atom2->b_rptr+len_atom-8;
        
        freeb(atom2);
#else
        len = fseek(fd, len_atom-8, SEEK_CUR);
        if (len!=0)
        {
            return -1;
        }
#endif
        len = fread(char_size, 1, 4, fd);
        if (len!=4)
        {
            return -1;
        }
        len_atom = UInt32FromBigEndian(char_size);
    }
    return -1;
}

-(BOOL)tgp_display_atom:(mblk_t *)atom {
    
    if (strncasecmp((char*)atom->b_rptr, "ftyp", 4)==0)
    {
        ms_message("3GPP: ftyp = %c%c%c%c", atom->b_rptr[0], atom->b_rptr[1],atom->b_rptr[2],atom->b_rptr[3]);   
    }
    return YES;
}

-(void)tgb_close {
    fclose(fd);
    fd=NULL;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *documentPath = NSTemporaryDirectory();
    NSString *readyPath = [documentPath stringByAppendingPathComponent: @"video-ready.3gp"];
    if ([fileManager fileExistsAtPath:readyPath]) {
        
        NSError *error;
        BOOL success = [fileManager removeItemAtPath:readyPath error:&error];
        if (!success)
            NSLog(@"error: file not removed in %@", readyPath);
        else
            NSLog(@"file removed in %@", readyPath);
    }
}

-(void) tgp_open:(MSFilter *)f {
 	EncData *d=(EncData*)f->data;
   
    NSString *documentPath = NSTemporaryDirectory();
    NSString *readyPath = [documentPath stringByAppendingPathComponent: @"video-ready.3gp"];
    
    if (fd!=NULL)
        return;
    
    char char_size[4];
        
    fd = fopen([readyPath cStringUsingEncoding:NSUTF8StringEncoding], "r");
    if (fd==NULL)
        return;

    ms_message("3GPP: opening (last framenum = %i)", d->framenum);
        
    /* search for avcC */
    int len = [self find_atom:"moov"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"trak"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"mdia"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"minf"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"stbl"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"stsd"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"avc1"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    len = [self find_atom:"avcC"];
    if (len<=0)
    {
        [self tgb_close];
        return;
    }
    
    //READ SPS
    uint8_t configurationVersion = 1; 
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    configurationVersion = char_size[0];
    uint8_t AVCProfileIndication; 
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    AVCProfileIndication = char_size[0];
    uint8_t profile_compatibility; 
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    profile_compatibility = char_size[0];
    uint8_t AVCLevelIndication;  
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    AVCLevelIndication = char_size[0];
    //bit(6) reserved = ‘111111’b;
    //unsigned int(2) lengthSizeMinusOne;
    uint8_t lengthSizeMinusOne;
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    lengthSizeMinusOne = (char_size[0] & 0x03);
    //bit(3) reserved = ‘111’b;
    //unsigned int(5) numOfSequenceParameterSets;
    uint8_t numOfSequenceParameterSets;
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    numOfSequenceParameterSets = char_size[0] & 0x1f;
    
    int i;
    for (i=0; i< numOfSequenceParameterSets;  i++) { 
        uint16_t sequenceParameterSetLength;
        len = fread(char_size, 1, 2, fd);
        if (len!=2)
        {
            [self tgb_close];
            return;
        }
        sequenceParameterSetLength = UInt16FromBigEndian(char_size);
#if 0
        d->sav_sps = allocb(sequenceParameterSetLength, 0);
        len = fread(d->sav_sps->b_rptr, 1, sequenceParameterSetLength, fd);
        if (len!=sequenceParameterSetLength)
        {
            [self tgb_close];
            return;
        }
        d->sav_sps->b_wptr = d->sav_sps->b_rptr+sequenceParameterSetLength;
#else
        mblk_t *sps = allocb(sequenceParameterSetLength, 0);
        len = fread(sps->b_rptr, 1, sequenceParameterSetLength, fd);
        if (len!=sequenceParameterSetLength)
        {
            [self tgb_close];
            return;
        }
        sps->b_wptr = sps->b_rptr+sequenceParameterSetLength;
        if (d->sav_sps)
        {
            if (sequenceParameterSetLength!=msgdsize(d->sav_sps)
                || memcmp(sps->b_rptr, d->sav_sps->b_rptr, sequenceParameterSetLength)!=0)
            {
                ms_message("3GPP: different SPS");
                d->framenum=0; /* force resending PPS/SPS */
            }
        }
        if (d->sav_sps)
            freeb(d->sav_sps);
        d->sav_sps = sps;
#endif        
    }
    
    uint8_t numOfPictureParameterSets;
    len = fread(char_size, 1, 1, fd);
    if (len!=1)
    {
        [self tgb_close];
        return;
    }
    numOfPictureParameterSets = char_size[0];
    for (i=0; i< numOfPictureParameterSets;  i++) { 
        uint16_t pictureParameterSetLength; 
        len = fread(char_size, 1, 2, fd);
        if (len!=2)
        {
            [self tgb_close];
            return;
        }
        
        //bit(8*pictureParameterSetLength) pictureParameterSetNALUnit; 
        pictureParameterSetLength = UInt16FromBigEndian(char_size);
        mblk_t *pps = allocb(pictureParameterSetLength, 0);
        len = fread(pps->b_rptr, 1, pictureParameterSetLength, fd);
        if (len!=pictureParameterSetLength)
        {
            [self tgb_close];
            return;
        }
        pps->b_wptr = pps->b_rptr+pictureParameterSetLength;
        if (d->sav_pps)
        {
            if (pictureParameterSetLength!=msgdsize(d->sav_pps)
                || memcmp(pps->b_rptr, d->sav_pps->b_rptr, pictureParameterSetLength)!=0)
            {
                ms_message("3GPP: different PPS");
                d->framenum=0; /* force resending PPS/SPS */
            }
        }
        
        if (d->sav_pps)
            freeb(d->sav_pps);
        d->sav_pps = pps;
    }
    //ms_message("avcC: lengthSizeMinusOne: %i %i*sps (size=%i) %i*pps (size=%i)", lengthSizeMinusOne,
    //           numOfSequenceParameterSets, msgdsize(d->sav_sps), numOfPictureParameterSets, msgdsize(d->sav_pps));
    
    fclose(fd);
    fd = fopen([readyPath cStringUsingEncoding:NSUTF8StringEncoding], "r");
    len = fread(char_size, 1, 4, fd);
    if (len!=4)
    {
        [self tgb_close];
        return;
    }        
    uint32_t len_atom = UInt32FromBigEndian(char_size);
    while (len==4)
    {
        
        len = fread(char_size, 1, 4, fd);
        if (len!=4)
        {
            [self tgb_close];
            return;
        }        
        
        //ms_message("3GPP: atom %c%c%c%c", char_size[0], char_size[1],char_size[2],char_size[3]);
        if (strncasecmp(char_size, "mdat", 4)==0)
            break;
        
#if 0
        mblk_t *atom = allocb(len_atom-8, 0);
        len = fread(atom->b_rptr, 1, len_atom-8, fd);
        if (len!=len_atom-8)
        {
            [self tgb_close];
            return;
        }
        atom->b_wptr = atom->b_rptr+len_atom-8;
        
        //[self tgp_display_atom:atom];
        
        freeb(atom);
#else
        len = fseek(fd, len_atom-8, SEEK_CUR);
        if (len!=0)
        {
            [self tgb_close];
            return;
        }
#endif
        len = fread(char_size, 1, 4, fd);
        if (len!=4)
        {
            [self tgb_close];
            return;
        }
        len_atom = UInt32FromBigEndian(char_size);
    }
    
    /* we have found mdat and len_atom is the total size of raw data */
    //ms_message("3GPP: raw data len = %i", len_atom);
    data_size = len_atom;
    //READ: http://atomicparsley.sourceforge.net/
    
    //TODO: search for the avcC: contains the SPS/PPS
    //ISO 14496 part 15 : 'avcC' box documented in section 5.3.4.1.2 and 5.2.4.1.1
    
    
}

-(BOOL) tgp_read:(MSFilter *)f withTs:(uint32_t)ts{

	EncData *d=(EncData*)f->data;
    //postion for NAL: stsc / stsz / stco
    //but 0000 might be enough?
    //AVC NAL units are in the following format in MDAT section: [4 bytes] = NAL length, network order; [NAL bytes] Shortly, start codes are simply replaced by lengths.
    
    if (fd==NULL)
        return NO;
    
    int len;
    char char_size[4];
    len = fread(char_size, 1, 4, fd);
    if (len!=4)
    {
        ms_message("3GPP: reading NAL length failed this was last NAL // closing file");
        [self tgb_close];
        return NO;
    }
    int len_nal = UInt32FromBigEndian(char_size);

    mblk_t *nal = allocb(len_nal, 0);
    len = fread(nal->b_rptr, 1, len_nal, fd);
    if (len!=len_nal)
    {
        [self tgb_close];
        return NO;
    }
    nal->b_wptr = nal->b_rptr+len_nal;
    data_size-=len_nal+4;
	MSQueue nalus;
	ms_queue_init(&nalus);
    
    if (d->framenum==0)
    {
        /* resend PPS/SPS */
        if (d->sav_sps!=NULL && d->sav_pps!=NULL)
        {
            ms_queue_put(&nalus,dupb(d->sav_sps));
            ms_queue_put(&nalus,dupb(d->sav_pps));
            rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
        }
    }
    d->framenum++;
    
    ms_queue_put(&nalus,nal);
    rfc3984_pack(&d->packer,&nalus,f->outputs[0],ts);
    //ms_message("3GPP: new NAL with size = %i (data left: %i)", len_nal, data_size);

    if (data_size<=8)
    {
        //ms_message("3GPP: this was last NAL // closing file");
        [self tgb_close];
        return NO;
    }
    return YES;
}

-(void) process:(MSFilter *)f withTs:(uint32_t)ts {
    if ([self tgp_read:f withTs:ts]==YES)
        return;
    
    [self tgp_open:f];    
    [self tgp_read:f withTs:ts];
}

@end


static void enc_init(MSFilter *f){
	EncData *d=ms_new(EncData,1);
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
	EncData *d=(EncData*)f->data;
	ms_free(d);
}


static void enc_preprocess(MSFilter *f){
	EncData *d=(EncData*)f->data;
	rfc3984_init(&d->packer);
	rfc3984_set_mode(&d->packer,d->mode);

    d->enc = [[IOSH264Enc alloc] init];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *documentPath = NSTemporaryDirectory();
    NSString *readyPath = [documentPath stringByAppendingPathComponent: @"video-ready.3gp"];
    if ([fileManager fileExistsAtPath:readyPath]) {
        
        NSError *error;
        BOOL success = [fileManager removeItemAtPath:readyPath error:&error];
        if (!success)
            NSLog(@"error: file not removed in %@", readyPath);
        else
            NSLog(@"file removed in %@", readyPath);
    }
    
	if (d->enc==NULL) ms_error("Fail to create iosh264 encoder.");
	d->framenum=0;
	d->sav_pps=NULL;
	d->sav_sps=NULL;
}

static void enc_postprocess(MSFilter *f){
	EncData *d=(EncData*)f->data;
	rfc3984_uninit(&d->packer);
	if (d->enc!=NULL){
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
	EncData *d=(EncData*)f->data;
	uint32_t ts=(uint32_t)(f->ticker->time*90LL);
	mblk_t *im;
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
        [d->enc process:f withTs:ts];            
        freemsg(im);
	}
}

static int enc_set_br(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
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
	EncData *d=(EncData*)f->data;
	d->fps=*(float*)arg;
	return 0;
}

static int enc_get_fps(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	*(float*)arg=d->fps;
	return 0;
}

static int enc_get_vsize(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
	*(MSVideoSize*)arg=d->vsize;
	return 0;
}

static int enc_add_fmtp(MSFilter *f, void *arg){
	EncData *d=(EncData*)f->data;
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

MSFilterDesc iosh264_enc_desc={
	MS_FILTER_PLUGIN_ID,
	"MSIOSH264Enc",
	"A H264 encoder based on iOS hardware encoder.",
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
}DecData;

static void ffmpeg_init(){
	static bool_t done=FALSE;
	if (!done){
		avcodec_init();
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
#if defined(ANDROID)
#elif TARGET_OS_IPHONE
#else
	d->av_context.flags2 |= CODEC_FLAG2_CHUNKS;
#endif
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

void libmsiosh264_init(void);

void libmsiosh264_init(void){
    
	ms_filter_register(&iosh264_enc_desc);
	ms_filter_register(&h264_dec_desc);
}
