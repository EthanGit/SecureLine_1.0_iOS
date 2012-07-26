//
//  RecentsCellTableView.h
//  vbyantisipgui
//
//  Created by  on 2012/7/2.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "Recents.h"

@interface RecentsCellTableView : UITableViewCell{

    Recents *recent;


    UIImageView *imageView;
    UILabel *nameLabel;    
    UIImageView *callflowImage;
    UILabel *durationLabel;

}

@property (nonatomic, retain) Recents *recent;

@property (nonatomic, retain) UIImageView *imageView;
@property (nonatomic, retain) UILabel *nameLabel;
@property (nonatomic, retain) UIImageView *callflowImage;
@property (nonatomic, retain) UILabel *durationLabel;
@property BOOL addbutton;


@end
