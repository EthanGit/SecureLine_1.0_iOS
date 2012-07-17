//
//  HistoryEntry.h
//  vbyantisipgui
//
//  Created by Aymeric Moizard on 4/10/12.
//  Copyright (c) 2012 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RecentEntry : NSObject {
    NSString *start_date;
    NSString *end_date;
    NSString *duration;
    int direction;
    int sip_code;

    

}

@property (nonatomic, retain) NSString *start_date;
@property (nonatomic, retain) NSString *end_date;
@property (nonatomic, retain) NSString *duration;
@property int direction;
@property int sip_code;
@end
