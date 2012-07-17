//
//  main.m
//  vbyantisip
//
//  Created by Aymeric MOIZARD on 9/14/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

#import "vbyantisipAppDelegate.h"
#import "UIViewControllerStatus.h"
#import "UIViewControllerDialpad.h"
#import "UIViewControllerAbout.h"
#import "UIViewControllerAbook.h"
#import "UIViewControllerHistory.h"

int main(int argc, char *argv[])
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  
  [vbyantisipAppDelegate _initialize];
  [UIViewControllerStatus _keepAtLinkTime];
  [UIViewControllerDialpad _keepAtLinkTime];
  [UIViewControllerAbook _keepAtLinkTime];
  [UIViewControllerHistory _keepAtLinkTime];
  [UIViewControllerAbout _keepAtLinkTime];
  
  int retVal = UIApplicationMain(argc, argv, nil, nil);
  [pool release];
  return retVal;
}
