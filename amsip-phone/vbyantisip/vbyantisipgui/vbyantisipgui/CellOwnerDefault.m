//
//  CellOwnerDefault.m
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import "CellOwnerDefault.h"


@implementation CellOwnerDefault


@synthesize cell;

- (BOOL)loadMyNibFile:(NSString *)nibName {
    // The myNib file must be in the bundle that defines self's class.
    if ([[NSBundle mainBundle] loadNibNamed:nibName owner:self options:nil] == nil)
    {
        NSLog(@"Warning! Could not load %@ file.\n", nibName);
        return NO;
    }
    return YES;
}

- (void)dealloc {
    [super dealloc];
}
@end
