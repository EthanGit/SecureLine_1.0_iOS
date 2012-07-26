//
//  NetworkTrackingDelegate.h
//  iamsip
//
//  Created by Aymeric MOIZARD on 06/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import "NetworkTracking.h"


@protocol NetworkTrackingDelegate

@optional
- (void)onNetworkTrackingUpdate:(NetworkTracking *)networkTracking;

@end
