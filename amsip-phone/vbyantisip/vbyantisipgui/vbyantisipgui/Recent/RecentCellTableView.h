//
//  UITableViewCellFavorites.h
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
//#import "RecentEntry.h"
#import "RecentsEntry.h"

@interface RecentCellTableView : UITableViewCell {
    RecentsEntry *recentEntry;
    //Recents *recent;
    UIImageView *imageView;
    UILabel *durationLabel;
    UILabel *timeLabel;
    //UILabel *prepTimeLabel;
}
@property (nonatomic, retain) RecentsEntry *recentEntry;
//@property (nonatomic, retain) Recents *recent;
/*
@property(nonatomic,retain) IBOutlet UILabel *firstname;
@property(nonatomic,retain) IBOutlet UILabel *lastname;
//@property(nonatomic,retain) IBOutlet UILabel *type;
@property(nonatomic,retain) IBOutlet UIImageView *image;
*/
@property (nonatomic, retain) UIImageView *imageView;
@property (nonatomic, retain) UILabel *durationLabel;
@property (nonatomic, retain) UILabel *timeLabel;
//@property (nonatomic, retain) UILabel *prepTimeLabel;
/*
- (void)applyDefaultStyle;
- (void)applyShinyBackgroundWithColor:(UIColor *)color;
@property (retain, nonatomic) IBOutlet UIButton *actionButton;
*/
@end
