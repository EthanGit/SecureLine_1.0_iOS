//
//  NSString+SECUREID.m
//  vbyantisipgui
//
//  Created by sanji on 12/7/5.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "NSString+SECUREID.h"

@implementation NSString(SECUREID)


- (NSString *)SECUREID{
    
    const char *ptr = [self UTF8String];
    //NSLog(@"%s",ptr);
    NSString *output = [[[NSString alloc] initWithFormat:@"%s",ptr] autorelease];
   // NSString *output = [[[NSString alloc] initWithString:ptr] autorelease];
    
    NSRange strRange = [output rangeOfString:@"sip:"];
    //NSLog(@">>>>> targetString = %@", result);
    if (strRange.location != NSNotFound) {
        output = [output substringFromIndex:strRange.location+strRange.length];
        //NSLog(@">>>>> targetString1 = %@", result);
        strRange = [output rangeOfString:@"@"];
        if (strRange.location!=NSNotFound) {
            output = [output substringToIndex:strRange.location];
            //NSLog(@">>>>> targetString2 = %@", result);
        }
    }
    
   // return result;
    return output;
}

@end
