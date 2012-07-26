//
//  AppController.m
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 12/1/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "AppController.h"

static AppController *gAppController=NULL;

void on_new_image_cb(int pin, int width, int height, int format, int size, void *pixel);

void on_new_image_cb(int pin, int width, int height, int format, int size, void *pixel)
{
	static int count=0;
	if (count%100==0)
		NSLog(@"on_new_image_cb: pin value:%i %ix%i (fmt=%i size=%i)\n", pin, width, height, format, size);
	count++;
	if (gAppController!=nil && pin==0){
		[gAppController setImage:pin Width:width Height:height Format:format Size:size Pixels:pixel];
	}
	if (gAppController!=nil && pin==1){
		[gAppController setImageSelfView:pin Width:width Height:height Format:format Size:size Pixels:pixel];
	}
}

@implementation AppController

- (id) init {
	appEngine = [[AppEngine alloc] init];
	
	//aCall=nil;
	call_list = [[NSMutableArray alloc] init];
	
	[appEngine addRegistrationDelegate:self];
	[appEngine addCallDelegate:self];
	gAppController=self;
  
  if (hid_hooks_start(&hid_device)==0)
  {
    //NSLog(@"HID device detected");
  }
  
  if (hookTimer==nil)
  {
    hookTimer = [NSTimer scheduledTimerWithTimeInterval:0.6 target:self selector:@selector(handleHidEvent) userInfo:nil repeats:YES];
  }
  
	return self;
}

-(void)handleHidEvent {
  int hid_evt;
  
  hid_evt = hid_hooks_get_events(&hid_device);
  while (hid_evt>0)
  {
    int idx;
    NSLog(@"HID event detected");
    switch (hid_evt) {
      case HID_EVENTS_TALK:
        NSLog(@"HID: HID_EVENTS_TALK");
        idx=0;
        while (idx<[call_list count]) {
          Call *pCall = [call_list objectAtIndex:idx];
          if ([pCall callState]==DISCONNECTED) {
            idx++;
            continue;
          }

          [self action_call_hang:pCall];
          break;
        }
        break;
      case HID_EVENTS_HOOK:
        NSLog(@"HID: HID_EVENTS_HOOK");
        idx=0;
        while (idx<[call_list count]) {
          Call *pCall = [call_list objectAtIndex:idx];
          if ([pCall isIncomingCall]==true && [pCall callState]==RINGING) {
            [self action_call_answer:pCall];
            break;
          }
          idx++;
          continue;
        }
        break;
      case HID_EVENTS_HANGUP:
        NSLog(@"HID: HID_EVENTS_HANGUP");
        idx=0;
        while (idx<[call_list count]) {
          Call *pCall = [call_list objectAtIndex:idx];
          if ([pCall callState]==DISCONNECTED) {
            idx++;
            continue;
          }
          
          [self action_call_hang:pCall];
          break;
        }
        break;
      case HID_EVENTS_MUTE:
        NSLog(@"HID: HID_EVENTS_MUTE");
        break;
      case HID_EVENTS_UNMUTE:
        NSLog(@"HID: HID_EVENTS_UNMUTE");
        break;
      case HID_EVENTS_VOLUP:
        NSLog(@"HID: HID_EVENTS_VOLUP");
        break;
      case HID_EVENTS_VOLDOWN:
        NSLog(@"HID: HID_EVENTS_VOLDOWN");
        break;
      case HID_EVENTS_SMART:
        NSLog(@"HID: HID_EVENTS_SMART");
        break;
      case HID_EVENTS_FLASHUP:
        NSLog(@"HID: HID_EVENTS_FLASHUP");
        break;
      case HID_EVENTS_FLASHDOWN:
        NSLog(@"HID: HID_EVENTS_FLASHDOWN");
        break;
      default:
        NSLog(@"HID: not implemented");
        break;
    }
    
    hid_evt = hid_hooks_get_events(&hid_device);
  }
}

-(void)setImage:(int)pin Width:(int)width Height:(int)height Format:(int)format Size:(int)size Pixels:(char*)pixels
{
    [opengl_view setImage:pin Width:width Height:height Format:format Size:size Pixels:pixels];
    [opengl_view displayCurrentImage];
}

-(void)setImageSelfView:(int)pin Width:(int)width Height:(int)height Format:(int)format Size:(int)size Pixels:(char*)pixels
{
    [opengl_selfview setImage:pin Width:width Height:height Format:format Size:size Pixels:pixels];
    [opengl_selfview displayCurrentImage];
}

- (void)onCallNew:(Call *)pCall {
	NSLog(@"onCallUpdate cid=%i did=%i state=%i %@ %@", [pCall cid], [pCall did], [pCall callState], [pCall from], [pCall to]);
  
	if ([call_list containsObject:pCall])
		return;
	
	[call_list addObject:pCall];
	[pCall addCallStateChangeListener:self];
	
	if ([pCall isIncomingCall]==true && [pCall callState]==RINGING) {
		[pCall accept:180];
    
    if ([call_list count]==1) {

      if (hid_hooks_get_audioenabled(&hid_device)>0)
        hid_hooks_set_audioenabled(&hid_device, 0);
      
      hid_hooks_set_ringer(&hid_device, true);
    }
    
  }
	
}

-(void)onCallExist:(Call *)pCall {
	[self onCallNew:pCall];
}

- (void)onCallRemove:(Call *)pCall {
	NSLog(@"onCallUpdate cid=%i did=%i state=%i %@ %@", [pCall cid], [pCall did], [pCall callState], [pCall from], [pCall to]);

	if ([call_list containsObject:pCall]) {
		[call_list removeObject:pCall];
		[pCall removeCallStateChangeListener:self];
	}
	[calls_tableview reloadData];
}

- (void)onCallUpdate:(Call *)pCall{
	NSLog(@"onCallUpdate cid=%i did=%i state=%i %@ %@", [pCall cid], [pCall did], [pCall callState], [pCall from], [pCall to]);
	
	//update tableview
	[calls_tableview reloadData];
  
  if ([pCall callState]>CONNECTED)
    hid_hooks_set_ringer(&hid_device, false);
}

- (void)onRegistrationNew:(Registration*)registration{
    NSLog(@"onRegistrationNew %i", [registration rid]);
    
    NSString *tmp;
    
    if ([registration code]>=200 && [registration code]<300)
        tmp = [NSString stringWithFormat:@"(%i) registered", [registration code]];
    else if ([registration code]>=300)
        tmp = [NSString stringWithFormat:@"(%i) - not registered", [registration code]];
    else 
        tmp = [NSString stringWithFormat:@"(%i) - registration pending", [registration code]];

    [registration_status setStringValue:tmp];
}

- (void)onRegistrationRemove:(Registration*)registration{
    NSLog(@"onRegistrationRemove %i", [registration rid]);
}

- (void)onRegistrationUpdate:(Registration*)registration{
    NSLog(@"onRegistrationUpdate %i", [registration rid]);    

    NSString *tmp;
    
    if ([registration code]>=200 && [registration code]<300)
        tmp = [NSString stringWithFormat:@"(%i) registered", [registration code]];
    else if ([registration code]>=300)
        tmp = [NSString stringWithFormat:@"(%i) - not registered", [registration code]];
    else 
        tmp = [NSString stringWithFormat:@"(%i) - registration pending", [registration code]];
    
    [registration_status setStringValue:tmp];
}

-(IBAction)action_enablevideo:(id)sender{
    [appEngine setVideoCallback:NULL];
    [appEngine enableVideo:0 Width:352 Height:288];
    [appEngine setVideoCallback:&on_new_image_cb];
    [appEngine enableVideo:1 Width:352 Height:288];
}

-(IBAction)action_login:(id)sender{
    [appEngine stop];
    [appEngine start];
    [appEngine setVideoCallback:&on_new_image_cb];
    [appEngine enableVideo:1 Width:352 Height:288];
}

-(IBAction)action_call:(id)sender{
  //[appEngine setVideoCallback:&on_new_image_cb];
  //[appEngine enableVideo:1 Width:352 Height:288];
  int i = [appEngine amsip_start:[remote_uri stringValue] withReferedby_did:0];
  
  if (i>0) {
    hid_hooks_set_audioenabled(&hid_device, true);
  }
}

-(void)action_call_answer:(Call*)pCall{
	if (pCall!=nil)	{
		if ([pCall callState]==RINGING) {
			[pCall accept:200];
      if (hid_hooks_get_audioenabled(&hid_device)==false)
        hid_hooks_set_audioenabled(&hid_device, true);
    }
  }
}

-(IBAction)action_answer:(id)sender{
	Call *pCall=nil;
	NSInteger idx = [calls_tableview selectedRow];
	if (idx<0 || idx>[call_list count])
	{
		idx=0;
	}
	if (idx<[call_list count])
	{
		pCall = [call_list objectAtIndex:idx];
	}
	if (pCall!=nil)
	{
		if ([pCall callState]==RINGING) {
			[pCall accept:200];
      if (hid_hooks_get_audioenabled(&hid_device)==false)
        hid_hooks_set_audioenabled(&hid_device, true);
    }
  }
}

-(void)action_call_hang:(Call*)pCall{
	if (pCall!=nil)	{
    if ([pCall isIncomingCall]==true && [pCall callState]==RINGING) {
      [pCall accept:486];
    } else if ([pCall callState]==DISCONNECTED) {
		} else {
      [pCall hangup];
    }
	}
  if ([call_list count]==1) {
    
    if (hid_hooks_get_audioenabled(&hid_device)>0)
      hid_hooks_set_audioenabled(&hid_device, 0);
    
    hid_hooks_set_ringer(&hid_device, false);
  }  
}
  
-(IBAction)action_hang:(id)sender{
	Call *pCall=nil;
	NSInteger idx = [calls_tableview selectedRow];
	if (idx<0 || idx>[call_list count])
	{
		idx=0;
	}
	if (idx<[call_list count])
	{
		pCall = [call_list objectAtIndex:idx];
	}
	if (pCall!=nil)
	{
    if ([pCall isIncomingCall]==true && [pCall callState]==RINGING) {
      [pCall accept:486];
    } else if ([pCall callState]==DISCONNECTED) {
		} else {
      [pCall hangup];
    }
	}
}

-(IBAction)action_addvideo:(id)sender{
	Call *pCall=nil;
	NSInteger idx = [calls_tableview selectedRow];
	if (idx<0 || idx>[call_list count])
	{
		idx=0;
	}
	if (idx<[call_list count])
	{
		pCall = [call_list objectAtIndex:idx];
	}
	if (pCall!=nil)
	{
		if ([pCall callState]==CONNECTED)
		{
			[pCall addvideo];
		}
	}
}

-(IBAction)action_hold:(id)sender{
	Call *pCall=nil;
	NSInteger idx = [calls_tableview selectedRow];
	if (idx<0 || idx>[call_list count])
	{
		idx=0;
	}
	if (idx<[call_list count])
	{
		pCall = [call_list objectAtIndex:idx];
	}
	if (pCall!=nil)
	{
		if ([pCall isOnhold]==true)
			[pCall unhold];
		else
			[pCall hold];
	}
}

-(IBAction)action_mute:(id)sender{
	Call *pCall=nil;
	NSInteger idx = [calls_tableview selectedRow];
	if (idx<0 || idx>[call_list count])
	{
		idx=0;
	}
	if (idx<[call_list count])
	{
		pCall = [call_list objectAtIndex:idx];
	}
	if (pCall!=nil)
	{
		if ([pCall isMuted]==true)
			[pCall unmute];
		else
			[pCall mute];
	}
}

-(IBAction)action_showPreferences:(id)sender{
    if(!preferenceController)
        preferenceController = [[PreferenceController alloc] initWithWindowNibName:@"Preferences"];
    
    [preferenceController showWindow:self];
}

-(IBAction)action_exit:(id)sender{
    //TODO
    //[self close];
}

- (void)dealloc {
  if (hookTimer!=nil)
  {
    [hookTimer invalidate];
    hookTimer=nil;
  }
  hid_hooks_stop(&hid_device);
  [appEngine stop];
  [preferenceController release];
  gAppController=nil;
  [super dealloc];
}


/* handle the call table view */

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
	
	return [call_list count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
	
	Call *pCall = [call_list objectAtIndex:row];
	if (pCall==nil)
	{
		return @"--";
	}
	if ([[tableColumn identifier] isEqualToString:@"cid-did"])
	{
		return [NSString stringWithFormat:@"%i/%i", [pCall cid], [pCall did]];
	}
	else if ([[tableColumn identifier] isEqualToString:@"remote uri"])
	{
		return [NSString stringWithFormat:@"%@", [pCall oppositeNumber]];
	}
	else if ([[tableColumn identifier] isEqualToString:@"status"])
	{
		if ([pCall callState]==NOTSTARTED)
			return [NSString stringWithFormat:@"pending"];
		else if ([pCall callState]==TRYING)
			return [NSString stringWithFormat:@"trying"];
		else if ([pCall callState]==RINGING)
			return [NSString stringWithFormat:@"ringing"];
		else if ([pCall callState]==CONNECTED)
			return [NSString stringWithFormat:@"established"];
		else if ([pCall callState]==DISCONNECTED)
			return [NSString stringWithFormat:@"disconnected"];
		else
			return [NSString stringWithFormat:@"unknown"];
	}
	return @"--";
}

/* NOTE: This method is not called for the View Based TableView.
 */
- (void)tableView:(NSTableView *)tableView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
	
}

@end
