#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "callControlUIViewController.h"
#import "UIViewControllerVideoCallControl.h"
#import "UIViewControllerDTMFControl.h"

#import "vbyantisipAppDelegate.h"

#import <CoreTelephony/CTCall.h>
#import <CoreTelephony/CTCallCenter.h>

#define SPEAKER_ON      YES
#define SPEAKER_OFF     NO

@implementation callControlUIViewController

@synthesize parent;
@synthesize UILabelEndCall;
@synthesize lStatusUILabel;
@synthesize lineQualityUILabel;


int speaker, contacts;
CTCallCenter *gCallCenter;


// Load the view nib and initialize the pageNumber ivar.
- (id)initWithPageNumber:(int)page {
    if (self = [super initWithNibName:@"callControl" bundle:nil]) {
        pageNumber = page;
    }
    return self;
}


- (IBAction)actionButtonhang:(id)sender {
    
    if (current_call==nil) {
        //better leave here soon
        [self.parentViewController dismissModalViewControllerAnimated:YES];
    } else if ([current_call callState]>=TRYING && [current_call callState]!=DISCONNECTED) {
#if 1
        if (endDate==nil) {
            endDate = [[NSDate alloc] init];
            [current_call setEnd_date:endDate];
            NSLog(@">>>>> actionButtonhang: endDate=%@", endDate);
            [endDate release];
            endDate = nil;
        }
#endif
		[current_call hangup];
        [self onCallRemove:current_call];
	}
}

- (IBAction)actionButtonhold:(id)sender {

	if (current_call!=nil) {
		if ([current_call isOnhold]==true) {
			[current_call unhold];
		}
		else {
			[current_call hold];
		}
	}
}

- (IBAction)actionButtonmute:(id)sender {
	if (current_call!=nil)
		if ([current_call isMuted]==true) {
			[current_call unmute];
			//[current_call record_stop];
            [buttonmute setImage:[UIImage imageNamed:@"button-mute.png"] forState:UIControlStateNormal];
		}
		else {
			[current_call mute];
            [buttonmute setImage:[UIImage imageNamed:@"button-mute(active).png"] forState:UIControlStateNormal];
		}
}

- (void)actionButtonspeaker {

	if (current_call!=nil && [current_call callState]>=TRYING && [current_call callState]<CONNECTED)
	{
	}
	else if (current_call!=nil && [current_call callState]==CONNECTED) {
		static int speaker=0;
		if (speaker==0)
		{
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
									 sizeof (audioRouteOverride),
									 &audioRouteOverride);
			speaker=1;
		}
		else
		{
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
									 sizeof (audioRouteOverride),
									 &audioRouteOverride);
			speaker=0;
		}
	}
}

- (void)actionSheet:(UIActionSheet *)modalView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    // Change the navigation bar style, also make the status bar match with it
	switch (buttonIndex)
	{
		case 0:
		{
			UIViewControllerVideoCallControl *controller = [[UIViewControllerVideoCallControl alloc] initWithNibName:@"VideoCallControlView" bundle:[NSBundle mainBundle]];
			[parent.navigationController pushViewController:controller animated:YES];	
			[controller release];
			controller=nil;																																																													
			break;
		}
		case 1:
		{
			[self actionButtonspeaker];
			break;
		}
		case 2:
		{
			break;
		}
	}
}

- (IBAction)actionButtonother:(id)sender {
	
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
	}
	else if (current_call!=nil && [current_call callState]==CONNECTED) {
		UIActionSheet *shareType = [[UIActionSheet alloc]initWithTitle:	@"Actions"
															  delegate:	self
													 cancelButtonTitle:	@"Cancel"
												destructiveButtonTitle:	nil
													 otherButtonTitles: @"Start Video",
									@"Speaker mode",
									nil];
		[shareType showFromTabBar:parent.navigationController.tabBarController.tabBar];
		[shareType release];
	}
}

- (IBAction)actionButtonDTMF:(id)sender {
    NSLog(@"actionButtonDTMF sendDTMF 1");
    
    if (current_call!=nil && [current_call callState]==CONNECTED) {	
        UIViewControllerDTMFControl *controller = [[UIViewControllerDTMFControl alloc] initWithNibName:@"DTMF" bundle:[NSBundle mainBundle]];
        [controller setCurrent_call_dtmf:current_call];
        [parent.navigationController pushViewController:controller animated:YES];	
        [controller release];
        controller=nil;		
    }
}


//#import "contactUIViewController.h"
#import "ContactsViewController.h"

- (IBAction)contactButtonPressed:(id)sender {
    
    if (contacts==0) {
        [contactUIButton setImage:[UIImage imageNamed:@"button-contacts(active).png"] forState:UIControlStateNormal];
#if 0
        contactUIViewController *contactView = [[contactUIViewController alloc] initWithNibName:@"contactUIViewController" bundle:nil];
        [contactView setIsReadyOnly:YES];
        [self.navigationController.navigationBar setHidden:NO];
        [self.navigationController pushViewController:contactView animated:YES];
        [contactView release];
#else
        ContactsViewController *contactView = [[ContactsViewController alloc] initWithNibName:@"ContactsViewController" bundle:nil];
        [contactView setIsReadyOnly:YES];
        [self.navigationController.navigationBar setHidden:NO];
        [self.navigationController pushViewController:contactView animated:YES];
        [contactView release];
#endif
        contacts = 1;
    }
}


- (void)setRemoteIdentity:(NSString*)str{
    //NSLog(@"^^^^^ identity=%@", str);
	[label_num setText:str];
}

-(void)updateRate
{
	if (current_call==nil)
		return;
	
	float upload_rate = [current_call getUploadBandwidth];
	float download_rate = [current_call getDownloadBandwidth];
	if (upload_rate!=0 || download_rate!=0)
	{
		NSString *str = [NSString stringWithFormat:@"rate up=%.1fKb/s, down=%.1fKb/s",
						 upload_rate*1e-3,
						 download_rate*1e-3];
		[self setRate:str];
	}
}

- (void)setRate:(NSString*)rate {
	if (rate!=nil)
		[label_rate setText: rate];
	
	if (current_call==nil)
		return;
	
	//[self printProcessInfo];
	if (startDate!=nil)
	{
		[NSDateFormatter setDefaultFormatterBehavior:NSDateFormatterBehavior10_4];
		NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
		[dateFormatter setDateStyle:kCFDateFormatterNoStyle];
		[dateFormatter setTimeStyle:kCFDateFormatterMediumStyle];
		NSDate *now_date = [[NSDate alloc] init];
		NSTimeInterval elapsedTime  = [now_date timeIntervalSinceDate:startDate];
		NSString *str;
#if 0
		if ((int)(elapsedTime / 3600) > 0)
			str = [NSString stringWithFormat:@"Duration: %02d:%02d:%02d", (int)(elapsedTime / 3600), ((int)elapsedTime % 3600) / 60, (int)elapsedTime % 60];
		else
			str = [NSString stringWithFormat:@"Duration: %02d:%02d", ((int)elapsedTime % 3600) / 60, (int)elapsedTime % 60];
#else
        str = [NSString stringWithFormat:@"%02d:%02d:%02d", (int)(elapsedTime / 3600), ((int)elapsedTime % 3600) / 60, (int)elapsedTime % 60];
#endif
        
		[now_date release];
		[label_duration setText: str];
		[dateFormatter release];
	}
}

- (void)setCurrentCall:(Call*)call
{
	if (current_call==call)
		return;
	
	if (call==nil)
	{
		if (current_call!=nil)
			[current_call removeCallStateChangeListener:self];

		[label_status setText:@"no connection"];
		[label_duration setText:@"-"];
		[label_rate setText:@""];
		
		if (myTimer!=nil)
		{
			[myTimer invalidate];
			myTimer=nil;
		}
		[self setRate:nil];
		current_call=nil;
		return;
	}
	
	//reset
	if (current_call!=nil)
	{
		[current_call removeCallStateChangeListener:self];
		current_call=nil;
	}
	if (myTimer!=nil)
	{
		[myTimer invalidate];
		myTimer=nil;
	}
	
	current_call=call;
	if (current_call!=nil)
	{
		[self onCallUpdate:call];
		[current_call addCallStateChangeListener:self];
	}
}

- (IBAction)actionButtonSpeakerPressed:(id)sender {
    
    if (current_call!=nil && [current_call callState]==CONNECTED) {
		//static int speaker=0;
		if (speaker==0){
            [self switchSpeaker:SPEAKER_ON];
		} else {            
            [self switchSpeaker:SPEAKER_OFF];
		}
	}
}

- (void)startImageAnimation {
  if ([image_animation isAnimating])
    return;
  
  [image_animation setAnimationDuration:1];
	image_animation.contentMode = UIViewContentModeCenter;
	[image_animation startAnimating];  
}

- (void)stopImageAnimation {
  if ([image_animation isAnimating])
    [image_animation stopAnimating];  
}

- (void)onCallUpdate:(Call *)call {
	NSLog(@">>>>>>>> callControlUIViewController: onCallUpdate");
	if (current_call==call)
	{
		//[self setRemoteIdentity:[call oppositeNumber]];
		
		if ([current_call callState]==NOTSTARTED) {
			[label_status setText:@"please wait..."];
			
			if ([current_call isIncomingCall]==true)
			{
				[buttonother setBackgroundColor:[UIColor colorWithRed:44.0/255.0 green:147.0/255.0 blue:26.0/255.0 alpha:0.87]];
				[buttonother setTitle:@"answer" forState:UIControlStateNormal];
 			} else {
				[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
				[buttonother setTitle:@"tool" forState:UIControlStateNormal];
			}
      //[self startImageAnimation];

		} else if ([current_call callState]==TRYING){
			if ([current_call isIncomingCall]==true)
			{
				[label_status setText:@"incoming call"];
				[buttonother setBackgroundColor:[UIColor colorWithRed:44.0/255.0 green:147.0/255.0 blue:26.0/255.0 alpha:0.87]];
				[buttonother setTitle:@"answer" forState:UIControlStateNormal];
 			} else {
        if ([current_call reason]!=nil)
          label_status.text = [NSString stringWithFormat: @"%i %@", [current_call code], [current_call reason]];
        else {
          [label_status setText:@"trying"];
        }
				[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
				[buttonother setTitle:@"tool" forState:UIControlStateNormal];
			}

      //[self startImageAnimation];
		} else if ([current_call callState]==RINGING){
			
			if ([current_call isIncomingCall]==true)
			{
				[label_status setText:@"incoming call"];
				[buttonother setBackgroundColor:[UIColor colorWithRed:44.0/255.0 green:147.0/255.0 blue:26.0/255.0 alpha:0.87]];
				[buttonother setTitle:@"tool" forState:UIControlStateNormal];
 			} else {
        if ([current_call reason]!=nil)
          label_status.text = [NSString stringWithFormat: @"%i %@", [current_call code], [current_call reason]];
        else {
          [label_status setText:@"ringing"];
        }

				[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
				[buttonother setTitle:@"tool" forState:UIControlStateNormal];
			}
			
      //[self startImageAnimation];
		} else if ([current_call callState]==CONNECTED){
			if (startDate==nil)
			{
				startDate = [[NSDate alloc] init];
                NSLog(@">>>>>startDate=%@", startDate);
			}
			//[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
			//[buttonother setTitle:@"tool" forState:UIControlStateNormal];
			
			//[label_status setText:@"established"];
            [label_status setText:NSLocalizedString(@"callConnected", nil)];
			if (myTimer==nil)
			{
				myTimer = [NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(updateRate) userInfo:nil repeats:YES];
			}
		} else if ([current_call callState]==DISCONNECTED){
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
                [endDate release];
                endDate = nil;
			}
			//[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
			//[buttonother setTitle:@"tool" forState:UIControlStateNormal];

			
      if ([current_call code]>=200 && [current_call code]<=299)
          label_status.text = NSLocalizedString(@"callDisconnected", nil);//@"disconnected";
      else if ([current_call reason]!=nil)
        label_status.text = [NSString stringWithFormat: @"%i %@", [current_call code], [current_call reason]];
      else
        label_status.text = NSLocalizedString(@"callDisconnected", nil);//@"disconnected";
      
			//[call removeCallStateChangeListener:self];
			//current_call=nil;
      //[self stopImageAnimation];
            [self onCallRemove:current_call];
		}
	}
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
#if 0
    if (contacts==0) {
        if (myTimer!=nil)
        {
            [myTimer invalidate];
            myTimer=nil;
        }
        if (current_call!=nil)
        {
            [current_call removeCallStateChangeListener:self];
            current_call=nil;
            [gAppEngine removeCallDelegate:self];//by arthur
        }
        [endDate release];
        endDate=nil;
        
        [self switchSpeaker:SPEAKER_OFF];
        [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
    }
#endif
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

	if (current_call!=nil && contacts==0)
	{
		[self onCallUpdate:current_call];
        [gAppEngine addCallDelegate:self];
	} else if (contacts==1) { //if we return from contactView
        contacts = 0;
        [contactUIButton setImage:[UIImage imageNamed:@"button-contacts.png"] forState:UIControlStateNormal];
        [self.navigationController.navigationBar setHidden:YES];
    }
    
    [[UIDevice currentDevice] setProximityMonitoringEnabled:YES];
}

- (void)viewDidUnload {
    [contactUIButton release];
    contactUIButton = nil;
    [self setUILabelEndCall:nil];
    [self setLStatusUILabel:nil];
    [self setLineQualityUILabel:nil];
	[super viewDidUnload];
  [imageArray release];
  imageArray=NULL;
}

- (void)viewDidLoad {
	[super viewDidLoad];
  imageArray  = [[NSArray alloc] initWithObjects:
                 [UIImage imageNamed:@"loading_animation_1.png"],
                 [UIImage imageNamed:@"loading_animation_2.png"],
                 [UIImage imageNamed:@"loading_animation_3.png"],
                 [UIImage imageNamed:@"loading_animation_4.png"],
                 [UIImage imageNamed:@"loading_animation_5.png"],
                 [UIImage imageNamed:@"loading_animation_6.png"],
                 [UIImage imageNamed:@"loading_animation_7.png"],
                 [UIImage imageNamed:@"loading_animation_8.png"],
                 [UIImage imageNamed:@"loading_animation_9.png"],
                 [UIImage imageNamed:@"loading_animation_10.png"],
                 [UIImage imageNamed:@"loading_animation_11.png"],
                 [UIImage imageNamed:@"loading_animation_12.png"],
                 nil];
	image_animation.animationImages = imageArray;
    
    [UILabelEndCall setText:NSLocalizedString(@"callEndCall", nil)];
    [lStatusUILabel setText:NSLocalizedString(@"qtyTitle", @"line status")];
    [lineQualityUILabel setText:NSLocalizedString(@"qtyGood", nil)];
    //[label_status setText:NSLocalizedString(@"callConnected", nil)];
   
    gCallCenter = [[[CTCallCenter alloc] init] autorelease];

    gCallCenter.callEventHandler = ^(CTCall *call) {
        if ([call.callState isEqualToString:CTCallStateIncoming] && current_call!=nil) {
            if ([current_call callState] == CONNECTED) {
                [current_call hangup];
            }
        }
    };
    
}

- (void)dealloc {
    [contactUIButton release];
    [UILabelEndCall release];
    [lStatusUILabel release];
    [lineQualityUILabel release];

    if (myTimer!=nil)
    {
        [myTimer invalidate];
        myTimer=nil;
    }
	if (current_call!=nil)
	{
		[current_call removeCallStateChangeListener:self];
        current_call=nil;
	}
    if (endDate!=nil) {
        [endDate release];
        endDate=nil;
    }
    if (startDate!=nil) {
        [startDate release];
        startDate=nil;
    }
        
    [super dealloc];
}



-(void)onCallExist:(Call *)call {
    NSLog(@">>>>>>>> callControlUIViewController: onCallExist");
	//[self onCallNew:call];
}

-(void)onCallNew:(Call *)call {
	NSLog(@">>>>> callControlUIViewController: onCallNew");
    
    //if (current_call==nil) {
    //    [self setCurrentCall:call];
    //}
}


-(void)onCallRemove:(Call *)call {
	NSLog(@">>>>>>>> callControlUIViewController: onCallRemove");
	
    if (call==current_call) {
        NSLog(@">>>>>>>> : remove current call.");
        
        if (current_call!=nil)
        {
            if ([call callState]==CONNECTED && endDate==nil) {
				endDate = [[NSDate alloc] init];
                [current_call setEnd_date:endDate];
                NSLog(@">>>>> force disconnect: endDate=%@", endDate);
                [endDate release];
                endDate = nil;
			}
            
            [self setCurrentCall:nil];
            [gAppEngine removeCallDelegate:self];//by arthur
        }
        //[endDate release];
        //endDate=nil;
        contacts = 0;
        
        [self switchSpeaker:SPEAKER_OFF];
        [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
        
        [self.parentViewController dismissModalViewControllerAnimated:YES];
    }
    
}



-(void)switchSpeaker:(BOOL)status 
{
    static BOOL entered = FALSE;
    
    
    if (entered == FALSE &&  status!=speaker) {
        entered = TRUE;
        if (status==SPEAKER_ON) {
            [buttonother setImage:[UIImage imageNamed:@"button-speaker(active).png"] forState:UIControlStateNormal];
            NSLog(@">>>>>switchSpeaker: ON");
            
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
									 sizeof (audioRouteOverride),
									 &audioRouteOverride);
		}
		else { //speaker off
            [buttonother setImage:[UIImage imageNamed:@"button-speaker.png"] forState:UIControlStateNormal];
            NSLog(@">>>>>switchSpeaker: OFF");
            
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
									 sizeof (audioRouteOverride),
									 &audioRouteOverride);
		}
        NSLog(@">>>>>switchSpeaker: speaker=%d, status=%d", speaker, status);
        speaker = status;
        entered = FALSE;
    } else {
        NSLog(@">>>>>switchSpeaker: speaker=%d, entered=%d", speaker, entered);
    }
}

@end
