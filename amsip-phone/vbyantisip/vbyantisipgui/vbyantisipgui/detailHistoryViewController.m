//
//  detailHistoryViewController.m
//  vbyantisipgui
//
//  Created by  on 2012/5/25.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "NetworkTrackingDelegate.h"
#import "AppEngine.h"

#import "UIViewControllerHistory.h"

#import "UIViewControllerDialpad.h"
#import "vbyantisipAppDelegate.h"
#import "RecentCellTableView.h"
#import "detailHistoryViewController.h"
#import "UITableViewCellHistory.h"
//#import "Recents.h"
//#import "RecentsEntry.h"

@interface detailHistoryViewController ()
- (void)configureCell:(UITableViewCell *)cell atIndexPath:(NSIndexPath *)indexPath;

@end

@implementation detailHistoryViewController


@synthesize first_name;
@synthesize callBtnTitle;
//@synthesize last_name;
@synthesize secure_id;
//@synthesize h_index;
@synthesize fnstring;
@synthesize phonenum;
@synthesize secureid;
@synthesize profileImage;
@synthesize callflow_image;
@synthesize duration;
@synthesize call_time;
@synthesize cell_addbutton;
@synthesize addContactButton;
@synthesize cell_profile;
@synthesize cell_record;
@synthesize tableView;
@synthesize showAddButton;

//@synthesize historyEntry;

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
    NSLog(@"############## detailHistoryViewController viewDidLoad ");
    [super viewDidLoad];
    self.title = NSLocalizedString(@"tabRecentInfo", nil);
    /*
    UIColor *background = [[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundColor = background;
    [background release];
    */
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundView = tableBgImage;
    [tableBgImage release]; 
    
    
    
    //HistoryEntry *historyEntry = [myHistoryList objectAtIndex:h_index];
    //self.first_name.text = fnstring;     

    self.first_name.text = fnstring;
    self.secure_id.text = secureid;
    callBtnTitle.text = NSLocalizedString(@"btnCall", nil);
    /*
    UIImageView *view = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"profile-default"]] ;
    //self.profileImage.frame = CGRectMake(25, 25, 100, 100);
    [view release];
    */
    myHistoryList = [[NSMutableArray arrayWithObjects:nil] retain];
    myHistoryDb = [SqliteRecentsHelper alloc];
    [myHistoryDb open_database];
    //[myHistoryDb load_history:myHistoryList];
    
    [myHistoryDb find_callid:myHistoryList:secureid];
    [myHistoryDb release];
    UIView *v=[[UIView alloc] init];
    [self.tableView setTableFooterView:v];
    [v release];    
}


- (void)viewWillAppear:(BOOL)animated 
{
    if([self.first_name.text isEqualToString:@"Unknown"]){    
        fnstring = [self lookupDisplayName:secureid];
        if(fnstring.length>0){
            self.first_name.text = fnstring;
            showAddButton = NO;
        }
    }
    
    [self.tableView reloadData];
    [super viewWillAppear:YES];    
}
-(void)back {
	// Tell the controller to go back
    NSLog(@"######## back action");
	[self.navigationController popViewControllerAnimated:YES];
}

- (void)viewDidUnload
{
    [self setFirst_name:nil];
//    [self setLast_name:nil];
    [self setSecure_id:nil];
    [self setDuration:nil];
    [self setCall_time:nil];
    [self setCallflow_image:nil];
    [self setCell_profile:nil];
    [self setCell_record:nil];
    [self setTableView:nil];
    [self setCallBtnTitle:nil];
    [self setShowAddButton:NO];
    [self setCell_addbutton:nil];
    [self setAddContactButton:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


#import "dialStatusUIViewController.h"
#import "loginUINavigationController.h"

- (IBAction)callNumber:(id)sender {
    
   int res;

    res = [self dial:phonenum];
    //return;
}





//20120627 modify tableview version

#pragma mark -
#pragma mark Table view data source methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // 1 section
    if(showAddButton==YES){
        return 3;
    }   
    return 2;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // 3 rows
    if(section==0){
        return 1;
    }
    else if(section==1){       
        return [myHistoryList count];
    }
    else if(section==2 && showAddButton==YES){       
        return 1;
    }    
    return 0;
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@">>> section:%i / row:%i",indexPath.section,indexPath.row);
    if(indexPath.section==0){
        //[cell_profile setBackgroundColor:[UIColor clearColor]];
        return cell_profile;
    }
    else if(indexPath.section==1)
    {
        
        static NSString *CellIdentifier = @"CellContact";
        
        RecentCellTableView *Cell = (RecentCellTableView *)[self.tableView dequeueReusableCellWithIdentifier:CellIdentifier];
        if (Cell == nil) {
            Cell = [[[RecentCellTableView alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
                //Cell.selectionStyle = nil;
                //Cell.selected = NO;
            [Cell setAccessoryType:UITableViewCellAccessoryNone];
            [Cell setSelectionStyle:UITableViewCellSelectionStyleNone];
            [Cell setBackgroundColor:[UIColor clearColor]];// setBackgroundColor
            //Cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        }
        NSLog(@"cellForRowAtIndexPath : %d",indexPath.row);
        [self configureCell:Cell atIndexPath:indexPath];
        
        return Cell;
    }
    else if(indexPath.section==2 && indexPath.row==0 && showAddButton==YES)
    {
        return cell_addbutton;
    }
    
    return nil;  
    
}


- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSLog(@">>> section:%i / row:%i",indexPath.section,indexPath.row);    
	if (indexPath.section == 0) {
		return 140;
	}
    else if(indexPath.section ==2){
		return 80;
	}
    
    return 44;

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
            return NSLocalizedString(@"Recent", nil);

        default:
            break;
    }
    return nil;
}

- (void)configureCell:(RecentCellTableView *)cell atIndexPath:(NSIndexPath *)indexPath {

    RecentsEntry *recent = nil;

    recent = [myHistoryList objectAtIndex:indexPath.row];
    NSLog(@"callid = %@,remoteuri= %@,secureid= %@,start_date= %@,end_date= %@,duration= %@,direction= %@,sip_code= %@,sip_reason= %@",recent.callid,recent.remoteuri,recent.secureid,recent.start_date,recent.end_date,recent.duration,recent.direction,recent.sip_code,recent.sip_reason);
    cell.recentEntry = recent;
    
  
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
        dialStatusUIViewController *dialView = [[dialStatusUIViewController alloc] initWithNibName:@"dialStatusUIViewController" bundle:nil];
        loginUINavigationController *dialNavView = [[loginUINavigationController alloc] initWithRootViewController:dialView];
        [dialNavView.navigationBar setHidden:YES];
        [self.tabBarController presentModalViewController:dialNavView animated:YES];
        //[self.navigationController.navigationBar setHidden:YES];
        //[self.navigationController pushViewController:dialView animated:YES]; 
        
        //NSLog(@">>>>> name length=%d, string=%@, secureid=%@", [first_name.text length], first_name.text, secure_id.text);
        if ([first_name.text isEqualToString:@" "]) {
            [dialView setRemoteIdentity:secure_id.text];
        } else {
            [dialView setRemoteIdentity:first_name.text];
        }
        
        [dialView release];
        [dialNavView release];
    }
    
#endif
    return res;
}


- (void)dealloc {
    [duration release];
    [call_time release];
    [callflow_image release];
    [cell_profile release];
    [cell_record release];
    [tableView release];
    [callBtnTitle release];
    [cell_addbutton release];
    [addContactButton release];
    [super dealloc];
}

#import "SqliteContactHelper.h"

- (NSString*) lookupDisplayName:(NSString*) caller {
    NSString *result;
    
   // NSString *secureID = [self stripSecureIDfromURL:caller];
    //NSLog(@"$$$$$ SecureID = %@", caller);
    
    SqliteContactHelper *contactsDB = [SqliteContactHelper alloc];
    [contactsDB open_database];
    result = [contactsDB find_contact_name:caller];

    return result;
}

#import "addContactViewController.h"


- (IBAction)addContactAction:(id)sender {

    addContactViewController *addViewController = [[addContactViewController alloc]  initWithNibName:@"addContactViewController" bundle:nil];	
//    addViewController.secureid_field.text = self.secure_id.text;
    
    ((addContactViewController *)addViewController).default_secureid = secureid;
    
    
    [addViewController setHidesBottomBarWhenPushed:YES];
    [self.navigationController pushViewController:addViewController animated:YES];
    //[detailView release];
    [addViewController autorelease];
}
@end
