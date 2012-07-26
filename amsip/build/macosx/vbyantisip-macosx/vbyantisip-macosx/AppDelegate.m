//
//  AppDelegate.m
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 11/30/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import "AppDelegate.h"

@implementation AppDelegate

@synthesize window = _window;

-(id)init{
    self = [super init];
    if (self){
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setValue:@"sip.antisip.com" forKey:@"proxy_preference"];
    [[NSUserDefaults standardUserDefaults] setValue:@"test8" forKey:@"user_preference"];
    [[NSUserDefaults standardUserDefaults] setValue:@"secret" forKey:@"password_preference"];

    
    NSString *proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
    NSString *login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
    NSString *tmp = [NSString stringWithFormat:@"<sip:%@@%@>", login, proxy];
    [[NSUserDefaults standardUserDefaults] setObject:tmp forKey:@"identity_preference"];
    
    [[NSUserDefaults standardUserDefaults] setObject:@"UDP" forKey:@"transport_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"checkcertificate_preference"];
    
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"naptr_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"aec_preference"];
    
    
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"g729d_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"g7291_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"g722_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"speex16k_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"g729_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"speex8k_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:false forKey:@"gsm8k_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"pcmu_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"pcma_preference"];
    
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"vp8_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"h264_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"mp4v_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"h2631998_preference"];
    
    [[NSUserDefaults standardUserDefaults] setInteger:256 forKey:@"uploadbandwidth_preference"];
    [[NSUserDefaults standardUserDefaults] setInteger:256 forKey:@"downloadbandwidth_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"h2631998_preference"];
    [[NSUserDefaults standardUserDefaults] setBool:true forKey:@"h2631998_preference"];
}

@end
