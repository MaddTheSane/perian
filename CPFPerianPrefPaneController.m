#import "CPFPerianPrefPaneController.h"

@implementation CPFPerianPrefPaneController

- (id)initWithBundle:(NSBundle *)bundle
{
    if ( ( self = [super initWithBundle:bundle] ) != nil ) {
		perianForumURL = [[NSURL alloc] initWithString:@"http://forums.cocoaforge.com/index.php?c=12"];
		perianDonateURL = [[NSURL alloc] initWithString:@"http://perian.org/donate.php"];
		perianWebSiteURL = [[NSURL alloc] initWithString:@"http://perian.org"];
    }
    
    return self;
}

- (void)mainViewDidLoad
{
	/* Read prefs here and display */
}

- (void)didUnselect
{
	/* Write prefs here */
}

- (void) dealloc {
	[perianForumURL release];
	[perianDonateURL release];
	[perianWebSiteURL release];
	[super dealloc];
}

#pragma mark Install/Uninstall
- (IBAction)installUninstall:(id)sender
{
}

#pragma mark Check Updates
- (IBAction)updateCheck:(id)sender 
{ 
} 

- (IBAction)setAutoUpdateCheck:(id)sender 
{ 
} 


#pragma mark AC3 
- (IBAction)setAC3DynamicRange:(id)sender 
{ 
} 

- (IBAction)setAC3StereoOverDolby:(id)sender 
{ 
} 

#pragma mark About 
- (IBAction)launchWebsite:(id)sender 
{
	[[NSWorkspace sharedWorkspace] openURL:perianWebSiteURL];
} 

- (IBAction)launchDonate:(id)sender 
{ 
	
	[[NSWorkspace sharedWorkspace] openURL:perianDonateURL];
} 

- (IBAction)launchForum:(id)sender 
{ 
	
	[[NSWorkspace sharedWorkspace] openURL:perianForumURL];
	
}

@end
