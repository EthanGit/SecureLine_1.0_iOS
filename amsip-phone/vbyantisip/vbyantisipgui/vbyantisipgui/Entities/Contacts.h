//
//  Contacts.h
//  vbyantisipgui
//
//  Created by sanji on 12/7/9.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreData/CoreData.h>


@interface Contacts : NSManagedObject

@property (nonatomic, retain) NSString * company;
@property (nonatomic, retain) NSString * firstname;
@property (nonatomic, retain) NSString * lastname;
@property (nonatomic, retain) NSString * other;
@property (nonatomic, retain) NSString * section_key;
@property (nonatomic, retain) NSString * secureid;

@end
