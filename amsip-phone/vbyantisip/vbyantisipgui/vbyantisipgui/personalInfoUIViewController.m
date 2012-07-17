//
//  personalInfoUIViewController.m
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/24.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "personalInfoUIViewController.h"

@implementation personalInfoUIViewController

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
    self.navigationItem.title = NSLocalizedString(@"morePSInfo",nil);
    
    //load web view
    NSString *  	urlPath=@"https://www.google.com/";
    NSURLRequest *  request;
    
    request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlPath]];
    assert(request != nil);
    [personalInfoWebView loadRequest:request];
}

- (void)viewDidUnload
{
    [personalInfoWebView release];
    personalInfoWebView = nil;
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
    [personalInfoWebView release];
    [super dealloc];
}
@end
