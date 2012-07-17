//
//  moreUITableViewController.m
//  vbyantisipgui
//
//  Created by arthur on 2012/5/23.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "moreUITableViewController.h"
//#import "aboutUIViewController.h"
#import "personalInfoUIViewController.h"
#import "inviteFriendUIViewController.h"
#import "reportUIViewController.h"
#import "aboutUITableViewController.h"
#import "vbyantisipAppDelegate.h"

@implementation moreUITableViewController
@synthesize moreTabUIImageView;
@synthesize passcode;

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
        //[self.view setBackgroundColor:[[UIColor alloc] initWithPatternImage:[UIImage imageNamed:@"BG.png"]]];
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
    
    self.navigationItem.title = NSLocalizedString(@"tabMore",nil); 
    self.tableView.backgroundView = moreTabUIImageView;
    
    moreItemsArray = [[NSMutableArray alloc] initWithObjects: NSLocalizedString(@"moreAbout",nil), NSLocalizedString(@"morePSInfo",nil), NSLocalizedString(@"moreInvite",nil), NSLocalizedString(@"morePasscode",nil), NSLocalizedString(@"moreReport",nil), NSLocalizedString(@"moreLogout",nil),  nil];
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
    
    UIView *v=[[UIView alloc] init];
    [self.tableView setTableFooterView:v];
    [v release];
    
}

- (void)viewDidUnload
{
    [self setMoreTabUIImageView:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
//#warning Potentially incomplete method implementation.
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
//#warning Incomplete method implementation.
    // Return the number of rows in the section.
    return moreItemsArray.count;
}



- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier; // = @"Cell";
    
    if ([indexPath row]==3) {
        CellIdentifier = @"CellPasscode";
    } else {
        CellIdentifier = @"Cell";
    }
    
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        if ([indexPath row]==3) {
            passcode = [[[UISwitch alloc] initWithFrame:CGRectZero] autorelease];
            [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:NO];
            [passcode addTarget:self action:@selector(passcodeSwitchValueChanged:) forControlEvents:UIControlEventValueChanged];
            [cell setAccessoryView:passcode];
            //[cell setSelectionStyle:UITableViewCellSelectionStyleNone]; 
        }
    }
    
    // Configure the cell...
    cell.textLabel.text = [moreItemsArray objectAtIndex:[indexPath row]];
    //cell.textLabel.textColor = [UIColor whiteColor];
    
    if ([indexPath row] != 3 && [indexPath row] != 5) { //skip the passcode & logout items
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        //cell.accessoryView = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"arrow.png"]] autorelease];
        //cell.accessoryView.backgroundColor = [UIColor whiteColor];
    } else {
        [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
    }
    
    return cell;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
 
    [textField becomeFirstResponder];
    return YES;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
        
    UIViewController *detailView=nil;
    vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];

    switch ([indexPath row]) {
        case 0: //about
            //detailView = [[ aboutUIViewController alloc] initWithNibName:@"aboutUIViewController" bundle:nil];
            detailView = [[ aboutUITableViewController alloc] initWithNibName:@"aboutUITableView" bundle:nil];
            detailView.title = @"";
            break;
        case 1: //personal information
            detailView = [[ personalInfoUIViewController alloc] initWithNibName:@"personalInfoUIViewController" bundle:nil];
            detailView.title = @"";
            break;
        case 2: //invite friends
            detailView = [[ inviteFriendUIViewController alloc] initWithNibName:@"inviteFriendUIViewController" bundle:nil];
            detailView.title = @"";
            break;
        case 3: //passcode
            
            break;
        case 4: //report
            detailView = [[ reportUIViewController alloc] initWithNibName:@"reportUIView" bundle:nil];
            detailView.title = @"";
            break;
        case 5: //logout
            NSLog(@"### Logout.");
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"moreLogout", @"") message:NSLocalizedString(@"altmlogoutcheck", @"") delegate:self cancelButtonTitle:NSLocalizedString(@"btnCancel", nil) otherButtonTitles:NSLocalizedString(@"Ok", nil), nil];
            [appDelegate setCurrentAlert:alert];
            [alert show];
            [alert release];
            break;
        default:
            break;
    }
    
    if (detailView!=nil) {
        [detailView setHidesBottomBarWhenPushed:YES];
        [self.navigationController pushViewController:detailView animated:YES];
        //[detailView release];
        [detailView autorelease];
    }
    
}


- (void)passcodeSwitchValueChanged:(id)sender {
    NSLog(@"###### passcodeSwitchValueChanged");
    //[[NSUserDefaults standardUserDefaults] setBool:[sender isOn] forKey:@"PasscodeEnabled"];
   
    //if([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]==YES){
    if([sender isOn]==YES){
        //[[NSUserDefaults standardUserDefaults] setObject:@"1234" forKey:@"PasscodeString"];
        [self setPasscode];
    }
    else{
        [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
    }
    
    
}


- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    
    if ([alertView.title isEqualToString:NSLocalizedString(@"altPasscodeSetting", @"")]) {
        if (buttonIndex != [alertView cancelButtonIndex]) {
            NSLog(@"Launching the store");
            //replace appname with any specific name you want
            
            if([checkpasscordString.text isEqualToString:passcordString.text] && checkpasscordString.text.length==4){
                [[NSUserDefaults standardUserDefaults] setObject:passcordString.text forKey:@"PasscodeString"];
                [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PasscodeEnabled"];
            }
            else{
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmSettingError", nil) 
                                                                message:NSLocalizedString(@"altmPasscodemismatch",nil)
                                                               delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                                      otherButtonTitles:nil];
                [alert show];
                [alert release];
                
                [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
                [passcode setOn:NO animated:YES];
                
                //NSLog(@"passcord switch :%@",[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]);
            }
            
        }else {
#if 0
            NSLog(@" Passcode set NO");
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
            [passcode setOn:NO animated:YES];
#endif            
            
        }
    }
}

#define MAXLENGTH 4

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    NSUInteger newLength = [textField.text length] + [string length] - range.length;
    return (newLength > MAXLENGTH) ? NO : YES;
    /*
    NSLog(@"string:%i",[passcordString.text length]);
    if ([passcordString.text length] >= MAXLENGTH) {
        passcordString.text = [passcordString.text substringToIndex:MAXLENGTH-];
        return YES;
    }
    if ([checkpasscordString.text length] >= MAXLENGTH) {
        checkpasscordString.text = [checkpasscordString.text substringToIndex:MAXLENGTH];
        return NO;
    }
    return YES;*/
}


-(void)setPasscode{
    
    settingAlertPasscode = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altPasscodeSetting", nil) message:@"\n\n\n" delegate:self cancelButtonTitle:NSLocalizedString(@"btnCancel", nil) otherButtonTitles:NSLocalizedString(@"Ok", nil), nil];
    
    
    passcordString = [[[UITextField alloc] initWithFrame:CGRectMake(12.0, 50.0, 260.0, 25.0)] autorelease];
    [passcordString setPlaceholder:NSLocalizedString(@"fphNewPasscode", nil)];
    [passcordString setBackgroundColor:[UIColor whiteColor]];
    [passcordString setAutocorrectionType:UITextAutocorrectionTypeNo];
    [passcordString setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [passcordString setSecureTextEntry:YES];    
    [passcordString setKeyboardType:UIKeyboardTypeNumberPad];
    [passcordString setDelegate:self];
    [settingAlertPasscode addSubview:passcordString];
    checkpasscordString = [[[UITextField alloc] initWithFrame:CGRectMake(12.0, 85.0, 260.0, 25.0)] autorelease];
    [checkpasscordString setPlaceholder:NSLocalizedString(@"fphConfirmPasscode", nil)];
    [checkpasscordString setSecureTextEntry:YES];
    [checkpasscordString setBackgroundColor:[UIColor whiteColor]];
    [checkpasscordString setAutocorrectionType:UITextAutocorrectionTypeNo];
    [checkpasscordString setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [checkpasscordString setKeyboardType:UIKeyboardTypeNumberPad];
    [checkpasscordString setDelegate:self];
    [settingAlertPasscode addSubview:checkpasscordString];
    
    [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] setCurrentAlert:settingAlertPasscode];
    
    [settingAlertPasscode show];
    //[alert release];
    
    [passcordString becomeFirstResponder];

}

#import "AppEngine.h"

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
NSLog(@"### alter view button index=%d", buttonIndex);
    
    [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] setCurrentAlert:nil];
    
    if ([alertView.title isEqualToString:NSLocalizedString(@"moreLogout", @"")]) {
        if (buttonIndex==1) { //ok
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"LoginStatus"];
            NSLog(@">>>LoginStatus=%@", [[NSUserDefaults standardUserDefaults] objectForKey:@"LoginStatus"]);
            [self toLoginUI];
            
            int oldRegInterval = [[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"];
            [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"reginterval_preference"];
            [gAppEngine refreshRegistration];
            sleep(1);
            [[NSUserDefaults standardUserDefaults] setInteger:oldRegInterval forKey:@"reginterval_preference"];
        }
    } else if ([alertView.title isEqualToString:NSLocalizedString(@"altPasscodeSetting", @"")]) {
        if (buttonIndex == [alertView cancelButtonIndex]) {
            NSLog(@" Passcode set NO");
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
            [passcode setOn:NO animated:YES];
        }
        
    }
    
}


#import "loginUIViewController.h"
#import "loginUINavigationController.h"

-(void)toLoginUI{
    
    UIViewController *loginView = [[loginUIViewController alloc] initWithNibName:@"loginUIViewController" bundle:nil];
    loginUINavigationController *loginNavView = [[loginUINavigationController alloc] initWithRootViewController:loginView];
    [loginNavView.navigationBar setHidden:YES];
    [self.tabBarController presentModalViewController:loginNavView animated:NO];
    [loginView release];
    [loginNavView release];
}

- (void)dealloc {
    [moreTabUIImageView release];
    [super dealloc];
}
@end
