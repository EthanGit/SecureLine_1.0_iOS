//
//  UIViewControllerCallList.h
//  iamsip
//
//  Created by Aymeric MOIZARD on 11/04/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "AppEngine.h"

@interface UIViewControllerCallList : UIViewController <UIScrollViewDelegate, EngineDelegate, CallDelegate> {
	UIScrollView *scrollView;
	UIPageControl *pageControl;
    NSMutableArray *viewControllers;
	
    // To be used when scrolls originate from the UIPageControl
    BOOL pageControlUsed;
	NSMutableArray *call_list;
	NSUInteger kNumberOfPages;
}

@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;
@property (nonatomic, retain) IBOutlet UIPageControl *pageControl;
@property (nonatomic, retain) NSMutableArray *viewControllers;

- (IBAction)changePage:(id)sender;

@end
