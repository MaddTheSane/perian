#import "CPFPerianPrefPaneController.h"

#define ComponentInfoDictionaryKey	@"Components"
#define BundleVersionKey @"CFBundleVersion"
#define ComponentNameKey @"Name"
#define ComponentTypeKey @"Type"

#define AC3DynamicRangeKey CFSTR("dynamicRange")
#define AC3StereoOverDolbyKey CFSTR("useStereoOverDolby")

@implementation CPFPerianPrefPaneController

#pragma mark Private Functions

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

- (NSString *)quickTimeComponentDir
{
	NSString *myPath = [[self bundle] bundlePath];
	NSString *basePath = nil;
	
	if([myPath hasPrefix:NSHomeDirectory()])
		basePath = NSHomeDirectory();
	else
		basePath = [NSString stringWithString:@"/"];
	
	return [basePath stringByAppendingPathComponent:@"Library/QuickTime"];
}

- (NSString *)coreAudioComponentDir
{
	NSString *myPath = [[self bundle] bundlePath];
	NSString *basePath = nil;
	
	if([myPath hasPrefix:NSHomeDirectory()])
		basePath = NSHomeDirectory();
	else
		basePath = [NSString stringWithString:@"/"];
	
	return [basePath stringByAppendingPathComponent:@"Library/Audio/Plug-Ins/Components"];
}

- (InstallStatus)installStatusForComponent:(NSString *)component type:(ComponentType)type withMyVersion:(NSString *)myVersion
{
	NSString *path = nil;
	
	switch(type)
	{
		case ComponentTypeCoreAudio:
			path = [self coreAudioComponentDir];
			break;
		case ComponentTypeQuickTime:
			path = [self quickTimeComponentDir];
			break;
	}
	path = [path stringByAppendingPathComponent:component];
	
	NSBundle *bundle = [NSBundle bundleWithPath:path];
	if(bundle == nil)
		return InstallStatusNotInstalled;
	
	NSString *currentVersion = [[bundle infoDictionary] objectForKey:BundleVersionKey];
	if([currentVersion compare:myVersion] == NSOrderedAscending)
		return InstallStatusOutdated;
	
	return InstallStatusInstalled;
}

#pragma mark Preference Pane Support

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

- (void)mainViewDidLoad
{
	/* General */
	NSDictionary *infoDict = [[self bundle] infoDictionary];
	installStatus = [self installStatusForComponent:@"Perian.component" type:ComponentTypeQuickTime withMyVersion:[infoDict objectForKey:BundleVersionKey]];
	if(installStatus == InstallStatusNotInstalled)
	{
		[textField_installStatus setStringValue:NSLocalizedString(@"Perian is not Installed", @"")];
		[button_install setStringValue:NSLocalizedString(@"Install", @"")];
	}
	else if(installStatus == InstallStatusOutdated)
	{
		[textField_installStatus setStringValue:NSLocalizedString(@"Perian is Installed, but Outdated", @"")];
		[button_install setStringValue:NSLocalizedString(@"Update", @"")];
	}
	else
	{
		//Perian is fine, but check components
		NSDictionary *myComponentsInfo = [infoDict objectForKey:ComponentInfoDictionaryKey];
		if(myComponentsInfo != nil)
		{
			NSEnumerator *componentEnum = [myComponentsInfo objectEnumerator];
			NSDictionary *componentInfo = nil;
			while((componentInfo = [componentEnum nextObject]) != nil)
			{
				InstallStatus tstatus = [self installStatusForComponent:[componentInfo objectForKey:ComponentNameKey] type:[[componentInfo objectForKey:ComponentTypeKey] intValue] withMyVersion:[componentInfo objectForKey:BundleVersionKey]];
				if(tstatus < installStatus)
					installStatus = tstatus;
			}
			switch(installStatus)
			{
				case InstallStatusNotInstalled:
					[textField_installStatus setStringValue:NSLocalizedString(@"Perian is Installed, but parts are Not Installed", @"")];
					[button_install setStringValue:NSLocalizedString(@"Install", @"")];
					break;
				case InstallStatusOutdated:
					[textField_installStatus setStringValue:NSLocalizedString(@"Perian is Installed, but parts are Outdated", @"")];
					[button_install setStringValue:NSLocalizedString(@"Update", @"")];
					break;
				case InstallStatusInstalled:
					[textField_installStatus setStringValue:NSLocalizedString(@"Perian is Installed", @"")];
					[button_install setStringValue:NSLocalizedString(@"Uninstall", @"")];
					break;
			}
		}
		else
		{
			[textField_installStatus setStringValue:NSLocalizedString(@"Perian is Installed", @"")];
			[button_install setStringValue:NSLocalizedString(@"Uninstall", @"")];
		}
		
	}
	
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
