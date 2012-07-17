//
//  PDFJSONParser.h
//  Leaves
//
//  Created by Aleksey Skutarenko on 9/2/10.
//  Copyright 2010 Tom Brow. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface NGJSONParser : NSObject {

}
+ (id) dictionaryOrArrayFromJSONString:(NSString*) jsonStr;
+ (NSString *)stringFromObject:(id)object;
@end
