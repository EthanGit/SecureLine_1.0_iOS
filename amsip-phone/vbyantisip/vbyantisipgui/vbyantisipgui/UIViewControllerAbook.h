#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

#import <AddressBook/AddressBook.h>
#import <AddressBookUI/AddressBookUI.h>

#import "CellOwnerDefault.h"
#import "ContactEntry.h"

#import "SqliteContactHelper.h"

//@interface UIViewControllerAbook : UIViewController <UIApplicationDelegate>  {
@interface UIViewControllerAbook : UIViewController <UITableViewDelegate, UIActionSheetDelegate, ABPeoplePickerNavigationControllerDelegate,
  UINavigationControllerDelegate>  {
    
    IBOutlet UILabel *contact_name;
    IBOutlet UIButton *contactbutton;
    IBOutlet UITableView *phonelist;
    IBOutlet CellOwnerDefault *cellOwnerLoadFavorites;

    
  @private
    ABPeoplePickerNavigationController *_picker;

    SqliteContactHelper *myFavoriteContactDb;
    NSMutableArray *myFavoriteList;
    
    ContactEntry *current_contact;
}

@property(strong,nonatomic) ABPeoplePickerNavigationController *picker;

+(void)_keepAtLinkTime;

- (IBAction)contactbuttonAction:(id)sender;

@end
