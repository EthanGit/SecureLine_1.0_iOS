//
//  Recents.h
//  vbyantisipgui
//
//  Created by sanji on 12/7/9.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>


@interface Recents : NSManagedObject

@property (nonatomic, retain) NSString * callid;
@property (nonatomic, retain) NSString * direction;
@property (nonatomic, retain) NSString * duration;
@property (nonatomic, retain) NSString * end_date;
@property (nonatomic, retain) NSString * remoteuri;
@property (nonatomic, retain) NSString * section_key;
@property (nonatomic, retain) NSString * secureid;
@property (nonatomic, retain) NSString * sip_code;
@property (nonatomic, retain) NSString * sip_reason;
@property (nonatomic, retain) NSString * start_date;

@end
