#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#import "CellOwnerDefault.h"
#import "SqliteHistoryHelper.h"

@interface UIViewControllerHistory : UIViewController <UITableViewDelegate, UIActionSheetDelegate> {
  
  IBOutlet UITableView *historyTable;

  IBOutlet CellOwnerDefault *cellOwnerLoadHistory;
  
  @private
  
  SqliteHistoryHelper *myHistoryDb;
  NSMutableArray *myHistoryList;

}

+(void)_keepAtLinkTime;

@end
