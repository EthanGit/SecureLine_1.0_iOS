//
//  CellOwnerDefault.h
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface CellOwnerDefault : NSObject {
	UITableViewCell *cell;
}

@property (nonatomic, retain) IBOutlet UITableViewCell *cell;

- (BOOL)loadMyNibFile:(NSString *)nibName;

@end
