//
//  hygContigVewControllerViewController.h
//  VocieTest
//
//  Created by mac on 14-4-9.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface hygContigVewControllerViewController : UITableViewController
@property (weak, nonatomic) IBOutlet UITextField *Address;
@property (weak, nonatomic) IBOutlet UITextField *Port;

- (IBAction)clickOK:(id)sender;
@end
