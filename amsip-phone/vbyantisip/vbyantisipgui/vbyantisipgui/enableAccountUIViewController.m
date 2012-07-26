//
//  enableAccountUIViewController.m
//  vbyantisipgui
//
//  Created by Arthur Tseng on 2012/5/30.
//  Copyright (c) 2012年 Gentrice. All rights reserved.
//

#import "enableAccountUIViewController.h"

@implementation enableAccountUIViewController
@synthesize enableAccountUIWebView;

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
    
    
#if 0
    [self.navigationController.navigationBar setHidden:NO];
    UIBarButtonItem *myBackButton = [[[UIBarButtonItem alloc] initWithTitle:@"Back" style:UIBarButtonItemStyleBordered target:self action:@selector(returnToLoginPage)] autorelease];
    [self.navigationItem setLeftBarButtonItem:myBackButton];
#else
    [self.navigationItem setTitle:NSLocalizedString(@"btnNewAccount", @"New Account") ];
#endif
    
    //load web view
    NSString *  	urlPath=@"https://www.securekingdom.com/demo/join.php";
    NSURLRequest *  request;
    
    request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlPath]];
    assert(request != nil);
    [enableAccountUIWebView loadRequest:request];
}

- (void)viewDidUnload
{
    [self setEnableAccountUIWebView:nil];
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
    [enableAccountUIWebView release];
    [super dealloc];
}


- (void)returnToLoginPage
{
    [self.navigationController.navigationBar setHidden:YES];
    [self.navigationController popToRootViewControllerAnimated:YES];
}


- (void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [self.navigationController.navigationBar setHidden:NO];
}

-(void) viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [self.navigationController.navigationBar setHidden:YES];
}

@end
