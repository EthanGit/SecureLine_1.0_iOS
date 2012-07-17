//
//  newContactUIViewController.m
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/29.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "newContactUIViewController.h"
#import "vbyantisipAppDelegate.h"
#import "UIViewControllerDialpad.h"

#import "AppEngine.h"
//#import "UITableViewCellFavorites.h"

@implementation newContactUIViewController
@synthesize headUIImageView;
@synthesize firstNameUITextField;
@synthesize lastNameUITextField;
@synthesize companyUITextField;
@synthesize securePhoneIDUITextField;
@synthesize othersUITextView;

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
    // Do any additional setup after loading the view from its nib.
}

- (void)viewDidUnload
{
    [self setHeadUIImageView:nil];
    [self setFirstNameUITextField:nil];
    [self setLastNameUITextField:nil];
    [self setCompanyUITextField:nil];
    [self setSecurePhoneIDUITextField:nil];
    [self setOthersUITextView:nil];
    [super viewDidUnload];
    

    
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    NSLog(@"##### textFieldShouldReturn.");
    
    [textField resignFirstResponder];
    
    return YES;
}


-(IBAction)addInContactList:(id)sender{
    NSLog(@"#########addInContactList");
    current_contact = [[ContactEntry alloc]init];    

    //if (firstNameUITextField)
        [current_contact setFirstname: [NSString stringWithFormat:@"%@", firstNameUITextField.text]];
    //if (lastname)
        [current_contact setLastname: [NSString stringWithFormat:@"%@", lastNameUITextField.text]];
    [current_contact setOther:[NSString stringWithFormat:@"%@", othersUITextView.text]];
    [current_contact setCompany:[NSString stringWithFormat:@"%@", companyUITextField.text]];
    [current_contact setPhone_string:[NSString stringWithFormat:@"%@", securePhoneIDUITextField.text]];
    //[current_contact setPhone_numbers:[NSString stringWithFormat:@"%@", securePhoneIDUITextField.text]];
    SipNumber *sip_number = [SipNumber alloc];
    [sip_number setPhone_type:@"voip"];
    [sip_number setPhone_number:[NSString stringWithFormat:@"%@", securePhoneIDUITextField.text]];    
    [current_contact.phone_numbers insertObject:sip_number atIndex:0];

     NSLog(@"######### %@ , %@, %@ , %@ , %@",firstNameUITextField.text,lastNameUITextField.text,othersUITextView.text,securePhoneIDUITextField.text,sip_number);   
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

    [myContactDb insert_contact:current_contact];
    [ContactEntry release];
    
    /*
    
    [myFavoriteList addObject:current_contact];
    [myFavoriteContactDb insert_contact:current_contact];
    [current_contact release];*/
    current_contact=nil;
    [self.navigationController popViewControllerAnimated:YES];
    //[phonelist reloadData];
}


-(IBAction)CancelPage:(id)sender{
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)viewWillAppear:(BOOL)animated 
{
    
    [super viewWillAppear:animated];
    
    //20120525 SANJI[
    NSLog(@"######## viewWillAppear start");
    //self.navigationController.navigationBar.tintColor = [UIColor blackColor];
    
    self.title = @"";
    
    UIBarButtonItem *rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(addInContactList:)];
    
    self.navigationItem.rightBarButtonItem = rightBarButtonItem;
    
    [rightBarButtonItem release];
    
      
    UIBarButtonItem *leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(CancelPage:)];
    self.navigationItem.leftBarButtonItem = leftBarButtonItem;
    [leftBarButtonItem release];
    
    
    NSLog(@"######## viewWillAppear end");    
    //] SANJI
}

@end
