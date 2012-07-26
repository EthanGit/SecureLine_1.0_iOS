//
//  reportUIViewController.m
//  vbyantisipgui
//
//  Created by arthur on 2012/5/24.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "reportUIViewController.h"
#import <QuartzCore/QuartzCore.h>
#import "apiGlobal.h"

@implementation reportUIViewController
@synthesize cell_sendbutton;
@synthesize memo_message;
@synthesize cell_title;
@synthesize cell_textview;

@synthesize tableView;
@synthesize reportUITextView,reportTextView;
@synthesize sendReportButton;

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

#import "gentriceGlobal.h"

- (void)viewDidLoad
{
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    self.navigationItem.title = NSLocalizedString(@"moreReport",nil);

    memo_message.text = NSLocalizedString(@"reportTitle",nil);
    [sendReportButton setTitle: NSLocalizedString(@"btnSendReports",nil) forState:UIControlStateNormal];

    NSString *initReport = [[[NSString alloc] initWithFormat:@"Version: %@\niOS: %@\n\n", APP_VERSION, [[UIDevice currentDevice] systemVersion]] autorelease];

    
    UIImageView *tableBgImage = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"BG.png"]];
    self.tableView.backgroundView = tableBgImage;
    [tableBgImage release];    

    
    //self.reportUITextView = [[[SSTextView alloc ]init] autorelease];
   // SSTextView *reportTextView = [[[SSTextView alloc] init] autorelease];
    reportTextView.tag = 1;

    reportTextView = [[[SSTextView alloc] init] autorelease];
    
    reportTextView.delegate = self;
    
    reportTextView.frame = CGRectMake(20, 5, 280, 160);
    reportTextView.layer.cornerRadius = 6;
    reportTextView.layer.masksToBounds = YES;    
    reportTextView.layer.borderWidth =1.0;
    reportTextView.layer.borderColor=[[UIColor colorWithWhite:0.702f alpha:1.0f] CGColor];
   // [reportTextView setPlaceholderHeight:47];
    [reportTextView setPlaceholder:NSLocalizedString(@"reportfphEMailBody",nil) setHeight:47];
    [reportTextView setFirstText:initReport];

    reportTextView.font = [UIFont fontWithName:@"Helvetica" size:16.0];
    //self.reportTextView.layer.borderWidth = 2.5;
    //self.reportTextView.layer.borderColor = [UIColor colorWithRed:200/255.0 green:200/255.0 blue:200/255.0 alpha:1.0].CGColor;
    //[self.reportTextView.layer setCornerRadius:5.0];
    reportTextView.autocorrectionType = UITextAutocorrectionTypeNo;
    [self.cell_textview addSubview:reportTextView];    
    
    //[reportUITextView setText:initReport];
    
}




- (void)viewDidUnload
{
    [self setReportUITextView:nil];
    [self setSendReportButton:nil];
    [self setMemo_message:nil];
   // [self setReportTextView:nil];

    [self setTableView:nil];
    [self setCell_sendbutton:nil];
    [self setCell_title:nil];
    [self setCell_textview:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}



- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}



//20120627 modify tableview version

#pragma mark -
#pragma mark Table view data source methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    // 1 section
    return 3;
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // 1 rows
    return 1;
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    switch (indexPath.section) {
        case 0: return cell_title;
        case 1: return cell_textview;
        case 2: return cell_sendbutton;
    }
    
    return nil;    
    
}


- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
   // NSLog(@">>>> section:%i",indexPath.section);
    if(indexPath.section==0){
        return 90;
    }
    else if (indexPath.section==1) {
        return 180;
    }
    else {
        return 80;
    }
	//if (indexPath.section == 0) {
		//return 480;
	//}
    /*
    else if (indexPath.section == 3) {
		return 150;
	} 
    else {
		return 44;
	}*/
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [reportTextView resignFirstResponder];
    
    
}



- (IBAction)sendReportButtonPressed:(id)sender {
/*    
    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:@"Report" message:reportUITextView.text delegate:self cancelButtonTitle:NSLocalizedString(@"Ok", @"") otherButtonTitles:nil];
    [alert show];
    [alert release];
 */
/*    
    NSString *subject = NSLocalizedString(@"reportEMailSubject",nil);;//@"Secure Line Problems";
    NSString *body = self.reportUITextView.text;
    NSString *mailString = [NSString stringWithFormat:@"mailto:?subject=%@&body=%@", [subject stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding], [body stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:mailString]]; 
*/
    if ([MFMailComposeViewController canSendMail])
    {
        MFMailComposeViewController *mailer = [[MFMailComposeViewController alloc] init];
        
        mailer.mailComposeDelegate = self;
        
        [mailer setSubject:NSLocalizedString(@"reportEMailSubject",nil)];
        
        NSArray *toRecipients = [NSArray arrayWithObjects:reportServiceMail, nil];
        [mailer setToRecipients:toRecipients];
        
        //UIImage *myImage = [UIImage imageNamed:@"mobiletuts-logo.png"];
        //NSData *imageData = UIImagePNGRepresentation(myImage);
        //[mailer addAttachmentData:imageData mimeType:@"image/png" fileName:@"mobiletutsImage"]; 
        
        NSString *emailBody = reportTextView.text;
        [mailer setMessageBody:emailBody isHTML:NO];
        
        [self presentModalViewController:mailer animated:YES];
        
        [mailer release];
    }
    else
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Failure"
                                                        message:@"Your device doesn't support the composer sheet"
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Ok",nil)
                                              otherButtonTitles: nil];
        [alert show];
        [alert release];
    }    
}




- (void)mailComposeController:(MFMailComposeViewController*)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError*)error 
{   
    switch (result)
    {
        case MFMailComposeResultCancelled:
            NSLog(@"Mail cancelled: you cancelled the operation and no email message was queued.");
            break;
        case MFMailComposeResultSaved:
            NSLog(@"Mail saved: you saved the email message in the drafts folder.");
            break;
        case MFMailComposeResultSent:
            NSLog(@"Mail send: the email message is queued in the outbox. It is ready to send.");
            break;
        case MFMailComposeResultFailed:
            NSLog(@"Mail failed: the email message was not saved or queued, possibly due to an error.");
            break;
        default:
            NSLog(@"Mail not sent.");
            break;
    }
    
    // Remove the mail view
    [self dismissModalViewControllerAnimated:YES];
}

/*

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{
    if ([text isEqualToString:@"\n"]) {
        [textView resignFirstResponder];
        return NO;
    }
    
    return YES;
}
*/

- (void)dealloc {
    [reportUITextView release];
    [sendReportButton release];
    [memo_message release];
   // [reportTextView release];

    [tableView release];
    [cell_sendbutton release];
    [cell_title release];
    [cell_textview release];
    [super dealloc];
}
@end
