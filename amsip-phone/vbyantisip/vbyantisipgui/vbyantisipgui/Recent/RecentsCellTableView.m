//
//  RecentsCellTableView.m
//  vbyantisipgui
//
//  Created by  on 2012/7/2.
//  Copyright (c) 2012年 antisip. All rights reserved.
//

#import "RecentsCellTableView.h"

@interface RecentsCellTableView (SubviewFrames)
- (CGRect)_imageViewFrame;
- (CGRect)_nameLabelFrame;
- (CGRect)_callflowImageFrame;
- (CGRect)_durationLabelFrame;

//- (CGRect)_prepTimeLabelFrame;
@end

@implementation RecentsCellTableView

@synthesize recent, nameLabel, imageView, callflowImage, durationLabel,addbutton;


- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    
	if (self = [super initWithStyle:style reuseIdentifier:reuseIdentifier]) {
        //NSLog(@">>>>>  initWithStyle reuseIdentifier");
        imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
		imageView.contentMode = UIViewContentModeScaleAspectFit;
        [self.contentView addSubview:imageView];

        
        nameLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        //nameLabel.textAlignment = UITextAlignmentRight;
        [nameLabel setFont:[UIFont systemFontOfSize:20.0]];
        [nameLabel setTextColor:[UIColor blackColor]];
        //[nameLabel setHighlightedTextColor:[UIColor whiteColor]];
        [nameLabel setBackgroundColor:[UIColor clearColor]];
        
//		nameLabel.minimumFontSize = 20.0;
//		nameLabel.adjustsFontSizeToFitWidth = YES;
        nameLabel.lineBreakMode = UILineBreakModeTailTruncation;
        
        [self.contentView addSubview:nameLabel];
        

        callflowImage = [[UIImageView alloc] initWithFrame:CGRectZero];
		callflowImage.contentMode = UIViewContentModeScaleAspectFit;
    
        [self.contentView addSubview:callflowImage];
        
        
        durationLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        [durationLabel setFont:[UIFont systemFontOfSize:13.0]];
        [durationLabel setTextColor:[UIColor blackColor]];
        //[durationLabel setHighlightedTextColor:[UIColor whiteColor]];
        [durationLabel setBackgroundColor:[UIColor clearColor]];
        
        [self.contentView addSubview:durationLabel];
    

      
        
        /*
         nameLabel = [[UILabel alloc] initWithFrame:CGRectZero];
         [nameLabel setFont:[UIFont boldSystemFontOfSize:14.0]];
         [nameLabel setTextColor:[UIColor blackColor]];
         [nameLabel setHighlightedTextColor:[UIColor whiteColor]];
         [self.contentView addSubview:nameLabel];*/
    }
    
    return self;
}


-(void)setRecent:(Recents *)newRecent{
    if (newRecent != recent) {
        [recent release];
        recent = [newRecent retain];
	}
    
	//imageView.image = @"contact-offline.png";//recipe.thumbnailImage;
    
     //UIImageView *tableBgImage = [[UIImage alloc] initWithImage:[UIImage imageNamed:@"contact-offline.png"]];
    imageView.image =[UIImage imageNamed:@"contact-offline.png"];
    
     //NSLog(@"###### start_date = %@ / end_date = %@ / direction = %@ / sip_code = %@ / duration = %@", recent.start_date,recent.end_date,recent.direction,recent.sip_code,recent.duration);
     // int direction = 1;//[recentEntry.direction];
     //int sip_code = 200;//[recentEntry.sip_code];
     
     
     
     if ([recent.direction isEqualToString:@"0"] && [recent.sip_code isEqualToString:@"200"])
     {
     callflowImage.image =[UIImage imageNamed:@"icon-call-out"];
     }
     else if ([recent.direction isEqualToString:@"1"] && [recent.duration isEqualToString:@"00:00:00"]){
     callflowImage.image =[UIImage imageNamed:@"icon-call-miss"];
     }
     else if ([recent.direction isEqualToString:@"1"] && recent.end_date == nil){
         callflowImage.image =[UIImage imageNamed:@"icon-call-miss"];
     }    
     else if ([recent.direction isEqualToString:@"1"]){
     callflowImage.image =[UIImage imageNamed:@"icon-call-in"];
     }
     else{
     callflowImage.image =[UIImage imageNamed:@"icon-call-miss"];
     }
     
    
    nameLabel.text = recent.secureid;

    durationLabel.text = recent.duration;

     
    
    //imageView.image =  [UIImage imageNamed:@"contact-offline.png"];;
    
    //[tableBgImage release];
    
	//durationLabel.text =  [NSString stringWithFormat:@"%@  %@" ,historyEntry.remoteuri,historyEntry.remoteuri];
    
	//descLabel.text = contact.other;
	
}


- (void)layoutSubviews {
    [super layoutSubviews];
	
    [imageView setFrame:[self _imageViewFrame]];
    [nameLabel setFrame:[self _nameLabelFrame]];    
    [callflowImage setFrame:[self _callflowImageFrame]];    
    
    // [nameLabel setBackgroundColor:[UIColor clearColor]];
    [durationLabel setFrame:[self _durationLabelFrame]];

    //[prepTimeLabel setFrame:[self _prepTimeLabelFrame]];
    
}

#define IMAGE_SIZE  40
#define CALLFLOW_IMAGE_SIZE 25
#define NAME_WIDTH   170
#define NAME_EDIT_WIDTH   165
#define DURATION_WIDTH   71


#define EDITING_INSET      12//10.0
#define TEXT_MARGIN    8.0
#define PREP_TIME_WIDTH     80.0


- (CGRect)_imageViewFrame {
    

    if (self.editing) {
           // NSLog(@">>>>>>>> editing");
        return CGRectMake(TEXT_MARGIN, 5.0, IMAGE_SIZE, IMAGE_SIZE);
    }
    else {
           // NSLog(@">>>>>>>> not sediting");
        return CGRectMake(TEXT_MARGIN, 5.0, IMAGE_SIZE, IMAGE_SIZE);
    }
    
}

- (CGRect)_nameLabelFrame {
    
    if (self.editing) {
        int width = self.contentView.bounds.size.width -TEXT_MARGIN-IMAGE_SIZE-EDITING_INSET;    
        //NSLog(@"_nameLabelFrame contentView:%i / width:%i",(int)self.contentView.bounds.size.width,width);
        
        
        if(width<=0) width = 0;
        
        if(width>NAME_EDIT_WIDTH) width = NAME_EDIT_WIDTH;

        return CGRectMake(TEXT_MARGIN*2+IMAGE_SIZE , 8.0,width, 32.0);
    }
    
    else{
        return CGRectMake(TEXT_MARGIN*2+IMAGE_SIZE, 8.0,NAME_WIDTH, 32.0);
    }    
    
}


- (CGRect)_callflowImageFrame {

    if (self.editing) {
        int width = self.contentView.bounds.size.width -IMAGE_SIZE - EDITING_INSET - TEXT_MARGIN-NAME_WIDTH;    
        //NSLog(@"_callflowImageFrame contentView:%i / width:%i",(int)self.contentView.bounds.size.width,width);
        
       
        if(width<=0) width = 0;
        
        if(width>CALLFLOW_IMAGE_SIZE) width = CALLFLOW_IMAGE_SIZE;
        
        return CGRectMake(TEXT_MARGIN*3+IMAGE_SIZE+NAME_WIDTH+26, 3.0, width , CALLFLOW_IMAGE_SIZE);
    }
    
    else{
        return CGRectMake(TEXT_MARGIN*3+IMAGE_SIZE+NAME_WIDTH+26, 3.0, CALLFLOW_IMAGE_SIZE, CALLFLOW_IMAGE_SIZE);
    }
}

- (CGRect)_durationLabelFrame {
    //int width = self.contentView.bounds.size.width -IMAGE_SIZE - EDITING_INSET - TEXT_MARGIN-NAME_WIDTH;    
    //NSLog(@"_durationLabelFrame contentView:%i / width:%i",(int)self.contentView.bounds.size.width,width); 
    
    //return CGRectMake(TEXT_LEFT_MARGIN + IMAGE_SIZE + TEXT_RIGHT_MARGIN, 5.0, self.contentView.bounds.size.width - IMAGE_SIZE - TEXT_RIGHT_MARGIN * 2 - PREP_TIME_WIDTH, 32.0);
    if (self.editing) {
        int width = self.contentView.bounds.size.width -IMAGE_SIZE - EDITING_INSET - TEXT_MARGIN-NAME_WIDTH;
       //  NSLog(@"_durationLabelFrame contentView:%i / width:%i",(int)self.contentView.bounds.size.width,width);  
        if(width<=0) width = 0;
        
        return CGRectMake(TEXT_MARGIN*3+IMAGE_SIZE+NAME_WIDTH, 25, width , 26);
    }
    else{
        return CGRectMake(TEXT_MARGIN*3+IMAGE_SIZE+NAME_WIDTH, 25, DURATION_WIDTH , 26);
    }
}




- (void)dealloc {
    //[actionButton release];
    [recent release];
    [durationLabel release];
    [nameLabel release];
    [imageView release];
    [callflowImage release]; 
    [super dealloc];
}


@end
