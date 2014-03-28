 //
//  hygTableViewController.m
//  VocieTest
//
//  Created by user on 14-1-20.
//  Copyright (c) 2014年 user. All rights reserved.
//

#import "hygTableViewController.h"
#import "hygRoomViewController.h"
#import "ServerError.h"
@interface hygTableViewController ()

@end

@implementation hygTableViewController

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
        
        
    }
    return self;
}
-(void)showErrMesage:(NSString*)error
{
    UIAlertView* alert = [[UIAlertView alloc]initWithTitle:@"error" message:error delegate:nil cancelButtonTitle:@"ok" otherButtonTitles:nil, nil];
    [alert show];
  
}
-(void)handleError:(int)error
{
    //
    if(error == HAVE_LOGIN)
    {
        [self showErrMesage:@"have login"];
    }
    //
}
-(IBAction)createRoom:(id)sender
{
    int roomId;
    srand(time(NULL));
    roomId = rand()%1000;
    int res = [_voice creaetRoom:roomId];
    if(res!=0)
    {
        [self handleError:res];
    }
    else{
        [_array removeAllObjects];
        if( (res =[_voice getRoomList:_array]) !=0)
        {
           [self handleError:res];
           
        }else
        {
            [self.tableView reloadData];
        }
    }
}
-(void)onServerDisconnect:(NSNotification*) aNotification
{
    //
    _connected=false;
    [_array removeAllObjects];
    [self.tableView reloadData];
    [self.myButton setTitle:@"connect server" forState: UIControlStateNormal];

    //
    //
}
-(void)connectServer:(id)object
{
    // [_voice startRecord];
    // [_voice startRecord];
    if(_connected == false)
    {
        int res=0;
        if((res = [_voice connectServer:@"192.168.1.248" port:9009]) !=0)
    {
        NSLog(@"conenct server failed");
        [self handleError:res];
         goto failed;    }
    int playerId;
    srand(time(NULL));
    playerId = rand()%1000;
    if((res = [_voice loginServer:playerId]) !=0)
    {
        NSLog(@"loginServer failed");
        [self handleError:res];
          goto failed;
    }
    _array = [NSMutableArray array];
    if((res = [_voice getRoomList:_array]) !=0)
    {
        NSLog(@"getRoomList failed");
        [self handleError:res];
        
        goto failed;
    }
    [self.tableView reloadData];
        [self.myButton setTitle:@"diconnect server" forState: UIControlStateNormal];
    _connected = true;
        return;
    failed:
          [_voice resetServer];
        
    }else{
        [_voice resetServer];
        _connected=false;
        [_array removeAllObjects];
        [self.tableView reloadData];
        [self.myButton setTitle:@"connect server" forState: UIControlStateNormal];
    }
}
- (void)viewDidLoad
{
    [super viewDidLoad];
       [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onServerDisconnect:) name:@"ServerDisconnect" object:nil];
    _voice = [VoiceController sharedInstance];
    if(_voice ==nil)
    {
        NSLog(@"init voice controller failed");
        //
    }
 
    //

   _connected=false;
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
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
    static NSString *CellIdentifier = @"MyTitle";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    // Configure the cell...
    cell.textLabel.text=[_array objectAtIndex:indexPath.row];
    return cell;
}


 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }

-(void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    NSArray* path = [self.tableView indexPathsForSelectedRows];
    NSIndexPath* index = [path objectAtIndex:0];
    NSString* str = [_array objectAtIndex:index.row];
    hygRoomViewController* room =(hygRoomViewController*) segue.destinationViewController;
    room.RoomId=[str intValue];
    int res =  [[VoiceController sharedInstance]  enterRoom:room.RoomId];
    if(res !=0)
    {
        [self showErrMesage:@"enterRoom error"];
        return;
    }
}
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
     [detailViewController release];
     */
   
 

    
}

@end