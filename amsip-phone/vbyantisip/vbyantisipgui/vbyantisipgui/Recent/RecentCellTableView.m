//
//  CellContactUITableView.m
//  vbyantisipgui
//
//  Created by  on 2012/5/31.
//  Copyright (c) 2012年 antisip. All rights reserved.
//
#import <QuartzCore/QuartzCore.h>
#import "RecentCellTableView.h"


@interface RecentCellTableView (SubviewFrames)
- (CGRect)_imageViewFrame;
- (CGRect)_durationLabelFrame;
- (CGRect)_timeLabelFrame;
//- (CGRect)_prepTimeLabelFrame;
@end


@implementation RecentCellTableView
//@synthesize actionButton;


@synthesize recentEntry, imageView, durationLabel, timeLabel;

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    
	if (self = [super initWithStyle:style reuseIdentifier:reuseIdentifier]) {
        imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
		imageView.contentMode = UIViewContentModeScaleAspectFit;
        [self.contentView addSubview:imageView];
        
        durationLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        [durationLabel setFont:[UIFont systemFontOfSize:16.0]];
        [durationLabel setTextColor:[UIColor blackColor]];
        [durationLabel setHighlightedTextColor:[UIColor whiteColor]];
        [durationLabel setBackgroundColor:[UIColor clearColor]];
    
        [self.contentView addSubview:durationLabel];
        
        timeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        timeLabel.textAlignment = UITextAlignmentRight;
        [timeLabel setFont:[UIFont systemFontOfSize:16.0]];
        [timeLabel setTextColor:[UIColor blackColor]];
        [timeLabel setHighlightedTextColor:[UIColor whiteColor]];
        [timeLabel setBackgroundColor:[UIColor clearColor]];

		timeLabel.minimumFontSize = 7.0;
		timeLabel.lineBreakMode = UILineBreakModeTailTruncation;
         
        [self.contentView addSubview:timeLabel];
    
         
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

-(void)setRecentEntry:(RecentsEntry *)newRecent{
    if (newRecent !=recentEntry) {
        [recentEntry release];
        recentEntry = [newRecent retain];
	}
    
	//imageView.image = @"contact-offline.png";//recipe.thumbnailImage;
    
    //UIImageView *tableBgImage = [[UIImage alloc] initWithImage:[UIImage imageNamed:@"contact-offline.png"]];
    imageView.image =[UIImage imageNamed:@"contact-offline.png"];
    
   // NSLog(@"###### start_date = %@ / end_date = %@ / direction = %@ / sip_code = %@ / duration = %@", recentEntry.start_date,recentEntry.end_date,recentEntry.direction,recentEntry.sip_code,recentEntry.duration);
    // int direction = 1;//[recentEntry.direction];
    //int sip_code = 200;//[recentEntry.sip_code];
    
    
    
    if ([recentEntry.direction isEqualToString:@"0"] && [recentEntry.sip_code isEqualToString:@"200"])
    {
        imageView.image =[UIImage imageNamed:@"icon-call-out"];
    }
    else if ([recentEntry.direction isEqualToString:@"1"] && [recentEntry.duration isEqualToString:@"00:00:00"]){
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }
    else if ([recentEntry.direction isEqualToString:@"1"] && recentEntry.end_date == nil){
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }    
    else if ([recentEntry.direction isEqualToString:@"1"]){
        imageView.image =[UIImage imageNamed:@"icon-call-in"];
    }
    else{
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }
    
    
    timeLabel.text = recentEntry.start_date;
    durationLabel.text = recentEntry.duration;
    
	
}


-(void)setRecentsEntry:(RecentsEntry *)newRecent{
    if (newRecent !=recentEntry) {
        [recentEntry release];
        recentEntry = [newRecent retain];
	}
    
	//imageView.image = @"contact-offline.png";//recipe.thumbnailImage;
    
    //UIImageView *tableBgImage = [[UIImage alloc] initWithImage:[UIImage imageNamed:@"contact-offline.png"]];
    imageView.image =[UIImage imageNamed:@"contact-offline.png"];
    
    NSLog(@"###### start_date = %@ / end_date = %@ / direction = %@ / sip_code = %@ / duration = %@", recentEntry.start_date,recentEntry.end_date,recentEntry.direction,recentEntry.sip_code,recentEntry.duration);
    // int direction = 1;//[recentEntry.direction];
    //int sip_code = 200;//[recentEntry.sip_code];
    
    
    
    if ([recentEntry.direction isEqualToString:@"0"] && [recentEntry.sip_code isEqualToString:@"200"])
    {
        imageView.image =[UIImage imageNamed:@"icon-call-out"];
    }
    else if ([recentEntry.direction isEqualToString:@"1"] && [recentEntry.duration isEqualToString:@"00:00:00"]){
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }
    else if ([recentEntry.direction isEqualToString:@"1"] && recentEntry.end_date == nil){
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }    
    else if ([recentEntry.direction isEqualToString:@"1"]){
        imageView.image =[UIImage imageNamed:@"icon-call-in"];
    }
    else{
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }
    
    
    timeLabel.text = recentEntry.start_date;
    durationLabel.text = recentEntry.duration;

	
}

/*
- (void)setRecentEntry:(RecentEntry *)newRecentEntry{
    if (newRecentEntry != recentEntry) {
        [recentEntry release];
        recentEntry = [newRecentEntry retain];
	}
    
	//imageView.image = @"contact-offline.png";//recipe.thumbnailImage;
   
   // UIImageView *tableBgImage = [[UIImage alloc] initWithImage:[UIImage imageNamed:@"contact-offline.png"]];

    /*
    NSLog(@"###### start_date = %@ / end_date = %@ / direction = %@ / sip_code = %d / duration = %@", newRecentEntry.start_date,newRecentEntry.end_date,recentEntry.direction,recentEntry.sip_code,recentEntry.duration);
   // int direction = 1;//[recentEntry.direction];
    //int sip_code = 200;//[recentEntry.sip_code];



    if (recentEntry.direction==0 && recentEntry.sip_code==200) {
        imageView.image =[UIImage imageNamed:@"icon-call-out"];
    }
    else if (recentEntry.direction==1 && ([recentEntry.end_date isEqualToString:@"(null)"] || recentEntry.end_date==nil)){
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }
    else if (recentEntry.direction==1){
        imageView.image =[UIImage imageNamed:@"icon-call-in"];
    }
    else{
        imageView.image =[UIImage imageNamed:@"icon-call-miss"];
    }
    

    
    durationLabel.text = recentEntry.duration;
    timeLabel.text = recentEntry.start_date;
    */
  
    //imageView.image =  [UIImage imageNamed:@"contact-offline.png"];;
    
    //[tableBgImage release];
    
	//durationLabel.text =  [NSString stringWithFormat:@"%@  %@" ,historyEntry.remoteuri,historyEntry.remoteuri];

	//descLabel.text = contact.other;
	
//}


- (void)layoutSubviews {
    [super layoutSubviews];
	
    [imageView setFrame:[self _imageViewFrame]];
   
   // [nameLabel setBackgroundColor:[UIColor clearColor]];
    [durationLabel setFrame:[self _durationLabelFrame]];
    [timeLabel setFrame:[self _timeLabelFrame]];
    //[prepTimeLabel setFrame:[self _prepTimeLabelFrame]];

}

#define IMAGE_SIZE          30
#define EDITING_INSET       10.0
#define TEXT_LEFT_MARGIN    10.0
#define TEXT_RIGHT_MARGIN   10.0
#define PREP_TIME_WIDTH     80.0
 

- (CGRect)_imageViewFrame {
        return CGRectMake(TEXT_LEFT_MARGIN, 6.0, IMAGE_SIZE, IMAGE_SIZE);
}

- (CGRect)_durationLabelFrame {

        //return CGRectMake(TEXT_LEFT_MARGIN + IMAGE_SIZE + TEXT_RIGHT_MARGIN, 5.0, self.contentView.bounds.size.width - IMAGE_SIZE - TEXT_RIGHT_MARGIN * 2 - PREP_TIME_WIDTH, 32.0);
    return CGRectMake(IMAGE_SIZE + TEXT_LEFT_MARGIN + TEXT_RIGHT_MARGIN, 5.0, 80 , 32.0);
}

- (CGRect)_timeLabelFrame {

        //return CGRectMake(IMAGE_SIZE + TEXT_LEFT_MARGIN, 22.0, self.contentView.bounds.size.width - IMAGE_SIZE - TEXT_LEFT_MARGIN, 16.0);
        return CGRectMake(140, 5.0,170, 32.0);
}

- (void)dealloc {
    //[actionButton release];
    [recentEntry release];
    [durationLabel release];
    [timeLabel release];
    [imageView release];
    [super dealloc];
}
@end
