#import "CPFPerianPrefPaneController.h"

#define AC3DynamicRangeKey CFSTR("dynamicRange")
#define AC3StereoOverDolbyKey CFSTR("useStereoOverDolby")

@implementation CPFPerianPrefPaneController

- (id)initWithBundle:(NSBundle *)bundle
{
    if ( ( self = [super initWithBundle:bundle] ) != nil ) {
		perianForumURL = [[NSURL alloc] initWithString:@"http://forums.cocoaforge.com/index.php?c=12"];
		perianDonateURL = [[NSURL alloc] initWithString:@"http://perian.org/donate.php"];
		perianWebSiteURL = [[NSURL alloc] initWithString:@"http://perian.org"];
		
		perianAppID = CFSTR("org.perian.perian");
		a52AppID = CFSTR("com.cod3r.a52codec");
    }
    
    return self;
}

- (void)setButton:(NSButton *)button fromKey:(CFStringRef)key forAppID:(CFStringRef)appID withDefault:(BOOL)defaultValue
{
	CFPropertyListRef value;	
	value = CFPreferencesCopyAppValue(key, appID);
	if(value && CFGetTypeID(value) == CFBooleanGetTypeID())
		[button setState:CFBooleanGetValue(value)];
	else
		[button setState:defaultValue];
	
	if(value)
		CFRelease(value);
}

- (void)setKey:(CFStringRef)key forAppID:(CFStringRef)appID fromButton:(NSButton *)button
{
	if([button state])
		CFPreferencesSetAppValue(key, kCFBooleanTrue, appID);
	else
		CFPreferencesSetAppValue(key, kCFBooleanFalse, appID);
}

- (void)mainViewDidLoad
{
	/* A52 Prefs */
	[self setButton:button_ac3DynamicRange fromKey:AC3DynamicRangeKey forAppID:a52AppID withDefault:NO];
	[self setButton:button_ac3StereoOverDolby fromKey:AC3StereoOverDolbyKey forAppID:a52AppID withDefault:NO];
}

- (void)didUnselect
{
	CFPreferencesAppSynchronize(perianAppID);
	CFPreferencesAppSynchronize(a52AppID);
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
	[self setKey:AC3DynamicRangeKey forAppID:a52AppID fromButton:button_ac3DynamicRange];
} 

- (IBAction)setAC3StereoOverDolby:(id)sender 
{
	[self setKey:AC3StereoOverDolbyKey forAppID:a52AppID fromButton:button_ac3StereoOverDolby];
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
