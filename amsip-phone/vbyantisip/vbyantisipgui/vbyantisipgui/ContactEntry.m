//
//  ContactEntry.m
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/9/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import "ContactEntry.h"

@implementation SipNumber

@synthesize phone_type;
@synthesize phone_number;

@end

@implementation ContactEntry

@synthesize firstname;
@synthesize lastname;
@synthesize company;
@synthesize other;
@synthesize phone_numbers;
@synthesize phone_string;
@synthesize cid;

-(id)init
{
  self = [super init];
  if (self) {
    // Initialization code here.
    phone_numbers = [[NSMutableArray arrayWithObjects:nil] retain];
    firstname = @"";
    lastname = @"";
    company = @"";
    other = @"";
    phone_string = @"";      
  }
  
  return self;
}
@end
