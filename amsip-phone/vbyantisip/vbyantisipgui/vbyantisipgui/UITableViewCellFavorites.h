//
//  UITableViewCellFavorites.h
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface UITableViewCellFavorites : UITableViewCell {
	UILabel *firstname;
	UILabel *lastname;
	UILabel *type;
  UIImageView *image;
}

@property(nonatomic,retain) IBOutlet UILabel *firstname;
@property(nonatomic,retain) IBOutlet UILabel *lastname;
@property(nonatomic,retain) IBOutlet UILabel *type;
@property(nonatomic,retain) IBOutlet UIImageView *image;

- (void)applyDefaultStyle;
- (void)applyShinyBackgroundWithColor:(UIColor *)color;

@end
