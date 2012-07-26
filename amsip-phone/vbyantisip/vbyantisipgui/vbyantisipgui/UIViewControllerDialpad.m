#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"
#import "UIViewControllerCallList.h"
#import "UIViewControllerVideoCallControl.h"

#import "AppEngine.h"

@implementation UIViewControllerDialpad

+(void)_keepAtLinkTime
{
    return;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	
	target_field.delegate = self;
	target_field.returnKeyType = UIReturnKeyDone;
	
	timerPlus=nil;
	return;
}

-(void)pushCallControlList {
    UIViewControllerCallList *uiviewcontroller=nil;
    for(UIView *view in self.navigationController.viewControllers)
    {
        if([view isKindOfClass:[UIViewControllerCallList class]])
        {
            uiviewcontroller = (UIViewControllerCallList*)view;
        }
    }

    if (uiviewcontroller==nil) {
        UIViewControllerCallList *uiviewcontroller = [[UIViewControllerCallList alloc]initWithNibName:@"ScrollViewCallList" bundle:[NSBundle mainBundle]];
        [self.navigationController pushViewController:uiviewcontroller animated:YES];
        [uiviewcontroller release];
    }
}

- (BOOL)textFieldShouldReturn:(UITextField *)field {
	[field resignFirstResponder];
	return YES;
}

- (IBAction)actionButton0:(id)sender {
	if ([timerPlus isValid]) {
		[timerPlus invalidate];
		[timerPlus release];
		timerPlus = nil;
	}	
	
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"0"];
	[target_field setText:old];
	
	timerPlus = [[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(actionbuttonplus) userInfo:nil repeats:NO] retain];
}

-(void)actionbuttonplus
{
	if ([timerPlus isValid]) {
		[timerPlus invalidate];
		[timerPlus release];
		timerPlus = nil;
	}	
    NSString *old = [target_field text];
	old = [old substringToIndex:([old length]-1)];
	old = [old stringByAppendingString:@"+"];
	[target_field setText:old];
}

- (IBAction)actionButton0up:(id)sender {
	if ([timerPlus isValid]) {
		[timerPlus invalidate];
		[timerPlus release];
		timerPlus = nil;
		return;
	}
}

- (IBAction)actionButton1:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"1"];
	[target_field setText:old];
}

- (IBAction)actionButton2:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"2"];
	[target_field setText:old];
}

- (IBAction)actionButton3:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"3"];
	[target_field setText:old];
}

- (IBAction)actionButton4:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"4"];
	[target_field setText:old];
}

- (IBAction)actionButton5:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"5"];
	[target_field setText:old];
}

- (IBAction)actionButton6:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"6"];
	[target_field setText:old];
}

- (IBAction)actionButton7:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"7"];
	[target_field setText:old];
}

- (IBAction)actionButton8:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"8"];
	[target_field setText:old];
}

- (IBAction)actionButton9:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"9"];
	[target_field setText:old];
}

- (IBAction)actionButtondelete:(id)sender {
    NSString *old = [target_field text];
	old = [old substringToIndex:([old length]-1)];
	[target_field setText:old];
}

- (IBAction)actionButtonhang:(id)sender {
	[self pushCallControlList];
}

- (IBAction)actionButtonpound:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"#"];
	[target_field setText:old];
}

- (IBAction)actionButtonstar:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"*"];
	[target_field setText:old];
}

- (IBAction)actionButtonstart:(id)sender {
  
	if ([gAppEngine isConfigured]==FALSE) {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account Creation", @"Account Creation") 
                                                    message:NSLocalizedString(@"Please configure your settings in the iphone preference panel.", @"Please configure your settings in the iphone preference panel.")
                                                   delegate:nil cancelButtonTitle:@"Cancel"
                                          otherButtonTitles:nil];
    [alert show];
    [alert release];
    return;
  }
	if (![gAppEngine isStarted]) {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"No Connection", @"No Connection") 
                                                    message:NSLocalizedString(@"The service is not available.", @"The service is not available.")
                                                   delegate:nil cancelButtonTitle:@"Cancel"
                                          otherButtonTitles:nil];
    [alert show];
    [alert release];
    return;
  }
  
	if ([gAppEngine getNumberOfActiveCalls]>3) {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Max Active Call Reached", @"Max Active Call Reached") 
                                                    message:NSLocalizedString(@"You already have too much active call.", @"You already have too much active call.")
                                                   delegate:nil cancelButtonTitle:@"Cancel"
                                          otherButtonTitles:nil];
    [alert show];
    [alert release];
    return;
  }
  
	int i = [gAppEngine amsip_start:[target_field text] withReferedby_did:0];
  if (i<0) {
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Syntax Error", @"Syntax Error") 
                                                    message:NSLocalizedString(@"Check syntax of your callee sip url.", @"Check syntax of your callee sip url.")
                                                   delegate:nil cancelButtonTitle:@"Cancel"
                                          otherButtonTitles:nil];
    [alert show];
    [alert release];
    return;
  }
	[self pushCallControlList];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
	
	//[gAppEngine removeCallDelegate:self];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
	
	//[gAppEngine addCallDelegate:self];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}


- (void)dealloc {
    [super dealloc];
}

@end
