//
//  UIPlaceHolderTextView.h
//  vbyantisipgui
//
//  Created by sanji on 12/7/10.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SSTextView : UITextView

/**
 The string that is displayed when there is no other text in the text view.
 
 The default value is `nil`.
 */
@property (nonatomic, strong) NSString *placeholder;

/**
 The color of the placeholder.
 
 The default is `[UIColor lightGrayColor]`.
 */
@property (nonatomic, strong) UIColor *placeholderTextColor;

@end



