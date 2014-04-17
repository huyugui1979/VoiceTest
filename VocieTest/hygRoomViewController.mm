//
//  hygRoomViewController.m
//  VocieTest
//
//  Created by user on 14-1-21.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import "hygRoomViewController.h"
#import "VoiceController.h"
#import "ServerError.h"
@interface hygRoomViewController ()

@end

@implementation hygRoomViewController

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
        _begin=false;
    }
    return self;
}
-(void)showErrMesage:(NSString*)error
{
      
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertView* alert = [[UIAlertView alloc]initWithTitle:@"error" message:error delegate:nil cancelButtonTitle:@"ok" otherButtonTitles:nil, nil];
      
        [alert show];
    });

 
    
}
-(void)onEnterRoom:(NSNotification*) aNotification
{
    //
    NSDictionary* dict = aNotification.userInfo;
    NSString* roomId = [dict objectForKey:@"roomId" ];
    NSString* playerId = [dict objectForKey:@"playerId"];
    //
 
    //
    if(self.RoomId == [roomId intValue])
    {
        //接收该客户
        NSLog(@"player id is %@",playerId);
        [_array addObject:playerId];
        NSLog(@"_array size is %d",_array.count);
        dispatch_async(dispatch_get_main_queue(), ^{
          
            [self.tableView reloadData];
        });
    //
    }
}
-(void)onServerDisconnect:(NSNotification*) aNotification
{
    //
    if(_begin)
    {
        //
      
        UIAlertView* view = [[UIAlertView alloc]initWithTitle:@"error" message:@"server disconnect" delegate:nil  cancelButtonTitle:@"ok" otherButtonTitles:nil, nil];
        [view show];
        [_array removeAllObjects];
        [[VoiceController sharedInstance] stopPlay];
        _begin=false;
    }
    
    [self.navigationController popToRootViewControllerAnimated:YES];
    //
}
-(void)onLeaveRoom:(NSNotification*) aNotification
{
    //
    NSDictionary* dict = aNotification.userInfo;
    NSString* roomId = [dict objectForKey:@"roomId" ];
    NSString* playerId = [dict objectForKey:@"playerId"];
    //
  
    if(self.RoomId == [roomId intValue])
    {
        dispatch_async(dispatch_get_main_queue(), ^{
             [_array removeObject:playerId];
            [self.tableView reloadData];
        });
        //
    }
}
-(void)onBeginTalk:(NSNotification*) aNotification
{
    NSDictionary* dict = aNotification.userInfo;
    NSString* playerId = [dict objectForKey:@"playerId"];
    //
    dispatch_async(dispatch_get_main_queue(), ^{
        //
        int pos = [_array indexOfObject:playerId];
        UITableViewCell *cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:pos inSection:0]];
         cell.textLabel.textColor = [UIColor redColor];
       
        //
    });

}
-(void)onStopTalk:(NSNotification*) aNotification
{
    //
    NSDictionary* dict = aNotification.userInfo;
    NSString* playerId = [dict objectForKey:@"playerId"];
    //
    dispatch_async(dispatch_get_main_queue(), ^{
        //
        int pos = [_array indexOfObject:playerId];
        UITableViewCell *cell = [self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:pos inSection:0]];
        cell.textLabel.textColor = [UIColor blackColor];
        //
    });
    //
}
-(void)viewDidAppear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onEnterRoom:) name:@"EnterRoom" object:nil];//:@"EnterRoom" object:nil userInfo:dic];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onLeaveRoom:) name:@"LeaveRoom" object:nil];//:@"EnterRoom" object:nil userInfo:dic];
      [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onBeginTalk:) name:@"BeginTalk" object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onStopTalk:) name:@"StopTalk" object:nil];
     [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onServerDisconnect:) name:@"ServerDisconnect" object:nil];//:@"EnterRoom" object:nil userInfo:dic];
    //
    
     int res = [[VoiceController sharedInstance] getMemberList:self.RoomId array:_array];
    if( res !=0)
    {
        [self showErrMesage:@"get member list error,error code"];
        return ;
    }
    if(_begin==false)
    {
    
        [[VoiceController sharedInstance] startPlay];
        _begin=true;
       
    }
    

[self.tableView reloadData];

}


- (void)viewDidLoad
{
    [super viewDidLoad];
    _array = [[NSMutableArray alloc]init];
    UIBarButtonItem *backButton = [[UIBarButtonItem alloc] initWithTitle:@"back"
                                                                   style:UIBarButtonItemStyleBordered
                                                                  target:self
                                                                  action:@selector(handleBack:)];
    
    self.navigationItem.leftBarButtonItem = backButton;
  
    //
    //self.button.backgroundColor=[UIColor greenColor];
        // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}
- (void) handleBack:(id)sender
{
    // pop to root view controller
    //
    if(_begin)
    {
        [[VoiceController sharedInstance] stopPlay];
        [_array removeAllObjects];
        _begin=false;
    }
    int res =  [[VoiceController sharedInstance]  leaveRoom];
    if(res !=0)
    {
        [self showErrMesage:@"leaveRoom error"];
        return;
    }

    [self.navigationController popToRootViewControllerAnimated:YES];
    
}
- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
#warning Potentially incomplete method implementation.
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
#warning Incomplete method implementation.
    // Return the number of rows in the section.
    return [_array count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"RoomCell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    // Configure the cell...
    cell.textLabel.text = [_array objectAtIndex:indexPath.row];
    cell.accessoryType =UITableViewCellAccessoryCheckmark;
    return cell;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Navigation logic may go here. Create and push another view controller.
    /*
     <#DetailViewController#> *detailViewController = [[<#DetailViewController#> alloc] initWithNibName:@"<#Nib name#>" bundle:nil];
     // ...
     // Pass the selected object to the new view controller.
     [self.navigationController pushViewController:detailViewController animated:YES];
     */
    NSString* playerId = [_array objectAtIndex:indexPath.row];
    UITableViewCell* cell = [tableView cellForRowAtIndexPath:indexPath];
    if(cell.accessoryType == UITableViewCellAccessoryCheckmark)
    {
        cell.accessoryType=UITableViewCellAccessoryNone;
        [[VoiceController sharedInstance] setAudioRecv:playerId.intValue redv:false ];
    }else{
        cell.accessoryType =UITableViewCellAccessoryCheckmark;
        [[VoiceController sharedInstance] setAudioRecv:playerId.intValue redv:true ];
    }
   
}

- (IBAction)stop:(id)sender {
    //
   // self.button.backgroundColor=[UIColor greenColor];
     [[VoiceController sharedInstance] stopTalk];
    [[VoiceController sharedInstance] stopRecord];
    NSLog(@"stop");
    //
}

- (IBAction)begin:(id)sender {
   // self.button.backgroundColor=[UIColor redColor];
    [[VoiceController sharedInstance] beginTalk];
    [[VoiceController sharedInstance] startRecord];
    NSLog(@"start");
   }
@end
