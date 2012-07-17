//
//  UINavigationBar_vbyantisip.m
//  vbyantisipgui
//
//  Created by Aymeric MOIZARD on 9/23/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import "UINavigationBar_vbyantisip.h"

@implementation UINavigationBar_vbyantisip

- (id)init
{
    self = [super init];
    if (self) {
        // Initialization code here.
    }
    
    return self;
}

- (void)drawRect:(CGRect)rect 
{
    // ColorSync manipulated image
    UIImage *image = [[UIImage imageNamed:@"BandeauNavigationBar.png"] retain];
    [image drawInRect:CGRectMake(0, 0, self.frame.size.width, self.frame.size.height)];
}

- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated{
    
}

- (void)navigationController:(UINavigationController *)navigationController didShowViewController:(UIViewController *)viewController animated:(BOOL)animated{
}

@end

