//
//  aboutUIViewController.m
//  vbyantisipgui
//
//  Created by arthur on 2012/5/23.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "aboutUIViewController.h"
#import "gentriceGlobal.h"

@implementation aboutUIViewController
@synthesize aboutUITextView;

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
    
    self.navigationItem.title = NSLocalizedString(@"moreAbout",nil); 
    //self.view = [[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"no_webcam.bmp"]] autorelease];

    aboutUITextView.text = [[[NSString alloc] initWithFormat:@"App version: %@\n\nSDK version: %@\n\niOS version: %@\n\n", APP_VERSION, APP_SDK_VERSION, APP_IOS_VERSION] autorelease];

}

- (void)viewDidUnload
{
    [self setAboutUITextView:nil];
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
    [aboutUITextView release];
    [super dealloc];
}
@end
