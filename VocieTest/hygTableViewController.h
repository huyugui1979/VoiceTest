//
//  hygTableViewController.h
//  VocieTest
//
//  Created by user on 14-1-20.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "VoiceController.h"
@interface hygTableViewController : UITableViewController
{
    VoiceController* _voice;
    NSMutableArray* _array;
    bool _connected;
}
-(IBAction)connectServer:(id)object;
@property (weak, nonatomic) IBOutlet UIButton *myButton;

@end
