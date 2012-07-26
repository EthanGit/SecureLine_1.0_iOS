//
//  PDFJSONParser.m
//  Leaves
//
//  Created by Aleksey Skutarenko on 9/2/10.
//  Copyright 2010 Tom Brow. All rights reserved.
//

#import "NGJSONParser.h"
#import "JSON.h"

@implementation NGJSONParser

+ (id) dictionaryOrArrayFromJSONString:(NSString*) jsonStr {
	if ([jsonStr length]==0) {
		return nil;
	}
	else {
		NSError *err = nil;
		SBJSON *json = [[[SBJSON alloc] init] autorelease];
		NSDictionary *item = [json objectWithString: jsonStr error: &err];
		if (err) {
			return nil;
		}
		else {
			return item;
		}
	}	
}

+ (NSString *)stringFromObject:(id)object {
  if (object == nil) {
    return nil;
  }
  else {
    NSError *error = nil;
    SBJSON *json = [[[SBJSON alloc] init] autorelease];
    NSString *string = [json stringWithObject:object error:&error];
    NSLog(@"error: %@", [error description]);

    return string;
  }
}

@end
