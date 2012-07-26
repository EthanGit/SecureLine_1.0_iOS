//
//  UITableViewCellHistory.h
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface CellHistoryUITableView : UITableViewCell {
	UILabel *remoteuri;
	UILabel *duration;
	UILabel *reason;
  UIImageView *image;
}

@property(nonatomic,retain) IBOutlet UILabel *remoteuri;
@property(nonatomic,retain) IBOutlet UILabel *duration;
@property(nonatomic,retain) IBOutlet UILabel *reason;
@property(nonatomic,retain) IBOutlet UIImageView *image;
@property (retain, nonatomic) IBOutlet UILabel *date;

- (void)applyDefaultStyle;
- (void)applyShinyBackgroundWithColor:(UIColor *)color;

@end
