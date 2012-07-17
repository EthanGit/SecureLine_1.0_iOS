//
//  HistoryEntry.h
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface HistoryEntry : NSObject {

  NSString *callid;
  NSString *remoteuri;
  NSString *pid;//Sanji add
}

@property (nonatomic, retain) NSString *callid;
@property (nonatomic, retain) NSString *remoteuri;
@property (nonatomic, retain) NSString *pid;//Sanji Add

@end
