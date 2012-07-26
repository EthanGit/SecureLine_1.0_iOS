//
//  creatAccountUIViewController.m
//  vbyantisipgui
//
//  Created by Arthur on 2012/5/28.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "creatAccountUIViewController.h"
#import "gentriceGlobal.h"

@implementation creatAccountUIViewController
@synthesize firstNameUITextField;
@synthesize lastNameUITextField;
@synthesize loginUITextField;
@synthesize passwdUITextField;
@synthesize confirmPasswdUITextField;

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
    
    [self.navigationController.navigationBar setHidden:NO];
    //[self.navigationItem setLeftBarButtonItem:[[[UIBarButtonItem alloc] initWithTitle:@"Cancel" style:UIBarButtonItemStyleBordered target:self action:@selector(returnToLastPage)] autorelease]];
    [self.navigationItem setBackBarButtonItem:[[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(returnToLastPage)] autorelease]];
    
    [self.navigationItem setRightBarButtonItem:[[[UIBarButtonItem alloc] initWithTitle:@"Done" style:UIBarButtonItemStyleBordered target:self action:@selector(doWebLoginProcess)] autorelease]];
    
    //self.navigationItem.title = @"SecurePhone";
    
}

- (void)viewDidUnload
{
    [self setFirstNameUITextField:nil];
    [self setLastNameUITextField:nil];
    [self setLoginUITextField:nil];
    [self setPasswdUITextField:nil];
    [self setConfirmPasswdUITextField:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


#import "enableAccountUIViewController.h"

- (void)doWebLoginProcess
{
#if GENTRICE_DEBUG>0
    NSLog(@">>>>>>> new account:\n%@\n%@\n%@\n%@\n", firstNameUITextField.text, lastNameUITextField.text, passwdUITextField.text, confirmPasswdUITextField.text);
#endif
    
    //show welcome
    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:@"歡迎" message:@"請至官方網站依照付費流程來啓用您的帳號" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
    [alert show];
    [alert release];
    
    
    UIViewController *enableView=nil;
    
    enableView = [[enableAccountUIViewController alloc] initWithNibName:@"enableAccountUIViewController" bundle:nil];
    enableView.title = @"";
     
    if (enableView!=nil) {
        //[enableView setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:enableView animated:YES];
        [enableView release];
    }
    
}

- (void)returnToLastPage
{
    [self.navigationController.navigationBar setHidden:YES];
    [self.navigationController popViewControllerAnimated:YES];
}
   

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    NSLog(@"##### textFieldShouldReturn.");
    
#if 0
    if (textField == confirmPasswdUITextField){
        [confirmPasswdUITextField resignFirstResponder];
        [self doWebLoginProcess];
    } else {
        if (textField == firstNameUITextField) {
            [lastNameUITextField becomeFirstResponder];
        } else if (textField == lastNameUITextField) {
            [loginUITextField becomeFirstResponder];
        } else if (textField == loginUITextField) {
            [passwdUITextField becomeFirstResponder];
        } else if (textField == passwdUITextField) {
            [confirmPasswdUITextField becomeFirstResponder];
        }
    }
#else
    //[textField resignFirstResponder];
    
    if (textField == firstNameUITextField) {
        [lastNameUITextField becomeFirstResponder];
    } else if (textField == lastNameUITextField) {
        [loginUITextField becomeFirstResponder];
    } else if (textField == loginUITextField) {
        [passwdUITextField becomeFirstResponder];
    } else if (textField == passwdUITextField) {
        [confirmPasswdUITextField becomeFirstResponder];
    } else {
        [textField resignFirstResponder];
    }
        
#endif
    
    return YES;
}



- (void)dealloc {
    [firstNameUITextField release];
    [lastNameUITextField release];
    [loginUITextField release];
    [passwdUITextField release];
    [confirmPasswdUITextField release];
    [super dealloc];
}
@end
