//
//  AppController.h
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 12/1/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PreferenceController.h"
#import "Vbyantisip_NSOpenGLView.h"

#import "AppEngine.h"
#import "Call.h"

#include <amsiptools/hid_hooks.h>

@interface AppController : NSObject <NSTableViewDataSource> {

@private
	PreferenceController *preferenceController;
	IBOutlet NSWindow *mainWindow;
	AppEngine *appEngine;
	NSMutableArray *call_list;
	//Call *aCall;
	IBOutlet NSTextField *remote_uri;
	IBOutlet NSTextField *registration_status;
	IBOutlet Vbyantisip_NSOpenGLView *opengl_view;
	IBOutlet Vbyantisip_NSOpenGLView *opengl_selfview;
	IBOutlet NSTableView *calls_tableview;
  
	NSTimer *hookTimer;
  hid_hooks_t hid_device;
}

-(void)setImage:(int)pin Width:(int)width Height:(int)height Format:(int)format Size:(int)size Pixels:(char*)pixels;
-(void)setImageSelfView:(int)pin Width:(int)width Height:(int)height Format:(int)format Size:(int)size Pixels:(char*)pixels;

-(IBAction)action_login:(id)sender;

-(IBAction)action_call:(id)sender;
-(IBAction)action_answer:(id)sender;
-(IBAction)action_hang:(id)sender;
-(IBAction)action_addvideo:(id)sender;
-(IBAction)action_hold:(id)sender;
-(IBAction)action_mute:(id)sender;

-(IBAction)action_enablevideo:(id)sender;


-(void)action_call_hang:(Call*)pCall;
-(void)action_call_answer:(Call*)pCall;

@end
