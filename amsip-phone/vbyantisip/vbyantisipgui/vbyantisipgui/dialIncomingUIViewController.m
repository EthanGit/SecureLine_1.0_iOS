//
//  dialIncomingUIViewController.m
//  vbyantisipgui
//
//  Created by Arthur Tseng on 2012/5/30.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "dialIncomingUIViewController.h"

@implementation dialIncomingUIViewController
@synthesize callStateUILable;
@synthesize actionStateUILabel;
@synthesize answerUIButton;
@synthesize endCallUIButton;
@synthesize callerUILabel;
@synthesize UILabelEndCall;
@synthesize UILabelAnswer;
@synthesize lStatusUILabel;
@synthesize lineQualityUILabel;
@synthesize autoAnswer;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        //callStateUILable.text = @"Ringing";
        //actionStateUILabel.text = @"Incoming call...";
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    [UILabelAnswer setText:NSLocalizedString(@"callAnswer", nil)];
    [UILabelEndCall setText:NSLocalizedString(@"callEndCall", nil)];
    [lStatusUILabel setText:NSLocalizedString(@"qtyTitle", @"line status")];
    [lineQualityUILabel setText:NSLocalizedString(@"qtyGood", nil)];
    [actionStateUILabel setText:NSLocalizedString(@"callIncoming", nil)];
}

- (void)viewDidUnload
{
    [self setAnswerUIButton:nil];
    [self setEndCallUIButton:nil];
    [self setCallStateUILable:nil];
    [self setActionStateUILabel:nil];
    [self setCallerUILabel:nil];
    [self setUILabelEndCall:nil];
    [self setUILabelAnswer:nil];
    [self setLStatusUILabel:nil];
    [self setLineQualityUILabel:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)dealloc {
    [answerUIButton release];
    [endCallUIButton release];
    [callStateUILable release];
    [actionStateUILabel release];
    [callerUILabel release];
    [UILabelEndCall release];
    [UILabelAnswer release];
    [lStatusUILabel release];
    [lineQualityUILabel release];
    [super dealloc];
}


- (void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [gAppEngine addCallDelegate:self];
    
    //turn on speaker (for ring tone)
    UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
    AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
                             sizeof (audioRouteOverride),
                             &audioRouteOverride);
}

-(void) viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [self setCurrentCall:nil];
    [gAppEngine removeCallDelegate:self];
    
    //turn off speaker (for ring tone)
    UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;
    AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
                             sizeof (audioRouteOverride),
                             &audioRouteOverride);
}

-(void) viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    if (autoAnswer == true) {
        [self answerButtonPressed:self];
    }
    
}


#import "callControlUIViewController.h"

- (IBAction)answerButtonPressed:(id)sender {
    
#if 0//test video preview
    UIActionSheet *shareType = [[UIActionSheet alloc]initWithTitle:	@"Actions"
                                                          delegate:	self
                                                 cancelButtonTitle:	@"Cancel"
                                            destructiveButtonTitle:	nil
                                                 otherButtonTitles: @"Start Video",
                                @"Speaker mode",
                                nil];
    [shareType showFromTabBar:parent.navigationController.tabBarController.tabBar];
    [shareType release];
    return;
#endif
    
	if (current_call!=nil && [current_call callState]>=TRYING && [current_call callState]<CONNECTED)
	{
		[current_call accept:200];
        
        //sleep(1);
        
        callControlUIViewController *detailView = [[callControlUIViewController alloc] initWithNibName:@"callControl" bundle:nil];
        
        if (detailView!=nil) {
            [detailView setCurrentCall:current_call];
            [self.navigationController pushViewController:detailView animated:YES];
            [detailView setRemoteIdentity:[callerUILabel text]];
            [detailView release];
            //[detailView autorelease];
        } else {
            NSLog(@">>>> Warning, can't allocate new view controller!");
        }
	}
	
    
    
}



- (IBAction)endCallButtonPressed:(id)sender {
    if (current_call!=nil && [current_call callState]>=TRYING && [current_call callState]!=DISCONNECTED)
	{
		[current_call hangup];
        //actionStateUILabel.text = @"Disconnecting...";
        [actionStateUILabel setText:NSLocalizedString(@"callDisconnecting", nil)];
        [self setCurrentCall:nil];
        sleep(1);
	}
    [self.parentViewController dismissModalViewControllerAnimated:YES];
}


- (void)setRemoteIdentity:(NSString*)str{
    NSLog(@"^^^^^ identity=%@", str);
	[callerUILabel setText:str];
}



- (void)setCurrentCall:(Call*)call
{
	if (current_call==call)
		return;
	
	if (call==nil)
	{
		if (current_call!=nil)
			[current_call removeCallStateChangeListener:self];
#if 0        
		[label_status setText:@"no connection"];
		[label_duration setText:@"-"];
		[label_rate setText:@""];
		
		if (myTimer!=nil)
		{
			[myTimer invalidate];
			myTimer=nil;
		}
		[self setRate:nil];
#endif
		current_call=nil;
		return;
	}
	
	//reset
	if (current_call!=nil)
	{
		[current_call removeCallStateChangeListener:self];
		current_call=nil;
	}
#if 0
	if (myTimer!=nil)
	{
		[myTimer invalidate];
		myTimer=nil;
	}
#endif
	
	current_call=call;
    if ([callerUILabel.text isEqualToString:@"User Name"]) {
        [callerUILabel setText:[self lookupDisplayName:[call from]]];
    }
	if (current_call!=nil)
	{
		//[self onCallUpdate:call];
		[current_call addCallStateChangeListener:self];
	}
}


-(void)onCallExist:(Call *)call {
	[self onCallNew:call];
}


-(void)onCallNew:(Call *)call {
	NSLog(@">>>>> dialIncomingUIViewController: onCallNew");
    
    if (current_call==nil) {
        [self setCurrentCall:call];
        if ([call isIncomingCall]==true && [call callState]==RINGING) {
            NSLog(@">>>>> Accept:180");
            [call accept:180];
        }
    }
}



#if 1
-(void)onCallRemove:(Call *)call {
	NSLog(@">>>>>>>> dialIncomingUIViewController: onCallRemove");
	
    if (call==current_call) {
        NSLog(@">>>>>>>> remove current call.");
        [self setCurrentCall:nil];
        sleep(1);
        [self.parentViewController dismissModalViewControllerAnimated:YES];
        //[self.navigationController popViewControllerAnimated: YES];
    }
    
    //[self.parentViewController dismissModalViewControllerAnimated:YES];
	
	//if ([call_list count]==0)
	//{
	//	[self.navigationController popViewControllerAnimated: YES];
	//}
}
#endif


- (void)onCallUpdate:(Call *)call {
	NSLog(@">>>>> dialIncomingUIViewController: onCallUpdate");
    
    if (call!=current_call) {
        return;
    }
    
    if ([current_call callState]==RINGING)
	{
        NSLog(@">>>> callState == RINGING");

	} else if ([current_call callState]==DISCONNECTED){
#if 0
        if (myTimer!=nil)
        {
            [myTimer invalidate];
            myTimer=nil;
        }
        if (endDate==nil)
        {
            endDate = [[NSDate alloc] init];
            [current_call setEnd_date:endDate];
            NSLog(@">>>>>endDate=%@", endDate);
            
        }
#endif
        [UILabelAnswer setHidden:YES];
        [answerUIButton setHidden:YES];
        
        if ([current_call code]>=200 && [current_call code]<=299)
            actionStateUILabel.text = NSLocalizedString(@"callDisconnected", nil);//@"disconnected";
        else if ([current_call reason]!=nil) {
            switch ([current_call code]) {
                case 486:
                case 603:
                    [actionStateUILabel setText:NSLocalizedString(@"callBusy", nil)];
                    break;
                case 404:
                    [actionStateUILabel setText:NSLocalizedString(@"callUserNotFound", nil)];
                    break;
                case 180: //Ringing
                case 487:
                    [actionStateUILabel setText:NSLocalizedString(@"callDisconnected", nil)];
                    break;
                default:
                    actionStateUILabel.text = [NSString stringWithFormat: @"%i %@", [current_call code], [current_call reason]];
                    break;
            }
            
        } else
            actionStateUILabel.text = NSLocalizedString(@"callDisconnected", nil);//@"disconnected";
        
    }
    
    
}


#if 1
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


#import "SqliteContactHelper.h"

- (NSString*) lookupDisplayName:(NSString*) caller {
    NSString *result;
    
    NSString *secureID = [self stripSecureIDfromURL:caller];
    NSLog(@"$$$$$ SecureID = %@", secureID);
    
    SqliteContactHelper *contactsDB = [SqliteContactHelper alloc];
    [contactsDB open_database];
    result = [contactsDB find_contact_name:secureID];
    
    if (result==nil) {
        result = secureID;
    }
    
    return result;
}

#endif


@end
