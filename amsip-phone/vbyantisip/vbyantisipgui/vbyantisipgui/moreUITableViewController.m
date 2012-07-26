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
@synthesize set_passcode_title;

@synthesize set_pass_bg;
@synthesize old_passcode_label;
@synthesize new_passcode_label;
@synthesize check_passcode_label;
@synthesize old_passcode_field;
@synthesize new_passcode_field;
@synthesize check_passcode_field;
@synthesize set_passcode_button,set_passcode_button_2;
@synthesize alert_message,message_string;
@synthesize moreTabUIImageView;
@synthesize cell_setting_passcode;
@synthesize passcode,showPassSetting;

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
    self.showPassSetting = NO;
        
    /*
    [self.new_passcode_field setPlaceholder:NSLocalizedString(@"fphNewPasscode", nil)];    
    [self.old_passcode_field setPlaceholder:NSLocalizedString(@"fphOldPasscode", nil)];    

    [self.check_passcode_field setPlaceholder:NSLocalizedString(@"fphConfirmPasscode", nil)];
    
    [self.old_passcode_label setText:NSLocalizedString(@"labOldPasscode", nil)];
    [self.new_passcode_label setText:NSLocalizedString(@"labNewPasscode", nil)];
    [self.check_passcode_label setText:NSLocalizedString(@"labCheckPasscode", nil)]; 
    [self.set_passcode_button setTitle:NSLocalizedString(@"btnEnter", nil) forState:UIControlStateNormal];
    */
    [self.set_passcode_title setText:NSLocalizedString(@"labSettingPasscodeTitle", nil)];
    [self defaultPassCodeSetting];
    [self set_passcode_setting_ui_display];
    
    
    UIView *v=[[UIView alloc] init];
    [self.tableView setTableFooterView:v];
    [v release];
    
}

- (void)viewDidUnload
{
    [self setMoreTabUIImageView:nil];
    [self setCell_setting_passcode:nil];
    
    [self setSet_pass_bg:nil];
    [self setOld_passcode_label:nil];
    [self setNew_passcode_label:nil];
    [self setCheck_passcode_label:nil];
    [self setOld_passcode_field:nil];
    [self setNew_passcode_field:nil];
    [self setCheck_passcode_field:nil];
    [self setSet_passcode_button:nil];
     
    [self setSet_passcode_title:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated
{
    
    NSLog(@"-------- viewWillAppear");
    [super viewWillAppear:animated];
    self.showPassSetting = NO;  
    [self set_passcode_setting_ui_display]; 
    [self.passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:NO];
    
    [self.tableView reloadData];
    
    

}
/*
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
 */

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
    /*
    if([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] == NO) return 6;
    else return 7;*/
    return 6;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
//#warning Incomplete method implementation.
    // Return the number of rows in the section.
 
//#if 0    
    if(([self.passcode isOn] || [[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]==YES) && section==3){
        
       // if([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] == YES && section==3) return 2;
        return 2;
    }
//#endif    
    
    return 1;
    
    
    //return moreItemsArray.count;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    /*
    if([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] == YES){
        if([indexPath section]==3 && [indexPath row]==1){        
            return 260;        
        }    
    }
    */
    if([self.passcode isOn] && [indexPath section]==3){
        if([indexPath row]==1 && self.showPassSetting==YES){        
            return 260;        
        }   
    }    
    
    
    return 50;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
   // NSLog(@"---------- section:%i /rows:%i",[indexPath section], [indexPath row]);
    //if(([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] == YES || self.showPassSetting==YES) && [indexPath section]==3 && [indexPath row]==1 ){ 
    if(([self.passcode isOn] || self.showPassSetting==YES) && [indexPath section]==3 && [indexPath row]==1 ){ 
            //cell_setting_passcode.accessoryType = UITableViewCellAccessoryDisclosureIndicator;    
        //NSLog(@">>>>>>>>> show cell_setting_passcode");
        NSString *settingmode = @"setting";
        if([[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]==YES){
            settingmode = @"modify";
        }
        
        [self PasscodeSettingCGRectMake:settingmode];
        
        return cell_setting_passcode;          
    }
    
    static NSString *CellIdentifier; // = @"Cell";
    
    if ([indexPath section]==3) CellIdentifier = @"CellPasscode";
    else  CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        if ([indexPath row]==0 && [indexPath section]==3) {
            passcode = [[[UISwitch alloc] initWithFrame:CGRectZero] autorelease];
            [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:NO];
            [passcode addTarget:self action:@selector(passcodeSwitchValueChanged:) forControlEvents:UIControlEventValueChanged];
            [cell setAccessoryView:passcode];
            //[cell setSelectionStyle:UITableViewCellSelectionStyleNone]; 
        }
    }
    
    // Configure the cell...
    if([indexPath row]==0) cell.textLabel.text = [moreItemsArray objectAtIndex:[indexPath section]];
    //cell.textLabel.textColor = [UIColor whiteColor];
    
    if ([indexPath section] != 3 && [indexPath section] != 5) { //skip the passcode & logout items
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        //cell.accessoryView = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"arrow.png"]] autorelease];
        //cell.accessoryView.backgroundColor = [UIColor whiteColor];
    }
    else {
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



- (BOOL)textFieldShouldReturn:(UITextField *)textField
{

    //[textField becomeFirstResponder];
    [textField resignFirstResponder];
    return YES;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
        
    UIViewController *detailView=nil;
    vbyantisipAppDelegate *appDelegate = (vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate];

    switch ([indexPath section]) {
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
            if([indexPath row]==1 && [[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]==YES && [self.passcode isOn]){

                
                self.showPassSetting = self.showPassSetting==YES?NO:YES;
                if(self.showPassSetting==YES)
                NSLog(@"------ update showPassSetting:YES");
                else {
                    NSLog(@"------ update showPassSetting:NO");
                }
                
                [self set_passcode_setting_ui_display];
                [self.tableView reloadData];
            
            }
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
        
        self.showPassSetting = YES;
        [self set_passcode_setting_ui_display];        
         
        //[self setPasscode];
    }
    else if([sender isOn]==NO && [[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]==YES){

        //[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
        [self alertPassCodeDisabled];
        self.showPassSetting = NO;
        [self set_passcode_setting_ui_display];

         
    }
    
    [self.tableView reloadData];    
}


- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
 /*   
    if ([alertView.title isEqualToString:NSLocalizedString(@"altPasscodeSetting", @"")]) {
        if (buttonIndex != [alertView cancelButtonIndex]) {
            //NSLog(@"Launching the store");
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
            //NSLog(@" Passcode set NO");
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
            [passcode setOn:NO animated:YES];
#endif            
            
        }
    }
    else */
    if ([alertView.title isEqualToString:NSLocalizedString(@"altPasscodeDisabledSetting", @"")]) {
        
        
        if (buttonIndex != [alertView cancelButtonIndex]) {
            
           message_string = [[[NSString alloc] initWithString:@""] autorelease];
            
            if([passcordString.text isEqualToString:[[NSUserDefaults standardUserDefaults] stringForKey:@"PasscodeString"]]){
               
                [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
                    
            }
            else{
                message_string = NSLocalizedString(@"altmOldPasscodeError",nil); 
                [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:NO];
            }
            
            if(message_string.length>0){
                alert_message = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmSettingError", nil) 
                                                                message:message_string
                                                               delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                                      otherButtonTitles:nil];
                [alert_message show];

                [alert_message release];
            }  
            

            //[message release];

        }
        else {
        
            [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:NO];
    
        }
        
        [self.tableView reloadData];                
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

-(void)alertPassCodeDisabled{
    settingAlertPasscode = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altPasscodeDisabledSetting", nil) message:@"\n\n" delegate:self cancelButtonTitle:NSLocalizedString(@"btnCancel", nil) otherButtonTitles:NSLocalizedString(@"Ok", nil), nil];
    
    
    passcordString = [[[UITextField alloc] initWithFrame:CGRectMake(12.0, 50.0, 260.0, 31.0)] autorelease];
    [passcordString setPlaceholder:NSLocalizedString(@"altmKeyinPassword", nil)];
    //[passcordString setBackgroundColor:[UIColor whiteColor]];
    [passcordString setAutocorrectionType:UITextAutocorrectionTypeNo];
    [passcordString setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [passcordString setSecureTextEntry:YES];    
    [passcordString setKeyboardType:UIKeyboardTypeNumberPad];
    [passcordString setBorderStyle:UITextBorderStyleRoundedRect];
    [passcordString setDelegate:self];
    [settingAlertPasscode addSubview:passcordString];
    //checkpasscordString = [[[UITextField alloc] initWithFrame:CGRectMake(12.0, 85.0, 260.0, 25.0)] autorelease];

    
    [(vbyantisipAppDelegate *)[[UIApplication sharedApplication] delegate] setCurrentAlert:settingAlertPasscode];
    
    [settingAlertPasscode show];
    [settingAlertPasscode autorelease];
    //[alert release];
    
    [passcordString becomeFirstResponder];
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
           // NSLog(@">>>LoginStatus=%@", [[NSUserDefaults standardUserDefaults] objectForKey:@"LoginStatus"]);
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
            [self toLoginUI];
            
            int oldRegInterval = [[NSUserDefaults standardUserDefaults] integerForKey:@"reginterval_preference"];
            [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:@"reginterval_preference"];
            [gAppEngine refreshRegistration];
            sleep(1);
            [[NSUserDefaults standardUserDefaults] setInteger:oldRegInterval forKey:@"reginterval_preference"];
        }
    }
    else if ([alertView.title isEqualToString:NSLocalizedString(@"altPasscodeSetting", @"")]) {
        if (buttonIndex == [alertView cancelButtonIndex]) {
            //NSLog(@" Passcode set NO");
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
            [passcode setOn:NO animated:YES];
        }
        
    }
    
}


-(void)set_passcode_setting_ui_display{

    if(self.showPassSetting==YES){
        NSLog(@"------ set_passcode_setting_ui_display :YES");
        
        self.old_passcode_field.text = @"";
        self.new_passcode_field.text = @"";
        self.check_passcode_field.text = @"";
        
        self.set_pass_bg.hidden = NO;
        self.old_passcode_field.hidden = NO;
        self.new_passcode_field.hidden = NO;
        self.check_passcode_field.hidden = NO;
        self.new_passcode_label.hidden = NO;
        self.old_passcode_label.hidden = NO;
        self.check_passcode_label.hidden = NO; 
        self.set_passcode_button.hidden = NO;
        self.set_passcode_button_2.hidden = NO;
        
    }
    else{
        NSLog(@"------ set_passcode_setting_ui_display :NO");        
        self.set_pass_bg.hidden = YES;
        self.old_passcode_field.hidden = YES;
        self.new_passcode_field.hidden = YES;
        self.check_passcode_field.hidden = YES;
        self.new_passcode_label.hidden = YES;
        self.old_passcode_label.hidden = YES;
        self.check_passcode_label.hidden = YES; 
        self.set_passcode_button.hidden = YES; 
        self.set_passcode_button_2.hidden = YES;        
        
    
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
    [cell_setting_passcode release];
/*    
    [set_pass_bg release];
    [old_passcode_label release];
    [new_passcode_label release];
    [check_passcode_label release];
    [old_passcode_field release];
    [new_passcode_field release];
    [check_passcode_field release];
    [set_passcode_button release];
*/ 
    [set_passcode_title release];
    [super dealloc];
}

- (IBAction)set_passcode_action:(id)sender {
    /*
    if([self.check_passcode_field.text isEqualToString:self.new_passcode_field.text] && self.check_passcode_field.text.length==4){
        [[NSUserDefaults standardUserDefaults] setObject:self.new_passcode_field.text forKey:@"PasscodeString"];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PasscodeEnabled"];
    }
    else{
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmSettingError", nil) 
                                                        message:NSLocalizedString(@"altmPasscodemismatch",nil)
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                              otherButtonTitles:nil];
        [alert show];
        [alert release];
        
        //[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
        [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:YES];
        

        //NSLog(@"passcord switch :%@",[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]);
    }  
    */
    self.showPassSetting = NO;
    [self set_passcode_setting_ui_display];
    
    [self.tableView reloadData];    
}

- (void)set_passcode_action_setting{
  
    
    self.message_string = [[[NSString alloc] initWithString:@""] autorelease];
    
     if([self.check_passcode_field.text isEqualToString:self.new_passcode_field.text] && self.check_passcode_field.text.length==4){
     [[NSUserDefaults standardUserDefaults] setObject:self.new_passcode_field.text forKey:@"PasscodeString"];
     [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PasscodeEnabled"];
     }
     else{
         message_string = NSLocalizedString(@"altmPasscodemismatch",nil);
     
     }  
    
    if(message_string.length>0){
        alert_message = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmSettingError", nil) 
                                                        message:message_string
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                              otherButtonTitles:nil];
        [alert_message show];
        [alert_message release];
        
        //[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
        [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:YES];
        
        
        //NSLog(@"passcord switch :%@",[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]);
    }  
    
    message_string = @"";
    //[message release];   
     
    self.showPassSetting = NO;
    [self set_passcode_setting_ui_display];
    
    [self.tableView reloadData];    
}

- (void)set_passcode_action_modify{

    message_string = [[[NSString alloc] initWithString:@""] autorelease];
    NSLog(@"--------- set_passcode_action_modify");
    
    /*
     else if(self.new_passcode_field.text.length<4){    
     message = NSLocalizedString(@"altmNewPasscodeFormatError",nil);
     }
     else if(self.check_passcode_field.text.length<4){    
     message = NSLocalizedString(@"altmCheckPasscodeFormatError",nil);
     }    
     else if (![self.check_passcode_field.text isEqualToString:self.new_passcode_field.text]) {
     message =  NSLocalizedString(@"altmPasscodemismatch",nil);
     }*/
    
    
    if(self.old_passcode_field.text.length<4){
        message_string = NSLocalizedString(@"altmOldPasscodeFormatError",nil);
    }
    else if(![self.old_passcode_field.text isEqualToString:[[NSUserDefaults standardUserDefaults] stringForKey:@"PasscodeString"]]){
        message_string = NSLocalizedString(@"altmOldPasscodeError",nil);    
    }
    else if([self.old_passcode_field.text isEqualToString:self.new_passcode_field.text]){
        message_string = NSLocalizedString(@"altmNewPasscodeNotSame",nil);    
    }    
    else if([self.check_passcode_field.text isEqualToString:self.new_passcode_field.text] && self.check_passcode_field.text.length==4){
        [[NSUserDefaults standardUserDefaults] setObject:self.new_passcode_field.text forKey:@"PasscodeString"];
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"PasscodeEnabled"];
        message_string = @"";
    }
    else{
        message_string = NSLocalizedString(@"altmPasscodemismatch",nil);
    }
    
    if(message_string.length>0){
        alert_message = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"altmSettingError", nil) 
                                                        message:message_string
                                                       delegate:nil cancelButtonTitle:NSLocalizedString(@"btnCancel", nil)
                                              otherButtonTitles:nil];
        [alert_message show];
        [alert_message release];
        
        //[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"PasscodeEnabled"];
        [passcode setOn:[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"] animated:YES];
        
        
        //NSLog(@"passcord switch :%@",[[NSUserDefaults standardUserDefaults] boolForKey:@"PasscodeEnabled"]);
    }  
    message_string = @"";

    self.showPassSetting = NO;
    [self set_passcode_setting_ui_display];
    
    [self.tableView reloadData];    
}



- (void)defaultPassCodeSetting{

    self.set_pass_bg = [[[UIImageView alloc] initWithFrame:CGRectMake(0.0, 48.0, 320.0, 191.0)] autorelease];

    [self.set_pass_bg setContentMode:UIViewContentModeScaleAspectFit];    
    [self.cell_setting_passcode addSubview:self.set_pass_bg];

    self.old_passcode_label = [[[UILabel alloc] initWithFrame:CGRectMake(30.0, 64.0, 129.0, 21.0)] autorelease];
    [self.old_passcode_label setFont:[UIFont systemFontOfSize:19.0]];
    [self.old_passcode_label setTextColor:[UIColor blackColor]];
    //[nameLabel setHighlightedTextColor:[UIColor whiteColor]];
    [self.old_passcode_label setBackgroundColor:[UIColor clearColor]];
    [self.old_passcode_label setLineBreakMode:UILineBreakModeTailTruncation];
    [self.old_passcode_label setText:NSLocalizedString(@"labOldPasscode", nil)];
    [self.cell_setting_passcode addSubview:self.old_passcode_label];
    
    
    self.new_passcode_label = [[[UILabel alloc] initWithFrame:CGRectMake(30.0, 106.0, 129.0, 21.0)] autorelease];
    [self.new_passcode_label setFont:[UIFont systemFontOfSize:19.0]];
    [self.new_passcode_label setTextColor:[UIColor blackColor]];
    [self.new_passcode_label setBackgroundColor:[UIColor clearColor]];
    [self.new_passcode_label setLineBreakMode:UILineBreakModeTailTruncation];
    [self.new_passcode_label setText:NSLocalizedString(@"labNewPasscode", nil)];
    [self.cell_setting_passcode addSubview:self.new_passcode_label];
    
    self.check_passcode_label = [[[UILabel alloc] initWithFrame:CGRectMake(30.0, 148, 129.0, 21.0)] autorelease];
    [self.check_passcode_label setFont:[UIFont systemFontOfSize:19.0]];
    [self.check_passcode_label setTextColor:[UIColor blackColor]];
    [self.check_passcode_label setBackgroundColor:[UIColor clearColor]];        
    [self.check_passcode_label setLineBreakMode:UILineBreakModeTailTruncation];
    [self.check_passcode_label setText:NSLocalizedString(@"labCheckPasscode", nil)];        
    [self.cell_setting_passcode addSubview:self.check_passcode_label];        
    
    self.old_passcode_field = [[[UITextField alloc] initWithFrame:CGRectMake(182.0, 59.0, 97.0, 31.0)] autorelease];
    [self.old_passcode_field setPlaceholder:NSLocalizedString(@"fphOldPasscode", nil)];
    [self.old_passcode_field setBackgroundColor:[UIColor whiteColor]];
    [self.old_passcode_field setAutocorrectionType:UITextAutocorrectionTypeNo];
    [self.old_passcode_field setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [self.old_passcode_field setSecureTextEntry:YES];    
    [self.old_passcode_field setKeyboardType:UIKeyboardTypeNumberPad];
    [self.old_passcode_field setFont:[UIFont systemFontOfSize:18]];
    
    [self.old_passcode_field setBorderStyle:UITextBorderStyleRoundedRect];
    [self.old_passcode_field setDelegate:self];
    [self.cell_setting_passcode addSubview:self.old_passcode_field];        
    
    
    self.new_passcode_field = [[[UITextField alloc] initWithFrame:CGRectMake(182.0, 103, 97.0, 31.0)] autorelease];
    [self.new_passcode_field setPlaceholder:NSLocalizedString(@"fphNewPasscode", nil)];
    [self.new_passcode_field setBackgroundColor:[UIColor whiteColor]];
    [self.new_passcode_field setAutocorrectionType:UITextAutocorrectionTypeNo];
    [self.new_passcode_field setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [self.new_passcode_field setSecureTextEntry:YES];    
    [self.new_passcode_field setKeyboardType:UIKeyboardTypeNumberPad];
    [self.new_passcode_field setFont:[UIFont systemFontOfSize:18]];
    
    [self.new_passcode_field setBorderStyle:UITextBorderStyleRoundedRect];
    [self.new_passcode_field setDelegate:self];
    [self.cell_setting_passcode addSubview:self.new_passcode_field];
    
    self.check_passcode_field = [[[UITextField alloc] initWithFrame:CGRectMake(182.0, 147.0, 97.0, 31.0)] autorelease];
    [self.check_passcode_field setPlaceholder:NSLocalizedString(@"fphConfirmPasscode", nil)];
    [self.check_passcode_field setBackgroundColor:[UIColor whiteColor]];
    [self.check_passcode_field setAutocorrectionType:UITextAutocorrectionTypeNo];
    [self.check_passcode_field setAutocapitalizationType:UITextAutocapitalizationTypeNone];
    [self.check_passcode_field setSecureTextEntry:YES];    
    [self.check_passcode_field setKeyboardType:UIKeyboardTypeNumberPad];
    [self.check_passcode_field setFont:[UIFont systemFontOfSize:18.0]];
    [self.check_passcode_field setBorderStyle:UITextBorderStyleRoundedRect];        
    [self.check_passcode_field setDelegate:self];
    [self.cell_setting_passcode addSubview:self.check_passcode_field];
    
    
    self.set_passcode_button = [[[UIButton alloc] initWithFrame:CGRectMake(211, 191, 64, 33)]  autorelease];//target:self action:@selector(addInContactList:) //
    
    self.set_passcode_button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
    //[btnPassSetting.buttonType=UIButtonTypeRoundedRect];
    // btnPassSetting = [UIButton buttonWithType:UIButtonTypeRoundedRect];  
    //[btnPassSetting.btnPassSetting:UIButtonTypeRoundedRect];
    
    //[self.set_passcode_button setBackgroundColor:[UIColor whiteColor]];
   // self.set_passcode_button.buttonType = UIButtonTypeRoundedRect;
    [self.set_passcode_button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];//setTextColor:[UIColor blackColor]


                        
    
    [self.set_passcode_button setTitle:NSLocalizedString(@"btnEnter", nil) forState:UIControlStateNormal]; 
    [self.set_passcode_button addTarget:nil action:@selector(set_passcode_action_setting) forControlEvents:UIControlEventTouchUpInside];
    [self.cell_setting_passcode addSubview:self.set_passcode_button];
    
    
    self.set_passcode_button_2 = [[[UIButton alloc] initWithFrame:CGRectMake(211, 191, 64, 33)]  autorelease];//target:self action:@selector(addInContactList:) 
    
    self.set_passcode_button_2 = [UIButton buttonWithType:UIButtonTypeRoundedRect];    
    //[btnPassSetting.buttonType:UIButtonTypeRoundedRect];
    // btnPassSetting = [UIButton buttonWithType:UIButtonTypeRoundedRect];  
    //[btnPassSetting.btnPassSetting:UIButtonTypeRoundedRect];
    
    //[self.set_passcode_button_2 setBackgroundColor:[UIColor redColor]];
    [self.set_passcode_button_2 setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];//setTextColor:[UIColor blackColor]
    
    
    [self.set_passcode_button_2 setTitle:NSLocalizedString(@"btnEnter", nil) forState:UIControlStateNormal];
    [self.set_passcode_button_2 addTarget:nil action:@selector(set_passcode_action_modify) forControlEvents:UIControlEventTouchUpInside]; 
    
    [self.cell_setting_passcode addSubview:self.set_passcode_button_2];    

}

- (void)PasscodeSettingCGRectMake:(NSString *)item{
   
    /*
     [self.new_passcode_field setPlaceholder:NSLocalizedString(@"fphNewPasscode", nil)];    
     [self.old_passcode_field setPlaceholder:NSLocalizedString(@"fphOldPasscode", nil)];    
     
     [self.check_passcode_field setPlaceholder:NSLocalizedString(@"fphConfirmPasscode", nil)];
     [self.set_passcode_title setText:NSLocalizedString(@"labSettingPasscodeTitle", nil)];
     [self.old_passcode_label setText:NSLocalizedString(@"labOldPasscode", nil)];
     [self.new_passcode_label setText:NSLocalizedString(@"labNewPasscode", nil)];
     [self.check_passcode_label setText:NSLocalizedString(@"labCheckPasscode", nil)]; 
     [self.set_passcode_button setTitle:NSLocalizedString(@"btnEnter", nil) forState:UIControlStateNormal];     
     
    */ 
    /*
    if(self.set_pass_bg!=nil)
    [self.set_pass_bg removeFromSuperview];

    if(self.old_passcode_label!=nil)    
    [self.old_passcode_label removeFromSuperview];
    
    if(self.old_passcode_field!=nil)
    [self.old_passcode_field removeFromSuperview];
    
    if(self.new_passcode_label!=nil)    
    [self.new_passcode_label removeFromSuperview];
    
    if(self.new_passcode_field!=nil)    
    [self.new_passcode_field removeFromSuperview];
    
    if(self.check_passcode_label!=nil)    
    [self.check_passcode_label removeFromSuperview];
    
    if(self.check_passcode_field!=nil)        
    [self.check_passcode_field removeFromSuperview];
    
    if(self.set_passcode_button!=nil)       
    [self.set_passcode_button removeFromSuperview];
    */
    if([item isEqualToString:@"setting"] && self.showPassSetting==YES){
        NSLog(@"-------------- PasscodeSettingCGRectMake setting"); 
        /*
        self.set_pass_bg = [[[UIImageView alloc] initWithFrame:CGRectMake(0.0, 48.0, 320.0, 191.0)] autorelease];
        self.set_pass_bg.image = [UIImage imageNamed:@"passcode_setting_table.png"]; 
		self.set_pass_bg.contentMode = UIViewContentModeScaleAspectFit;
        [self.cell_setting_passcode addSubview:self.set_pass_bg];
        */

        [self.set_pass_bg setFrame:CGRectMake(0.0, 48.0, 320.0, 155.0)];
        [self.set_pass_bg setImage:[UIImage imageNamed:@"passcode_setting_table.png"]];
        [self.new_passcode_label setFrame:CGRectMake(30.0, 64.0, 129.0, 21.0)] ;               
        [self.check_passcode_label setFrame:CGRectMake(30.0, 106, 129.0, 21.0)] ;       
        [self.new_passcode_field setFrame:CGRectMake(182.0, 59.0, 97.0, 31.0)] ;               
        [self.check_passcode_field setFrame:CGRectMake(182.0, 103.0, 97.0, 31.0)] ;                      
        [self.set_passcode_button setFrame:CGRectMake(211, 147, 64, 33)] ;//target:self action:@selector(addInContactList:)
        

        self.set_passcode_button_2.hidden = YES;
        self.old_passcode_field.hidden = YES;
        self.old_passcode_label.hidden = YES;

    }
    else if([item isEqualToString:@"modify"] && self.showPassSetting==YES){
        NSLog(@"-------------- PasscodeSettingCGRectMake modify");  
        [self.set_pass_bg setImage:[UIImage imageNamed:@"passcode_modify_table.png"]];
        
        [self.set_pass_bg setFrame:CGRectMake(0.0, 48.0, 320.0, 191.0)];
        [self.old_passcode_label setFrame: CGRectMake(30.0, 64.0, 129.0, 21.0)];
        [self.new_passcode_label setFrame:CGRectMake(30.0, 106.0, 129.0, 21.0)];        
        [self.check_passcode_label setFrame:CGRectMake(30.0, 148, 129.0, 21.0)];
        [self.old_passcode_field setFrame:CGRectMake(182.0, 59.0, 97.0, 31.0) ];     
        [self.new_passcode_field setFrame:CGRectMake(182.0, 103, 97.0, 31.0)];
        [self.check_passcode_field setFrame:CGRectMake(182.0, 147.0, 97.0, 31.0)];
        [self.set_passcode_button_2 setFrame:CGRectMake(211, 191, 64, 33)];  


        
        self.set_passcode_button.hidden = YES;
    }

    

}
@end
