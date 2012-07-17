//
//  UIViewControllerCallList.m
//  iamsip
//
//  Created by Aymeric MOIZARD on 11/04/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import "UIViewControllerCallList.h"
#import "UIViewControllerCallControl.h"

@interface UIViewControllerCallList (PrivateMethods)

- (void)loadScrollViewWithPage:(int)page;
- (void)scrollViewDidScroll:(UIScrollView *)sender;

@end

@implementation UIViewControllerCallList

@synthesize scrollView, pageControl, viewControllers;

-(void)onCallExist:(Call *)call {
	[self onCallNew:call];
}

-(void)onCallNew:(Call *)call {
	NSLog(@"UIViewControllerCallList: onCallAdded");
	
	if ([call_list containsObject:call])
		return;
	
	[call_list addObject:call];
	//[call addCallStateChangeListener:self];
	if ([call isIncomingCall]==true && [call callState]==RINGING)
		[call accept:180];
	
	//re-assign current call for each call control view:
	for (int idx=0;idx<[call_list count];idx++)
	{
		Call *_call = [call_list objectAtIndex:idx];
		if (idx<[viewControllers count])
		{
			UIViewControllerCallControl *controller = [viewControllers objectAtIndex:idx];
			if ((NSNull *)controller == [NSNull null]) {
				controller = [[UIViewControllerCallControl alloc] initWithPageNumber:idx];
				[controller setParent:self];
				[controller setCurrentCall:_call];
				[viewControllers replaceObjectAtIndex:idx withObject:controller];
				[controller release];
			} else {
				[controller setCurrentCall:_call];
			}
		}
		else {
			UIViewControllerCallControl *controller = [[UIViewControllerCallControl alloc] initWithPageNumber:idx];
			[controller setParent:self];
			[controller setCurrentCall:_call];
			[viewControllers addObject:controller];
			[controller release];
		}
	}
	kNumberOfPages = [viewControllers count];
	
    // a page is the width of the scroll view
    scrollView.pagingEnabled = YES;
    scrollView.contentSize = CGSizeMake(scrollView.frame.size.width * kNumberOfPages, scrollView.frame.size.height);
    scrollView.showsHorizontalScrollIndicator = NO;
    scrollView.showsVerticalScrollIndicator = NO;
    scrollView.scrollsToTop = NO;
    scrollView.delegate = self;
	
    pageControl.numberOfPages = kNumberOfPages;
    pageControl.currentPage = 0;
	
    // pages are created on demand
    // load the visible page
    // load the page on either side to avoid flashes when the user starts scrolling
    //[self loadScrollViewWithPage:0];
    //[self loadScrollViewWithPage:1];
}

-(void)onCallRemove:(Call *)call {
	NSLog(@"UIViewControllerCallList: onCallRemove");
	
	[call_list removeObject:call];
	//[call removeCallStateChangeListener:self];

	//re-assign current call for each call control view:
	for (int idx=0;idx<[viewControllers count];idx++)
	{
		UIViewControllerCallControl *controller = [viewControllers objectAtIndex:idx];
		if (idx<[call_list count])
		{
			Call *_call = [call_list objectAtIndex:idx];
			if ((NSNull *)controller == [NSNull null]) {
				controller = [[UIViewControllerCallControl alloc] initWithPageNumber:idx];
				[controller setParent:self];
				[controller setCurrentCall:_call];
				[viewControllers replaceObjectAtIndex:idx withObject:controller];
				[controller release];
			} else {
				[controller setCurrentCall:_call];
			}
		}
		else {
			[controller setCurrentCall:nil];
			if (idx>0)
				[viewControllers removeObjectAtIndex:idx];
		}
	}

	kNumberOfPages = [viewControllers count];
	
    // a page is the width of the scroll view
    scrollView.pagingEnabled = YES;
    scrollView.contentSize = CGSizeMake(scrollView.frame.size.width * kNumberOfPages, scrollView.frame.size.height);
    scrollView.showsHorizontalScrollIndicator = NO;
    scrollView.showsVerticalScrollIndicator = NO;
    scrollView.scrollsToTop = NO;
    scrollView.delegate = self;
	
    pageControl.numberOfPages = kNumberOfPages;
    pageControl.currentPage = 0;
	
    //[self loadScrollViewWithPage:0];
    //[self loadScrollViewWithPage:1];
	
	
	if ([call_list count]==0)
	{
		[self.navigationController popViewControllerAnimated: YES];
	}
}

- (void)onCallUpdate:(Call *)call {
	NSLog(@"UIViewControllerCallList: onCallUpdate");

}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
	
	[gAppEngine removeCallDelegate:self];
	[call_list removeAllObjects];
	[call_list release];
	call_list=nil;
	
	for (int idx=0;idx<[viewControllers count];idx++)
	{
		UIViewControllerCallControl *controller = [viewControllers objectAtIndex:idx];
        [controller viewWillDisappear:YES];
        [controller.view removeFromSuperview];
    }
	[viewControllers removeAllObjects];
	//[viewControllers release];
    viewControllers=nil;
	kNumberOfPages = 1;
	
	[[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
	
	[[UIDevice currentDevice] setProximityMonitoringEnabled:YES];
	kNumberOfPages = 1;
	call_list = [[NSMutableArray alloc] init];
	
	// view controllers are created lazily
    // in the meantime, load the array with placeholders which will be replaced on demand
    NSMutableArray *controllers = [[NSMutableArray alloc] init];
    for (unsigned i = 0; i < kNumberOfPages; i++) {
        [controllers addObject:[NSNull null]];
    }
    self.viewControllers = controllers;
    [controllers release];
	
    // a page is the width of the scroll view
    scrollView.pagingEnabled = YES;
    scrollView.contentSize = CGSizeMake(scrollView.frame.size.width * kNumberOfPages, scrollView.frame.size.height);
    scrollView.showsHorizontalScrollIndicator = NO;
    scrollView.showsVerticalScrollIndicator = NO;
    scrollView.scrollsToTop = NO;
    scrollView.delegate = self;
	
    pageControl.numberOfPages = kNumberOfPages;
    pageControl.currentPage = 0;
	
    // pages are created on demand
    // load the visible page
    // load the page on either side to avoid flashes when the user starts scrolling
    [self loadScrollViewWithPage:0];
    [self loadScrollViewWithPage:1];
	
	[gAppEngine addCallDelegate:self];
}

/*
 // Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
 */
- (void)viewDidLoad {
	[super viewDidLoad];
	
	kNumberOfPages = 1;

}

- (void)loadScrollViewWithPage:(int)page {
    if (page < 0) return;
    if (page >= kNumberOfPages) return;
	
    // replace the placeholder if necessary
    UIViewControllerCallControl *controller = [viewControllers objectAtIndex:page];
    if ((NSNull *)controller == [NSNull null]) {
        controller = [[UIViewControllerCallControl alloc] initWithPageNumber:page];
		[controller setParent:self];
        [viewControllers replaceObjectAtIndex:page withObject:controller];
        [controller release];
    } else if ([controller isKindOfClass:[UIViewControllerCallControl class]])
	{
		[controller viewWillAppear:YES];
	}
	
    // add the controller's view to the scroll view
    if (nil == controller.view.superview) {
        CGRect frame = scrollView.frame;
        frame.origin.x = frame.size.width * page;
        frame.origin.y = 0;
        controller.view.frame = frame;
        [scrollView addSubview:controller.view];
    }
}

- (void)scrollViewDidScroll:(UIScrollView *)sender {
    // We don't want a "feedback loop" between the UIPageControl and the scroll delegate in
    // which a scroll event generated from the user hitting the page control triggers updates from
    // the delegate method. We use a boolean to disable the delegate logic when the page control is used.
    if (pageControlUsed) {
        // do nothing - the scroll was initiated from the page control, not the user dragging
        return;
    }
	
    // Switch the indicator when more than 50% of the previous/next page is visible
    CGFloat pageWidth = scrollView.frame.size.width;
    int page = floor((scrollView.contentOffset.x - pageWidth / 2) / pageWidth) + 1;
    pageControl.currentPage = page;
	
    // load the visible page and the page on either side of it (to avoid flashes when the user starts scrolling)
    [self loadScrollViewWithPage:page - 1];
    [self loadScrollViewWithPage:page];
    [self loadScrollViewWithPage:page + 1];
	
    // A possible optimization would be to unload the views+controllers which are no longer visible
}

// At the begin of scroll dragging, reset the boolean used when scrolls originate from the UIPageControl
- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
    pageControlUsed = NO;
}

// At the end of scroll animation, reset the boolean used when scrolls originate from the UIPageControl
- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
    pageControlUsed = NO;
}

- (IBAction)changePage:(id)sender {
    int page = pageControl.currentPage;
	
    // load the visible page and the page on either side of it (to avoid flashes when the user starts scrolling)
    [self loadScrollViewWithPage:page - 1];
    [self loadScrollViewWithPage:page];
    [self loadScrollViewWithPage:page + 1];
    
	// update the scroll view to the appropriate page
    CGRect frame = scrollView.frame;
    frame.origin.x = frame.size.width * page;
    frame.origin.y = 0;
    [scrollView scrollRectToVisible:frame animated:YES];
    
	// Set the boolean used when scrolls originate from the UIPageControl. See scrollViewDidScroll: above.
    pageControlUsed = YES;
}

/*
 // Override to allow orientations other than the default portrait orientation.
 - (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
 // Return YES for supported orientations
 return (interfaceOrientation == UIInterfaceOrientationPortrait);
 }
 */

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


- (void)dealloc {
    if (viewControllers)
        [viewControllers release];
    [scrollView release];
    [pageControl release];
    [super dealloc];
}

@end
