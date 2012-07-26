//
//  addContactViewController.m
//  vbyantisipgui
//
//  Created by  on 2012/6/21.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

//#import "UIViewControllerDialpad.h"
#import <CoreData/CoreData.h>
#import <QuartzCore/QuartzCore.h>

//#import "Contacts.h"
#import "SqliteContactHelper.h"
#import "addContactViewController.h"
#import "vbyantisipAppDelegate.h"
#import "AppEngine.h"

@implementation addContactViewController


@synthesize contact,default_secureid,sqlContactDB;
@synthesize tableView;

@synthesize cell_name,cell_company,cell_secureid,cell_other;


@synthesize company_field,secureid_field,other_field, lastname, firstname_field;
//@synthesize delegate;

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
    /*
    UIColor *background = [[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"BG.png"]];
    self.view.backgroundColor = background;
    [background release];
    */
    //NSLog(@"----------- addContactViewController viewDidLoad");
    self.title = NSLocalizedString(@"tabAddContact", nil);    
    UIBarButtonItem *rightBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(save)] autorelease];
    
    self.navigationItem.rightBarButtonItem = rightBarButtonItem;
    
    //[rightBarButtonItem release];
    
    UIBarButtonItem *leftBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(cancel)] autorelease];//CancelPage:
    self.navigationItem.leftBarButtonItem = leftBarButtonItem;
    
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundView = tableBgImage;
    [tableBgImage release];
    
    self.firstname_field.placeholder = NSLocalizedString(@"fphUserFirstname", nil);
    self.lastname.placeholder = NSLocalizedString(@"fphUserLastname", nil);
    self.secureid_field.placeholder = NSLocalizedString(@"fphUserSecureLineID", nil);
    self.company_field.placeholder = NSLocalizedString(@"fphUserCompanyName", nil);

    other_field.layer.cornerRadius = 6;
    other_field.layer.masksToBounds = YES;    
    other_field.layer.borderWidth =1.0;
    other_field.layer.borderColor=[[UIColor colorWithWhite:0.702f alpha:1.0f] CGColor];
    
/*
    //[leftBarButtonItem release];   
    self.tableView.scrollEnabled = YES;
    self.firstname_field.clearButtonMode = UITextFieldViewModeWhileEditing;	// has a clear 'x' button to the right
    self.lastname.clearButtonMode = UITextFieldViewModeWhileEditing;	// has a clear 'x' button to the right
    self.secureid_field.clearButtonMode = UITextFieldViewModeWhileEditing;	// has a clear 'x' button to the right
    self.company_field.clearButtonMode = UITextFieldViewModeWhileEditing;	// has a clear 'x' button to the right
    //self.other_field.clearButtonMode = UITextFieldViewModeWhileEditing;	// has a clear 'x' button to the right
*/    
    if(default_secureid!=nil) secureid_field.text = default_secureid;
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}

- (void)viewDidUnload
{
    [self setTableView:nil];
    [self setCell_company:nil];
    [self setCell_secureid:nil];
    [self setCompany_field:nil];
    [self setSecureid_field:nil];
    [self setCell_other:nil];
    [self setOther_field:nil];
    [self setLastname:nil];
    [self setFirstname_field:nil];
    [self setCell_name:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


-(void)cancel{

    [self.navigationController popViewControllerAnimated:YES];
}



- (void)save{
    //NSLog(@"######### save");
    contact =  [[[ContactsEntry alloc]init] autorelease];
    
    contact.firstname = self.firstname_field.text;
    contact.lastname = self.lastname.text;
    contact.company = self.company_field.text;
    contact.secureid = self.secureid_field.text;
    contact.other = self.other_field.text;
    
    
    for(int i=0;i<contact.firstname.length;i++){
        unichar current = [contact.firstname characterAtIndex:i];
        NSLog(@"%i: %C\n",i,current);
    }
    

    
    
    if(self.lastname.text.length>0){
        
        contact.section_key = [[[NSString stringWithFormat:@"%@", self.lastname.text] substringToIndex:1] uppercaseString];

    }else{contact.section_key = nil;}
    
    //NSLog(@">>>>>>> section_key:%@",contact.section_key);
    
    NSString *alert_warning = [[[NSString alloc] init] autorelease];
    /*
    if(self.lastname.text.length==0){
        alert_warning = NSLocalizedString(@"altmContactWarningNoLN", nil);
    }
    else if(firstname_field.text.length==0){
        alert_warning = NSLocalizedString(@"altmContactWarningNoFN", nil);
    }
    else */if(secureid_field.text.length==0){
        alert_warning = NSLocalizedString(@"altmContactWarningNoSID", nil);
    
    }

    
    if(alert_warning!=nil && alert_warning.length>0){
    
        UIAlertView *alert_view = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmAddContactWarning", nil) 
                                                        message:alert_warning
                                                       delegate:self cancelButtonTitle:NSLocalizedString(@"btnEnter", nil)
                                              otherButtonTitles:nil];
        
    [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] setCurrentAlert:alert_view];
        
        [alert_view show];
        [alert_view release];        
        return;
    }
    
    sqlContactDB = [[[SqliteContactHelper alloc] init] autorelease];
    
    [sqlContactDB insert_contact:contact];

	//[delegate addContactViewController:self didFinishWithSave:YES];
    [self.navigationController popViewControllerAnimated:YES];
}


- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    //NSLog(@"### addContactViewController alter view button index=%d", buttonIndex);
    
    [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] setCurrentAlert:nil];
    
}


-(void)dealloc{

    [tableView release];
    [cell_company release];
    [cell_secureid release];
    [cell_name release];
    [cell_other release];
    [other_field release];
    [lastname release];
    [firstname_field release];

    [company_field release];
    [secureid_field release];    
    [super dealloc];    
    
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



#define LM_MAXLENGTH 256
#define FM_MAXLENGTH 256
#define SID_MAXLENGTH 64
#define CP_MAXLENGTH 256
#define UI_MAXLENGTH 256



- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {

    if(textField == self.firstname_field){    
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        return (newLength > FM_MAXLENGTH) ? NO : YES;            
    }
    else if(textField == self.lastname)    
    {
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        return (newLength > LM_MAXLENGTH) ? NO : YES;      
    }
    else if(textField == self.secureid_field)    
    {
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        return (newLength > SID_MAXLENGTH) ? NO : YES;          
    }    
    else if(textField == self.company_field)    
    {
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        return (newLength > CP_MAXLENGTH) ? NO : YES;          
    }
    
    return YES;
}

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text {

    if(textView == self.other_field){    
        NSUInteger newLength = [textView.text length] + [text length] - range.length;

        return (newLength > UI_MAXLENGTH) ? NO : YES;            
    }
        
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
 */


//20120627 modify tableview version

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
        case 1: return cell_secureid;                
        case 2: return cell_company;
        case 3: return cell_other;            
    }
    
    return nil;    

}


- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	if (indexPath.section == 0) {
		return 100;
	}
    else if (indexPath.section == 3) {
		return 150;
	} 
    else {
		return 44;
	}
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

@end
