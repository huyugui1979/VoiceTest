//
//  hygRoomViewController.h
//  VocieTest
//
//  Created by user on 14-1-21.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface hygRoomViewController : UITableViewController
{
    NSMutableArray* _array;
    bool _begin;
}
- (IBAction)begin:(id)sender;
- (IBAction)stop:(id)sender;
-(IBAction)createRoom:(id)sender;
 @property (retain,nonatomic) IBOutlet UIButton* button;
@property (assign,nonatomic) int RoomId;
@end
