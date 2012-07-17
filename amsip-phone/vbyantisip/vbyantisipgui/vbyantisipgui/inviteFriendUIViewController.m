//
//  inviteFriendUIViewController.m
//  vbyantisipgui
//
//  Created by arthur on 2012/5/24.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "inviteFriendUIViewController.h"

@implementation inviteFriendUIViewController
@synthesize emailInviteButton;
@synthesize smsInviteButton;
@synthesize memo_message;

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
    self.navigationItem.title = NSLocalizedString(@"moreInvite",nil);
    self.memo_message.text = NSLocalizedString(@"inviteTitle",@"You can invite more friends through these ways:");
    [self.emailInviteButton setTitle:NSLocalizedString(@"btnViaEMail",@"Via E-Mail")  forState:UIControlStateNormal];
    [self.smsInviteButton setTitle:NSLocalizedString(@"btnViaSMS",@"Via SMS")  forState:UIControlStateNormal];

}


- (void)viewDidUnload
{
    [self setEmailInviteButton:nil];
    [self setSmsInviteButton:nil];
    [self setMemo_message:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}


- (IBAction)emailButtonPressed:(id)sender {
/*    
    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:@"Invited" message:@"e-mail sent!" delegate:self cancelButtonTitle:NSLocalizedString(@"Ok", @"") otherButtonTitles:nil];
    [alert show];
    [alert release];
*/
    
/*    
    NSString *subject = NSLocalizedString(@"inviteEmailSubject",@"Come and join me in Secure Line!");
    NSString *body = NSLocalizedString(@"inviteEmailBody",nil);//@"Join today!!";
    NSString *mailString = [NSString stringWithFormat:@"mailto:?subject=%@&body=%@", [subject stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding], [body stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:mailString]];    
*/
    
    if ([MFMailComposeViewController canSendMail])
    {
        MFMailComposeViewController *mailer = [[MFMailComposeViewController alloc] init];
        
        mailer.mailComposeDelegate = self;
        
        [mailer setSubject:NSLocalizedString(@"inviteEmailSubject",@"Come and join me in Secure Line!")];
        
        //NSArray *toRecipients = [NSArray arrayWithObjects:@"fisrtMail@example.com", @"secondMail@example.com", nil];
        //[mailer setToRecipients:toRecipients];
        
        //UIImage *myImage = [UIImage imageNamed:@"mobiletuts-logo.png"];
        //NSData *imageData = UIImagePNGRepresentation(myImage);
        //[mailer addAttachmentData:imageData mimeType:@"image/png" fileName:@"mobiletutsImage"]; 
        
        NSString *emailBody = NSLocalizedString(@"inviteEmailBody",nil);
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




- (IBAction)smsButtonPressed:(id)sender {
    /*
    
    UIAlertView *alert = [[UIAlertView alloc]
                          initWithTitle:@"Invited" message:@"SMS sent!" delegate:self cancelButtonTitle:NSLocalizedString(@"Ok", @"") otherButtonTitles:nil];
    [alert show];
    [alert release];
    */
    MFMessageComposeViewController *controller = [[[MFMessageComposeViewController alloc] init] autorelease];
	if([MFMessageComposeViewController canSendText])
	{
		controller.body = NSLocalizedString(@"inviteSMSBody",nil);// @"Come and join me in Secure Line!";
		//controller.recipients = [NSArray arrayWithObjects:@"12345678", @"87654321", nil];
        controller.recipients = nil;//[NSArray arrayWithObjects: nil];
		controller.messageComposeDelegate = self;
		[self presentModalViewController:controller animated:YES];
	}    
}



- (void) messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result {
    
    switch (result) {
		case MessageComposeResultCancelled:
			NSLog(@"Cancelled");
			break;
		case MessageComposeResultFailed:{
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"SecureLine" message:NSLocalizedString(@"altUnknownError", @"Unknown Error") delegate:self cancelButtonTitle:NSLocalizedString(@"Ok", nil) otherButtonTitles:nil];
			[alert show];
			[alert release];}
			break;
		case MessageComposeResultSent:
            NSLog(@"MessageComposeResultSent");
			break;
		default:
			break;
	}
    
	[self dismissModalViewControllerAnimated:YES];
    
}


- (void)dealloc {
    [emailInviteButton release];
    [smsInviteButton release];
    [memo_message release];
    [super dealloc];
}
@end
