//
//  UITableViewCellFavorites.h
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "Contacts.h"

@interface ContactCellTableView : UITableViewCell {
    Contacts *contact;

    UIImageView *imageView;
    UILabel *nameLabel;
    UILabel *descLabel;
    //UILabel *prepTimeLabel;
}
@property (nonatomic, retain) Contacts *contact;
/*
@property(nonatomic,retain) IBOutlet UILabel *firstname;
@property(nonatomic,retain) IBOutlet UILabel *lastname;
//@property(nonatomic,retain) IBOutlet UILabel *type;
@property(nonatomic,retain) IBOutlet UIImageView *image;
*/
@property (nonatomic, retain) UIImageView *imageView;
@property (nonatomic, retain) UILabel *nameLabel;
@property (nonatomic, retain) UILabel *descLabel;
//@property (nonatomic, retain) UILabel *prepTimeLabel;
/*
- (void)applyDefaultStyle;
- (void)applyShinyBackgroundWithColor:(UIColor *)color;
@property (retain, nonatomic) IBOutlet UIButton *actionButton;
*/
@end
