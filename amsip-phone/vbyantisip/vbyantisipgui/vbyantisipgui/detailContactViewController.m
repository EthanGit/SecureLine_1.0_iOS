//
//  detailContactViewController.m
//  vbyantisipgui
//
//  Created by  on 2012/6/5.
//  Copyright (c) 2012年 antisip. All rights reserved.
//


#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "contactUIViewController.h"

#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"

#import "detailContactViewController.h"

#import "ContactEntry.h"

@implementation detailContactViewController
@synthesize profileImage;
@synthesize firstname;
@synthesize lastname;
@synthesize company;
@synthesize secureid;
@synthesize other;

@synthesize firstname_v;
@synthesize lastname_v;
@synthesize secureid_v;
@synthesize company_v;
@synthesize other_v;
@synthesize callBtnTitle;

@synthesize callButton;

@synthesize fnstring;
@synthesize lnstring;
@synthesize cstring;
@synthesize snumber;
@synthesize ostring;
@synthesize cid;

@synthesize isReadyOnly;

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

    myContactDb = [SqliteContactHelper alloc];
    [myContactDb open_database];  

    UIColor *background = [[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"BG.png"]];
    self.view.backgroundColor = background;
    [background release];
    self.firstname.text = fnstring;     
    self.lastname.text = lnstring;
    self.company.text = cstring;
    self.secureid.text = snumber; 
    self.other.text = ostring;


    self.firstname_v.text = fnstring;     
    self.lastname_v.text = lnstring;
    self.company_v.text = cstring;
    self.secureid_v.text = snumber; 
    self.other_v.text = ostring;

    
    self.firstname.hidden = YES;
    self.lastname.hidden = YES;
    self.company.hidden = YES; 
    self.secureid.hidden = YES;
    self.other.hidden = YES;

   
    // Do any additional setup after loading the view from its nib.
    if (isReadyOnly == NO) {
        self.navigationItem.rightBarButtonItem = self.editButtonItem;
    } else {
        [callButton setEnabled:NO]; //readOnly mode
        [callButton setHidden:YES];
        [callBtnTitle setHidden:YES];
    }
        
}

- (void)viewDidUnload
{
    [self setProfileImage:nil];
    [self setFirstname:nil];
    [self setLastname:nil];
    [self setCompany:nil];
    [self setSecureid:nil];
    [self setOther:nil];
    [self setFirstname_v:nil];
    [self setLastname_v:nil];
    [self setSecureid_v:nil];
    [self setCompany_v:nil];
    [self setOther_v:nil];
    [self setCallButton:nil];
    [self setCallBtnTitle:nil];
    [self setIsReadyOnly:NO];
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
    [profileImage release];
    [firstname release];
    [lastname release];
    [company release];
    [secureid release];
    [other release];
    [firstname_v release];
    [lastname_v release];
    [secureid_v release];
    [company_v release];
    [other_v release];
    [callButton release];
    [callBtnTitle release];
    [super dealloc];
}

#import "dialStatusUIViewController.h"

- (IBAction)callNumber:(id)sender {
    int res;
    /* 
     if ([historyEntry.remoteuri rangeOfString : @"sip:"].location == NSNotFound)
     {
     NSMutableString *phonem = [[historyEntry.remoteuri mutableCopy] autorelease];
     [phonem replaceOccurrencesOfString:@"-"
     withString:@""
     options:NSLiteralSearch 
     range:NSMakeRange(0, [phonem length])];
     
     res = [self dial:phonem];
     return;
     
     }*/

    
    NSString *phonenum = [[NSString alloc] init];
    //NSMutableString *phonem = [[@"199" mutableCopy] autorelease];
    if ([secureid.text rangeOfString : @"sip:"].location == NSNotFound)
    {
      phonenum = [@"sip:" stringByAppendingString:secureid.text];
      phonenum = [phonenum stringByAppendingString:@"@"];         
      phonenum = [phonenum stringByAppendingString:[[NSUserDefaults standardUserDefaults] stringForKey:@"proxy_preference"]]; 
    }
    else {
        phonenum = secureid.text;
    }
    NSLog(@"%@",phonenum);
    res = [self dial:phonenum];
    //return;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    NSLog(@"##### textFieldShouldReturn.");
    
    [textField resignFirstResponder];
    
    return YES;
}

-(BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString*)text  
{  
    if ([text isEqualToString:@"\n"]) {  
        [textView resignFirstResponder];   
        return NO;  
    }  
    return YES;  
}  

- (int)dial:(NSString*)phonem
{
    int res;
    if ([gAppEngine isConfigured]==FALSE) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Account Creation", @"Account Creation") 
                                                        message:NSLocalizedString(@"Please configure your settings in the iphone preference panel.", @"Please configure your settings in the iphone preference panel.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    if (![gAppEngine isStarted]) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"No Connection", @"No Connection") 
                                                        message:NSLocalizedString(@"The service is not available.", @"The service is not available.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    
    if ([gAppEngine getNumberOfActiveCalls]>3) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Max Active Call Reached", @"Max Active Call Reached") 
                                                        message:NSLocalizedString(@"You already have too much active call.", @"You already have too much active call.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    
    res = [gAppEngine amsip_start:phonem withReferedby_did:0];
    if (res<0) {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Syntax Error", @"Syntax Error") 
                                                        message:NSLocalizedString(@"Check syntax of your callee sip url.", @"Check syntax of your callee sip url.")
                                                       delegate:nil cancelButtonTitle:@"Cancel"
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        return -1;
    }
    
#ifndef GENTRICE
    vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];
    if ([appDelegate.tabBarController selectedIndex]!=1)
    {
        [appDelegate.tabBarController setSelectedIndex: 1];
    }
    UIViewControllerDialpad *_viewControllerDialpad = (UIViewControllerDialpad *)appDelegate->viewControllerDialpad;
    [_viewControllerDialpad pushCallControlList];
#else
    if ([gAppEngine getNumberOfActiveCalls]>0 ) {
        //added by arthur, 06062012
        UIViewController *dialView = [[dialStatusUIViewController alloc] initWithNibName:@"dialStatusUIViewController" bundle:nil];
        UINavigationController *dialNavView = [[UINavigationController alloc] initWithRootViewController:dialView];
        [dialNavView.navigationBar setHidden:YES];
        [self.tabBarController presentModalViewController:dialNavView animated:YES]; 
        
        [dialView release];
        [dialNavView release];
    }
    
#endif
    
    
    
    return res;
}


-(void) setEditing:(BOOL)editing animated:(BOOL)animated  
{  
    [super setEditing:editing animated:animated];  
    //self.navigationItem.rightBarButtonItem.enabled = !editing;  
    //[contactUITableView beginUpdates];  
    NSLog(@"######## setEditing");
    
    // Add or remove the Add row as appropriate.  
  
    if (editing) {  
        
        self.firstname_v.hidden = YES;
        self.lastname_v.hidden = YES;
        self.secureid_v.hidden = YES;
        self.other_v.hidden = YES;
        self.company_v.hidden = YES; 
        
        self.firstname.hidden = NO;
        self.lastname.hidden = NO;
        self.secureid.hidden = NO;
        self.other.hidden = NO;
        self.company.hidden = NO;  
        self.callButton.hidden = YES;
        self.callBtnTitle.hidden = YES;        
        self.navigationItem.rightBarButtonItem.title = @"Done";  
  
    }  
    else {   
        self.firstname.hidden = YES;
        self.lastname.hidden = YES;
        self.secureid.hidden = YES;
        self.other.hidden = YES;
        self.company.hidden = YES; 
        
        self.firstname_v.hidden = NO;
        self.lastname_v.hidden = NO;
        self.secureid_v.hidden = NO;
        self.other_v.hidden = NO;
        self.company_v.hidden = NO; 
        
        self.callButton.hidden = NO; 
        self.callBtnTitle.hidden = NO;        
        NSLog(@"#########update");
        
        current_contact = [[ContactEntry alloc]init];    
        NSLog(@"######### %@ , %@, %@ , %@ , %@ , %@",firstname.text,lastname.text,company.text,secureid.text,other.text,cid);
        
        //if (firstNameUITextField)
        [current_contact setFirstname: [NSString stringWithFormat:@"%@", self.firstname.text]];
        //if (lastname)
        [current_contact setLastname: [NSString stringWithFormat:@"%@", self.lastname.text]];
        [current_contact setOther:[NSString stringWithFormat:@"%@", self.other.text]];
        [current_contact setCompany:[NSString stringWithFormat:@"%@", self.company.text]];
        [current_contact setPhone_string:[NSString stringWithFormat:@"%@", self.secureid.text]];
        [current_contact setCid:[NSString stringWithFormat:@"%@", self.cid]];
        NSLog(@"######## set end");
        //[current_contact setPhone_numbers:[NSString stringWithFormat:@"%@", securePhoneIDUITextField.text]];
        /*
        SipNumber *sip_number = [SipNumber alloc];
        [sip_number setPhone_type:@"voip"];
        [sip_number setPhone_number:[NSString stringWithFormat:@"%@", secureid.text]];    
        [current_contact.phone_numbers insertObject:sip_number atIndex:0];
        */
        NSLog(@"######### %@ , %@, %@ , %@ , %@, %@",firstname.text,lastname.text,company.text,secureid.text,other.text,cid);   
        //if (firstname)
        //CFRelease(firstname);
        //if (lastname)
        //CFRelease(lastname);
        
        if (current_contact==nil){
            [self.navigationController popViewControllerAnimated:YES];
            return;
        }
        /*
         if ([current_contact.phone_numbers count]<=0)
         {
         [current_contact release];
         current_contact = nil;
         return;
         }
         */
        
        //ContactEntry *current_contact = [ContactEntry alloc];
        NSLog(@"######## run update_contact");
        [myContactDb update_contact:current_contact];
        [self.navigationController popViewControllerAnimated:YES];
        
        [ContactEntry release];
        
        
        //[self updateContact];
    }  
    //[contactUITableView endUpdates];      
}  
@end
