//  Created by Aymeric MOIZARD on 06/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import "Call.h"
#import "MultiCastDelegate.h"

#import "AppEngine.h"

#include <amsip/am_options.h>

static struct am_bandwidth_stats band_stats;

@implementation Call

@synthesize tid;
@synthesize cid;
@synthesize did;
@synthesize referedby_did;
@synthesize from;
@synthesize to;
@synthesize oppositeNumber;
@synthesize isMuted;
@synthesize isOnhold;
@synthesize callState;
@synthesize isIncomingCall;
@synthesize hasMedia;

@synthesize callDelegates;

- (id)init;
{
	memset(&band_stats, 0, sizeof(band_stats));
	
	callDelegates = [[MulticastDelegate alloc] init];
	
	tid = 0;
	cid = 0;
	did = 0;
	referedby_did = 0;
	from = nil;
	to = nil;
	oppositeNumber = nil;
	isMuted = false;
	isOnhold = false;
	callState = NOTSTARTED;
	isIncomingCall = false;
	hasMedia = false;

	return self;
}

- (int) hangup
{
	return [gAppEngine amsip_stop:486 forCall:self];
}

- (int) accept:(int)code
{
	return [gAppEngine amsip_answer:code forCall:self];
}

- (int) decline
{
	return [gAppEngine amsip_answer:603 forCall:self];
}

- (int) hold
{
	NSString *file = [[NSBundle mainBundle] pathForResource:@"holdmusic" ofType:@"wav"];
	if (file==nil)
		file = @"holdmusic.wav"; //cannot happen
	
	int i = am_session_hold(did, [file cStringUsingEncoding:NSUTF8StringEncoding]);

	if (i==0)
	{		
		isOnhold=true;
		isMuted=false;
		[self onCallUpdate];
	}
	
	return i;
}

- (int) unhold
{
	int i = am_session_off_hold([self did]);
	if (i==0)
	{		
		isOnhold=false;
		isMuted=false;
		[self onCallUpdate];
	}
	return i;
}

- (int) record:(NSString*)file
{
	int i = am_session_record(did, [file cStringUsingEncoding:NSUTF8StringEncoding]);
	if (i==0)
	{
	}
	return i;
}

- (int) record_stop
{
	int i = am_session_stop_record(did);
	if (i==0)
	{
	}
	return i;
}


- (int) mute
{
	int i = am_session_mute(did);
	if (i==0)
	{
		isMuted=true;
		[self onCallUpdate];
	}
	return i;
}

- (int) unmute
{
	int i = am_session_unmute(did);
	if (i==0)
	{		
		isMuted=false;
		[self onCallUpdate];
	}
	return i;
}

- (int) addvideo
{
	int i = am_session_add_video(did);
	if (i==0)
	{		
	}
	return i;
}

- (float) getUploadBandwidth
{
	int i;
	if (did<=0)
		return 0;
	memset(&band_stats, 0, sizeof(band_stats));
	i = am_session_get_audio_bandwidth(did, &band_stats);
	if (i>=0)
	{
		return band_stats.upload_rate;
	}
	return 0;
}

- (float) getDownloadBandwidth
{
	return band_stats.download_rate;
}

- (float) getPacketLossIn
{
	return -1;
}

- (float) getPacketLossOut
{
	return -1;
}

- (int) sendDtmf:(char)dtmf_number
{
	NSString *_dtmf = [[NSUserDefaults standardUserDefaults] stringForKey:@"dtmf_preference"];
	if (_dtmf==nil)
		return am_session_send_rtp_dtmf([self did], dtmf_number);
	else if ([_dtmf isEqualToString:@"telephone-event"]==YES)
		return am_session_send_rtp_dtmf([self did], dtmf_number);
	else if ([_dtmf isEqualToString:@"audio"]==YES)
		return am_session_send_inband_dtmf([self did], dtmf_number);
	else if ([_dtmf isEqualToString:@"sip-info"]==YES)
		return am_session_send_dtmf_with_duration([self did], dtmf_number, 250);
	return -1;
}

- (int) transfer:(NSString*)numberOrsipId
{
	osip_to_t *target_to;
	int i;

	NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	
	i = osip_to_init(&target_to);
	if (i!=0)
	{
		//most probably allocation issue
		return i;
	}
		
	i = osip_to_parse(target_to, [numberOrsipId cStringUsingEncoding:NSUTF8StringEncoding]);
	osip_to_free(target_to);

	if (i==0) {
		i = am_session_refer(did, [numberOrsipId cStringUsingEncoding:NSUTF8StringEncoding], [_identity cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	else {
		char target[256];
		snprintf(target, sizeof(target), "sip:%s@%s", [numberOrsipId cStringUsingEncoding:NSUTF8StringEncoding],
				 [_proxy cStringUsingEncoding:NSUTF8StringEncoding]);
		i = am_session_refer(did, target, [_identity cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	return i;
}

- (int) connect:(Call*)pCall
{
	int i;
	char buf[1024];
	char refer_to[1024];
	
	NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	
	i = am_session_get_referto(did, buf, sizeof(buf));
	if (i!=0)
	{
		return -1;
	}
	snprintf(refer_to, sizeof(refer_to), "<%s>", buf);	
	
	i = am_session_refer([pCall did], refer_to, [_identity cStringUsingEncoding:NSUTF8StringEncoding]);
	return i;
}


- (void) addCallStateChangeListener:(id)_adelegate
{
	[callDelegates addDelegate:_adelegate];
}

- (void) removeCallStateChangeListener:(id)_adelegate
{
	[callDelegates removeDelegate:_adelegate];
}

- (void) onCallUpdate
{
	[callDelegates onCallUpdate:self];
}

@end
