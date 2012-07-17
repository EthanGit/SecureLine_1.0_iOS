#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "passcodeUIViewController.h"
#import "vbyantisipAppDelegate.h"
#import "UIViewControllerCallList.h"
#import "UIViewControllerVideoCallControl.h"

#import "AppEngine.h"

@implementation passcodeUIViewController
@synthesize title_message;
@synthesize current_call;

+(void)_keepAtLinkTime
{
    return;
}

- (void)viewDidLoad {
	[super viewDidLoad];
    //pass1_field.secureTextEntry = YES;
    //pass2_field.secureTextEntry = YES;
    //pass3_field.secureTextEntry = YES;
    //pass4_field.secureTextEntry = YES;
    
    
    //UIColor *background = [[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"BG-Login2.png"]];
    

    //self.view.backgroundColor = background;
    //[background release];
	
   // [self.navigationItem setTitle:@"Passcode"];
    
	//target_field.delegate = self;
	//target_field.returnKeyType = UIReturnKeyDone;
    self.title_message.text = NSLocalizedString(@"altmKeyinPassword", nil) ;
	[self.navigationController.navigationBar setHidden:YES];
	//timerPlus=nil;
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
    NSLog(@"####### textFieldShouldReturn");
	[field resignFirstResponder];
	return YES;
}

- (IBAction)actionButton0:(id)sender {
    /*
	if ([timerPlus isValid]) {
		[timerPlus invalidate];
		[timerPlus release];
		timerPlus = nil;
	}*/	
	/*
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"0"];
	[target_field setText:old];
*/
    NSString *old = [target_field text];
    old = [old stringByAppendingString:@"0"];
    [self setShowfield:old];
    
	
    
	//timerPlus = [[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(actionbuttonplus) userInfo:nil repeats:NO] retain];
}
/*
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
*/

- (IBAction)actionButton0up:(id)sender {
/*
	if ([timerPlus isValid]) {
		[timerPlus invalidate];
		[timerPlus release];
		timerPlus = nil;
		return;
	}
 */
}

- (IBAction)actionButton1:(id)sender {
    NSString *old = [target_field text];
    if(old.length<=4){
        old = [old stringByAppendingString:@"1"];
        [self setShowfield:old];
    }
    return;
	//[target_field setText:old];
}

- (IBAction)actionButton2:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"2"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButton3:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"3"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButton4:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"4"];
    [self setShowfield:old];
	
}

- (IBAction)actionButton5:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"5"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButton6:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"6"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButton7:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"7"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButton8:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"8"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButton9:(id)sender {
    NSString *old = [target_field text];
	old = [old stringByAppendingString:@"9"];
    [self setShowfield:old];
	//[target_field setText:old];
}

- (IBAction)actionButtondelete:(id)sender {
    NSString *old = [target_field text];
    if ([old length] > 0) {
        old = [old substringToIndex:([old length]-1)];
    }
	//old = [old substringToIndex:([old length]-1)];
	//[target_field setText:old];
    [self setShowfield:old];
}

-(void)setShowfield:(NSString *)string{

   // [textField becomeFirstResponder ];
    NSLog(@"######## setShowfield:%@",string);
    
    if(string.length<=4) [target_field setText:string];
    /*
    else if(string.length>4){
        [target_field setText:[string substringToIndex:3]];
    }
     */
    switch (string.length){
        case 0:
           // [pass1_field becomeFirstResponder];
            pass1_field.text = @"";
            pass2_field.text = @""; 
            pass3_field.text = @"";
            pass4_field.text = @"";             
            break;
        case 1:
            //[pass2_field becomeFirstResponder];
            pass1_field.text = @"●";
            pass2_field.text = @""; 
            pass3_field.text = @"";
            pass4_field.text = @""; 
            break;
        case 2:
            //[pass3_field becomeFirstResponder];
            pass1_field.text = @"●";
            pass2_field.text = @"●"; 
            pass3_field.text = @"";
            pass4_field.text = @""; 
            break;
        case 3:
           // [pass4_field becomeFirstResponder];
            pass1_field.text = @"●";
            pass2_field.text = @"●"; 
            pass3_field.text = @"●";
            pass4_field.text = @""; 
            break; 
        case 4:
            pass1_field.text = @"●";
            pass2_field.text = @"●"; 
            pass3_field.text = @"●";
            pass4_field.text = @"●"; 
           //check passcode
            [self checkPasscode];
            break;              
        default:
            break;
    }  
    return;
}

-(void)checkPasscode{
   
        //NSString *passcode = [[NSString alloc] initWithFormat:[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"];
   // NSString *passcode = [[NSUserDefaults standardUserDefaults] stringForKey:@"PasscodeString"];

   // [[NSUserDefaults standardUserDefaults] setObject:@"1234" forKey:@"PasscodeString"];
    
    NSString *keypassword = target_field.text;
    NSLog(@"##### passcode:%@ / target_field:%@ ",[[NSUserDefaults standardUserDefaults] stringForKey:@"PasscodeString"],keypassword);
    
    
    if(![keypassword isEqualToString:[[NSUserDefaults standardUserDefaults] stringForKey:@"PasscodeString"]]){
        pass1_field.text = @"";
        pass2_field.text = @""; 
        pass3_field.text = @"";
        pass4_field.text = @"";
        target_field.text = @"";
        
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altPasscodeError", nil) 
                                                        message:NSLocalizedString(@"altmWrongPasscode", nil)
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];        
    }
    else{
        [self pushView];
    }

}


#import "loginUIViewController.h"

- (IBAction)actionButtonhang:(id)sender {
    
	//[self pushCallControlList];
    if(target_field.text.length==4){
        [self checkPasscode];
    }
    else{
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altPasscodeWarning", nil) 
                                                        message:NSLocalizedString(@"altmKeyinPassword", nil)
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];     
    
    }

}



#import "loginUINavigationController.h"
#import "dialIncomingUIViewController.h"

-(void)pushView{
#if 0
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"] == NO) {
        
        //[self.parentViewController dismissModalViewControllerAnimated:NO];
        
        UIViewController *loginView = [[loginUIViewController alloc] initWithNibName:@"loginUIViewController" bundle:nil];
        //loginUINavigationController *loginNavView = [[loginUINavigationController alloc] initWithRootViewController:loginView];
        //UINavigationController *loginNavView = [[UINavigationController alloc] initWithRootViewController:loginView];
        //[loginNavView.navigationBar setBarStyle:UIBarStyleBlack];
        //[self.tabBarController presentModalViewController:loginNavView animated:YES];
        [self.navigationController pushViewController:loginView animated:YES];
        [loginView release];
        //[loginNavView release];
    } else {
        [self.parentViewController dismissModalViewControllerAnimated:YES];   
    }
#else
    //[self.parentViewController dismissModalViewControllerAnimated:YES];
    //[super.tabBarController viewWillAppear:NO];
    //[super.tabBarController viewDidAppear:NO];
    //vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
    //[appDelegate.tabBarController viewWillAppear:NO];
    //[appDelegate.tabBarController viewDidAppear:NO];
    
    //for incoming call after passcode is passed, 07122012
    if (current_call!=nil && [current_call callState] == RINGING) {
        dialIncomingUIViewController *dialView = [[dialIncomingUIViewController alloc] initWithNibName:@"dialIncomingUIViewController" bundle:nil];
        [self.navigationController.navigationBar setHidden:YES];
        [dialView setAutoAnswer:false];
        [self.navigationController pushViewController:dialView animated:YES];
        [dialView release];
    } else {
        [self.parentViewController dismissModalViewControllerAnimated:YES];
    }
    
#endif

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
	
	[gAppEngine removeCallDelegate:self];
}

-(void)textFieldDidEndEditing:(UITextField *)textField{
    NSLog(@"######## textFieldDidEndEditing");
   

}
         
             
#import "loginUINavigationController.h"
//#import "blueUINavigationBar.h"

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    
    NSLog(@">>>>>>>> viewDidDisappear.....");
    vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
    //[appDelegate.tabBarController setSelectedIndex:2];
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"LoginStatus"] == NO) {
        
        //[self.parentViewController dismissModalViewControllerAnimated:NO];
        
        UIViewController *loginView = [[loginUIViewController alloc] initWithNibName:@"loginUIViewController" bundle:nil];
        UINavigationController *loginNavView = [[loginUINavigationController alloc] initWithRootViewController:loginView];
        [loginNavView.navigationBar setHidden:YES];
        [appDelegate.tabBarController presentModalViewController:loginNavView animated:NO];
        [loginView release];
        [loginNavView release];
    } 
    
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
	
	[gAppEngine addCallDelegate:self];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning]; // Releases the view if it doesn't have a superview
    // Release anything that's not essential, such as cached data
}


- (void)dealloc {
    [pass1_field release];
    [pass2_field release];
    [pass3_field release];
    [pass4_field release];
    [title_message release];
    [super dealloc];
}

- (void)viewDidUnload {
    [pass1_field release];
    pass1_field = nil;
    [pass2_field release];
    pass2_field = nil;
    [pass3_field release];
    pass3_field = nil;
    [pass4_field release];
    pass4_field = nil;
    [self setTitle_message:nil];
    [super viewDidUnload];
}



#pragma mark - EngineDelegate

-(void)onCallExist:(Call *)call {
	[self onCallNew:call];
}


-(void)onCallNew:(Call *)call {
	NSLog(@">>>>> passcodeUIViewController: onCallNew");
    
    if (current_call==nil) {
        [self setCurrent_call:call];
    }
    if ([call isIncomingCall]==true && [call callState]==RINGING) {
        NSLog(@">>>>> Accept:180");
        [call accept:180];
    }
}



-(void)onCallRemove:(Call *)call {
	NSLog(@">>>>>>>> passcodeUIViewController: onCallRemove");
	
    if (call==current_call) {
        NSLog(@">>>>>>>> remove current call.");
        [self setCurrent_call:nil];
    }
}


@end
