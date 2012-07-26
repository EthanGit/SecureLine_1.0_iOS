/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

/* this file is specifically distributed under a BSD license */

/**
* Copyright (C) 2007  Hiroki Mori (himori@users.sourceforge.net)
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY <copyright holder> ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/


/** 
* 
* mediastreamer2 library - modular sound and video processing and streaming
* Copyright (C) 2010  Aymeric Moizard (jack@atosc.org)
*
* All rights reserved.
*/

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#include <UIKit/UIKit.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msfilter.h"

MSFilter *ms_ca_read_new(MSSndCard *card);
MSFilter *ms_ca_write_new(MSSndCard *card);

static float gain_volume_in=1.0;
static float gain_volume_out=1.0;
static bool gain_changed_in = true;
static bool gain_changed_out = true;

#if TARGET_OS_IPHONE
#define CFStringRef void *
#define CFRelease(A) {}
#define CFStringGetCString(A, B, LEN, encoding)  {}
#define CFStringCreateCopy(A, B) NULL

#define AudioDeviceID int
#endif

typedef struct CAData{
	int dev;
	
	AudioUnit caOutAudioUnit;
	AudioUnit caInAudioUnit;
	AURenderCallbackStruct caOutRenderCallback;
	AURenderCallbackStruct caInRenderCallback;
	int rate;
	int bits;
	ms_mutex_t mutex;
	queue_t rq;
	MSBufferizer * bufferizer;
	bool_t read_started;
	bool_t write_started;
	bool_t stereo;
	
	bool_t reset;
	bool_t pending_interrupt;
} CAData;

static void cacard_set_level(MSSndCard *card, MSSndCardMixerElem e, int percent)
{
	switch(e){
		case MS_SND_CARD_PLAYBACK:
		case MS_SND_CARD_MASTER:
			gain_volume_out =((float)percent)/100.0f;
			gain_changed_out = true;
			return;
		case MS_SND_CARD_CAPTURE:
			gain_volume_in =((float)percent)/100.0f;
			gain_changed_in = true;
			return;
		default:
			ms_warning("cacard_set_level: unsupported command.");
	}
}

static int cacard_get_level(MSSndCard *card, MSSndCardMixerElem e)
{
	switch(e){
		case MS_SND_CARD_PLAYBACK:
		case MS_SND_CARD_MASTER:
			return (int)(gain_volume_out*100.0f);
		case MS_SND_CARD_CAPTURE:
			return (int)(gain_volume_in*100.0f);
		default:
			ms_warning("cacard_get_level: unsupported command.");
	}
	return -1;
}

static void cacard_set_source(MSSndCard *card, MSSndCardCapture source)
{
}

static void onAudioRouteChanged(void *inRefCon,
						 AudioSessionPropertyID	inID,
						 UInt32                 inDataSize,
						 const void *           inData)
{
    CAData *d=(CAData*)inRefCon;
	
    if (inID == kAudioSessionProperty_AudioRouteChange) {
		
		if (d->pending_interrupt)
			return;
		ms_message("onAudioRouteChanged: restarting if needed\n");
		
		if (d->read_started == TRUE || d->write_started == TRUE) {
			d->reset=TRUE;
		}
    }
}

static void OnInterruption(void *inRefCon, UInt32 inInterruption)
{
	CAData *d=(CAData*)inRefCon;
	
    if (inInterruption == kAudioSessionEndInterruption) {
		ms_message("OnInterruption: kAudioSessionEndInterruption\n");
		d->pending_interrupt = FALSE;
		onAudioRouteChanged(inRefCon, kAudioSessionProperty_AudioRouteChange, 0, NULL);
    } else if (inInterruption == kAudioSessionBeginInterruption) {
		ms_message("OnInterruption: kAudioSessionBeginInterruption\n");
		d->pending_interrupt = TRUE;
    }
}

static void cacard_init(MSSndCard * card)
{
	CAData *d = ms_new0(CAData, 1);
	d->read_started=FALSE;
	d->write_started=FALSE;
	d->bits=16;
	d->rate=8000;
	d->stereo=FALSE;
	qinit(&d->rq);
	d->bufferizer=ms_bufferizer_new();
	ms_mutex_init(&d->mutex,NULL);
	d->reset=FALSE;
	d->pending_interrupt=FALSE;
	card->data = d;
	
	if (AudioSessionInitialize(NULL, NULL, OnInterruption, d) !=
		kAudioSessionNoError)
    {
		ms_warning("Warning: cannot initialize audio session services");
    }
	
	AudioSessionAddPropertyListener(kAudioSessionProperty_AudioRouteChange,
									onAudioRouteChanged, d);
}

static void cacard_uninit(MSSndCard * card)
{
	CAData *d=(CAData*)card->data;
	
	AudioSessionRemovePropertyListenerWithUserData(
		kAudioSessionProperty_AudioRouteChange, onAudioRouteChanged, d);
	
	ms_bufferizer_destroy(d->bufferizer);
	flushq(&d->rq,0);
	ms_mutex_destroy(&d->mutex);
	ms_free(d);
}

static void cacard_detect(MSSndCardManager *m);
static MSSndCard *cacard_duplicate(MSSndCard *obj);

MSSndCardDesc ca_card_desc={
	.driver_type="CA",
	.detect=cacard_detect,
	.init=cacard_init,
	.set_level=cacard_set_level,
	.get_level=cacard_get_level,
	.set_capture=cacard_set_source,
	.set_control=NULL,
	.get_control=NULL,
	.create_reader=ms_ca_read_new,
	.create_writer=ms_ca_write_new,
	.uninit=cacard_uninit,
	.duplicate=cacard_duplicate
};

static MSSndCard *cacard_duplicate(MSSndCard * obj)
{
	MSSndCard *card = ms_snd_card_new(&ca_card_desc);
	card->name = ms_strdup(obj->name);
	card->capabilities = obj->capabilities;
	return card;
}

static MSSndCard *ca_card_new(const char *name, CFStringRef uidname, AudioDeviceID dev, unsigned cap)
{
	MSSndCard *card = ms_snd_card_new(&ca_card_desc);
	card->name = ms_strdup(name);
	card->capabilities = cap;
	return card;
}

static void show_format(char *name,
						AudioStreamBasicDescription * deviceFormat)
{
	ms_message("Format for %s", name);
	ms_message("mSampleRate = %g", deviceFormat->mSampleRate);
	char *the4CCString = (char *) &deviceFormat->mFormatID;
	char outName[5];
	outName[0] = the4CCString[0];
	outName[1] = the4CCString[1];
	outName[2] = the4CCString[2];
	outName[3] = the4CCString[3];
	outName[4] = 0;
	ms_message("mFormatID = %s", outName);
	ms_message("mFormatFlags = %08lX", deviceFormat->mFormatFlags);
	ms_message("mBytesPerPacket = %ld", deviceFormat->mBytesPerPacket);
	ms_message("mFramesPerPacket = %ld", deviceFormat->mFramesPerPacket);
	ms_message("mChannelsPerFrame = %ld", deviceFormat->mChannelsPerFrame);
	ms_message("mBytesPerFrame = %ld", deviceFormat->mBytesPerFrame);
	ms_message("mBitsPerChannel = %ld", deviceFormat->mBitsPerChannel);
}

static void cacard_detect(MSSndCardManager * m)
{
	AudioStreamBasicDescription deviceFormat;
	memset(&deviceFormat, 0, sizeof(AudioStreamBasicDescription));
	
	MSSndCard *card = ca_card_new("AudioUnit Device", NULL, 0 /*?*/, MS_SND_CARD_CAP_PLAYBACK|MS_SND_CARD_CAP_CAPTURE);
	ms_snd_card_manager_add_card(m, card);
}

// Convenience function to dispose of our audio buffers
static void DestroyAudioBufferList(AudioBufferList* list)
{
	UInt32						i;
	
	if(list) {
		for(i = 0; i < list->mNumberBuffers; i++) {
			if(list->mBuffers[i].mData)
				free(list->mBuffers[i].mData);
		}
		free(list);
	}
}

// Convenience function to allocate our audio buffers
static AudioBufferList *AllocateAudioBufferList(UInt32 numChannels, UInt32 size)
{
	AudioBufferList*			list;
	UInt32						i;
	
	list = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + numChannels * sizeof(AudioBuffer));
	if(list == NULL)
		return NULL;
	
	list->mNumberBuffers = numChannels;
	for(i = 0; i < numChannels; ++i) {
		list->mBuffers[i].mNumberChannels = 1;
		list->mBuffers[i].mDataByteSize = size;
		list->mBuffers[i].mData = malloc(size);
		if(list->mBuffers[i].mData == NULL) {
			DestroyAudioBufferList(list);
			return NULL;
		}
	}
	return list;
}

static OSStatus readRenderProc(void *inRefCon, 
						AudioUnitRenderActionFlags *inActionFlags,
						const AudioTimeStamp *inTimeStamp, 
						UInt32 inBusNumber,
						UInt32 inNumFrames, 
						AudioBufferList *ioData)
{
	CAData *d=(CAData*)inRefCon;
	OSStatus	err = noErr;
	mblk_t * rm=NULL;
	rm=allocb(ioData->mBuffers[0].mDataByteSize,0);
	ioData->mBuffers[0].mData=rm->b_wptr;
	err = AudioUnitRender(d->caInAudioUnit, inActionFlags, inTimeStamp, inBusNumber,
						  inNumFrames, ioData);
	if(err != noErr)
	{
		ms_error("AudioUnitRender %d", err);
		freeb(rm);
		return err;
	}
	
	if (d->read_started) {
		rm->b_wptr += ioData->mBuffers[0].mDataByteSize;
	
		//NSLog(@"ioData->mBuffers[0].mDataByteSize=%i", ioData->mBuffers[0].mDataByteSize);
		if (gain_volume_in != 1.0f)
		{
			int16_t *ptr=(int16_t *)rm->b_rptr;
			for (;ptr<(int16_t *)rm->b_wptr;ptr++)
			{
				*ptr=(int16_t)(((float)(*ptr))*gain_volume_in);
			}
		}
		ms_mutex_lock(&d->mutex);
		putq(&d->rq,rm);
		ms_mutex_unlock(&d->mutex);
	}
	else {
		freeb(rm);
	}

	
	return err;
}

static OSStatus writeRenderProc(void *inRefCon, 
						 AudioUnitRenderActionFlags *inActionFlags,
						 const AudioTimeStamp *inTimeStamp, 
						 UInt32 inBusNumber,
						 UInt32 inNumFrames, 
						 AudioBufferList *ioData)
{
    OSStatus err= noErr;
	CAData *d=(CAData*)inRefCon;
#if !TARGET_OS_IPHONE
	if (gain_changed_out == true)
	{
		err = AudioUnitSetParameter(d->caOutAudioUnit, kAudioUnitParameterUnit_LinearGain,
									   kAudioUnitScope_Global, 0, (Float32)gain_volume_out, 0);
		if(err != noErr)
		{
			ms_error("failed to set output volume %i", err);
		}
	    gain_changed_out = false;
		err= noErr;
	}	
#endif
	if(d->write_started != FALSE) {
		ms_mutex_lock(&d->mutex);
		if(ms_bufferizer_get_avail(d->bufferizer) >= inNumFrames*d->bits/8) {
			ms_bufferizer_read(d->bufferizer, ioData->mBuffers[0].mData, inNumFrames*d->bits/8);
			
			if (ms_bufferizer_get_avail(d->bufferizer) >10*inNumFrames*d->bits/8) {
				//too much data
				ms_bufferizer_flush(d->bufferizer);
				ms_message("too much data for sound card");
			}
			ioData->mBuffers[0].mDataByteSize=inNumFrames*d->bits/8;
			
#if TARGET_OS_IPHONE
			if (gain_volume_out != 1.0f)
			{
				int16_t *end=(int16_t *)ioData->mBuffers[0].mData;
				int16_t *ptr=(int16_t *)ioData->mBuffers[0].mData;
				end += ioData->mBuffers[0].mDataByteSize/2;
				for (;ptr<end;ptr++)
				{
					*ptr=(int16_t)(((float)(*ptr))*gain_volume_out);
				}
			}
#endif
			ms_mutex_unlock(&d->mutex);
		} else {
			//not enough data
			NSLog(@"not enough data for sound card");
			ms_mutex_unlock(&d->mutex);
			memset(ioData->mBuffers[0].mData, 0, inNumFrames*d->bits/8); //ioData->mBuffers[0].mDataByteSize);
		}
	}
	AudioBufferList buf;
	buf.mBuffers[0].mDataByteSize=inNumFrames*d->bits/8; 
	buf.mNumberBuffers=1;
	buf.mBuffers[0].mData=NULL;
	buf.mBuffers[0].mNumberChannels=d->stereo?2:1;
	readRenderProc(d, inActionFlags, inTimeStamp, 1, inNumFrames, &buf);
	
    return err;
}

static int ca_open_rw(CAData *d){
	OSStatus result;
	UInt32 param;
	
	AudioComponentDescription desc;  
	AudioComponent comp;
	
	NSLog(@"starting audio");
	
	// Get Default Input audio unit
	desc.componentType = kAudioUnitType_Output;
#if !TARGET_IPHONE_SIMULATOR
	desc.componentSubType = kAudioUnitSubType_VoiceProcessingIO;
#else
	desc.componentSubType = kAudioUnitSubType_RemoteIO;
#endif
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	if (d->caInAudioUnit!=NULL || d->caOutAudioUnit!=NULL)
	{
		return 0;
	}
	
	comp = AudioComponentFindNext (NULL,&desc);
	if (comp == NULL)
	{
		ms_message("Cannot find audio component");
		return -1;
	}
	
	result = AudioComponentInstanceNew(comp, &d->caOutAudioUnit);
	if(result != noErr)
	{
		ms_message("Cannot open audio component %x", result);
		return -1;
	}
	d->caInAudioUnit = d->caOutAudioUnit;
	result=AudioUnitUninitialize (d->caOutAudioUnit);

#if 1
	// Turn AEC off
	param = 1;
	AudioUnitSetProperty(d->caInAudioUnit,
						 kAUVoiceIOProperty_BypassVoiceProcessing,
						 kAudioUnitScope_Global,
						 1,
						 &param,
						 sizeof(param));
	
	// Use 127 quality
	param = 127;
	AudioUnitSetProperty(d->caInAudioUnit,
						 kAUVoiceIOProperty_VoiceProcessingQuality,
						 kAudioUnitScope_Global,
						 1,
						 &param,
						 sizeof(param));
	
	// Turn AGC off
	param = 0;
	AudioUnitSetProperty(d->caInAudioUnit,
						 kAUVoiceIOProperty_VoiceProcessingEnableAGC,
						 kAudioUnitScope_Global,
						 1,
						 &param,
						 sizeof(param));
	
	param = 0;
	AudioUnitSetProperty(d->caInAudioUnit,
						 kAUVoiceIOProperty_DuckNonVoiceAudio,
						 kAudioUnitScope_Global,
						 1,
						 &param,
						 sizeof(param));
#endif
		
	param = 1;
	result = AudioUnitSetProperty(d->caOutAudioUnit,
								  kAudioOutputUnitProperty_EnableIO,
								  kAudioUnitScope_Output,
								  0,
								  &param,
								  sizeof(UInt32));
	ms_message("AudioUnitSetProperty %i %x", result, result);
	
	param = 1;
	result = AudioUnitSetProperty(d->caOutAudioUnit,
								  kAudioOutputUnitProperty_EnableIO,
								  kAudioUnitScope_Input,
								  1,
								  &param,
								  sizeof(UInt32));
	ms_message("AudioUnitSetProperty %i %x", result, result);
	

	AudioStreamBasicDescription caASBD;
	caASBD.mSampleRate		= d->rate;
	caASBD.mFormatID		= kAudioFormatLinearPCM;
	caASBD.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	caASBD.mFramesPerPacket	= 1;
	caASBD.mChannelsPerFrame= d->stereo?2:1;
	caASBD.mBitsPerChannel	= d->bits;
	caASBD.mBytesPerPacket	= d->bits / 8;
	caASBD.mBytesPerFrame	= (d->stereo?2:1) * d->bits / 8;
	
	// Setup Output audio unit
	result = AudioUnitSetProperty (d->caOutAudioUnit,
								   kAudioUnitProperty_StreamFormat,
								   kAudioUnitScope_Output,
								   1,
								   &caASBD,
								   sizeof(caASBD));
	ms_message("AudioUnitSetProperty %i %x", result, result);
	result = AudioUnitSetProperty (d->caOutAudioUnit,
								   kAudioUnitProperty_StreamFormat,
								   kAudioUnitScope_Input,
								   0,
								   &caASBD,
								   sizeof(caASBD));
	ms_message("AudioUnitSetProperty %i %x", result, result);
	
	param = 0;
	result = AudioUnitSetProperty (d->caOutAudioUnit,
								   kAudioUnitProperty_ShouldAllocateBuffer,
								   kAudioUnitScope_Output,
								   0,
								   &param,
								   sizeof(UInt32));
	
	Float64 rate = d->rate;
	result=AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate
									 ,sizeof(rate)
									 , &rate);
	ms_message("AudioSessionSetProperty %i %x", result, result);
	
	result = AudioUnitInitialize(d->caInAudioUnit);
	if(result != noErr)
	{
		ms_error("failed to AudioUnitInitialize input %i", result);
		return -1;
	}
	ms_message("AudioUnitInitialize %i %x", result, result);

	Float32 duration=0.020f;
	result=AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration
									 ,sizeof(duration)
									 , &duration);
	ms_message("AudioSessionSetProperty %i %x", result, result);
	
	
	memset((char*)&d->caOutRenderCallback, 0, sizeof(AURenderCallbackStruct));
	d->caOutRenderCallback.inputProc = writeRenderProc;
	d->caOutRenderCallback.inputProcRefCon = d;
	result = AudioUnitSetProperty (d->caOutAudioUnit, 
								   kAudioUnitProperty_SetRenderCallback, 
								   kAudioUnitScope_Input, 
								   0,
								   &d->caOutRenderCallback, 
								   sizeof(AURenderCallbackStruct));
	if(result != noErr)
		ms_error("AudioUnitSetProperty %x", result);
	ms_message("AudioUnitSetProperty %i %x", result, result);

	
	return 0;
}

static void ca_audio_start(CAData *d){
	OSStatus result = AudioOutputUnitStart(d->caInAudioUnit);
	if(result != noErr) {
		ms_message("AudioOutputUnitStart %i %x", result, result);        
    } else {
		d->read_started = TRUE;
		d->write_started = TRUE;		
		ms_message("AudioOutputUnitStart");
	}
}

static void ca_stop_rw(CAData *d){
	if (d->caInAudioUnit!=NULL)
	{
		AudioUnitUninitialize(d->caInAudioUnit);
		AudioComponentInstanceDispose(d->caInAudioUnit);
		d->caInAudioUnit=NULL;
		d->caOutAudioUnit=NULL;
	}

	flushq(&d->rq,0);
	ms_bufferizer_flush(d->bufferizer);
}

static void ca_audio_stop(CAData *d){
	if (d->caInAudioUnit!=NULL)
	{
        OSStatus result;
		AudioUnitElement inputBus = 1;
		AudioUnitReset(d->caInAudioUnit,
					   kAudioUnitScope_Global,
					   inputBus);
		AudioUnitElement outputBus = 0;
		AudioUnitReset(d->caInAudioUnit,
					   kAudioUnitScope_Global,
					   outputBus);
        result = AudioOutputUnitStop(d->caInAudioUnit);
        if(result != noErr) {
            ms_message("AudioOutputUnitStop %i %x", result, result);
        } else {
			d->read_started=FALSE;
			d->write_started=FALSE;
            ms_message("AudioOutputUnitStop");
		}
    }
	flushq(&d->rq,0);
	ms_bufferizer_flush(d->bufferizer);
}

static void ca_stop_r(CAData *d){
	if (d->read_started == TRUE && d->write_started == TRUE ) {
		ca_audio_stop(d);
		//ca_stop_rw(d);
		d->read_started=FALSE;
		return;
	}
	d->read_started=FALSE;
}

static void ca_stop_w(CAData *d){
	if (d->read_started == TRUE && d->write_started == TRUE ) {
		ca_audio_stop(d);
		//ca_stop_rw(d);
		d->write_started=FALSE;
		return;
	}
	d->write_started=FALSE;
}


static mblk_t *ca_get(CAData *d){
	mblk_t *m;
	ms_mutex_lock(&d->mutex);
	m=getq(&d->rq);
	ms_mutex_unlock(&d->mutex);
	return m;
}

static void ca_put(CAData *d, mblk_t *m){
	ms_mutex_lock(&d->mutex);
	ms_bufferizer_put(d->bufferizer,m);
	ms_mutex_unlock(&d->mutex);
}

static void ca_read_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	int i;
	i = ca_open_rw(d);
	if (i<0)
	{
		ca_stop_rw(d);
	} else {
		ca_audio_start(d);
	}
}

static void ca_read_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	ca_stop_r(d);
}

static void ca_read_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	mblk_t *m;
	
	if (d->reset==TRUE)
	{
		if ([[UIApplication sharedApplication] applicationState] == UIApplicationStateActive)
		{
			ms_message("restarting device after route change\n");
			d->reset=FALSE;
			ca_audio_stop(d);
			//ca_stop_rw(d);
			//ca_open_rw(d);
			ca_audio_start(d);
		}
	}
	
	//if (d->rq.q_mcount>1)
	//{
	//	printf("RTP burst from sound card %i\n", d->rq.q_mcount);
	//}
	
	while((m=ca_get(d))!=NULL){
		ms_queue_put(f->outputs[0],m);
	}
}

static void ca_read_uninit(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	ca_stop_rw(d);
}

static void ca_write_preprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	int i = ca_open_rw(d);
	if (i<0)
	{
		ca_stop_rw(d);
	} else {
		ca_audio_start(d);
	}
}

static void ca_write_postprocess(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	ca_stop_w(d);
}

static void ca_write_process(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	mblk_t *m;
	while((m=ms_queue_get(f->inputs[0]))!=NULL){
		ca_put(d,m);
	}
}

static void ca_write_uninit(MSFilter *f){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	ca_stop_rw(d);
}

static int set_rate(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	d->rate = *((int *) arg);
	if (d->read_started == TRUE || d->write_started == TRUE ) {
		d->reset=TRUE;
	} else {
		int i = ca_open_rw(d);
		if (i<0)
		{
			ca_stop_rw(d);
		}
	}

	return 0;
}

static int get_rate(MSFilter * f, void *arg)
{
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	*((int *) arg) = d->rate;
	return 0;
}

static int set_nchannels(MSFilter *f, void *arg){
	MSSndCard *card=(MSSndCard*)f->data;
	CAData *d = (CAData *) card->data;
	d->stereo=(*((int*)arg)==2);
	return 0;
}

static MSFilterMethod ca_methods[]={
	{	MS_FILTER_SET_SAMPLE_RATE	, set_rate	},
	{	MS_FILTER_GET_SAMPLE_RATE	, get_rate },
	{	MS_FILTER_SET_NCHANNELS		, set_nchannels	},
	{	0				, NULL		}
};

MSFilterDesc ca_read_desc={
	.id=MS_CA_READ_ID,
	.name="MSCARead",
	.text=N_("Sound capture filter for MacOS X Core Audio drivers"),
	.category=MS_FILTER_OTHER,
	.ninputs=0,
	.noutputs=1,
	.preprocess=ca_read_preprocess,
	.process=ca_read_process,
	.postprocess=ca_read_postprocess,
	.uninit=ca_read_uninit,
	.methods=ca_methods
};

MSFilterDesc ca_write_desc={
	.id=MS_CA_WRITE_ID,
	.name="MSCAWrite",
	.text=N_("Sound playback filter for MacOS X Core Audio drivers"),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.preprocess=ca_write_preprocess,
	.process=ca_write_process,
	.postprocess=ca_write_postprocess,
	.uninit=ca_write_uninit,
	.methods=ca_methods
};

MSFilter *ms_ca_read_new(MSSndCard *card){
	MSFilter *f = ms_filter_new_from_desc(&ca_read_desc);
	//CaSndDsCard *wc = (CaSndDsCard *) card->data;
	//CAData *d = (CAData *) f->data;
	//d->dev = wc->dev;
	f->data = card;
	return f;
}


MSFilter *ms_ca_write_new(MSSndCard *card){
	MSFilter *f=ms_filter_new_from_desc(&ca_write_desc);
	//CaSndDsCard *wc = (CaSndDsCard *) card->data;
	//CAData *d = (CAData *) f->data;
	//d->dev = wc->dev;
	f->data = card;
	return f;
}

MS_FILTER_DESC_EXPORT(ca_read_desc)
MS_FILTER_DESC_EXPORT(ca_write_desc)
