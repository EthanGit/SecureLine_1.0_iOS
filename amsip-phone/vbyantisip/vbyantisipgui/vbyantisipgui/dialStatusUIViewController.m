//
//  dialStatusUIViewController.m
//  vbyantisipgui
//
//  Created by Arthur Tseng on 2012/5/30.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "dialStatusUIViewController.h"
#import "callControlUIViewController.h"
#import <CoreTelephony/CTCall.h>
#import <CoreTelephony/CTCallCenter.h>

@implementation dialStatusUIViewController
@synthesize endCallUIButton;
@synthesize actionStateUILabel;
@synthesize callToUILabel;
@synthesize UILabelEndCall;
@synthesize lStatusUILabel;
@synthesize lineQualityUILabel;
@synthesize redialUIButton;
@synthesize redialUILabel;

NSString *secureID;
CTCallCenter *gCallCenter;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
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
    
    [UILabelEndCall setText:NSLocalizedString(@"callEndCall", nil)];
    [lStatusUILabel setText:NSLocalizedString(@"qtyTitle", @"line status")];
    [lineQualityUILabel setText:NSLocalizedString(@"qtyDialing", nil)];
    [actionStateUILabel setText:NSLocalizedString(@"callDialing", nil)];
    
    gCallCenter = [[[CTCallCenter alloc] init] autorelease];
    
    gCallCenter.callEventHandler = ^(CTCall *call) {
        if ([call.callState isEqualToString:CTCallStateIncoming] && current_call!=nil) {
            if ([current_call callState]>=TRYING && [current_call callState]!=DISCONNECTED) {
                [current_call hangup];
            }
        }
    };
    
}

- (void)viewDidUnload
{
    [self setEndCallUIButton:nil];
    [self setActionStateUILabel:nil];
    [self setCallToUILabel:nil];
    [self setUILabelEndCall:nil];
    [self setLStatusUILabel:nil];
    [self setLineQualityUILabel:nil];
    [self setRedialUIButton:nil];
    [self setRedialUILabel:nil];
    secureID = nil;
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
    [endCallUIButton release];
    [actionStateUILabel release];
    [callToUILabel release];
    [UILabelEndCall release];
    [lStatusUILabel release];
    [lineQualityUILabel release];
    [redialUIButton release];
    [redialUILabel release];
    //if (secureID!=nil) {
    //    [secureID release];
    //}
    
    [super dealloc];
}


- (void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [gAppEngine addCallDelegate:self];
}

-(void) viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];

#if 1
    if (current_call!=nil) {
        [self setCurrentCall:nil];
    }
    [gAppEngine removeCallDelegate:self];
#endif
    
    [redialUIButton setHidden:YES];
    [redialUILabel setHidden:YES];
}



- (IBAction)endCallButtonPressed:(id)sender {
    NSLog(@"########## endCallButtonPressed... state=%d", (current_call!=nil)? [current_call callState]:-1);
#if 0
    //[self.navigationController.navigationBar setHidden:NO];
    //[self.navigationController popViewControllerAnimated:YES];
    if (current_call!=nil&& ([current_call callState]>=TRYING && [current_call callState]!=DISCONNECTED)) {
        [current_call hangup];
        [gAppEngine removeCallDelegate:self];
        if (current_call!=nil) {
            [self setCurrentCall:nil];
        }
    }
    [self.parentViewController dismissModalViewControllerAnimated:YES];
#else
    if (current_call!=nil && ([current_call callState]>=TRYING && [current_call callState]!=DISCONNECTED))
	{
        NSLog(@"++++++ return code = %d", [current_call hangup]);
		//[current_call hangup];
        //actionStateUILabel.text = @"Disconnecting...";
        [actionStateUILabel setText:NSLocalizedString(@"callDisconnecting", nil)];
        //sleep(1);
        //[self setCurrentCall:nil];
	} else {//if (current_call == nil) {
        if (current_call!=nil) {
            [current_call hangup];
        }
        [self.parentViewController dismissModalViewControllerAnimated:YES];
    }
#endif
}

- (IBAction)redialButtonPressed:(id)sender {
    NSLog(@"########## redialButtonPressed... state=%d", (current_call!=nil)? [current_call callState]:-1);
    
    if ([gAppEngine getNumberOfActiveCalls]>3 || secureID == nil) {
        [actionStateUILabel setText:NSLocalizedString(@"wait...", nil)];
        return;
    }
    
    [actionStateUILabel setText:NSLocalizedString(@"callDialing", nil)];
    
    int res = [gAppEngine amsip_start:secureID withReferedby_did:0];
    //[secureID release];
    secureID = nil;
    
    if (res<0) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Network Error", @"Syntax Error") 
                                                        message:NSLocalizedString(@"Redial error, try later.", @"Check syntax of your callee sip url.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
    } else {
        [redialUIButton setHidden:YES];
        //[redialUILabel setText:NSLocalizedString(@"callRedial", nil)];
        [redialUILabel setHidden:YES];
    }
    
    
}

- (void)setRemoteIdentity:(NSString*)str{
    NSLog(@"^^^^^ identity=%@", str);
	[callToUILabel setText:str];
}


#pragma mark - EngineDelegate & CallDelegate

- (void)setCurrentCall:(Call*)call
{
	if (current_call==call)
		return;
	
	if (call==nil)
	{
		if (current_call!=nil)
			[current_call removeCallStateChangeListener:self];

		current_call=nil;
		return;
	}
	
	//reset
	if (current_call!=nil)
	{
		[current_call removeCallStateChangeListener:self];
		current_call=nil;
	}
	
	current_call=call;
    //[callToUILabel setText:[self stripSecureIDfromURL:[call to]]];
	if (current_call!=nil)
	{
		//[self onCallUpdate:call];
		[current_call addCallStateChangeListener:self];
	}
}



-(void)onCallExist:(Call *)call {
    NSLog(@">>>>> dialStatusUIViewController: onCallExist");
	//[self onCallNew:call];
}


#import "dialIncomingUIViewController.h"
#import "loginUINavigationController.h"

-(void)onCallNew:(Call *)call {
	NSLog(@">>>>> dialStatusUIViewController: onCallNew");
    
    if ([call callState] == RINGING && secureID!=nil) { //incoming call
        if (current_call==nil && [call isIncomingCall] == true) {
#if 1
            //[self.parentViewController dismissModalViewControllerAnimated:YES];
            dialIncomingUIViewController *dialView = [[dialIncomingUIViewController alloc] initWithNibName:@"dialIncomingUIViewController" bundle:nil];
            [self.navigationController.navigationBar setHidden:YES];
            [self.navigationController pushViewController:dialView animated:YES];
            //[dialView setRemoteIdentity:[self lookupDisplayName:[call from]]];
            //[dialView setRemoteIdentity:nil];
            [dialView release];
#else
            dialIncomingUIViewController *dialView = [[dialIncomingUIViewController alloc] initWithNibName:@"dialIncomingUIViewController" bundle:nil];
            loginUINavigationController *dialNavView = [[loginUINavigationController alloc] initWithRootViewController:dialView];
            [dialNavView.navigationBar setHidden:YES];
            [self.tabBarController presentModalViewController:dialNavView animated:YES];
            
            [dialView release];
            [dialNavView release];
#endif
        } else {
            if ([call isIncomingCall]==true) {
                NSLog(@"discard incoming call... from %@", [call from]);
                [call decline];
            } else {
                [self setCurrentCall:call];
                secureID = [[[NSString alloc] initWithString:[call to]] autorelease];
            }
        }
    } else {
        if (current_call==nil) {
            [self setCurrentCall:call];
            //secureID = [[NSString alloc] initWithString:[self stripSecureIDfromURL:[call to]]];
            secureID = [[[NSString alloc] initWithString:[call to]] autorelease];
        }
    }
}


//#import "callControlUIViewController.h"

- (void)onCallUpdate:(Call *)call {
	NSLog(@">>>>> dialStatusUIViewController: onCallUpdate");
    
    if (call!=current_call) {
        NSLog(@">>>>> dialStatusUIViewController: not current call %@ %@", call, current_call);
        return;
    }
   
    if ([current_call callState]==CONNECTED)
	{
        
        callControlUIViewController *detailView = [[callControlUIViewController alloc] initWithNibName:@"callControl" bundle:nil];
        
        if (detailView!=nil) {
            [detailView setCurrentCall:current_call];
            [self.navigationController pushViewController:detailView animated:YES];
            [detailView setRemoteIdentity:[callToUILabel text]];
            [detailView release];
            //[detailView autorelease];
        } else {
            NSLog(@">>>> Warning, can't allocate new view controller!");
        }
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


-(void)onCallRemove:(Call *)call {
	NSLog(@">>>>>>>> dialStatusUIViewController: onCallRemove");
	
    if (call==current_call) {
        NSLog(@">>>>>>>> dialStatusUIViewController: remove current call.");
        [self setCurrentCall:nil];
        [redialUIButton setHidden:NO];
        [redialUILabel setText:NSLocalizedString(@"callRedial", nil)];
        [redialUILabel setHidden:NO];
        //[self.parentViewController dismissModalViewControllerAnimated:YES];
        //[dialControlView doViewExit];
        //dialControlView = nil;
    }

}

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

@end
