//
//  AppDelegate.h
//  vbyantisip-macosx
//
//  Created by Aymeric Moizard on 11/30/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
{
    NSWindow *_window;
}

@property (assign) IBOutlet NSWindow *window;

@end
