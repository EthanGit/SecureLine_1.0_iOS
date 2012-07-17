#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "UIViewControllerCallControl.h"
#import "UIViewControllerVideoCallControl.h"
#import "UIViewControllerDTMFControl.h"

#import "vbyantisipAppDelegate.h"

@implementation UIViewControllerCallControl

@synthesize parent;

// Load the view nib and initialize the pageNumber ivar.
- (id)initWithPageNumber:(int)page {
    if (self = [super initWithNibName:@"CallControlView" bundle:nil]) {
        pageNumber = page;
    }
    return self;
}


- (IBAction)actionButtonhang:(id)sender {
	if (current_call!=nil && [current_call callState]>=TRYING && [current_call callState]!=DISCONNECTED)
	{
		[current_call hangup];
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
		}
		else {
			[current_call mute];
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


- (void)setRemoteIdentity:(NSString*)str{
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
		if ((int)(elapsedTime / 3600) > 0)
			str = [NSString stringWithFormat:@"Duration: %02d:%02d:%02d", (int)(elapsedTime / 3600), ((int)elapsedTime % 3600) / 60, (int)elapsedTime % 60];
		else
			str = [NSString stringWithFormat:@"Duration: %02d:%02d", ((int)elapsedTime % 3600) / 60, (int)elapsedTime % 60];
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
	NSLog(@"UIViewControllerCallControl: onCallUpdate");
	if (current_call==call)
	{
		[self setRemoteIdentity:[call oppositeNumber]];
		
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
      [self startImageAnimation];

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

      [self startImageAnimation];
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
			
      [self startImageAnimation];
		} else if ([current_call callState]==CONNECTED){
			if (startDate==nil)
			{
				startDate = [[NSDate alloc] init];
			}
			[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
			[buttonother setTitle:@"tool" forState:UIControlStateNormal];
			
			[label_status setText:@"established"];
			if (myTimer==nil)
			{
				myTimer = [NSTimer scheduledTimerWithTimeInterval:10 target:self selector:@selector(updateRate) userInfo:nil repeats:YES];
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
			}
			[buttonother setBackgroundColor:[UIColor colorWithRed:21.0/255.0 green:43.0/255.0 blue:65.0/255.0 alpha:0.69]];
			[buttonother setTitle:@"tool" forState:UIControlStateNormal];

			
      if ([current_call code]>=200 && [current_call code]<=299)
        label_status.text = @"disconnected";
      else if ([current_call reason]!=nil)
        label_status.text = [NSString stringWithFormat: @"%i %@", [current_call code], [current_call reason]];
      else
        label_status.text = @"disconnected";
      
			//[call removeCallStateChangeListener:self];
			//current_call=nil;
      [self stopImageAnimation];
		}
	}
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
	
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
    [endDate release];
    endDate=nil;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];

	if (current_call!=nil)
	{
		[self onCallUpdate:current_call];
	}
}

- (void)viewDidUnload {
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
}

- (void)dealloc {
    [super dealloc];

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
    [endDate release];
    endDate=nil;
}

@end
