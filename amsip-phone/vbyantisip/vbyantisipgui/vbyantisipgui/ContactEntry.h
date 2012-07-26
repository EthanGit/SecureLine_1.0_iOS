//
//  ContactEntry.h
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/9/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SipNumber : NSObject {
	NSString *phone_type;
	NSString *phone_number;
}

@property (nonatomic, retain) NSString *phone_type;
@property (nonatomic, retain) NSString *phone_number;

@end

@interface ContactEntry : NSObject {
	NSString *cid;    
	NSString *firstname;
	NSString *lastname;
	NSString *company;
	NSString *other;  
	NSString *phone_string;    
	NSMutableArray *phone_numbers;
}
@property (nonatomic, retain) NSString *cid;
@property (nonatomic, retain) NSString *firstname;
@property (nonatomic, retain) NSString *lastname;
@property (nonatomic, retain) NSString *company;
@property (nonatomic, retain) NSString *other;
@property (nonatomic, retain) NSString *phone_string;
@property (nonatomic, retain) NSMutableArray *phone_numbers;

@end
