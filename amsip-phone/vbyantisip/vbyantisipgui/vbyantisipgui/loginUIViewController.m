//
//  loginUIViewController.m
//  vbyantisipgui
//
//  Created by arthur on 2012/5/24.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//
#import "vbyantisipAppDelegate.h"
#import "loginUIViewController.h"
#import "creatAccountUIViewController.h"
#import "gentriceGlobal.h"
#import "apiGlobal.h"
#import "NSString+MD5.h"
#import "NGJSONParser.h"


@implementation loginUIViewController
//@synthesize loginIDTextField;
//@synthesize loginPasswdTextField;
@synthesize createAccountButton;
//@synthesize LoginNavigationBar;
//@synthesize loginBarButton;
@synthesize loginUIButton;
@synthesize user_account;
@synthesize user_password;
@synthesize tmp_password,connect_finished,tmp_data,tmp_keystone;

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
    [createAccountButton setTitle:NSLocalizedString(@"btnNewAccount", @"New Account") forState:UIControlStateNormal];
    [loginUIButton setTitle:NSLocalizedString(@"btnLogin",@"Login")  forState:UIControlStateNormal];
    [user_account setPlaceholder:NSLocalizedString(@"fphSecureLineID",@"Secure Line ID")];
    [user_password setPlaceholder:NSLocalizedString(@"fphPassword",@"Password")];
    // Do any additional setup after loading the view from its nib.
    [user_account becomeFirstResponder];
    
    //[self.navigationItem setRightBarButtonItem:[[[UIBarButtonItem alloc] initWithTitle:@"Login" style:UIBarButtonItemStyleBordered target:self action:@selector(doLoginProcess)] autorelease]];
    //[self.navigationItem setTitle:@"SecurePhone"];
    
    
}


- (void)viewDidUnload
{
    [self setCreateAccountButton:nil];
    //[self setLoginIDTextField:nil];
    //[self setLoginPasswdTextField:nil];
    //[self setLoginNavigationBar:nil];
    //[self setLoginBarButton:nil];
    [self setLoginUIButton:nil];
    [self setUser_account:nil];
    [self setUser_password:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (BOOL)hidesBottomBarWhenPushed {
    return TRUE;
}


- (void) doLoginProcess
{
    //pass username/passwd to URL
#if GENTRICE_DEBUG > 0
    //NSLog(@"login = %@\npasswd = %@\n", loginIDTextField.text, loginPasswdTextField.text);
#endif //GENTRICE_DEBUG


    
#if KeystoneAuth > 0    
    NSError *error = NULL;
    // NSString *jsonString = [NSString stringWithFormat:@"{\"uid\":\"%@\",\"token\":\"%@\",\"devicetoken\":\"%@\"}",user_account.text,passwd.text,@""];
    
    NSString *account = [NSString stringWithFormat:@"%@",user_account.text];
    NSString *password = [NSString stringWithFormat:@"%@", user_password.text];
    
    NSArray *valueArray = [[NSArray alloc] initWithObjects:account,password,[[NSUserDefaults standardUserDefaults] stringForKey:@"deviceToken"], nil];
    NSArray *keyArray = [[NSArray alloc] initWithObjects:@"uid",@"token",@"devicetoken", nil];
    
    
    NSDictionary *info = [NSDictionary dictionaryWithObjects:valueArray forKeys:keyArray];
    //NSLog(@"-----------------  info");
    //壓縮成NSData，這時的編碼會成UTF8格式
    NSData *jsonData = [NSJSONSerialization
                        dataWithJSONObject:info
                        options:NSJSONWritingPrettyPrinted
                        error:&error];    
    //NSLog(@"-----------------  jsonData");
    [self performSelectorOnMainThread : @selector(login_keystone_api:) withObject:jsonData waitUntilDone:YES];
    [valueArray release];
    [keyArray release];
    
    NSLog(@"------------------ login tmp_keystone = %@",tmp_keystone);
    NSString *message = [[[NSString alloc] init] autorelease];
    NSString *expiredDate = [[[NSString alloc] init] autorelease];
    
    if(tmp_keystone.length>0){
        
        NSDictionary *dic = [NGJSONParser dictionaryOrArrayFromJSONString:tmp_keystone];
        
        message = [dic objectForKey:@"message"];
        expiredDate = [dic objectForKey:@"date"];
        NSLog(@"----------- message:%@ , expiredDate:%@",message,expiredDate);
        
    }
    //expiredDate = @"20120607";
    int expiredDateInt = [expiredDate intValue];
    
    
    NSDateFormatter *dateFormatter=[[NSDateFormatter alloc] init];
    [dateFormatter setDateFormat:@"yyyyMMdd"];  
    [dateFormatter setFormatterBehavior:NSDateFormatterBehavior10_4];
    NSDate *nowDate = [NSDate date];
    NSString *nowDateString = [dateFormatter stringFromDate:nowDate];
    int nowDateInt = [nowDateString intValue];
    
    NSLog(@"----- expiredDateInt:%i ,  nowDateInt=%i",expiredDateInt,nowDateInt);
    
    if(expiredDate.length>0 && expiredDateInt<nowDateInt){
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altErrorMessage",@"Error Message") 
                                                        message:NSLocalizedString(@"altmLoginExpired",@"Your account expired")
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnEnter",nil)
                                              otherButtonTitles:nil];
        [alert show];
        [alert release]; 
        return;
        
    }
    else if(message.length>0){
        NSString *alertMessage = [[[NSString alloc] init] autorelease];
        
        if([message isEqualToString:@"401"]){
            alertMessage = NSLocalizedString(@"altmLoginDataError",@"Your account or password error");
        }
        else if([message isEqualToString:@"400"]){
            alertMessage = NSLocalizedString(@"altmLoginDataError",@"Your account or password error");
        }
        else if([message isEqualToString:@"500"]){
            alertMessage = NSLocalizedString(@"altmLoginAuthServerError",@"auth server error");            
        } 
        else{
            alertMessage = NSLocalizedString(@"altmLoginAuthServer",nil);            
        }
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altErrorMessage",@"Error Message") 
                                                        message:alertMessage
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnEnter",nil)
                                              otherButtonTitles:nil];
        [alert show];
        [alert release]; 
        return;
    }
    
#else
    NSString *account = [NSString stringWithFormat:@"%@",user_account.text];
#endif //KeystoneAuth
    
    //get voip's password string
    //[self performSelectorOnMainThread : @selector(get_voip_password:) withObject:account waitUntilDone:YES];
    self.tmp_password = account;
    NSLog(@"------------------ login tmp_password = %@",self.tmp_password);
    //[self loginkeystoneserver:jsonData];
    
    
    
    //Login success
    [self.parentViewController dismissModalViewControllerAnimated:YES];
    
    // NSLog(@"loginName.text = %@",loginName.text);
    // NSLog(@"loginName.text = %@",passwd.text);
    [[NSUserDefaults standardUserDefaults] setObject:user_account.text forKey:@"user_preference"];
    //[self getSIPPassword];
    //[self performSelectorOnMainThread : @ selector(getSIPPassword) withObject:nil waitUntilDone:YES];
    //self.tmp_password = user_password.text;
    
    if(![self.tmp_password isEqualToString:@"Error"] && self.tmp_password.length>0){
        
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"LoginStatus"];
        [[NSUserDefaults standardUserDefaults] setObject:self.tmp_password forKey:@"password_preference"];    
        
        NSString *tmp = [NSString stringWithFormat:@"<sip:%@@%@>", user_account.text, [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"]];
        [[NSUserDefaults standardUserDefaults] setObject:tmp forKey:@"identity_preference"];
        [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] restartAll];
        [[(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate]tabBarController] setSelectedIndex:1];
        
    }
    
#if 1
    //check if we are configured
    NSString *_proxy = [[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"];
	NSString *_login = [[NSUserDefaults standardUserDefaults] stringForKey:@"user_preference"];
    
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
#endif
    
}


- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse*)response 
{
    NSLog(@"############ login connection didReceiveResponse response:%@\n",response);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection 
{
    NSLog(@"login connectionDidFinishLoading connection ");
    self.connect_finished = YES;
    //[_waitingDialog dismissWithClickedButtonIndex:0 animated:NO];
    
    // [connection release];
    //[DownloadConnection release];
}

-(void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{ 
    NSLog(@" ########### login connection error:%@\n",error);
    self.connect_finished = NO;
    //[[NSUserDefaults standardUserDefaults] setObject:@"Error" forKey:@"p_connect_finished"];
    
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection{
    NSLog(@"login connectionShouldUseCredentialStorage connection ");
    return NO;
}

//下面两段是重点，要服务器端单项HTTPS 验证，iOS 客户端忽略证书验证。
// 無論如何回傳 YES
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
    
    return YES;
    // return [protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust];
}

// 不管那一種 challenge 都相信
- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge{
    NSLog(@"login received authen challenge");
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
    NSString *data_tmp = [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease];
    NSLog(@"######### login connection didReceiveData:");
    NSLog(@"###### login got data %@",data_tmp );
    //[[NSUserDefaults standardUserDefaults] setObject:data_tmp forKey:@"p_connect_data_tmp"];
    self.tmp_data = data_tmp;
    //[data_tmp release];
} 



#import "enableAccountUIViewController.h"

- (IBAction)createAccountButtonPressed:(id)sender {
    
#if 0
    UIViewController *detailView=nil;
    
    detailView = [[creatAccountUIViewController alloc] initWithNibName:@"creatAccountUIViewController" bundle:nil];
    
    if (detailView!=nil) {
        [detailView setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:detailView animated:YES];
        [detailView release];
        //[detailView autorelease];
    }
#else
    
    UIViewController *enableView=nil;
    
    enableView = [[enableAccountUIViewController alloc] initWithNibName:@"enableAccountUIViewController" bundle:nil];
    //enableView.title = @"";
    
    if (enableView!=nil) {
        //[enableView setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:enableView animated:YES];
        [enableView release];
    }
#endif    
    
}


- (IBAction)loginButtonPressed:(id)sender {
    NSLog(@"login=%@, passwd=%@", user_account.text, user_password.text); 
    
    if(user_account.text.length>0 && user_password.text.length){
        [self doLoginProcess];
    }
    else{
        UIAlertView *alert = [[UIAlertView alloc]
                              initWithTitle:NSLocalizedString(@"altErrorMessage",@"Error Message") message:NSLocalizedString(@"altmKeyinError",@"Keyin Error") delegate:self cancelButtonTitle:NSLocalizedString(@"btnEnter", nil) otherButtonTitles:nil];
        [alert show];
        [alert release];
    }
    //UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Login:", @"") message:@"\n\n\n" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Ok", nil];
/*
    loginAlertView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Login:", @"") message:@"\n\n\n" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Ok", nil];

    
    loginName = [[[UITextField alloc] initWithFrame:CGRectMake(12.0, 50.0, 260.0, 25.0)] autorelease];
    [loginName setPlaceholder:@"Username"];
    [loginName setBackgroundColor:[UIColor whiteColor]];
    [loginName setAutocorrectionType:UITextAutocorrectionTypeNo];
    [loginName setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [loginName setDelegate:self];
    [loginAlertView addSubview:loginName];
    passwd = [[[UITextField alloc] initWithFrame:CGRectMake(12.0, 85.0, 260.0, 25.0)] autorelease];
    [passwd setPlaceholder:@"Password"];
    [passwd setSecureTextEntry:YES];
    [passwd setBackgroundColor:[UIColor whiteColor]];
    [passwd setAutocorrectionType:UITextAutocorrectionTypeNo];
    [passwd setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [passwd setDelegate:self];
    [loginAlertView addSubview:passwd];
    
    [loginAlertView show];
    //[alert release];
    
    [loginName becomeFirstResponder];
*/ 
    
}

-(void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex==1) { //OK
        NSLog(@"login=%@, passwd=%@", loginName.text, passwd.text); 
        [loginAlertView release];
        [self doLoginProcess];
    } else {
        NSLog(@">>>>> Cancel pressed.");
    }
     
}

-(void)alertView:(UIAlertView *)alertView willDismissWithButtonIndex:(NSInteger)buttonIndex
{
    if (buttonIndex==0) {
        NSLog(@"##### Cancel pressed.");
    }
    
}

/*
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    NSLog(@"##### textFieldShouldReturn.");

    if (textField == loginName) {
        [passwd becomeFirstResponder];
    } else if (textField == passwd) {
        NSLog(@">>login=%@, passwd=%@", loginName.text, passwd.text); 
        //[passwd endEditing:YES];
        [loginAlertView dismissWithClickedButtonIndex:1 animated:YES];
        [loginAlertView release];
        [self doLoginProcess];
    }

    return YES;
}
*/
#if 0
- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    NSLog(@"##### textFieldShouldReturn.");
    
    if (textField == loginPasswdTextField){
        [loginPasswdTextField resignFirstResponder];
        [self doLoginProcess];
    } else if (textField == loginIDTextField){
        [loginPasswdTextField becomeFirstResponder];
    }
    
    return YES;
}
#endif


- (BOOL)textFieldShouldReturn:(UITextField *)field {
    NSLog(@"####### textFieldShouldReturn");
	[field resignFirstResponder];
	return YES;
}


//傳送至web Service
-(void)login_keystone_api:(NSData*)json
{
    NSLog(@"--------------------- loginkeystoneserver");
    connect_finished = NO;
    tmp_data = nil;
    
    NSString *urlAsString = [[[NSString alloc] initWithString:keystoneGetAPIURL] autorelease];//absoluteString
    NSString *account = [NSString stringWithFormat:@"%@", user_account.text];
    NSString *password = [[NSString stringWithFormat:@"%@", user_password.text] MD5];
    NSLog(@"MD5->%@",password);
    //
    //[user_password.text MD5]
    urlAsString = [urlAsString stringByAppendingString:[NSString stringWithFormat:keystoneLoginVar,account,password,[[NSUserDefaults standardUserDefaults] stringForKey:@"deviceToken"]]];
    NSLog(@"---------------------  urlAsString:%@",urlAsString);
    NSURL *url = [NSURL URLWithString:urlAsString];
    
    NSMutableURLRequest *urlRequest = [NSMutableURLRequest requestWithURL:url];
    [urlRequest setTimeoutInterval:30.0f];
    [urlRequest setHTTPMethod:@"POST"];
    //直接把NSData(這時的編碼為UTF8)做為傳送的內容
    [urlRequest setHTTPBody:json];
    NSLog(@"--------------------- REQUEST: %@", [urlRequest HTTPBody]);
    [NSURLConnection connectionWithRequest:urlRequest delegate:self];
    
    while(connect_finished == NO) {
        NSLog(@"--------------------- login LOOP");
        
		[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        NSLog(@"--------------------- login tmp:%@",tmp_data);
	} 
    if(tmp_data.length > 0){ tmp_keystone = tmp_data;}
    

    return;
}



//傳送至web Service
- (void)get_voip_password:(NSString *)username
{
    NSLog(@"--------------------- get_voip_password");
    tmp_data = nil;
    connect_finished = NO;
    
    NSString *registerURL = [[[NSString alloc] initWithString:proxyRegisterAPIURL] autorelease];//absoluteString
    
    registerURL = [registerURL stringByAppendingString:[NSString stringWithFormat:proxyRegisterVar,username]];
    
    
    NSLog(@"--------------------- registerURL:%@",registerURL);
    NSURL *url = [NSURL URLWithString:registerURL];
    
    NSMutableURLRequest *urlRequest = [NSMutableURLRequest requestWithURL:url];
    [urlRequest setTimeoutInterval:30.0f];
    //[urlRequest setHTTPMethod:@"POST"];
    //直接把NSData(這時的編碼為UTF8)做為傳送的內容
    //[urlRequest setHTTPBody:data];
    //NSLog(@"REQUEST: %@", [urlRequest HTTPBody]);
    
    //NSLog(@"--------------------- registerURL:%@",registerURL);
    
    
    //NSString *return_data = [[[NSString alloc] init] autorelease];
    // NSData *result = [NSURLConnection sendSynchronousRequest:urlRequest returningResponse:&response error:&error ];
    [NSURLConnection connectionWithRequest:urlRequest delegate:self];
    
    while(connect_finished == NO) {
        NSLog(@"--------------------- login LOOP");
        
		[[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
        NSLog(@"--------------------- login tmp:%@",tmp_data);
	} 
    if(tmp_data.length > 0){ tmp_password = tmp_data;}
    
    return;
}



- (void)dealloc {
    //[LoginNavigationBar release];
    //[loginBarButton release];
    [loginUIButton release];
    [user_account release];
    [user_password release];
    [super dealloc];
}

@end
