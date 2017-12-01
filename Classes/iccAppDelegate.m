//
//  iccAppDelegate.m
//  See the file "LICENSE.md" for the full license governing this code.
//

#import "iccAppDelegate.h"
#import "RootViewController.h"
#import "CardIOMacros.h"
#import "CardIOPaymentViewController.h"
#import "CardIOUtilities.h"

#import "WDA4CardViewController.h"

@implementation iccAppDelegate

#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  
  self.window.backgroundColor = [UIColor whiteColor];  // otherwise, on iPad apparently defaults to {red:1 green:0.0471187 blue:0}
  self.window.rootViewController = [[WDA4CardViewController alloc] init];
  
  [self.window makeKeyAndVisible];
  CardIOLog(@"Client version is %@.", [CardIOUtilities cardIOLibraryVersion]);
	return YES;
}

@end
