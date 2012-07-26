//
//  PreferenceController.m
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 12/1/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "PreferenceController.h"

#include <amsip/am_options.h>

@implementation PreferenceController

-(id)init{
    if (![super initWithWindowNibName:@"Preferences"])
        return nil;
    return self;
}

- (void)windowDidLoad {
    [super windowDidLoad];
}

-(void)awakeFromNib {
  
  NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
  NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
	NSString *_password = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
	NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	NSString *_transport = [[NSUserDefaults standardUserDefaults] stringForKey:@"transport_preference"];
	NSString *_outboundproxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"outboundproxy_preference"];
	NSString *_stun = [[NSUserDefaults standardUserDefaults] stringForKey:@"stun_preference"];
	
	[proxy setStringValue:_proxy];
	[username setStringValue:_login];
	[password setStringValue:_password];
	[identity setStringValue:_identity];
	[transport selectItemWithObjectValue:_transport];
	[outbound setStringValue:_outboundproxy];
	[stun setStringValue:_stun];
  
	struct am_sndcard sndcard;
  memset(&sndcard, 0, sizeof(struct am_sndcard));
	sndcard.card=0;
	for (;;)
	{
		int i = am_option_find_in_sound_card(&sndcard);
		if (i < 0)
		{
			break;
		}
		NSString *tmp = [[NSString alloc]initWithFormat:@"%s: %s",sndcard.driver_type, sndcard.name];
		[combobox_input_sndcard addItemWithObjectValue:tmp];
		[tmp release];
		sndcard.card++;
	}	

	memset(&sndcard, 0, sizeof(struct am_sndcard));
	sndcard.card=0;
	for (;;)
	{
		int i = am_option_find_out_sound_card(&sndcard);
		if (i < 0)
		{
			break;
		}
		NSString *tmp = [[NSString alloc]initWithFormat:@"%s: %s",sndcard.driver_type, sndcard.name];
		[combobox_output_sndcard addItemWithObjectValue:tmp];
		[tmp release];
		sndcard.card++;
	}	
	
  struct am_camera camera;
  memset(&camera, 0, sizeof(struct am_camera));
	camera.card=0;
	for (;;)
	{
		int i = am_option_find_camera(&camera);
		if (i < 0)
		{
			break;
		}
		NSString *tmp = [[NSString alloc]initWithFormat:@"%s",camera.name];
		[combobox_camera addItemWithObjectValue:tmp];
		[tmp release];
		camera.card++;
	}	
  
  
  NSString *audioinput_preference = [[NSUserDefaults standardUserDefaults] stringForKey:@"audioinput_preference"];
  if (audioinput_preference!=nil)
    [combobox_input_sndcard selectItemWithObjectValue:audioinput_preference];
  else
    [combobox_input_sndcard selectItemAtIndex:0];
  NSString *audiooutput_preference = [[NSUserDefaults standardUserDefaults] stringForKey:@"audiooutput_preference"];
  if (audiooutput_preference!=nil)
    [combobox_output_sndcard selectItemWithObjectValue:audiooutput_preference];
  else
    [combobox_output_sndcard selectItemAtIndex:0];
  NSString *cameradevice_preference = [[NSUserDefaults standardUserDefaults] stringForKey:@"cameradevice_preference"];
  if (cameradevice_preference!=nil)
    [combobox_camera selectItemWithObjectValue:cameradevice_preference];
  else
    [combobox_camera selectItemAtIndex:0];
  
}

- (IBAction)actionButtonCancel:(id)sender {

    [self close];
}

- (IBAction)actionButtonSave:(id)sender {
    
  NSString *_proxy = [proxy stringValue];
  NSString *_login = [username stringValue];
  NSString *_password = [password stringValue];
  NSString *_identity = [identity stringValue];
  NSString *_transport = [transport stringValue];
  NSString *_outboundproxy = [outbound stringValue];
  NSString *_stun = [stun stringValue];

  NSString *_audioinput = [combobox_input_sndcard stringValue];
  NSString *_audiooutput = [combobox_output_sndcard stringValue];
  NSString *_cameradevice = [combobox_camera stringValue];
  
  [[NSUserDefaults standardUserDefaults] setObject:_proxy forKey:@"proxy_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_login forKey:@"user_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_password forKey:@"password_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_identity forKey:@"identity_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_transport forKey:@"transport_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_outboundproxy forKey:@"outboundproxy_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_stun forKey:@"stun_preference"];
  
  [[NSUserDefaults standardUserDefaults] setObject:_audioinput forKey:@"audioinput_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_audiooutput forKey:@"audiooutput_preference"];
  [[NSUserDefaults standardUserDefaults] setObject:_cameradevice forKey:@"cameradevice_preference"];
  
  if (_proxy==nil || [_proxy length]==0)
    return;
  if (_login==nil || [_login length]==0)
    return;
  
  if (_identity==nil || [_identity length]==0)
  {
    NSString *tmp = [NSString stringWithFormat:@"<sip:%@@%@>", [username stringValue], [proxy stringValue]];
    [[NSUserDefaults standardUserDefaults] setObject:tmp forKey:@"identity_preference"];
  }
  [self close];
}

@end
