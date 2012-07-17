//
//  ContactDetailViewController.m
//  vbyantisipgui
//
//  Created by  on 2012/6/5.
//  Copyright (c) 2012年 antisip. All rights reserved.
//


#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "ContactsViewController.h"

#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"

#import "ContactDetailViewController.h"
#import "Contacts.h"
//#import "ContactEntry.h"
//#import <CoreData/CoreData.h>
/*
@interface ContactDetailViewController ()
//- (void)configureView;
@end
*/
@implementation ContactDetailViewController
@synthesize cell_name;
@synthesize cell_secure;
@synthesize cell_company;
@synthesize cell_other;

//@synthesize detailItem = _detailItem;
//@synthesize fetchedResultsController = __fetchedResultsController;
//@synthesize managedObjectContext = __managedObjectContext;
//@synthesize editedObject;
//@synthesize undoManager;
@synthesize contact;

@synthesize profileImage,firstname,lastname,company, secureid,other;

@synthesize firstname_v,lastname_v,secureid_v,company_v,other_v;
@synthesize callBtnTitle;

@synthesize callButton;
@synthesize callBtnIcon;
@synthesize tableView;
@synthesize isReadyOnly;
/*
@synthesize fnstring;
@synthesize lnstring;
@synthesize cstring;
@synthesize snumber;
@synthesize ostring;
@synthesize cid;
*/
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


 //   myContactDb = [SqliteContactHelper alloc];
  //  [myContactDb open_database];  
/*
    UIColor *background = [[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"BG.png"]];
    self.view.backgroundColor = background;
    [background release];
*/
    self.title = NSLocalizedString(@"tabContactInfo", nil);
    
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundView = tableBgImage;
    [tableBgImage release];
    
    self.firstname.hidden = YES;
    self.lastname.hidden = YES;
    self.company.hidden = YES; 
    self.secureid.hidden = YES;
    self.other.hidden = YES;

    self.firstname.placeholder = NSLocalizedString(@"fphUserFirstname", nil);
    self.lastname.placeholder = NSLocalizedString(@"fphUserLastname", nil);
    self.secureid.placeholder = NSLocalizedString(@"fphUserSecureLineID", nil);
    self.company.placeholder = NSLocalizedString(@"fphUserCompanyName", nil);
    
    [self loadData];
    
    callBtnTitle.text = NSLocalizedString(@"btnCall", nil);
    if (isReadyOnly == NO) {
        self.navigationItem.rightBarButtonItem = self.editButtonItem;
    } else {
        [callButton setEnabled:NO]; //readOnly mode
        [callButton setHidden:YES];
        [callBtnIcon setHidden:YES];
        [callBtnTitle setHidden:YES];
    }

    other.layer.cornerRadius = 6;
    other.layer.masksToBounds = YES;    
    other.layer.borderWidth =1.0;
    other.layer.borderColor=[[UIColor colorWithWhite:0.702f alpha:1.0f] CGColor];
    
    [super viewDidLoad];   
}
/*
- (void)viewWillAppear:(BOOL)animated {
    // Redisplay the data.

	[self updateRightBarButtonItemState];
}
*/

-(void)loadData{

    self.firstname.text = [contact valueForKey:@"firstname"];     
    self.lastname.text = [contact valueForKey:@"lastname"];
    self.company.text =[contact valueForKey:@"company"];
    self.secureid.text = [contact valueForKey:@"secureid"];
    self.other.text = [contact valueForKey:@"other"];
    
    
    self.firstname_v.text = [contact valueForKey:@"firstname"]; 
    self.lastname_v.text = [contact valueForKey:@"lastname"];
    self.company_v.text = [contact valueForKey:@"company"];
    self.secureid_v.text = [contact valueForKey:@"secureid"];
    self.other_v.text = [contact valueForKey:@"other"];
    


}
- (void)viewDidUnload
{
    [self setCallBtnIcon:nil];
    [self setTableView:nil];
    [self setCell_other:nil];
    [self setCell_company:nil];
    [self setCell_secure:nil];
    [self setCell_name:nil];
    [super viewDidUnload];

    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)updateRightBarButtonItemState {
	// Conditionally enable the right bar button item -- it should only be enabled if the book is in a valid state for saving.
    self.navigationItem.rightBarButtonItem.enabled = [contact validateForUpdate:NULL];
}	



- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)dealloc {
    //[_detailItem release];

    //[editedObject release];
    [contact release];
    
    [profileImage release];
    [firstname release];
    [lastname release];
    [company release];
    [secureid release];
    [other release];
    [firstname_v release];
    [lastname_v release];
    [company_v release];
    [secureid_v release];
    [other_v release];
    
    [callButton release];
    [callBtnTitle release];

    
    [cell_name release];
    [cell_secure release];
    [cell_company release];
    [cell_other release];
    [tableView release];
    [callBtnIcon release];
    [super dealloc];
}

#import "dialStatusUIViewController.h"
#import "loginUINavigationController.h"

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
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.  
    
    NSTimeInterval animationDuration = 0.30f;          
    [UIView beginAnimations:@"ResizeForKeyboard" context:nil];          
    [UIView setAnimationDuration:animationDuration];          
    CGRect rect = CGRectMake(0.0f, 0.0f, self.view.frame.size.width, self.view.frame.size.height);          
    self.view.frame = rect;          
    [UIView commitAnimations];         
    [textField resignFirstResponder];  
    return YES;          
}  
/*
- (void)textFieldDidBeginEditing:(UITextField *)textField  
{          
    CGRect frame = textField.frame;  
    int offset = frame.origin.y + 72 - (self.view.frame.size.height - 216.0);//键盘高度216  
    NSTimeInterval animationDuration = 0.30f;                  
    [UIView beginAnimations:@"ResizeForKeyBoard" context:nil];                  
    [UIView setAnimationDuration:animationDuration];  
    float width = self.view.frame.size.width;                  
    float height = self.view.frame.size.height;          
    if(offset > 0)  
    {  
        CGRect rect = CGRectMake(0.0f, -offset,width,height);                  
        self.view.frame = rect;          
    }          
    [UIView commitAnimations];                  
} 
*/
- (BOOL)textViewShouldReturn:(UITextView *)textView   
{          
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.          
    NSTimeInterval animationDuration = 0.30f;          
    [UIView beginAnimations:@"ResizeForKeyboard" context:nil];          
    [UIView setAnimationDuration:animationDuration];          
    CGRect rect = CGRectMake(0.0f, 0.0f, self.view.frame.size.width, self.view.frame.size.height);          
    self.view.frame = rect;          
    [UIView commitAnimations];          
    [textView resignFirstResponder];  
    return YES;          
}  

/*
-(void)textViewDidBeginEditing:(UITextView *)textView{
    CGRect frame = textView.frame;  
    int offset = frame.origin.y + 120 - (self.view.frame.size.height - 216.0);//键盘高度216  
    NSTimeInterval animationDuration = 0.30f;                  
    [UIView beginAnimations:@"ResizeForKeyBoard" context:nil];                  
    [UIView setAnimationDuration:animationDuration];  
    float width = self.view.frame.size.width;                  
    float height = self.view.frame.size.height;          
    if(offset > 0)  
    {  
        CGRect rect = CGRectMake(0.0f, -offset,width,height);                  
        self.view.frame = rect;          
    }          
    [UIView commitAnimations]; 
    
}
 


-(BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString*)text  
{  
    
    if ([text isEqualToString:@"\n"]) {
        
        NSTimeInterval animationDuration = 0.30f;          
        [UIView beginAnimations:@"ResizeForKeyboard" context:nil];          
        [UIView setAnimationDuration:animationDuration];          
        CGRect rect = CGRectMake(0.0f, 0.0f, self.view.frame.size.width, self.view.frame.size.height);          
        self.view.frame = rect;          
        [UIView commitAnimations]; 
        
        [textView resignFirstResponder];   
        return NO;  
    }  
    return YES;  
}  
 */



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
        dialStatusUIViewController *dialView = [[dialStatusUIViewController alloc] initWithNibName:@"dialStatusUIViewController" bundle:nil];
        loginUINavigationController *dialNavView = [[loginUINavigationController alloc] initWithRootViewController:dialView];
        [dialNavView.navigationBar setHidden:YES];
        [self.tabBarController presentModalViewController:dialNavView animated:YES];
        if ([firstname.text isEqualToString:@""]&& [lastname.text isEqualToString:@""]) {
            [dialView setRemoteIdentity:secureid.text];
        } else {
            [dialView setRemoteIdentity:[NSString stringWithFormat:@"%@ %@", firstname.text, lastname.text]];
        }
        
        [dialView release];
        [dialNavView release];
    }
    
#endif
    
    
    
    return res;
}



#pragma mark -
#pragma mark Save and cancel operations



- (IBAction)save {
	
	// Set the action name for the undo operation.
 
    // Pass current value to the edited object, then pop.

	NSLog(@"####### %@ - %@ - %@ ",contact.firstname,contact.lastname,contact.secureid);
		NSManagedObjectContext *context = contact.managedObjectContext;
		NSError *error = nil;
		if (![context save:&error]) {
			/*
			 Replace this implementation with code to handle the error appropriately.
			 
			 abort() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development. If it is not possible to recover from the error, display an alert panel that instructs the user to quit the application by pressing the Home button.
			 */
			NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
			abort();
		}
	

    [self.navigationController popViewControllerAnimated:YES];
}



- (void)cancel{
    // Don't pass current value to the edited object, just pop.
    [self.navigationController popViewControllerAnimated:YES];
}


-(void) setEditing:(BOOL)editing animated:(BOOL)animated  
{  
    [super setEditing:editing animated:animated];
    [self.navigationItem setHidesBackButton:editing animated:animated];    
//    [super setEditing:editing animated:animated];  
    //self.navigationItem.rightBarButtonItem.enabled = !editing;  
    //[contactUITableView beginUpdates];  
    NSLog(@"######## setEditing");
    
    [self becomeFirstResponder];    
    // Add or remove the Add row as appropriate.  
    [self.tableView reloadData];
    if (editing){  
        
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
        [callBtnIcon setHidden:YES];
        self.callBtnTitle.hidden = YES;  
        
        //self.navigationItem.rightBarButtonItem.title = @"Done";  
  
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
        [callBtnIcon setHidden:NO];
        self.callBtnTitle.hidden = NO;        
        NSLog(@"#########update");
        //[self save];
        //[self updateContact];
    }    
    
 	if (!editing) {
		
        contact.firstname = self.firstname.text;
        contact.lastname = self.lastname.text;
        contact.company = self.company.text;
        contact.secureid = self.secureid.text;
        contact.other = self.other.text;
        if(self.lastname.text.length>0){
            contact.section_key = [[self.lastname.text substringToIndex:1] uppercaseString];
        }
        else{
            contact.section_key = nil;
        }
        
        NSString *alert_warning = [[[NSString alloc] init] autorelease];
        
        if(self.lastname.text.length==0){
            alert_warning = NSLocalizedString(@"altmContactWarningNoLN", nil);
        }
        else if(firstname.text.length==0){
            alert_warning = NSLocalizedString(@"altmContactWarningNoFN", nil);
        }
        else if(secureid.text.length==0){
            alert_warning = NSLocalizedString(@"altmContactWarningNoSID", nil);
            
        }
        
        
        if(alert_warning!=nil && alert_warning.length>0){
            
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmEditContactWarning", nil) 
                                                            message:alert_warning
                                                           delegate:nil cancelButtonTitle:NSLocalizedString(@"btnEnter", nil)
                                                  otherButtonTitles:nil];
            [alert show];
            [alert release]; 
            self.editing = YES;
            return;
        }
        
		// Save the changes.
		NSError *error;
		if (![contact.managedObjectContext save:&error]) {
			// Update to handle the error appropriately.
			NSLog(@"Unresolved error %@, %@", error, [error userInfo]);
			exit(-1);  // Fail
		}
        else{
         
         [self loadData];
        }
        
             
	} 
}  




#pragma mark -
#pragma mark Table view data source methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // 1 section
    return 4;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // 3 rows
    return 1;
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    switch (indexPath.section) {
        case 0: return cell_name;
        case 1: return cell_secure;                
        case 2: return cell_company;
        case 3: return cell_other;            
    }
    
    return nil;    

}


- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {


	if (indexPath.section == 0) {
		return 100;
	}
    else if (indexPath.section == 1 && self.editing == NO) {
        if(isReadyOnly == NO)
            return 115;
        else return 44;
	} 
    else if (indexPath.section == 3) {
		return 150;
	} 
    else if(self.editing){
		return 50;
	}
    else return 44;
}

/**
 Manage row selection: If a row is selected, create a new editing view controller to edit the property associated with the selected row.
 */
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	
    
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    switch (section) {
        case 1:
            return NSLocalizedString(@"sntSecureLineID", @"Secure Line ID");
        case 2:
            return NSLocalizedString(@"sntCompany", @"Company");
            
        case 3:
            return NSLocalizedString(@"sntUserInformation", @"User Information");
            
        default:
            break;
    }
    return nil;
}





- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath{
    NSLog(@"############# tableView canMoveRowAtIndexPath");
    return NO;
}


@end
