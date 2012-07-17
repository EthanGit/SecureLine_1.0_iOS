#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#import "CellOwnerDefault.h"
#import "SqliteHistoryHelper.h"

@interface HistoryUIViewController : UIViewController <UITableViewDelegate, UIActionSheetDelegate> {
  
  IBOutlet UITableView *historyTable;

  IBOutlet CellOwnerDefault *cellOwnerLoadHistory;
  
  @private
  
  SqliteHistoryHelper *myHistoryDb;
  NSMutableArray *myHistoryList;

}


+(void)_keepAtLinkTime;
-(void)searchAll;
-(void)searchLost;
-(void)segmentAction:(UISegmentedControl *)Seg;

- (NSString*) stripSecureIDfromURL:(NSString*) targetString;
@end
