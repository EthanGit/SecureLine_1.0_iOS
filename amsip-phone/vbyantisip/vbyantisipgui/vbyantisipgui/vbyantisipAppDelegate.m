//
//  vbyantisipAppDelegate.m
//  AppEngine
//
//  Created by Aymeric MOIZARD on 16/10/09.
//  Copyright antisip 2009. All rights reserved.
//

#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"
#import "Registration.h"
#import "apiGlobal.h"
#import "vbyantisipAppDelegate.h"
#import "UIViewControllerDialpad.h"

#import "NetworkTracking.h"


#ifdef GENTRICE
#import <CoreTelephony/CTCall.h>
#import <CoreTelephony/CTCallCenter.h>
#import "dialIncomingUIViewController.h"
#import "loginUINavigationController.h"
#endif //GENTRICE


static UIBackgroundTaskIdentifier bgTask = 0;

void (^keepAliveHandler)(void) = ^{
	vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
	
	NSLog(@"keepalive handler is running");
	
	[appDelegate dokeepAliveHandler];
};

@implementation vbyantisipAppDelegate

#ifdef GENTRICE
@synthesize managedObjectContext = __managedObjectContext;
@synthesize managedObjectModel = __managedObjectModel;
@synthesize persistentStoreCoordinator = __persistentStoreCoordinator;
@synthesize currentAlert, doingPasscodeLogin;
@synthesize tmp_data,connect_finished;
CTCallCenter *gCallCenter;
NSString *gCallState;
#endif

@synthesize window;
@synthesize tabBarController;
//@synthesize viewControllerCallControl;
//@synthesize viewControllerStatus;

+(void)_initialize
{
	NSError *setCategoryError = nil;
	[[AVAudioSession sharedInstance]
	 setCategory: AVAudioSessionCategoryPlayAndRecord
	 error: &setCategoryError];
	
	[NetworkTracking getInstance];
	gAppEngine = [[AppEngine alloc] init];
    
	[gAppEngine initialize];
    return;
}


- (void)onNetworkTrackingUpdate:(NetworkTracking *)networkTracking
{
    NetworkStatus netStatus = [networkTracking currentReachabilityStatus];
    BOOL connectionRequired= [networkTracking connectionRequired];
    
    
#ifdef GENTRICE
    if (([[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"] == NO)) {
        return;
    }
#endif
    
    switch (netStatus)
    {
        case NotReachable:
        {
            connectionRequired= NO;
#ifdef GENTRICE
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Network error", nil) message:NSLocalizedString(@"You are disconnected from internet, please check your network setting.", nil) delegate:nil cancelButtonTitle:@"Ok" otherButtonTitles:nil];
            [alert show];
            [alert release];
#endif
            break;
        }
            
        case ReachableViaWWAN:
        {
			if (connectionRequired==NO)
			{
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"wifimode_preference"];
				vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];	
				[appDelegate performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
			}
            break;
        }
        case ReachableViaWiFi:
        {     
			if (connectionRequired==NO)
			{
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"wifimode_preference"];
				vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];	
				[appDelegate performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
			}
            break;
		}
    }
}

-(void)showCallControlView
{
	//[UIView beginAnimations:nil context:NULL];
	//[UIView setAnimationDuration:1.0];
	//[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft forView:window cache:YES];
	//[tabBarController.view removeFromSuperview];
	//[self.window addSubview:[viewControllerCallControl view]];
	//[UIView commitAnimations];
	
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	NSLog(@"----------------> applicationDidFinishLaunchingWithOptions\n");
    
    
    
    [[UIApplication sharedApplication] 
	 registerForRemoteNotificationTypes:(UIRemoteNotificationTypeBadge | 
										 UIRemoteNotificationTypeSound |
										 UIRemoteNotificationTypeAlert)]; 
	
	isInBackground = true;
    

    
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"firstRun"] == false){
        NSString *sip_proxy_addr = [[[NSString alloc] init] autorelease];
        sip_proxy_addr = [SIPProxyServer stringByAppendingString:SIPProxyPort];
        
		[[NSUserDefaults standardUserDefaults] setObject:sip_proxy_addr forKey:@"proxy_preference"];
		[[NSUserDefaults standardUserDefaults] setObject:@"tls" forKey:@"transport_preference"];
        //[[NSUserDefaults standardUserDefaults] setObject:@"299" forKey:@"username_preference"];
        //[[NSUserDefaults standardUserDefaults] setObject:@"gensanji" forKey:@"password_preference"];
        [[NSUserDefaults standardUserDefaults] setObject:@"" forKey:@"tmp_user_preference"];
		[[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"ptime_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"g729_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"speex16k_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"g722_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"speex8k_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"iLBC_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"gsm8k_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"pcmu_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"pcma_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"aec_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"elimiter_preference"];
		[[NSUserDefaults standardUserDefaults] setInteger:2 forKey:@"srtp_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"naptr_preference"];
		[[NSUserDefaults standardUserDefaults] setInteger:900 forKey:@"reginterval_preference"];
		
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"vp8_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"h264_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"mp4v_preference"];
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"h2631998_preference"];
		
		[[NSUserDefaults standardUserDefaults] setInteger:128 forKey:@"uploadbandwidth_preference"];
		[[NSUserDefaults standardUserDefaults] setInteger:128 forKey:@"downloadbandwidth_preference"];
		
		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"firstRun"];
        [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"IconBadgeNumber"];
        
        [[NSUserDefaults standardUserDefaults] setObject:@"" forKey:@"PasscodeString"];
	}
	
  // Add the tab bar controller's current view as a subview of the window
  [self.window addSubview:tabBarController.view];
  [self.window makeKeyAndVisible];

	
  if ([application applicationIconBadgeNumber]>0)
    [application setApplicationIconBadgeNumber:0];
   // NSLog(@"------------ setApplicationIconBadgeNumber:0");
    [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"IconBadgeNumber"];
  
	NetworkTracking *gNetworkTracking = [NetworkTracking getInstance];
	[gNetworkTracking addNetworkTrackingDelegate:self];
	
	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
	
        
	if (_proxy != nil
      &&_login != nil)
	{
		if ([[UIApplication sharedApplication] applicationState] == UIApplicationStateBackground)
		{
			if ([[UIDevice currentDevice] isMultitaskingSupported]) {
				if (bgTask!=0)
					[[UIApplication sharedApplication] endBackgroundTask:bgTask];
				
				bgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
					NSLog(@"endBackgroundTask - end of background task\n");
					[[UIApplication sharedApplication] endBackgroundTask:bgTask];
					bgTask=0;
				}];
				if ([application setKeepAliveTimeout:600 handler:keepAliveHandler]) {
					NSLog(@"applicationDidEnterBackground: setKeepAliveTimeout worked!");
					isInBackground = true;
				} else {
					NSLog(@"applicationDidEnterBackground: setKeepAliveTimeout failed.");
				}
				//allow initial registration
			}
		}
	}

#ifdef GENTRICE
    gCallCenter = [[[CTCallCenter alloc] init] autorelease];
    gCallState = CTCallStateDisconnected;

    gCallCenter.callEventHandler = ^(CTCall *call) {
        gCallState = [call callState];
        NSLog(@">>>>>>> gCallState=%@", gCallState);
    };
    
    [self checkPasscodeLoginStatus:nil];    
    [self.tabBarController setSelectedIndex:1];
    
#endif
   
	return true;
}


- (void)applicationDidEnterBackground:(UIApplication *)application
{
    NSLog(@"applicationDidEnterBackground");


    
    
#ifdef GENTRICE
       
    if( [gAppEngine getNumberOfActiveCalls]==0){
        
       // NSLog(@"--------------- setSelectedIndex:1");

        //[self.tabBarController.view reloadInputViews];// remoteControlReceivedWithEvent:UIEventTypeTouches];
        //NSLog(@"view :%@",self.tabBarController.viewControllers);
        if([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]!=YES && 
           [[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"] == YES){
            [self.tabBarController.view removeFromSuperview];
            [self.window reloadInputViews];
            [self.window addSubview:self.tabBarController.view];      
        }
    }

    [self checkPasscodeLoginStatus:nil];      
    
#endif    
    if (currentAlert!=nil) {
        [currentAlert dismissWithClickedButtonIndex:[currentAlert cancelButtonIndex] animated:NO];
    }    
    
    //[self.tabBarController viewDidLoad];
    
	if ([[UIDevice currentDevice] isMultitaskingSupported]) {
		
		NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
		NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
		if (_proxy != nil
			&&_login != nil)
		{
			[gAppEngine refreshRegistration];
		}
		
		if (_proxy != nil
			&&_login != nil)
		{
			if (bgTask!=0)
				[[UIApplication sharedApplication] endBackgroundTask:bgTask];
			
			//allow additionnal registration just before being suspended for the next 10 minutes.
			bgTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
				NSLog(@"endBackgroundTask - end of background task\n");
				[[UIApplication sharedApplication] endBackgroundTask:bgTask];
				bgTask=0;
			}];
			
			if ([application setKeepAliveTimeout:600 handler:keepAliveHandler]) {
				NSLog(@"applicationDidEnterBackground: setKeepAliveTimeout worked!");
				isInBackground = true;
			} else {
				NSLog(@"applicationDidEnterBackground: setKeepAliveTimeout failed.");
			}
		}
		
	}
    

}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
#ifdef GENTRICE
    //[self checkPasscodeLoginStatus:nil];
#if GENTRICE_DEBUG >0
    //show the contact tab 
    //[self.tabBarController setSelectedIndex:1];//2
    
#endif //GENTRICE_DEBUG    
#endif    
    
	NSLog(@"---------->applicationDidBecomeActive\n");
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
    [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"IconBadgeNumber"];
   // NSLog(@"------------ setApplicationIconBadgeNumber:0");
    //application.applicationIconBadgeNumber = 0;
    
	if (bgTask!=0)
		[[UIApplication sharedApplication] endBackgroundTask:bgTask];
	bgTask=0;
	isInBackground = false;
	
	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
	NSString *_password = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
#ifndef GENTRICE	
    NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
#endif    
	NSString *_transport = [[NSUserDefaults standardUserDefaults] stringForKey:@"transport_preference"];
	NSString *_outboundproxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"outboundproxy_preference"];
	NSString *_stun = [[NSUserDefaults standardUserDefaults] stringForKey:@"stun_preference"];

#ifdef GENTRICE
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"]==YES) {
        if (_proxy == nil
            ||_login == nil)
        {
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account Creation", @"Account Creation") 
                                                            message:NSLocalizedString(@"Please configure your settings in the iphone preference panel.", @"Please configure your settings in the iphone preference panel.")
                                                           delegate:nil cancelButtonTitle:@"Cancel"
                                                  otherButtonTitles:nil];
            [alert show];
            [alert release];
        }
    }
#else
    if (_proxy == nil
        ||_login == nil)
	{
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account Creation", @"Account Creation") 
                                                        message:NSLocalizedString(@"Please configure your settings in the iphone preference panel.", @"Please configure your settings in the iphone preference panel.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
	}	
#endif //GENTRICE
    
	NetworkTracking *gNetworkTracking = [NetworkTracking getInstance];
	NetworkStatus internetStatus =  [gNetworkTracking currentReachabilityStatus];
	BOOL connectionRequired= [gNetworkTracking connectionRequired];

	if (connectionRequired==YES || internetStatus==NotReachable)
	{
		return;
	}
#ifndef GENTRICE
	/* check a change in settings */
	if ((proxy!=nil && _proxy==nil)
		|| (proxy==nil && _proxy!=nil)
		|| (proxy!=nil && [proxy isEqualToString:_proxy]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
#endif    
	if ((login!=nil && _login==nil)
		|| (login==nil && _login!=nil)
		|| (login!=nil && [login isEqualToString:_login]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
	if ((password!=nil && _password==nil)
		|| (password==nil && _password!=nil)
		|| (password!=nil && [password isEqualToString:_password]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
    //NSLog(@"@@@@@@@@@@@@  %@ vs %@", identity, _identity); 
#ifndef GENTRICE
	if ((identity!=nil && _identity==nil)
		|| (identity==nil && _identity!=nil)
		|| (identity!=nil && [identity isEqualToString:_identity]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
#endif
    
	if ((transport!=nil && _transport==nil)
		|| (transport==nil && _transport!=nil)
		|| (transport!=nil && [transport isEqualToString:_transport]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
	if ((outboundproxy!=nil && _outboundproxy==nil)
		|| (outboundproxy==nil && _outboundproxy!=nil)
		|| (outboundproxy!=nil && [outboundproxy isEqualToString:_outboundproxy]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
	if ((stun!=nil && _stun==nil)
		|| (stun==nil && _stun!=nil)
		|| (stun!=nil && [stun isEqualToString:_stun]==NO))
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"g729_preference"]!=g729
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"speex16k_preference"]!=speex16k
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"g722_preference"]!=g722
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"speex8k_preference"]!=speex8k
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"iLBC_preference"]!=iLBC
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"gsm8k_preference"]!=gsm8k
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"pcmu_preference"]!=pcmu
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"pcma_preference"]!=pcma
			
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"aec_preference"]!=aec
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"elimiter_preference"]!=elimiter
			||[[NSUserDefaults standardUserDefaults] integerForKey:@"srtp_preference"]!=srtp
			
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"naptr_preference"]!=naptr
			||[[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"]!=reginterval		
			
			||[[NSUserDefaults standardUserDefaults] integerForKey:@"ptime_preference"]!=ptime
			
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"vp8_preference"]!=vp8
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"h264_preference"]!=h264
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"mp4v_preference"]!=mp4v
			||[[NSUserDefaults standardUserDefaults] boolForKey:@"h2631998_preference"]!=h2631998
			
			||[[NSUserDefaults standardUserDefaults] integerForKey:@"uploadbandwidth_preference"]!=uploadbandwidth
			||[[NSUserDefaults standardUserDefaults] integerForKey:@"downloadbandwidth_preference"]!=downloadbandwidth
        
        )
	{
		[self performSelectorOnMainThread : @ selector(restartAll) withObject:nil waitUntilDone:NO];
		return;
	}
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	NSLog(@"applicationWillEnterForeground\n");
	NSLog(@"bgTask %i\n",bgTask);    
#ifdef GENTRICE
    //[self checkPasscodeLoginStatus:nil];
    
#endif    
    if (currentAlert!=nil) {
        [currentAlert dismissWithClickedButtonIndex:[currentAlert cancelButtonIndex] animated:NO];
    }    
    
	if (bgTask!=0)
		[[UIApplication sharedApplication] endBackgroundTask:bgTask];
	bgTask=0;
	isInBackground = false;
}

- (void) stopBackgroundTask
{
	NSLog(@"stopBackgroundTask\n");
	if (bgTask!=0)
		[[UIApplication sharedApplication] endBackgroundTask:bgTask];
	bgTask=0;
}

- (void) dokeepAliveHandler
{
	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
	if (_proxy != nil
		&&_login != nil)
	{
		[gAppEngine refreshRegistration];
	}
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[gAppEngine stop];
    
#ifdef GENTRICE
    //[self saveContext];
    NSError *error;  
    if (__managedObjectContext != nil) {  
        if ([__managedObjectContext hasChanges] && ![__managedObjectContext save:&error]) {  
            // Handle the error.  
        }  
    }     
#endif     
}


#ifdef GENTRICE
UIAlertView *waitAlert;
UIActivityIndicatorView *waitAIV;

- (void)initSystemAlert 
{
    if (waitAlert==nil) {
        if (currentAlert!=nil) {
            [currentAlert dismissWithClickedButtonIndex:[currentAlert cancelButtonIndex] animated:NO];
            [self setCurrentAlert:nil];
        }
        waitAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"sysRegistering", nil) message:@"\n" delegate:self cancelButtonTitle:nil otherButtonTitles:nil, nil];
        [waitAlert show];
        waitAIV = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
        waitAIV.center = CGPointMake(139.5, 75.5);
        [waitAlert addSubview:waitAIV];
        [waitAIV startAnimating];
        [self setCurrentAlert:waitAlert];
    }
}


- (void)dismissSystemAlert
{
    if (waitAlert!=nil) {
        [waitAlert dismissWithClickedButtonIndex:0 animated:YES];
        [waitAIV stopAnimating];
        [waitAIV removeFromSuperview];
        [waitAlert release];
        waitAlert = nil;
        [waitAIV release];
        waitAIV = nil;
        [self setCurrentAlert:nil];
    }
}

#endif

-(void) restartAll {
#ifdef GENTRICE
    if (doingPasscodeLogin==YES) {
        return;
    } else {
        [self initSystemAlert];
    }
#endif
    
	NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
    //NSString *_tmp_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"tmp_user_preference"];
	NSString *_password = [[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"];
	NSString *_identity = [[NSUserDefaults standardUserDefaults] stringForKey:@"identity_preference"];
	NSString *_transport = [[NSUserDefaults standardUserDefaults] stringForKey:@"transport_preference"];
	NSString *_outboundproxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"outboundproxy_preference"];
	NSString *_stun = [[NSUserDefaults standardUserDefaults] stringForKey:@"stun_preference"];
    
	//NSLog(@"%@ vs %@", proxy, _proxy);
	NSLog(@"%@ vs %@", login, _login);
	NSLog(@"%@ vs %@", password, _password);
	//NSLog(@"%@ vs %@", identity, _identity);
    //NSLog(@"identity = %@",identity);
    //NSLog(@"_identity = %@",_identity);
	//NSLog(@"%@ vs %@", transport, _transport);
	//NSLog(@"%@ vs %@", outboundproxy, _outboundproxy);
	//NSLog(@"%@ vs %@", stun, _stun);
	
    /*(
    if(_tmp_login!=_login && _login!=nil){
        [self performSelectorOnMainThread : @ selector(updatetokeninfo) withObject:nil waitUntilDone:YES];
    }
    */
    //NSLog(@"------------------------ jump updatetokeninfo");
    [[NSUserDefaults standardUserDefaults] setObject:_login forKey:@"tmp_user_preference"];

    
	proxy = _proxy;
	login = _login;
	password = _password;
	identity = _identity;
	transport = _transport;
	outboundproxy = _outboundproxy;
	stun = _stun;
	ptime = [[NSUserDefaults standardUserDefaults] integerForKey:@"ptime_preference"];
	g729 = [[NSUserDefaults standardUserDefaults] boolForKey:@"g729_preference"];
	speex16k = [[NSUserDefaults standardUserDefaults] boolForKey:@"speex16k_preference"];
	g722 = [[NSUserDefaults standardUserDefaults] boolForKey:@"g722_preference"];
	speex8k = [[NSUserDefaults standardUserDefaults] boolForKey:@"speex8k_preference"];
	iLBC = [[NSUserDefaults standardUserDefaults] boolForKey:@"iLBC_preference"];
	gsm8k = [[NSUserDefaults standardUserDefaults] boolForKey:@"gsm8k_preference"];
	pcmu = [[NSUserDefaults standardUserDefaults] boolForKey:@"pcmu_preference"];
	pcma = [[NSUserDefaults standardUserDefaults] boolForKey:@"pcma_preference"];
	aec = [[NSUserDefaults standardUserDefaults] boolForKey:@"aec_preference"];
	elimiter = [[NSUserDefaults standardUserDefaults] boolForKey:@"elimiter_preference"];
	srtp = [[NSUserDefaults standardUserDefaults] integerForKey:@"srtp_preference"];
	naptr = [[NSUserDefaults standardUserDefaults] boolForKey:@"naptr_preference"];
	reginterval = [[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"];

	vp8 = [[NSUserDefaults standardUserDefaults] boolForKey:@"vp8_preference"];
	h264 = [[NSUserDefaults standardUserDefaults] boolForKey:@"h264_preference"];
	mp4v = [[NSUserDefaults standardUserDefaults] boolForKey:@"mp4v_preference"];
	h2631998 = [[NSUserDefaults standardUserDefaults] boolForKey:@"h2631998_preference"];
	
	uploadbandwidth = [[NSUserDefaults standardUserDefaults] integerForKey:@"uploadbandwidth_preference"];
	downloadbandwidth = [[NSUserDefaults standardUserDefaults] integerForKey:@"downloadbandwidth_preference"];
	
	[gAppEngine stop];
	[gAppEngine start];
	[gAppEngine addCallDelegate:self];
	[gAppEngine addRegistrationDelegate:self];
}


-(void)updatetokeninfo{

    
	NSString *_p_devicetoken = @"";
    if([[NSUserDefaults standardUserDefaults] stringForKey:@"deviceToken"]!=nil) _p_devicetoken = [[NSUserDefaults standardUserDefaults] stringForKey:@"deviceToken"];
	NSString *_p_user = @"";
    
    if([[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"]!=nil) _p_user = 
        [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
    
	//NSLog(@"%@ vs %@", _p_devicetoken, _p_user);
	// !!! CHANGE "/apns.php?" TO THE PATH TO WHERE apns.php IS INSTALLED
	// !!! ( MUST START WITH / AND END WITH ? ).
	// !!! SAMPLE: "/path/to/apns.php?"
	//NSString *urlString = [@"/RAPI/?"stringByAppendingString:@"Action=setPhonePIN"];

    
    
    NSString *urlAsString = [[[NSString alloc] initWithString:tokenRegisterAPIURL] autorelease];//absoluteString
    
    urlAsString = [urlAsString stringByAppendingString:[NSString stringWithFormat:tokenRegisterVar,_p_devicetoken,_p_user]];
    NSLog(@"---------------------  urlAsString:%@",urlAsString);
    NSURL *url = [NSURL URLWithString:urlAsString];
    
    
    self.connect_finished = NO;
    self.tmp_data = nil;
    
    
   // NSLog(@"########## connect finished:%@",self.connect_finished);
        
    
    //NSMutableURLRequest *urlRequest = [NSMutableURLRequest requestWithURL:url];
    NSMutableURLRequest *urlRequest = [[[NSMutableURLRequest alloc] initWithURL:url] autorelease]; 
    
    [urlRequest setTimeoutInterval:10.0f];
    //    [urlRequest setHTTPMethod:@"POST"];
    
    //NSLog(@"--------------------- REQUEST: %@", [urlRequest HTTPBody]);
    //[NSURLConnection connectionWithRequest:urlRequest delegate:self];
    [[[NSURLConnection alloc] initWithRequest:urlRequest delegate:self] autorelease];
    
    NSInteger try_count = 0;
    
    while(self.connect_finished == NO && try_count <10) {
       // NSLog(@"--------------------- login LOOP");
        
		[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        
        sleep(1);  
        try_count++;
	} 
    NSLog(@"--------------------- try_count:%i, send tmp:%@",try_count,self.tmp_data);
    //sleep(1);
    //NSLog(@"--------------------- updatetokeninfo end");
    
    //return;
    
}

-(void)onCallExist:(Call *)call {
}

-(void)onCallNew:(Call *)call {
	NSLog(@"vbyantisipAppDelegate: onCallNew");
	
	//if ([call isIncomingCall]==false)
	//{	
	//	vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
	//	if ([appDelegate.tabBarController selectedIndex]!=1)
	//	{
	//		[appDelegate.tabBarController setSelectedIndex: 1];
    //      UIViewControllerDialpad *lViewControllerDialpad = (UIViewControllerDialpad*)viewControllerDialpad;
	//		[lViewControllerDialpad pushCallControlList];
	//		return;
	//	}
	//}
#ifdef GENTRICE    
    if ([gAppEngine getNumberOfActiveCalls]>1||[gCallState isEqualToString:CTCallStateConnected])
	{
		[call decline];
		return;
	}
    
    NSString *displayName = [self lookupDisplayName:[self stripSecureIDfromURL:[call oppositeNumber]]];
#else
	if ([gAppEngine getNumberOfActiveCalls]>3)
	{
		[call decline];
		return;
	}
#endif
	
#ifdef MULTITASKING_ENABLED
	if ([[UIDevice currentDevice] isMultitaskingSupported]) {
		if (isInBackground==true)
		{
			// Create a new notification
			UIApplication* app = [UIApplication sharedApplication];
			UILocalNotification* alarm = [[[UILocalNotification alloc] init] autorelease];
			if (alarm)
			{
				alarm.fireDate = [[NSDate date] dateByAddingTimeInterval:0];;
				alarm.timeZone = [NSTimeZone defaultTimeZone];
				alarm.repeatInterval = 0;
				alarm.soundName = @"alarmsound.caf";//@"oldphone-mono-30s.caf";
                NSLog(@"------ local notify = %i",[[NSUserDefaults standardUserDefaults] integerForKey:@"IconBadgeNumber"]);    
                NSInteger iconbadge = [[NSUserDefaults standardUserDefaults] integerForKey:@"IconBadgeNumber"];
                if(iconbadge<=0) iconbadge = 1;    
                else iconbadge+=1;
                
                [[NSUserDefaults standardUserDefaults] setInteger:iconbadge forKey:@"IconBadgeNumber"];
                
                NSLog(@"------ local notify = %i",iconbadge);
                alarm.applicationIconBadgeNumber = iconbadge;
				NSString *body = [NSString stringWithFormat:@"%@ %@",NSLocalizedString(@"notifyCallFrom",nil), displayName];
				alarm.alertBody = body;
				alarm.alertAction = NSLocalizedString(@"callAnswer",nil);
            
				
				[app presentLocalNotificationNow:alarm];
			}
#ifdef GENTRICE
            //[self checkPasscodeLoginStatus:call];   
#endif
		}
	}
	
	//To be called later if call is not answered:
	//[[UIApplication sharedApplication] cancelAllLocalNotifications];
#endif
	//[viewControllerCallControl performSelectorOnMainThread : @ selector(setCallState:) withObject:[NSNumber numberWithInteger:-2] waitUntilDone:NO];
	
	//dead lock possible when we want to stop the amsipLoop
	//[self performSelectorOnMainThread : @ selector(showIncomingCallView) withObject:nil waitUntilDone:YES];
    
    NSLog(@"\n\n>>>>>onCallNew: isImcomingCall=%d, state=%d\n\n", [call isIncomingCall], [call callState]);
    
	if ([call isIncomingCall]==true)
	{
#if 1 //def GENTRICE
        if ([call callState] == RINGING) {
            
            if (currentAlert!=nil) {
                [currentAlert dismissWithClickedButtonIndex:[currentAlert cancelButtonIndex] animated:NO];
            }
            
            //added by arthur, 06062012
            dialIncomingUIViewController *dialView = [[dialIncomingUIViewController alloc] initWithNibName:@"dialIncomingUIViewController" bundle:nil];
            loginUINavigationController *dialNavView = [[loginUINavigationController alloc] initWithRootViewController:dialView];
            [dialNavView.navigationBar setHidden:YES];
            //[dialView setAutoAnswer:isInBackground];
            [dialView setAutoAnswer:false];
            [dialView setRemoteIdentity:displayName];
            [tabBarController presentModalViewController:dialNavView animated:YES];
            
            [dialView release];
            [dialNavView release];
        }
#else
        vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
        if ([appDelegate.tabBarController selectedIndex]!=1)
        {
            [appDelegate.tabBarController setSelectedIndex: 1];
        }
        UIViewControllerDialpad *_viewControllerDialpad = (UIViewControllerDialpad *)viewControllerDialpad;
        [_viewControllerDialpad pushCallControlList];

#endif //GENTRICE
    }
}


#import "SqliteContactHelper.h"

- (NSString*) lookupDisplayName:(NSString*) caller {
    NSString *result;
    
    NSString *secureID = [self stripSecureIDfromURL:caller];
    //NSLog(@"$$$$$ SecureID = %@", secureID);
    
    SqliteContactHelper *contactsDB = [SqliteContactHelper alloc];
    [contactsDB open_database];
    result = [contactsDB find_contact_name:secureID];
    [contactsDB release];
    if (result==nil) {
        result = secureID;
    }
    
    return result;
}



-(void)onCallRemove:(Call *)call {
	NSLog(@"vbyantisipAppDelegate: onCallRemove");
	//[self performSelectorOnMainThread : @ selector(showIncomingCallView) withObject:nil waitUntilDone:NO];
}

- (void)onRegistrationNew:(Registration*)_registration;
{
	registration = _registration;
}

- (void)onRegistrationRemove:(Registration*)_registration;
{
	registration = nil;

#ifdef GENTRICE
    [self dismissSystemAlert];
#endif
}

- (void)onRegistrationUpdate:(Registration*)_registration;
{
	if (_registration.rid!=_registration.rid)
	{
		NSLog(@"onRegistrationUpdate: <bug> 2 active registration with different rid pending");
	}
	registration = _registration;
    
#ifdef GENTRICE
    NSLog(@">>>>> AppDelegate: onRegistrationUpdate, code=%d", [_registration code]);

	if ([_registration code]>=200 && [_registration code]<=299) {
		//label_sipstatus.text = @"Registered on server";
        [self dismissSystemAlert];
	} else if ([_registration code]==0) {
		//label_sipstatus.text = [NSString stringWithFormat: @"--- %@", [registration reason]];
        //[aiv startAnimating];
	} else {
		//label_sipstatus.text = [NSString stringWithFormat: @"%i %@", [registration code], [registration reason]];
        //[aiv startAnimating];
	}
#endif
    
}




- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo  
{  
    NSLog(@"--------------> AppDelegate didReceiveRemoteNotification \n");
    /*
     // NSLog(@"收到推送消息 ：%@",[[objectForKey:@"aps"] objectForKey:@"alert"]);  
     if ([[userInfo objectForKey:@"aps"] objectForKey:@"alert"]!=NULL) {  
     UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"推送通知"   
     message:[[userInfo objectForKey:@"aps"] objectForKey:@"alert"]           
     delegate:self          
     cancelButtonTitle:@"关闭"       
     otherButtonTitles:@"更新状态",nil];  
     [alert show];  
     [alert release];  
     }
     */
    
#if !TARGET_IPHONE_SIMULATOR
    
	NSLog(@"remote notification: %@",[userInfo description]);
	NSDictionary *apsInfo = [userInfo objectForKey:@"aps"];
    
	NSString *alert = [apsInfo objectForKey:@"alert"];
	NSLog(@"Received Push Alert: %@", alert);
    
	NSString *sound = @"default";//[apsInfo objectForKey:@"sound"];
	NSLog(@"Received Push Sound: %@", sound);
    
    
    //AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
    //AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
	//NSString *badge = [apsInfo objectForKey:@"badge"];
	//NSLog(@"Received Push Badge: %@", badge);
    NSLog(@"------ remote notify = %i",[[NSUserDefaults standardUserDefaults] integerForKey:@"IconBadgeNumber"]);    
    NSInteger iconbadge = [[NSUserDefaults standardUserDefaults] integerForKey:@"IconBadgeNumber"];
    if(iconbadge<=0) iconbadge = 1;    
    else iconbadge+=1;
    
    [[NSUserDefaults standardUserDefaults] setInteger:iconbadge forKey:@"IconBadgeNumber"];

    NSLog(@"------ remote notify = %i",iconbadge);

	//application.applicationIconBadgeNumber = iconbadge;//[[apsInfo objectForKey:@"badge"] integerValue];
    
    //[UIApplication sharedApplication].applicationIconBadgeNumber = iconbadge;
    [UIApplication sharedApplication].applicationIconBadgeNumber = [UIApplication sharedApplication].applicationIconBadgeNumber - 1;

    /*
    for (id key1 in apsInfo){
        NSLog(@"key1: %@", key1);
        id alert = [apsInfo objectForKey:key1];
        if ([alert isKindOfClass:[NSDictionary class]]) {
            message = [alert objectForKey:@"body"];
            NSLog(@"body: %@, value: %@", key1, message);
            message = [alert objectForKey:@"loc-args"];
            NSLog(@"loc-args: %@, value: %@", key1, message);
            NSArray *args = (NSArray *) [alert objectForKey:@"loc-args"] ;
            for (id key2 in args){
                NSLog(@"key2: %@, value: ", key2);
            }
            message = [alert objectForKey:@"action-loc-key"];
            NSLog(@"action-loc-key: %@, value: %@", key1, message);
            
        }
        else if ([alert isKindOfClass:[NSArray class]]) {
            for (id key2 in key1){
                NSLog(@"key2: %@, value: %@", key2, [key1 objectForKey:key2]);
            }
        }
        else if([key1 isKindOfClass:[NSString class]]) {
            message = [apsInfo objectForKey:key1];
            NSLog(@"key1: %@, value: %@", key1, message);
        } 
        
    }*/   
    
    //[[[[[self tabBarController] tabBar] items] objectAtIndex:2] setBadgeValue:[NSString stringWithFormat:@"%@",badge]];
    //[[[[[self tabBarController] tabBar] items] objectAtIndex:20] setBadgeValue:badge];
    //[[[[[self tabBarController] tabBar] objectAtIndex:20] tabBarItem] setBadgeValue:badge];
#endif

    
    //UIAlertView *alert = [[UIAlertView alloc]initWithTitle:@"Super" message:@"welcome" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
    /*
     [alert show];
     [alert release];
     
     for (id key in userInfo) {
     
     
     NSLog(@"key: %@, value: %@", key, [userInfo objectForKey:key]);
     UIAlertView *myAlertView = [[UIAlertView alloc] initWithTitle:@"Your title here!" message:@"this gets covered" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"OK", nil];
     UITextField *myTextField = [[UITextField alloc] initWithFrame:CGRectMake(12.0, 45.0, 260.0, 25.0)];
     [myTextField setBackgroundColor:[UIColor whiteColor]];
     myTextField.text = [userInfo objectForKey:key];
     [myAlertView addSubview:myTextField];
     [myAlertView show];
     [myAlertView release];
     
     } */     
} 

/**
 * Fetch and Format Device Token and Register Important Information to Remote Server
 */
- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
    
    NSLog(@"AppDelegate didRegisterForRemoteNotificationsWithDeviceToken deviceToken: %@ \n", deviceToken);
#if !TARGET_IPHONE_SIMULATOR
    
	// Get Bundle Info for Remote Registration (handy if you have more than one app)
	/*
     NSString *appName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"];
     NSString *appVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
     */
	// Check what Notifications the user has turned on.  We registered for all three, but they may have manually disabled some or all of them.
	//NSUInteger rntypes = [[UIApplication sharedApplication] enabledRemoteNotificationTypes];
    
    
    
	// Get the users Device Model, Display Name, Unique ID, Token & Version Number
	/*
     UIDevice *dev = [UIDevice currentDevice];
     NSString *deviceUuid = dev.uniqueIdentifier;
     NSString *deviceName = dev.name;
     NSString *deviceModel = dev.model;
     NSString *deviceSystemVersion = dev.systemVersion;
     */
	// Prepare the Device Token for Registration (remove spaces and < >)
	
    
    NSString *deviceTokenStr = [[[[deviceToken description]
                                  stringByReplacingOccurrencesOfString:@"<"withString:@""]
                                 stringByReplacingOccurrencesOfString:@">"withString:@""]
                                stringByReplacingOccurrencesOfString:@" "withString:@""];
    NSLog(@"######## AppDelegate %@ \n",deviceTokenStr);
    

    NSString *_deviceTokenStr = [[NSUserDefaults standardUserDefaults] stringForKey:@"deviceToken"];
    if(_deviceTokenStr!=deviceTokenStr)
        [[NSUserDefaults standardUserDefaults] setObject:deviceTokenStr forKey:@"deviceToken"];
    
    
#endif
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
    // NSLog(@"Error in registration. Error: %@", error);  
#if !TARGET_IPHONE_SIMULATOR
    
	NSLog(@"Error in registration. Error: %@", error);
    
#endif
}   





- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse*)response 
{
    NSLog(@"############ connection didReceiveResponse response:%@\n",response);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection 
{
   // NSLog(@"connectionDidFinishLoading connection ");
    self.connect_finished = YES;
    //[_waitingDialog dismissWithClickedButtonIndex:0 animated:NO];
    
    //[connection release];
    //[DownloadConnection release];
}

-(void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{ 
    NSLog(@" ########### connection error:%@\n",error);
    self.connect_finished = NO;
    
}



- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection{
   // NSLog(@"connectionShouldUseCredentialStorage connection ");
    
    return NO;
}

#ifdef GENTRICE

//下面两段是重点，要服务器端单项HTTPS 验证，iOS 客户端忽略证书验证。
// 無論如何回傳 YES
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
    
    return YES;
    // return [protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust];
}

// 不管那一種 challenge 都相信
- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge{
    NSLog(@"received authen challenge");
    [challenge.sender useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] forAuthenticationChallenge:challenge];
}

/*
 // 不管那一種 challenge 都相信
 - (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {    
 NSLog(@"didReceiveAuthenticationChallenge %@ %zd", [[challenge protectionSpace] authenticationMethod], (ssize_t) [challenge previousFailureCount]);
 
 if ([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]){
 [[challenge sender]  useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] forAuthenticationChallenge:challenge];
 [[challenge sender]  continueWithoutCredentialForAuthenticationChallenge: challenge];
 }
 } */

//处理数据 
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    NSString *data_tmp = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    NSLog(@"######### connection didReceiveData:%@",data_tmp);
    self.tmp_data = data_tmp;
    [data_tmp release];
    //    [[NSUserDefaults standardUserDefaults] setObject:data_tmp forKey:@"p_connect_data_tmp"];
    //[data_tmp release];
} 


#import "passcodeUIViewController.h"
#import "loginUIViewController.h"
#import "loginUINavigationController.h"

- (void) checkPasscodeLoginStatus:(Call *) call
{

    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] == YES) {
        NSLog(@"-------- PasscodeEnabled YES");        
        passcodeUIViewController *passcodeView = [[passcodeUIViewController alloc] initWithNibName:@"passcode" bundle:nil];
        loginUINavigationController *passcodeNavView = [[loginUINavigationController alloc] initWithRootViewController:passcodeView];
        [tabBarController presentModalViewController:passcodeNavView animated:NO];
        //[passcodeView setCurrent_call:call];
        [passcodeView release];
        [passcodeNavView release];        
    } else if ([[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"] == NO) {
        
        NSLog(@"-------- LoginStatus NO");           
        UIViewController *loginView = [[loginUIViewController alloc] initWithNibName:@"loginUIViewController" bundle:nil];
        loginUINavigationController *loginNavView = [[loginUINavigationController alloc] initWithRootViewController:loginView];
        [loginNavView.navigationBar setHidden:YES];
        [tabBarController presentModalViewController:loginNavView animated:NO];
        [loginView release];
        [loginNavView release];
    }
    
}



- (void)saveContext
{
    NSError *error = nil;
    NSManagedObjectContext *managedObjectContext = self.managedObjectContext;
    if (managedObjectContext != nil)
    {
        if ([managedObjectContext hasChanges] && ![managedObjectContext save:&error])
        {
            /*
             Replace this implementation with code to handle the error appropriately.
             
             abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development. 
             */
            NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
            abort();
        } 
    }
    //sleep(1);
}

#pragma mark - Core Data stack

/**
 Returns the managed object context for the application.
 If the context doesn't already exist, it is created and bound to the persistent store coordinator for the application.
 */
- (NSManagedObjectContext *)managedObjectContext
{
    if (__managedObjectContext != nil)
    {
        return __managedObjectContext;
    }
    
    NSPersistentStoreCoordinator *coordinator = [self persistentStoreCoordinator];
    if (coordinator != nil)
    {
        __managedObjectContext = [[NSManagedObjectContext alloc] init];
        [__managedObjectContext setPersistentStoreCoordinator:coordinator];
    }
    return __managedObjectContext;
}

/**
 Returns the managed object model for the application.
 If the model doesn't already exist, it is created from the application's model.
 */
- (NSManagedObjectModel *)managedObjectModel
{
    if (__managedObjectModel != nil)
    {
        return __managedObjectModel;
    }
    NSURL *modelURL = [[NSBundle mainBundle] URLForResource:@"appdata" withExtension:@"momd"];
    __managedObjectModel = [[NSManagedObjectModel alloc] initWithContentsOfURL:modelURL];
    
    id x = self.persistentStoreCoordinator.managedObjectModel.entities;
    for (id y in x) {
        NSLog(@"############# entity name: %@", [y name]);
    }
    
    return __managedObjectModel;
}

/**
 Returns the persistent store coordinator for the application.
 If the coordinator doesn't already exist, it is created and the application's store added to it.
 */
- (NSPersistentStoreCoordinator *)persistentStoreCoordinator
{
    if (__persistentStoreCoordinator != nil)
    {
        return __persistentStoreCoordinator;
    }
    
    NSURL *storeURL = [[self applicationDocumentsDirectory] URLByAppendingPathComponent:@"appdata.sqlite"];
    
    NSError *error = nil;
    __persistentStoreCoordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:[self managedObjectModel]];
    if (![__persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:storeURL options:nil error:&error])
    {
        
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    }    
    
    return __persistentStoreCoordinator;
}

#pragma mark - Application's Documents directory

/**
 Returns the URL to the application's Documents directory.
 */
- (NSURL *)applicationDocumentsDirectory
{
    return [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
}



- (void)createPersistenStoreCoordinator {
    NSURL *storeURL = [[self applicationDocumentsDirectory] URLByAppendingPathComponent:@"appdata.sqlite"];
    NSLog(@"URL: %@", [storeURL path]);
    
    NSError *error = nil;
    __persistentStoreCoordinator = [[NSPersistentStoreCoordinator alloc] initWithManagedObjectModel:[self managedObjectModel]];
    if (![__persistentStoreCoordinator addPersistentStoreWithType:NSSQLiteStoreType configuration:nil URL:storeURL options:nil error:&error])
    {
        NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
        abort();
    }    
    
    __managedObjectContext = nil;
}

#endif //GENTRICE

- (NSString*) stripSecureIDfromURL:(NSString*) targetString {
    NSString *result = [[[NSString alloc] initWithString:targetString] autorelease];
    
    NSRange strRange = [targetString rangeOfString:@"sip:"];
    //NSLog(@">>>>> targetString = %@", result);
    if (strRange.location != NSNotFound) {
        result = [result substringFromIndex:strRange.location+strRange.length];
        //NSLog(@">>>>> targetString1 = %@", result);
        strRange = [result rangeOfString:@"@"];
        if (strRange.location!=NSNotFound) {
            result = [result substringToIndex:strRange.location];
            //NSLog(@">>>>> targetString2 = %@", result);
        }
    }
    
    return result;
}


- (void)dealloc {
    [tabBarController release];
    [window release];
    [super dealloc];
}

@end

