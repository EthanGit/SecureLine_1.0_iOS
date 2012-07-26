//
//  AppEngine.h
//  AppEngine
//
//  Created by Aymeric MOIZARD on 30/10/09.
//  Copyright 2009 antisip. All rights reserved.
//

#import "Call.h"
#import "Registration.h"

#include "MultiCastDelegate.h"

#import "AppEngine.h"

#include <amsip/am_options.h>

AppEngine *gAppEngine=nil;
extern enum PhoneDnsCapability phoneDnsCapability;

@implementation AppEngine

@synthesize userAgent;
@synthesize engineDelegates;
@synthesize registrationDelegates;


- (id) init {
	
	engineDelegates = [[MulticastDelegate alloc] init];
	registrationDelegates = [[MulticastDelegate alloc] init];
	
	
	iAudioPlayerApp = [[iAudioPlayer alloc] init]; ;
	
	call_list = [[NSMutableDictionary alloc] init];
	
	//rid = 0;
	//registration_code=0;
	registration=nil;
	thread_start=false;
	thread_started=false;
	[self setUserAgent:@"vbyantisip/vX.Y.Z"];
	am_init("vbyantisip/vX.Y.Z", 5);
	
	NSString *bpath = [[NSBundle mainBundle] bundlePath];
	char bpath_utf8[256];
	snprintf(bpath_utf8, sizeof(bpath_utf8), "%s/Contents/PlugIns", [bpath cStringUsingEncoding:NSUTF8StringEncoding]);
	am_option_load_plugins(bpath_utf8);
	
	gAppEngine=self;
	return self;
}

-(void)initialize{
}

-(void)stop{
	
    [self enableVideo:0 Width:MS_VIDEO_SIZE_CIF_W Height:MS_VIDEO_SIZE_CIF_H];
	if (thread_started==false)
		return; //already stopped
	thread_start=false;
	
	while (thread_started!=false)
		osip_usleep(200);
	
	NSArray *keys = [call_list allKeys];

	// values in foreach loop
	for (NSString *key in keys) {
		Call *pCall = [call_list objectForKey:key];
		[engineDelegates onCallRemove:pCall];
		[call_list removeObjectForKey:key];
		[pCall release];
	}

	if (registration!=nil)
	{
		[engineDelegates onRegistrationRemove:registration];
		[registration release];
		registration=nil;
	}
	//rid = 0;
	//registration_code=0;
}

-(BOOL) isConfigured
{
	NSString *proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
	NSString *transport = [[NSUserDefaults standardUserDefaults] stringForKey:@"transport_preference"];
	
	if (transport == nil || [transport caseInsensitiveCompare:@"auto"]==0)
	{
		[[NSUserDefaults standardUserDefaults] setObject:@"TCP" forKey:@"transport_preference"];
	} else if ([transport caseInsensitiveCompare:@"UDP"]==0
			   ||[transport caseInsensitiveCompare:@"TCP"]==0
			   ||[transport caseInsensitiveCompare:@"TLS"]==0){
		//accepted
	} else {
		[[NSUserDefaults standardUserDefaults] setObject:@"TCP" forKey:@"transport_preference"];
	}
	
	NSString *identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	
	if (identity==nil || [identity length]==0)
	{
		if (proxy!=nil && login!=nil)
		{
			NSString *tmp = [NSString stringWithFormat:@"<sip:%@@%@>", login, proxy];
			[[NSUserDefaults standardUserDefaults] setObject:tmp forKey:@"identity_preference"];
		}
	}
	
	NSInteger val = [[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"];
	if (val<=0 || val>3600*24)
	{
		val = 900;
		[[NSUserDefaults standardUserDefaults] setInteger:val forKey:@"reginterval_preference"];
	}
	
	if (proxy==nil || login==nil || [proxy length]==0 || [login length]==0)
	{		
		return NO;
	}
	return YES;
}

-(void)start{

	//[gAccount loadSettings];
	if ([self isConfigured] == true)
	{
		NSArray *keys = [call_list allKeys];

		// values in foreach loop
		for (NSString *key in keys) {
			Call *pCall = [call_list objectForKey:key];
			[engineDelegates onCallRemove:pCall];
			[call_list removeObjectForKey:key];
			[pCall release];
		}

		//rid = 0;
		//registration_code=0;
		
		if (thread_started==true)
			return; //already started
		thread_start=true;
        
		[NSThread detachNewThreadSelector:@selector(amsipThread) toTarget:self withObject:nil];

        int k=0;
		while (thread_started!=true && k<20000) {
            k++;
			osip_usleep(200);
        }
	}
}

- (void) setVideoCallback:(on_video_module_new_image_cb) func
{
    am_option_set_image_callback(func);
}

- (void) enableVideo:(int)enable Width:(int)width Height:(int)height
{
    am_option_set_window_handle(0, width, height);
    am_option_enable_preview(enable);
}

- (void) addCallDelegate:(id)_adelegate
{
	NSArray *keys = [call_list allKeys];
	for (NSString *key in keys) {
		Call *pCall = [call_list objectForKey:key];
		[_adelegate onCallExist:pCall];
	}
	[engineDelegates addDelegate:_adelegate];
}

- (void) removeCallDelegate:(id)_adelegate
{
    [engineDelegates removeDelegate:_adelegate];
}

- (void) addRegistrationDelegate:(id)_adelegate
{
	[registrationDelegates addDelegate:_adelegate];
}

- (void) removeRegistrationDelegate:(id)_adelegate
{
    [registrationDelegates removeDelegate:_adelegate];
}

- (int) getNumberOfActiveCalls{
	int count=0;
	NSArray *keys = [call_list allKeys];

	// values in foreach loop
	for (NSString *key in keys) {
		Call *pCall = [call_list objectForKey:key];
		if ([pCall callState]<=CONNECTED)
			count++;
	}
	return count;
}

-(int)amsip_start:(NSString*)target_user withReferedby_did:(int)referedby_did{
	char target[1024];
	char sipproxy[256];
	char outboundproxy[256];
	
	if ([self getNumberOfActiveCalls]>3)
		return -1;
	
	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	NSString *_outboundproxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"outboundproxy_preference"];
	
	int mNumberOfActiveCall = (int)[call_list count];
	NSLog(@"Number of active calls: %i", mNumberOfActiveCall);
	
	if ([self isConfigured] == false || target_user==nil)
	{
		return -1;
	}
	if ([target_user hasPrefix:@"sip:"]
		|| [target_user hasPrefix:@"sips:"]
		|| [target_user hasPrefix:@"tel:"]
		|| [[target_user componentsSeparatedByString:@"<sip:"] count]>1
		|| [[target_user componentsSeparatedByString:@"<sips:"] count]>1
		|| [[target_user componentsSeparatedByString:@"<tel:"] count]>1)
	{
		snprintf(target, sizeof(target), "%s", [target_user cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	else
	{
		snprintf(target, sizeof(target), "sip:%s@%s", [target_user cStringUsingEncoding:NSUTF8StringEncoding],
				 [_proxy cStringUsingEncoding:NSUTF8StringEncoding]);
	}

	snprintf(sipproxy, sizeof(sipproxy), "sip:%s", [_proxy cStringUsingEncoding:NSUTF8StringEncoding]);
	
	memset(outboundproxy, 0, sizeof(outboundproxy));
	if (_outboundproxy!=nil && [_outboundproxy length]>0)
	{
		snprintf(outboundproxy, sizeof(outboundproxy), "<sip:%s;lr>", [_outboundproxy cStringUsingEncoding:NSUTF8StringEncoding]);
	}
	
	
	int cid = am_session_start_with_video([_identity cStringUsingEncoding:NSUTF8StringEncoding], target, sipproxy, outboundproxy);

	if (cid>0)
	{
		NSString *str = [NSString stringWithFormat:@"%s", target];
		Call *pCall = [[Call alloc] init];
		[pCall setCid:cid];
		[pCall setDid:0];
		[pCall setTid:0];
		[pCall setReferedby_did:referedby_did];

		[pCall setIsIncomingCall:false];
		[pCall setTo:str];
		[pCall setFrom:_identity];
		[pCall setOppositeNumber:str];
	
		[pCall setCallState:NOTSTARTED];

		[call_list setObject:pCall forKey:[NSNumber numberWithInteger:cid]];
	
		[engineDelegates performSelectorOnMainThread : @ selector(onCallNew:) withObject:pCall waitUntilDone:NO];
		[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
	}
	return cid;
}

-(int)amsip_answer:(int)code forCall:(Call*)pCall{
	int i;

	if (pCall==nil)
		return -1;
	
	if (code<200){
		if (code==180)
		{
			if ([self getNumberOfActiveCalls]>1) //this one + other?
			{
				NSString *file = [[NSBundle mainBundle] pathForResource:@"ringtone" ofType:@"m4a"];
				if (file!=nil)
					[iAudioPlayerApp performSelectorOnMainThread : @ selector(playOnce:) withObject:file waitUntilDone:NO];
			}
			else {
				NSString *file = [[NSBundle mainBundle] pathForResource:@"ringtone" ofType:@"m4a"];
				if (file!=nil)
					[iAudioPlayerApp performSelectorOnMainThread : @ selector(play:) withObject:file waitUntilDone:NO];
			}

		}
		i = am_session_answer([pCall tid], [pCall did], code, 0);
		return i;
	}
	else if (code>299){
		[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
		i = am_session_answer([pCall tid], [pCall did], code, 0);
	}
	else
	{
		[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
		i = am_session_answer([pCall tid], [pCall did], code, 1);

		//TODO: check this???
		//[engineDelegates onCallNew:pCall];

	}
	if (i==0)
	{
		if (code>=300)
			[pCall setCallState:DISCONNECTED];
		else {
			[pCall setCallState:CONNECTED];
		}
		
		[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
	}
	
	return i;
}

-(int)amsip_stop:(int)code forCall:(Call*)pCall{
	int i;
	if (pCall==nil)
		return -1;
	
	i = am_session_answer([pCall tid], [pCall did], code, 0);
	if (i!=0)
		i = am_session_stop([pCall cid], [pCall did], code);
	
	[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];

	[pCall setCallState:DISCONNECTED];
	[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
	
	return i;
}

-(void)refreshRegistration{

	if (registration!=nil && [registration rid]>0)
	{
		NSInteger val = [[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"];
		am_register_refresh([registration rid], (int)val);
	}
	return;
}

-(void)amsipLoop {
	
	eXosip_event_t evt;
	int k;
	int i;
	
	for (;;)
	{
		memset (&evt, 0, sizeof (eXosip_event_t));
		
		k = am_event_get (&evt);
		if (k == AMSIP_TIMEOUT)
			break;
		
		if (k<0 && k!=OSIP_TIMEOUT)
		{
			const char *myerr = osip_strerror(k);
			am_trace(__FILE__, __LINE__, OSIP_ERROR, "status: %s", myerr);
		}
		if (evt.rid>0)
		{
			/* unregistration DONE */
			if (evt.type == EXOSIP_REGISTRATION_SUCCESS){
				if (thread_start==false)
				{
					if (registration!=nil)
					{
						[registration setRid:0];
						[registration setCode:evt.response->status_code];
						[registrationDelegates performSelectorOnMainThread : @ selector(onRegistrationUpdate:) withObject:registration waitUntilDone:NO];							
					}
					am_event_release (&evt);
					return;
				}
				if (registration!=nil)
				{
					[registration setRid:evt.rid];
					[registration setCode:evt.response->status_code];
					[registrationDelegates performSelectorOnMainThread : @ selector(onRegistrationUpdate:) withObject:registration waitUntilDone:NO];							
				}
			}
			else
			{
				if (registration!=nil)
				{
					[registration setRid:evt.rid];
					if (evt.response==NULL)
						[registration setCode:0];
					else
						[registration setCode:evt.response->status_code];
					[registrationDelegates performSelectorOnMainThread : @ selector(onRegistrationUpdate:) withObject:registration waitUntilDone:NO];							
				}
			}
            am_event_release (&evt);
			continue;
		}
		
		if (evt.cid>0)
		{
			Call *pCall = [call_list objectForKey:[NSNumber numberWithInteger:evt.cid]];
			int mNumberOfActiveCall = (int)[call_list count];
			NSLog(@"Number of active calls: %i", mNumberOfActiveCall);

			if (pCall==nil)
			{
				if (evt.type != EXOSIP_CALL_INVITE)
				{
					am_session_stop(evt.cid, evt.did, 486);
					am_event_release (&evt);
					continue;
				}
			}
			if (pCall==nil)
			{
				if (evt.type == EXOSIP_CALL_INVITE)	{
					osip_header_t *replace_hdr=NULL;
					am_messageinfo_t minfo_req;
					memset (&minfo_req, 0, sizeof (am_messageinfo_t));
					am_message_get_messageinfo (evt.request, &minfo_req);
					
					//am_session_answer(evt.tid, evt.did, 180, 0);
					
					//Need to check for Replaces
					i = osip_message_header_get_byname(evt.request, "replaces",
													   0, &replace_hdr);
					if (i>=0 && replace_hdr!=NULL && replace_hdr->hvalue!=NULL)
					{
						/* there is a Replaces header to replace an existing call */
						i = am_session_find_by_replaces (evt.request);
						if (i==-3)
						{
							/* already confirmed */
							am_session_answer (evt.tid, evt.did, 486, 0);
						}
						else if (i==-2)
						{
							/* I'm the callee?? */
							am_session_answer (evt.tid, evt.did, 481, 0);
						}
						else if (i<=0)
						{
							/* bad syntax? */
							am_session_answer (evt.tid, evt.did, 481, 0);
						}
						else
						{
							/* call found! */
							Call *pCallOld = [call_list objectForKey:[NSNumber numberWithInteger:i]];
							if (pCallOld==nil)
							{
								am_session_answer (evt.tid, evt.did, 500, 0);
								am_event_release (&evt);
								continue;
							}
							
							[pCallOld hangup];
							
							pCall = [[Call alloc] init];
							
							[pCall setCid:evt.cid];
							[pCall setDid:evt.did];
							[pCall setTid:evt.tid];
							
							[pCall setIsIncomingCall:true];
							if (minfo_req.from[0]!='\0')
							{
								NSString *str = [NSString stringWithFormat:@"%s", minfo_req.from];
								[pCall setFrom:str];
								[pCall setOppositeNumber:str];
							}
							else {
								[pCall setFrom:@"anonymous"];
								[pCall setOppositeNumber:@"anonymous"];
							}
							
							NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
							[pCall setTo:_identity];
							[pCall setCallState:RINGING];
							
							[call_list setObject:pCall forKey:[NSNumber numberWithInteger:evt.cid]];
							
							[engineDelegates performSelectorOnMainThread : @ selector(onCallNew:) withObject:pCall waitUntilDone:NO];							
							[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
							
							[pCall accept:200];
						}
						am_event_release (&evt);
						continue;
					}
					
					if ([self getNumberOfActiveCalls]>=4)
					{					
						am_session_answer_request(evt.tid, evt.did, 486); //busy case
						am_session_stop(evt.cid, evt.did, 486);
						am_event_release (&evt);
						continue;
					}
					
					pCall = [[Call alloc] init];
					
					[pCall setCid:evt.cid];
					[pCall setDid:evt.did];
					[pCall setTid:evt.tid];
					
					[pCall setIsIncomingCall:true];
					if (minfo_req.from[0]!='\0')
					{
						NSString *str = [NSString stringWithFormat:@"%s", minfo_req.from];
						[pCall setFrom:str];
						[pCall setOppositeNumber:str];
					}
					else {
						[pCall setFrom:@"anonymous"];
						[pCall setOppositeNumber:@"anonymous"];
					}
					
					NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
					[pCall setTo:_identity];
					[pCall setCallState:RINGING];

					[call_list setObject:pCall forKey:[NSNumber numberWithInteger:evt.cid]];
					
					[engineDelegates performSelectorOnMainThread : @ selector(onCallNew:) withObject:pCall waitUntilDone:NO];
					[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];					
				}
				else if (evt.type == EXOSIP_CALL_RELEASED) {
				}
				else if (evt.type == EXOSIP_CALL_CLOSED) {
				}
				else if (evt.type == EXOSIP_CALL_MESSAGE_NEW) {
					am_session_answer_request(evt.tid, evt.did, 500);
					am_session_stop(evt.cid, evt.did, 486);
				}
				else {
					am_session_stop(evt.cid, evt.did, 486);
				}
				am_event_release (&evt);
				continue;
			}
			
			if (pCall!=nil && [pCall cid]==evt.cid)
			{
				[pCall setDid:evt.did];
			}
			
			if (evt.type == EXOSIP_CALL_RELEASED)
			{
				if (pCall!=nil)
				{
					[engineDelegates performSelectorOnMainThread : @ selector(onCallRemove:) withObject:pCall waitUntilDone:NO];							
					[call_list removeObjectForKey:[NSNumber numberWithInteger:evt.cid]];
					[pCall release];
					pCall=nil;
				}
			}
			
			if (evt.type == EXOSIP_CALL_CLOSED)
			{
				[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
				
				if (pCall!=nil)
				{
					[pCall setCallState:DISCONNECTED];
					[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
				}
			}
			
			if (pCall!=nil && [pCall callState]<CONNECTED)
			{
				if (evt.type == EXOSIP_CALL_PROCEEDING
					||evt.type == EXOSIP_CALL_RINGING
					||evt.type == EXOSIP_CALL_ANSWERED
					||evt.type == EXOSIP_CALL_REDIRECTED
					||evt.type == EXOSIP_CALL_REQUESTFAILURE
					||evt.type == EXOSIP_CALL_SERVERFAILURE
					||evt.type == EXOSIP_CALL_GLOBALFAILURE)
				{
					char body[2048];
					char sub_state[1024];
				
					if ([pCall referedby_did]>0)
					{
						
						if (evt.type==EXOSIP_CALL_NOANSWER) {
							snprintf(sub_state, sizeof(sub_state), "%s", "terminated;reason=noresource");
							snprintf(body, sizeof(body), "%s", "SIP/2.0 408 Timeout");
						} else if (evt.type==EXOSIP_CALL_ANSWERED)	{
							snprintf(sub_state, sizeof(sub_state), "%s", "terminated;reason=noresource");
							if (evt.response!=NULL
								&& evt.response->reason_phrase!=NULL)
								snprintf(body, sizeof(body), "SIP/2.0 %i %s",
										 evt.response->status_code,
										 evt.response->reason_phrase);
							else
								snprintf(body, sizeof(body), "%s", "SIP/2.0 200 Ok");
						}else if (evt.type==EXOSIP_CALL_RINGING
								  ||evt.type==EXOSIP_CALL_PROCEEDING)
						{
							snprintf(sub_state, sizeof(sub_state), "%s", "active;expires=120");
							if (evt.response!=NULL
								&& evt.response->reason_phrase!=NULL)
								snprintf(body, sizeof(body), "SIP/2.0 %i %s",
										 evt.response->status_code,
										 evt.response->reason_phrase);
							else
								snprintf(body, sizeof(body), "%s", "SIP/2.0 400 Bad request");
						}
						else if (evt.type==EXOSIP_CALL_REDIRECTED)
						{
							snprintf(sub_state, sizeof(sub_state), "%s", "active;expires=120");
							if (evt.response!=NULL
								&& evt.response->reason_phrase!=NULL)
								snprintf(body, sizeof(body), "SIP/2.0 %i %s",
										 evt.response->status_code,
										 evt.response->reason_phrase);
							else
								snprintf(body, sizeof(body), "%s", "SIP/2.0 400 Bad request");
						}
						else if (evt.type==EXOSIP_CALL_REQUESTFAILURE
								 && evt.response!=NULL &&
								 (evt.response->status_code==407 ||evt.response->status_code==401 || evt.response->status_code==422))
						{
							snprintf(sub_state, sizeof(sub_state), "%s", "active;expires=120");
							if (evt.response!=NULL
								&& evt.response->reason_phrase!=NULL)
								snprintf(body, sizeof(body), "SIP/2.0 %i %s",
										 evt.response->status_code,
										 evt.response->reason_phrase);
							else
								snprintf(body, sizeof(body), "%s", "SIP/2.0 400 Bad request");
						}
						else if (evt.type==EXOSIP_CALL_REQUESTFAILURE
								 ||evt.type==EXOSIP_CALL_SERVERFAILURE
								 ||evt.type==EXOSIP_CALL_GLOBALFAILURE)
						{
							snprintf(sub_state, sizeof(sub_state), "%s", "terminated;reason=noresource");
							if (evt.response!=NULL
								&& evt.response->reason_phrase!=NULL)
								snprintf(body, sizeof(body), "SIP/2.0 %i %s",
										 evt.response->status_code,
										 evt.response->reason_phrase);
							else
								snprintf(body, sizeof(body), "%s", "SIP/2.0 400 Bad request");
						}						
						
						if ([pCall referedby_did]>0)
						{
							//answer for an INVITE with Replaces...
							am_session_send_notify([pCall referedby_did], sub_state,
												   NULL, body,
												   (int)strlen(body));
							if (evt.type==EXOSIP_CALL_ANSWERED)
								[pCall setReferedby_did:0];
						}
					}
				}
	
				if (evt.type == EXOSIP_CALL_PROCEEDING)
				{
					if (pCall!=nil)
					{
						[pCall setCallState:TRYING];
						[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
					}
				}
				if (evt.type == EXOSIP_CALL_RINGING)
				{
					i = am_message_get_audio_rtpdirection (evt.response);
					if (i<0)
					{
						if ([pCall hasMedia]==false)
						{
							NSString *file = [[NSBundle mainBundle] pathForResource:@"ringback" ofType:@"m4a"];
							if (file!=nil)
								[iAudioPlayerApp performSelectorOnMainThread : @ selector(play:) withObject:file waitUntilDone:NO];
						}
					}
					else{
						[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
						[pCall setHasMedia:true];
					}
						
						
					if (pCall!=nil)
					{
						[pCall setCallState:RINGING];
						[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
					}
				}
				if (evt.type == EXOSIP_CALL_ANSWERED)
				{
					[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
					if (pCall!=nil)
					{
						[pCall setHasMedia:true];
						[pCall setCallState:CONNECTED];
						[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
					}
				}
				if (evt.type == EXOSIP_CALL_REDIRECTED)
				{
					[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
					
					if (pCall!=nil)
					{
						//FIXME: if redirection doesn't work automatically -> call failed
						[pCall setCallState:TRYING];
						[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
					}
				}
				if (evt.type == EXOSIP_CALL_REQUESTFAILURE
					|| evt.type == EXOSIP_CALL_SERVERFAILURE
					|| evt.type == EXOSIP_CALL_GLOBALFAILURE)
				{
					if (evt.response!=NULL &&
						(evt.response->status_code==401
						 ||evt.response->status_code==407
						 ||evt.response->status_code==422))
					{
						//Checking credentials or timers
					}
					else
					{
						[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
						
						if (pCall!=nil)
						{
							[pCall setCallState:DISCONNECTED];
							[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
						}
					}
				}
			}
			else
			{
				if (evt.type == EXOSIP_CALL_ANSWERED)
				{
				}
				if (evt.type == EXOSIP_CALL_REDIRECTED)
				{
					//should not happen
					
					if (pCall!=nil)
					{
						[self amsip_stop:500 forCall:pCall];
						[pCall setCallState:DISCONNECTED];
						[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
					}
					else {
						i = am_session_answer(evt.tid, evt.did, 500, 0);
						if (i!=0)
							i = am_session_stop(evt.cid, evt.did, 500);
					}

				}
				if (evt.type == EXOSIP_CALL_REQUESTFAILURE
					|| evt.type == EXOSIP_CALL_SERVERFAILURE
					|| evt.type == EXOSIP_CALL_GLOBALFAILURE)
				{
					//something bad happened
				}
			}
			
			
			if (evt.type == EXOSIP_CALL_MESSAGE_NEW)
			{
				if (evt.request == NULL)
				{
					am_event_release (&evt);
					continue;
				}
				
				if ((MSG_IS_REQUEST(evt.request)
					 && 0==strcmp(evt.request->sip_method,"BYE")))
				{
					[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
					
					if (pCall!=nil)
					{
						[pCall setCallState:DISCONNECTED];
						[pCall performSelectorOnMainThread : @ selector(onCallUpdate) withObject:nil waitUntilDone:NO];
					}
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"MESSAGE")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"PRACK")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"UPDATE")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"INFO")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"PING")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"REFER")))
				{
					osip_header_t *refer_to=NULL;
					int i;
					i = osip_message_header_get_byname(evt.request, "refer-to",
													   0, &refer_to);
					if (i<0 || refer_to==NULL || refer_to->hvalue==NULL)
					{
						am_session_answer_request(evt.tid, evt.did, 400);
						am_event_release (&evt);
						return;
					}
					
					NSString *str = [NSString stringWithFormat:@"%s", refer_to->hvalue];
					if (pCall==nil)
					{
						am_session_answer_request(evt.tid, evt.did, 491);
						am_event_release (&evt);
						continue;
					}

					int new_cid = [self amsip_start:str withReferedby_did:evt.did];
					if (new_cid>0)
					{
						am_session_answer_request(evt.tid, evt.did, 202);
						
						//TODO: we should delay this until we receive a proper confirmation
						//TODO: we also need to send NOTIFYs
						//[self amsip_stop:486 forCall:pCall];
					}
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"NOTIFY")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);

					if (pCall!=nil)
					{
						osip_content_type_t *ctt=NULL;
						osip_body_t *oldbody=NULL;
						int pos;
						
						/* get content-type info */
						ctt = osip_message_get_content_type(evt.request);
						if (ctt != NULL && ctt->type != NULL && ctt->subtype != NULL)
						{
							if (osip_strcasecmp(ctt->type, "multipart") == 0) {
								/* probably within the multipart attachement */
							} else if (osip_strcasecmp(ctt->subtype, "sipfrag") == 0) {
								oldbody = (osip_body_t *) osip_list_get(&evt.request->bodies, 0);
							}
							if (oldbody==NULL)
							{
								pos = 0;
								while (!osip_list_eol(&evt.request->bodies, pos)) {
									oldbody = (osip_body_t *) osip_list_get(&evt.request->bodies, pos);
									pos++;
									if (oldbody->content_type != NULL
										&& oldbody->content_type->type != NULL
										&& oldbody->content_type->subtype != NULL
										&& osip_strcasecmp(oldbody->content_type->subtype,
														   "sipfrag") == 0)
										break;
									else {
										oldbody=NULL;
									}
									
									//return osip_strdup(oldbody->body);
								}
							}
							if (oldbody!=NULL && oldbody->body!=NULL && strlen(oldbody->body)>12)
							{
								int code=atoi(oldbody->body+8); /* after "SIP/2.0 " */
								NSLog(@"found a message/sipfrag (%i)", code);
								if (code>=200 && code<300)
									[self amsip_stop:486 forCall:pCall];
							}
						}
					}
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"OPTIONS")))
				{
					am_session_answer_request(evt.tid, evt.did, 200);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"REGISTER")))
				{
					am_session_answer_request(evt.tid, evt.did, 405);
				}
				else if ((MSG_IS_REQUEST(evt.request)
						  && 0==strcmp(evt.request->sip_method,"FETCH")))
				{
					am_session_answer_request(evt.tid, evt.did, 405);
				}
				else
				{
					am_session_answer_request(evt.tid, evt.did, 405);
				}
				am_event_release (&evt);
				continue;
			}
			
			
            am_event_release (&evt);
			continue;
		}
		
		if (evt.nid>0)
		{
			//emansip_nid_processevent(&evt);
            am_event_release (&evt);
			continue;
		}
		
		if (evt.sid>0)
		{
			//emansip_sid_processevent(&evt);
            am_event_release (&evt);
			continue;
		}
		
		/* all other events */
		//emansip_other_processevent(&evt);
        am_event_release (&evt);
	}
	
}

-(void)amsipThread {
	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	am_codec_info_t codec;
	char identity[1024];
	char sipproxy[256];
	int i;
	
	if ([self isConfigured] == false)
	{	
		[pool release];
		thread_started=false;	
		return;
	}

	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
	NSString *_password = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
	NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	NSString *_transport = [[NSUserDefaults standardUserDefaults] stringForKey:@"transport_preference"];
	NSString *_outboundproxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"outboundproxy_preference"];
	NSString *_stun = [[NSUserDefaults standardUserDefaults] stringForKey:@"stun_preference"];
		
	am_reset([userAgent cStringUsingEncoding:NSUTF8StringEncoding], 5);

#ifdef ENABLE_VIDEO
	{
		am_video_codec_attr_t codec_attr;
        codec_attr.ptime = 0;
        codec_attr.maxptime  = 0;
        codec_attr.upload_bandwidth = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"uploadbandwidth_preference"];
        codec_attr.download_bandwidth = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"downloadbandwidth_preference"];
        am_video_codec_attr_modify(&codec_attr);		
        
        if (codec_attr.upload_bandwidth<=64)
            am_option_set_input_video_size(MS_VIDEO_SIZE_QCIF_W, MS_VIDEO_SIZE_QCIF_H);
        else
            am_option_set_input_video_size(MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H);
        if (codec_attr.download_bandwidth<=64)
            am_option_set_window_handle(0, MS_VIDEO_SIZE_QCIF_W, MS_VIDEO_SIZE_QCIF_H);
        else
            am_option_set_window_handle(0, MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H);
    }

	NSString *bpath = [[NSBundle mainBundle] bundlePath];
	char bpath_utf8[256];
	snprintf(bpath_utf8, sizeof(bpath_utf8), "%s/Contents/Resources/nowebcamCIF.jpg", [bpath cStringUsingEncoding:NSUTF8StringEncoding]);
	am_option_set_nowebcam(bpath_utf8);
	
  NSString *cameradevice_preference = [[NSUserDefaults standardUserDefaults] stringForKey:@"cameradevice_preference"];

  struct am_camera camera;
  memset(&camera, 0, sizeof(struct am_camera));
	camera.card=0;
  i=-1;
	for (;;)
	{
		i = am_option_find_camera(&camera);
    if (cameradevice_preference==nil)
      break;
		if (i < 0)
		{
			break;
		}
		NSString *tmp = [[NSString alloc]initWithFormat:@"%s",camera.name];
    if ([tmp isEqualToString:cameradevice_preference])
    {
      [tmp release];
      break;      
    }
    [tmp release];
		camera.card++;
	}	
  if (i<0)
    am_option_select_camera(camera.card);
  else
    am_option_select_camera(camera.card);
  
	//uncomment to test STATIC IMAGE:
	//am_option_select_camera(1);
#endif
    
	NSLog(@"amsip reset done");
	thread_started=true;
    
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"checkcertificate_preference"])
    i=1;
	else
		i=0;
    am_option_set_option(AMSIP_OPTION_TLS_CHECK_CERTIFICATE, &i);
    
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"naptr_preference"])
		am_option_set_dns_capabilities(2);
	else
		am_option_set_dns_capabilities(0);
	
	am_option_set_rate(16000);
	
	am_option_enable_optionnal_encryption(0);
	am_option_enable_rport(1);
	am_option_enable_echo_canceller(0, 0, 0);
	am_option_enable_101(0);
	am_option_set_audio_profile("RTP/AVP");
	am_option_set_video_profile("RTP/AVP");
	am_option_set_text_profile("RTP/AVP");
	
	am_option_enable_keepalive(17*1000);

	am_option_set_initial_audio_port(12600);
	
	am_option_enable_rport(1);

	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"aec_preference"])
	{
		NSLog(@"app-engine: echoLimiterEnabled==true");
		am_option_enable_echo_canceller(1, 128, 128*12);
	}
	else {
		NSLog(@"app-engine: echoLimiterEnabled==false");
		am_option_enable_echo_canceller(0, 128, 128*12);
	}
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"aec_preference"]==NO)
	{
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"elimiter_preference"])
		{
			NSLog(@"app-engine: echoLimiterEnabled==true");
			am_option_set_echo_limitation(1, 0.02, 0.02, 8.0, 300);
		}
		else {
			NSLog(@"app-engine: echoLimiterEnabled==false");
			am_option_set_echo_limitation(0, 0.02, 0.02, 8.0, 300);
		}
	}
	
	am_option_enable_optionnal_encryption(0);
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"srtp_preference"]) {
		NSLog(@"app-engine: srtpOptionalEnabled==true");
		am_option_enable_optionnal_encryption(1);
	}
	//else {
	//	NSLog(@"app-engine: srtpOnlyEnabled==true");
	//	am_option_set_audio_profile("RTP/SAVP");
	//	am_option_set_text_profile("RTP/SAVP");
	//}

	struct am_codec_attr attr;
	memset(&attr, 0, sizeof(struct am_codec_attr));
	

	 /* ptime -> 0 for default */
	attr.ptime = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"ptime_preference"];
	attr.maxptime = 0; /* UNUSED */
	attr.bandwidth = 0; /* UNUSED */
	
	am_codec_attr_modify(&attr);
	
	i=0;
	memset(&codec, 0, sizeof(codec));
	am_codec_info_modify(&codec, 0);
	am_codec_info_modify(&codec, 1);
	am_codec_info_modify(&codec, 2);
	am_codec_info_modify(&codec, 3);
	am_codec_info_modify(&codec, 4);
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"g729d_preference"])
	{
		snprintf(codec.name, 64, "g729d");
		codec.payload = 97;
		codec.enable = 1;
		codec.freq = 8000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 0;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"g7291_preference"])
	{
		snprintf(codec.name, 64, "g7291");
		codec.payload = 98;
		codec.enable = 1;
		codec.freq = 16000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 0;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"g722_preference"])
	{
		snprintf(codec.name, 64, "g722");
		codec.payload = 99;
		codec.enable = 1;
		codec.freq = 8000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 0;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}		
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex16k_preference"])
	{
		snprintf(codec.name, 64, "speex");
		codec.payload = 99;
		codec.enable = 1;
		codec.freq = 16000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 6;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}		
#ifdef MY_ENABLE_G729
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"g729_preference"])
	{
		memset(&codec, 0, sizeof(codec));
		snprintf(codec.name, 64, "g729");
		codec.payload = 18;
		codec.enable = 1;
		codec.freq = 8000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 0;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}
#endif
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"speex8k_preference"])
	{
		memset(&codec, 0, sizeof(codec));
		snprintf(codec.name, 64, "speex");
		codec.payload = 100;
		codec.enable = 1;
		codec.freq = 8000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 6;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"gsm8k_preference"])
	{
		snprintf(codec.name, 64, "GSM");
		codec.payload = 3;
		codec.enable = 1;
		codec.freq = 8000;
		codec.vbr = 0;
		codec.cng = 0;
		codec.mode = 0;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}
	
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"pcmu_preference"])
	{
		memset(&codec, 0, sizeof(codec));
		snprintf(codec.name, 64, "PCMU");
		codec.payload = 0;
		codec.enable = 1;
		codec.freq = 8000;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}
	
	if (i<5 && [[NSUserDefaults standardUserDefaults] boolForKey:@"pcma_preference"])
	{
		memset(&codec, 0, sizeof(codec));
		snprintf(codec.name, 64, "PCMA");
		codec.payload = 8;
		codec.enable = 1;
		codec.freq = 8000;
		if (am_codec_info_modify(&codec, i)>=0)
			i++;
	}

#ifdef ENABLE_VIDEO
	{
		am_video_codec_info_t video_codec;

		memset(&video_codec, 0, sizeof(video_codec));
		am_video_codec_info_modify (&video_codec, 0);
		am_video_codec_info_modify (&video_codec, 1);
		am_video_codec_info_modify (&video_codec, 2);
		am_video_codec_info_modify (&video_codec, 3);
		am_video_codec_info_modify (&video_codec, 4);
		
		i=0;
		if (i<5 && [[NSUserDefaults standardUserDefaults] boolForKey:@"vp8_preference"])
		{
			memset(&video_codec, 0, sizeof(video_codec));
			snprintf(video_codec.name, 64, "VP8");
			video_codec.payload = 113;
			video_codec.enable = 1;
			video_codec.mode = 0;
			video_codec.freq = 90000;
			if (am_video_codec_info_modify(&video_codec, i)>=0)
				i++;
		}
		
		if (i<5 && [[NSUserDefaults standardUserDefaults] boolForKey:@"h264_preference"])
		{
			memset(&video_codec, 0, sizeof(video_codec));
			snprintf(video_codec.name, 64, "H264");
			video_codec.payload = 114;
			video_codec.enable = 1;
			video_codec.mode = 1;
			video_codec.freq = 90000;
			if (am_video_codec_info_modify(&video_codec, i)>=0)
				i++;
		}
		
		if (i<5 && [[NSUserDefaults standardUserDefaults] boolForKey:@"mp4v_preference"])
		{
			memset(&video_codec, 0, sizeof(video_codec));
			snprintf(video_codec.name, 64, "MP4V-ES");
			video_codec.payload = 117;
			video_codec.enable = 1;
			video_codec.freq = 90000;
			if (am_video_codec_info_modify(&video_codec, i)>=0)
				i++;
		}
		
		if (i<5 && [[NSUserDefaults standardUserDefaults] boolForKey:@"h2631998_preference"])
		{
			memset(&video_codec, 0, sizeof(video_codec));
			snprintf(video_codec.name, 64, "H263-1998");
			video_codec.payload = 115;
			video_codec.enable = 1;
			video_codec.freq = 90000;
			if (am_video_codec_info_modify(&video_codec, i)>=0)
				i++;
		}
		
	}
	
#endif
	    
	if (_password!=nil && _login!=nil && [_password length]>0 && [_login length]>0)
		am_option_set_password("", [_login cStringUsingEncoding:NSUTF8StringEncoding],
							   [_password cStringUsingEncoding:NSUTF8StringEncoding]);
	
	if (_stun!=nil && [_stun length]>0)
	{
		am_option_enable_turn_server([_stun cStringUsingEncoding:NSUTF8StringEncoding], 1);
	}
	
	am_option_enable_symmetric_rtp(1);


  
  NSString *audioinput_preference = [[NSUserDefaults standardUserDefaults] stringForKey:@"audioinput_preference"];
  NSString *audiooutput_preference = [[NSUserDefaults standardUserDefaults] stringForKey:@"audiooutput_preference"];
  
  am_option_select_out_sound_card(0);
  
	struct am_sndcard sndcard;
  memset(&sndcard, 0, sizeof(struct am_sndcard));
	sndcard.card=0;
  i=-1;
	for (;;)
	{
		i = am_option_find_in_sound_card(&sndcard);
    if (audioinput_preference==nil)
      break;
		if (i < 0)
		{
			break;
		}
		NSString *tmp = [[NSString alloc]initWithFormat:@"%s: %s",sndcard.driver_type, sndcard.name];
    if ([tmp isEqualToString:audioinput_preference])
    {
      [tmp release];
      break;      
    }
    [tmp release];
		sndcard.card++;
	}	
  if (i<0)
    am_option_select_in_sound_card(0);
  else
    am_option_select_in_sound_card(sndcard.card);
  
	memset(&sndcard, 0, sizeof(struct am_sndcard));
	sndcard.card=0;
  i=-1;
	for (;;)
	{
		i = am_option_find_out_sound_card(&sndcard);
    if (audiooutput_preference==nil)
      break;
		if (i < 0)
		{
			break;
		}
		NSString *tmp = [[NSString alloc]initWithFormat:@"%s: %s",sndcard.driver_type, sndcard.name];
    if ([tmp isEqualToString:audiooutput_preference])
    {
      [tmp release];
      break;      
    }
    [tmp release];
		sndcard.card++;
	}	
  if (i<0)
    am_option_select_out_sound_card(0);
  else
    am_option_select_out_sound_card(sndcard.card);
	
  
 		
	i = am_network_start([_transport cStringUsingEncoding:NSUTF8StringEncoding], 6010);
	if (i < 0)
		i = am_network_start([_transport cStringUsingEncoding:NSUTF8StringEncoding], 0);
	
	if (i < 0) {
		am_trace(__FILE__, __LINE__, OSIP_ERROR, "Cannot start network layer");
		[pool release];
		thread_started=false;	
		return;
	}
	
	//<sip:0282235642@210.50.23.10?Route=%3Csip%3A211.27.255.6%3Blr%3E>
	snprintf(identity, sizeof(identity), "%s", [_identity cStringUsingEncoding:NSUTF8StringEncoding]);
	if (_outboundproxy!=nil && [_outboundproxy length]>0)
	{
		osip_to_t *ato;
		osip_to_init(&ato);
		if (ato!=nil)
		{
			int i = osip_to_parse(ato, identity);
			if (i!=0 || ato==NULL || ato->url==NULL)
			{
				am_trace(__FILE__, __LINE__, OSIP_ERROR, "Bad Identity");
			} else {
				char routeheader[1024];
				char *tmp_identity=NULL;
				snprintf(routeheader, sizeof(routeheader), "<sip:%s;lr>", [_outboundproxy cStringUsingEncoding:NSUTF8StringEncoding])\
				;
				osip_uri_uheader_add(ato->url,osip_strdup("Route"),osip_strdup(routeheader));
				i = osip_to_to_str(ato, &tmp_identity);
				if (i==0 && tmp_identity!=NULL)
				{
					snprintf(identity, sizeof(identity), "%s", tmp_identity);
					osip_free(tmp_identity);
				} else {
					am_trace(__FILE__, __LINE__, OSIP_ERROR, "Bad outbound Proxy");
				}
			}
			osip_to_free(ato);
		}
	}
	
	snprintf(sipproxy, sizeof(sipproxy), "sip:%s", [_proxy cStringUsingEncoding:NSUTF8StringEncoding]);
	int val = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"];

	int _rid = am_register_start(identity, sipproxy, val, val);
	if (_rid <= 0) {
		am_trace(__FILE__, __LINE__, OSIP_ERROR, "Cannot start registration");
		[pool release];
		thread_started=false;	
		return;
	}

	registration = [[Registration alloc] init];
	[registration setRid:_rid];
	[registration setCode:0];
	[registrationDelegates performSelectorOnMainThread : @ selector(onRegistrationNew:) withObject:registration waitUntilDone:NO];							
	
    for (;thread_start!=false;)
	{
		/* fifo for amsip commands */
		/* TODO */
		
		osip_usleep(20000);
		[self amsipLoop];
	}

    for (;thread_start!=false;)
	{
		/* fifo for amsip commands */
		/* TODO */
		
		osip_usleep(20000);
		[self amsipLoop];
	}
	
	[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
	if (registration!=nil && [registration rid]>0)
	{
		i = am_register_stop([registration rid]);
		if (i!=0)
		{
			am_reset([userAgent cStringUsingEncoding:NSUTF8StringEncoding], 5);
			
			[pool release];
			thread_started=false;
			return;
		}
		
		i=0;
		for (;(registration!=nil && [registration rid]>0) && i<50*2;)
		{
			/* fifo for amsip commands */
			/* TODO */
			
			osip_usleep(20000);
			[self amsipLoop];
			i++;
		}
	}
	
	am_reset([userAgent cStringUsingEncoding:NSUTF8StringEncoding], 5);
	[iAudioPlayerApp performSelectorOnMainThread : @ selector(stop) withObject:nil waitUntilDone:NO];
	
	[pool release];
	thread_started=false;	
}

@end
