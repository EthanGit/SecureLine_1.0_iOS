//
//  CellContactUITableView.m
//  vbyantisipgui
//
//  Created by  on 2012/5/31.
//  Copyright (c) 2012年 antisip. All rights reserved.
//
#import <QuartzCore/QuartzCore.h>
#import "ContactCellTableView.h"

@interface ContactCellTableView (SubviewFrames)
- (CGRect)_imageViewFrame;
- (CGRect)_nameLabelFrame;
- (CGRect)_descLabelFrame;
//- (CGRect)_prepTimeLabelFrame;
@end


@implementation ContactCellTableView
//@synthesize actionButton;


@synthesize contact, imageView, nameLabel, descLabel;

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    
	if (self = [super initWithStyle:style reuseIdentifier:reuseIdentifier]) {
        imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
		imageView.contentMode = UIViewContentModeScaleAspectFit;
        [self.contentView addSubview:imageView];
        
        nameLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        [nameLabel setFont:[UIFont systemFontOfSize:20.0]];
        [nameLabel setTextColor:[UIColor blackColor]];
        [nameLabel setHighlightedTextColor:[UIColor whiteColor]];
        [nameLabel setBackgroundColor:[UIColor clearColor]];
    
        [self.contentView addSubview:nameLabel];
        /*
        descLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        descLabel.textAlignment = UITextAlignmentRight;
        [descLabel setFont:[UIFont systemFontOfSize:12.0]];
        [descLabel setTextColor:[UIColor blackColor]];
        [descLabel setHighlightedTextColor:[UIColor whiteColor]];
		descLabel.minimumFontSize = 7.0;
		descLabel.lineBreakMode = UILineBreakModeTailTruncation;
         
        [self.contentView addSubview:descLabel];
         */
        /*
        nameLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        [nameLabel setFont:[UIFont boldSystemFontOfSize:14.0]];
        [nameLabel setTextColor:[UIColor blackColor]];
        [nameLabel setHighlightedTextColor:[UIColor whiteColor]];
        [self.contentView addSubview:nameLabel];*/
    }
    
    return self;
}
/*
- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // Initialization code.
    }
    return self;
}*/

- (void)setContact:(Contacts *)newContact{
    if (newContact != contact) {
        [contact release];
        contact = [newContact retain];
	}
	//imageView.image = @"contact-offline.png";//recipe.thumbnailImage;
   
   // UIImageView *tableBgImage = [[UIImage alloc] initWithImage:[UIImage imageNamed:@"contact-offline.png"]];
    imageView.image =  [UIImage imageNamed:@"contact-offline.png"];;
    
    //[tableBgImage release];
    
	nameLabel.text =  [NSString stringWithFormat:@"%@  %@" ,contact.lastname,contact.firstname];

	//descLabel.text = contact.other;
	
}


- (void)layoutSubviews {
    [super layoutSubviews];
	
    [imageView setFrame:[self _imageViewFrame]];
   
   // [nameLabel setBackgroundColor:[UIColor clearColor]];
    [nameLabel setFrame:[self _nameLabelFrame]];
    //[descLabel setFrame:[self _descLabelFrame]];
    //[prepTimeLabel setFrame:[self _prepTimeLabelFrame]];

}

#define IMAGE_SIZE          40.0
#define EDITING_INSET       10.0
#define TEXT_LEFT_MARGIN    8.0
#define TEXT_RIGHT_MARGIN   5.0
#define IMAGE_RIGHT_MARGIN   5.0
#define PREP_TIME_WIDTH     80.0
#define NAME_WIDTH     200.0

- (CGRect)_imageViewFrame {
    
    if (self.editing) {
        return CGRectMake(EDITING_INSET+IMAGE_RIGHT_MARGIN, 5.0, IMAGE_SIZE, IMAGE_SIZE);
    }
	else {
        return CGRectMake(EDITING_INSET, 5.0, IMAGE_SIZE, IMAGE_SIZE);
    }
     
}

- (CGRect)_nameLabelFrame {
    if (self.editing) {
        return CGRectMake(IMAGE_SIZE + IMAGE_RIGHT_MARGIN + EDITING_INSET + TEXT_LEFT_MARGIN, 8.0, self.contentView.bounds.size.width - IMAGE_SIZE - EDITING_INSET - TEXT_LEFT_MARGIN, 32.0);
    }
	else {
        return CGRectMake(IMAGE_SIZE + IMAGE_RIGHT_MARGIN + TEXT_LEFT_MARGIN, 8.0, NAME_WIDTH , 32.0);//- PREP_TIME_WIDTH
    }
}

- (CGRect)_descLabelFrame {
    if (self.editing) {
        //return CGRectMake(IMAGE_SIZE + EDITING_INSET + TEXT_LEFT_MARGIN, 22.0, self.contentView.bounds.size.width - IMAGE_SIZE - EDITING_INSET - TEXT_LEFT_MARGIN, 16.0);
        return CGRectMake(IMAGE_SIZE + IMAGE_RIGHT_MARGIN + EDITING_INSET + TEXT_LEFT_MARGIN, 4.0, self.contentView.bounds.size.width - IMAGE_SIZE - EDITING_INSET - TEXT_LEFT_MARGIN, 32.0);
    }
	else {
        //return CGRectMake(IMAGE_SIZE + TEXT_LEFT_MARGIN, 22.0, self.contentView.bounds.size.width - IMAGE_SIZE - TEXT_LEFT_MARGIN, 16.0);
        return CGRectMake(IMAGE_SIZE + IMAGE_RIGHT_MARGIN + TEXT_LEFT_MARGIN, 4.0, self.contentView.bounds.size.width - IMAGE_SIZE - TEXT_LEFT_MARGIN, 32.0);
    }
}
/*
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
*/
- (void)dealloc {
    //[actionButton release];
    [contact release];
    [nameLabel release];
    [descLabel release];
    [imageView release];
    [super dealloc];
}
@end
