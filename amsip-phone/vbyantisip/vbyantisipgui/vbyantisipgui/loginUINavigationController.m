//
//  loginUINavigationController.m
//  vbyantisipgui
//
//  Created by gentrice on 2012/5/28.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "loginUINavigationController.h"
#import "blueUINavigationBar.h"

@implementation loginUINavigationController

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
    [self setValue:[[[blueUINavigationBar alloc]init] autorelease] forKeyPath:@"navigationBar"];
    [self.navigationBar setTintColor:[UIColor colorWithRed:62.0/255.0 green:84.0/255.0 blue:97.0/255.0 alpha:1.0]];
    
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

@end
