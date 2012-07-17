//
//  UITableViewCellHistory.m
//  mobeedeals
//
//  Created by Aymeric MOIZARD on 07/03/11.
//  Copyright 2011 antisip. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "UITableViewCellHistory.h"

@implementation UITableViewCellHistory

@synthesize remoteuri;
@synthesize duration;
@synthesize reason;
@synthesize image;

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // Initialization code.
    }
    return self;
}


- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    
    [super setSelected:selected animated:animated];
    
    // Configure the view for the selected state.
}

- (void)applyDefaultStyle {
    // curve the corners
	self.layer.cornerRadius = 8;
	
    // apply the border
    self.layer.borderWidth = 1.0;
    self.layer.borderColor = [[UIColor lightGrayColor] CGColor];
	
    // add the drop shadow
    self.layer.shadowColor = [[UIColor blackColor] CGColor];
    self.layer.shadowOffset = CGSizeMake(2.0, 2.0);
    self.layer.shadowOpacity = 0.25;
	
}

- (void)applyShinyBackgroundWithColor:(UIColor *)color {
	
	return;
    // create a CAGradientLayer to draw the gradient on
    CAGradientLayer *layer = [CAGradientLayer layer];
	
    // get the RGB components of the color
    const CGFloat *cs = CGColorGetComponents(color.CGColor);
	
    // create the colors for our gradient based on the color passed in
    layer.colors = [NSArray arrayWithObjects:
					(id)[color CGColor],
					(id)[[UIColor colorWithRed:0.98f*cs[0] 
										 green:0.98f*cs[1] 
										  blue:0.98f*cs[2] 
										 alpha:1] CGColor],
					(id)[[UIColor colorWithRed:0.98f*cs[0] 
										 green:0.98f*cs[1] 
										  blue:0.98f*cs[2] 
										 alpha:1] CGColor],
					(id)[[UIColor colorWithRed:0.90f*cs[0] 
										 green:0.90f*cs[1] 
										  blue:0.90f*cs[2] 
										 alpha:1] CGColor],
					nil];
	
    // create the color stops for our gradient
    layer.locations = [NSArray arrayWithObjects:
					   [NSNumber numberWithFloat:0.0f],
					   [NSNumber numberWithFloat:0.49f],
					   [NSNumber numberWithFloat:0.51f],
					   [NSNumber numberWithFloat:1.0f],
					   nil];
	
    layer.frame = self.bounds;
    [self.layer insertSublayer:layer atIndex:0];
}

- (void)dealloc {
    [super dealloc];
}


@end
