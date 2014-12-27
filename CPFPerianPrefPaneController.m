/*
 * CPFPerianPrefPaneController.m
 *
 * This file is part of Perian.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#import "CPFPerianPrefPaneController.h"
#import "UpdateCheckerAppDelegate.h"

#define AC3DynamicRangeKey @"dynamicRange"
#define LastInstalledVersionKey @"LastInstalledVersion"
#define AC3TwoChannelModeKey @"twoChannelMode"
#define ExternalSubtitlesKey @"LoadExternalSubtitles"
#define DontShowMultiChannelWarning @"DontShowMultiChannelWarning"

//Old
#define AC3StereoOverDolbyKey @"useStereoOverDolby"
#define AC3ProLogicIIKey @"useDolbyProLogicII"

//A52 Constants
#define A52_STEREO 2
#define A52_DOLBY 10
#define A52_CHANNEL_MASK 15
#define A52_LFE 16
#define A52_ADJUST_LEVEL 32
#define A52_USE_DPLII 64

@interface NSString (VersionStringCompare)
- (BOOL)isVersionStringOlderThan:(NSString *)older;
@end

@implementation NSString (VersionStringCompare)
- (BOOL)isVersionStringOlderThan:(NSString *)older
{
	if([self compare:older] == NSOrderedAscending)
		return TRUE;
	if([self hasPrefix:older] && [self length] > [older length] && [self characterAtIndex:[older length]] == 'b')
		//1.0b1 < 1.0, so check for it.
		return TRUE;
	return FALSE;
}
@end

@interface CPFPerianPrefPaneController()
- (void)setAC3DynamicRange:(float)newVal;
- (void)saveAC3DynamicRange:(float)newVal;
@property (copy) NSString *perianAppID;
@property (copy) NSString *a52AppID;
@end

#define CPFLocalizedStringFromBundle(key, bundle, comment) \
    [bundle localizedStringForKey:(key) value:@"" table:nil]

@implementation CPFPerianPrefPaneController

#pragma mark Preferences Functions

- (void)synchronizePreferencesForApp:(NSString*)appName
{
	CFPreferencesAppSynchronize((__bridge CFStringRef)appName);
}

- (BOOL)getBoolFromKey:(NSString*)key forAppID:(NSString*)appID withDefault:(BOOL)defaultValue
{
	Boolean ret, exists = FALSE;
	
	ret = CFPreferencesGetAppBooleanValue((__bridge CFStringRef)key, (__bridge CFStringRef)appID, &exists);
	
	return exists ? ret : defaultValue;
}

- (void)setKey:(NSString*)key forAppID:(NSString*)appID fromBool:(BOOL)value
{
	CFPreferencesSetAppValue((__bridge CFStringRef)key, value ? kCFBooleanTrue : kCFBooleanFalse, (__bridge CFStringRef)appID);
}

- (float)getFloatFromKey:(NSString*)key forAppID:(NSString*)appID withDefault:(float)defaultValue
{
	CFPropertyListRef value;
	float ret = defaultValue;
	
	value = CFPreferencesCopyAppValue((__bridge CFStringRef)key, (__bridge CFStringRef)appID);
	if(value && CFGetTypeID(value) == CFNumberGetTypeID())
		CFNumberGetValue(value, kCFNumberFloatType, &ret);
	
	if(value)
		CFRelease(value);
	
	return ret;
}

- (void)setKey:(NSString*)key forAppID:(NSString*)appID fromFloat:(float)value
{
	CFNumberRef numRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &value);
	CFPreferencesSetAppValue((__bridge CFStringRef)key, numRef, (__bridge CFStringRef)appID);
	CFRelease(numRef);
}

- (NSUInteger)getUnsignedIntFromKey:(NSString*)key forAppID:(NSString*)appID withDefault:(NSUInteger)defaultValue
{
	NSUInteger ret; Boolean exists = FALSE;
	
	ret = CFPreferencesGetAppIntegerValue((__bridge CFStringRef)key, (__bridge CFStringRef)appID, &exists);
	
	return exists ? ret : defaultValue;
}

- (void)setKey:(NSString*)key forAppID:(NSString*)appID fromInt:(int)value
{
	CFNumberRef numRef = CFNumberCreate(NULL, kCFNumberIntType, &value);
	CFPreferencesSetAppValue((__bridge CFStringRef)key, numRef, (__bridge CFStringRef)appID);
	CFRelease(numRef);
}

- (NSString *)getStringFromKey:(NSString*)key forAppID:(NSString*)appID
{
	CFPropertyListRef value;
	NSString *nsVal;
	
	value = CFPreferencesCopyAppValue((__bridge CFStringRef)key, (__bridge CFStringRef)appID);
	
	if(value) {
		nsVal = CFBridgingRelease(value);
		
		if (CFGetTypeID(value) != CFStringGetTypeID())
			return nil;
	}
	
	return nsVal;
}

- (void)setKey:(NSString*)key forAppID:(NSString*)appID fromString:(NSString *)value
{
	CFPreferencesSetAppValue((__bridge CFStringRef)key, (__bridge CFPropertyListRef)value, (__bridge CFStringRef)appID);
}

- (NSDate *)getDateFromKey:(NSString*)key forAppID:(NSString*)appID
{
	CFPropertyListRef value;
	NSDate *ret = nil;
	
	value = CFPreferencesCopyAppValue((__bridge CFStringRef)key, (__bridge CFStringRef)appID);
	ret = CFBridgingRelease(value);
	
	if(value && CFGetTypeID(value) == CFDateGetTypeID())
		return ret;
	else
		return nil;
}

- (void)setKey:(NSString*)key forAppID:(NSString*)appID fromDate:(NSDate *)value
{
	CFPreferencesSetAppValue((__bridge CFStringRef)key, (__bridge CFPropertyListRef)value, (__bridge CFStringRef)appID);
}

#pragma mark Private Functions

- (NSString *)installationBasePath:(BOOL)userInstallation
{
	if(userInstallation)
		return NSHomeDirectory();
	return @"/";
}

- (NSString *)quickTimeComponentDir:(BOOL)userInstallation
{
	return [[self installationBasePath:userInstallation] stringByAppendingPathComponent:@"Library/QuickTime"];
}

- (NSString *)coreAudioComponentDir:(BOOL)userInstallation
{
	return [[self installationBasePath:userInstallation] stringByAppendingPathComponent:@"Library/Audio/Plug-Ins/Components"];
}

- (NSString *)frameworkComponentDir:(BOOL)userInstallation
{
	return [[self installationBasePath:userInstallation] stringByAppendingPathComponent:@"Library/Frameworks"];
}

- (NSString *)basePathForType:(ComponentType)type user:(BOOL)userInstallation
{
	NSString *path = nil;
	
	switch(type)
	{
		case ComponentTypeCoreAudio:
			path = [self coreAudioComponentDir:userInstallation];
			break;
		case ComponentTypeQuickTime:
			path = [self quickTimeComponentDir:userInstallation];
			break;
		case ComponentTypeFramework:
			path = [self frameworkComponentDir:userInstallation];
			break;
	}
	return path;
}

- (InstallStatus)installStatusForComponent:(NSString *)component type:(ComponentType)type withMyVersion:(NSString *)myVersion
{
	NSString *path = nil;
	InstallStatus ret = InstallStatusNotInstalled;
	
	path = [[self basePathForType:type user:userInstalled] stringByAppendingPathComponent:component];
	
	NSDictionary *infoDict = [NSDictionary dictionaryWithContentsOfFile:[path stringByAppendingPathComponent:@"Contents/Info.plist"]];
	if(infoDict != nil)
	{
		NSString *currentVersion = infoDict[BundleVersionKey];
		if([currentVersion isVersionStringOlderThan:myVersion])
			ret = InstallStatusOutdated;
		else
			ret = InstallStatusInstalled;
	}
	
	/* Check other installation type */
	path = [[self basePathForType:type user:!userInstalled] stringByAppendingPathComponent:component];
	
	infoDict = [NSDictionary dictionaryWithContentsOfFile:[path stringByAppendingPathComponent:@"Contents/Info.plist"]];
	if(infoDict == nil)
	/* Above result is all there is */
		return ret;
	
	return setWrongLocationInstalled(ret);
}

- (void)setInstalledVersionString
{
	NSString *path = [[self basePathForType:ComponentTypeQuickTime user:userInstalled] stringByAppendingPathComponent:@"Perian.component"];
	NSString *currentVersion = @"-";
	NSDictionary *infoDict = [NSDictionary dictionaryWithContentsOfFile:[path stringByAppendingPathComponent:@"Contents/Info.plist"]];
	if (infoDict != nil)
		currentVersion = infoDict[BundleVersionKey];
	[textField_currentVersion setStringValue:[CPFLocalizedStringFromBundle(@"Installed Version: ", self.bundle, @"") stringByAppendingString:currentVersion]];
}

#pragma mark Preference Pane Support

- (id)initWithBundle:(NSBundle *)bundle
{
	if ( ( self = [super initWithBundle:bundle] ) != nil ) {
		perianForumURL = [[NSURL alloc] initWithString:@"http://forums.cocoaforge.com/index.php?c=12"];
		perianDonateURL = [[NSURL alloc] initWithString:@"http://perian.org"];
		perianWebSiteURL = [[NSURL alloc] initWithString:@"http://perian.org"];
		
		self.perianAppID = @"org.perian.Perian";
		self.a52AppID = @"com.cod3r.a52codec";
		
		NSString *myPath = [[self bundle] bundlePath];
		
		if([myPath hasPrefix:@"/Library"])
			userInstalled = NO;
		else
			userInstalled = YES;
		
		//#warning TODO(durin42) Should filter out components that aren't installed from this list.
		componentReplacementInfo = [[NSArray alloc] initWithContentsOfFile:[[[self bundle] resourcePath] stringByAppendingPathComponent:ComponentInfoPlist]];
	}
	
	return self;
}

- (NSDictionary *)myInfoDict
{
	return [NSDictionary dictionaryWithContentsOfFile:[[[self bundle] bundlePath] stringByAppendingPathComponent:@"Contents/Info.plist"]];
}

- (void)checkForInstallation:(BOOL)freshInstall
{
	NSDictionary *infoDict = [self myInfoDict];
	NSString *myVersion = infoDict[BundleVersionKey];
	
	[self setInstalledVersionString];
	installStatus = [self installStatusForComponent:@"Perian.component" type:ComponentTypeQuickTime withMyVersion:myVersion];
	if(currentInstallStatus(installStatus) == InstallStatusNotInstalled)
	{
		[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is not installed", self.bundle, @"")];
		[button_install setTitle:CPFLocalizedStringFromBundle(@"Install Perian", self.bundle, @"")];
	}
	else if(currentInstallStatus(installStatus) == InstallStatusOutdated)
	{
		[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed, but outdated", self.bundle, @"")];
		[button_install setTitle:CPFLocalizedStringFromBundle(@"Update Perian", self.bundle, @"")];
	}
	else
	{
		//Perian is fine, but check components
		NSDictionary *myComponentsInfo = infoDict[ComponentInfoDictionaryKey];
		if(myComponentsInfo != nil)
		{
			for (NSDictionary *componentInfo in myComponentsInfo) {
				InstallStatus tstatus = [self installStatusForComponent:componentInfo[ComponentNameKey] type:[componentInfo[ComponentTypeKey] intValue] withMyVersion:componentInfo[BundleVersionKey]];
				if(tstatus < installStatus)
					installStatus = tstatus;
			}
			switch (installStatus) {
				case InstallStatusInstalledInWrongLocation:
				case InstallStatusNotInstalled:
					[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed, but parts are not installed", self.bundle, @"")];
					[button_install setTitle:CPFLocalizedStringFromBundle(@"Install Perian", self.bundle, @"")];
					break;
				case InstallStatusOutdatedWithAnotherInWrongLocation:
				case InstallStatusOutdated:
					[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed, but parts are outdated", self.bundle, @"")];
					[button_install setTitle:CPFLocalizedStringFromBundle(@"Update Perian", self.bundle, @"")];
					break;
				case InstallStatusInstalledInBothLocations:
					[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed twice", self.bundle, @"")];
					[button_install setTitle:CPFLocalizedStringFromBundle(@"Correct Installation", self.bundle, @"")];
					break;
				case InstallStatusInstalled:
					if(freshInstall)
						[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed; please restart your applications that use Perian", self.bundle, @"")];
					else
						[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed", self.bundle, @"")];
					[button_install setTitle:CPFLocalizedStringFromBundle(@"Remove Perian", self.bundle, @"")];
					break;
			}
		}
		else if(isWrongLocationInstalled(installStatus))
		{
			[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed twice", self.bundle, @"")];
			[button_install setTitle:CPFLocalizedStringFromBundle(@"Correct Installation", self.bundle, @"")];
		}
		else
		{
			[textField_installStatus setStringValue:CPFLocalizedStringFromBundle(@"Perian is installed", self.bundle, @"")];
			[button_install setTitle:CPFLocalizedStringFromBundle(@"Remove Perian", self.bundle, @"")];
		}
		
	}
	
	if (errorString) {
		[textField_statusMessage setStringValue:[NSString stringWithFormat:@"Error: %@", errorString]];
	} else
		[textField_statusMessage setStringValue:@""];
	
	[button_install setEnabled:YES];
}

- (int)upgradeA52Prefs
{
	int twoChannelMode;
	if([self getBoolFromKey:AC3StereoOverDolbyKey forAppID:_a52AppID withDefault:NO])
		twoChannelMode = A52_STEREO;
	else if([self getBoolFromKey:AC3ProLogicIIKey forAppID:_a52AppID withDefault:NO])
		twoChannelMode = A52_DOLBY | A52_USE_DPLII;
	else
		twoChannelMode = A52_DOLBY;
	
	[self setKey:AC3TwoChannelModeKey forAppID:_a52AppID fromInt:twoChannelMode];
	return twoChannelMode;
}

- (void)didSelect
{
	/* General */
	NSString *lastInstVersion = [self getStringFromKey:LastInstalledVersionKey forAppID:_perianAppID];
	NSString *myVersion = [self myInfoDict][BundleVersionKey];
	
	NSAttributedString		*about;
	about = [[NSAttributedString alloc] initWithPath:[[self bundle] pathForResource:@"Read Me" ofType:@"rtf"]
								   documentAttributes:nil];
	[[textView_about textStorage] setAttributedString:about];
	[[textView_about enclosingScrollView] setLineScroll:10];
	[[textView_about enclosingScrollView] setPageScroll:20];
	
	if((lastInstVersion == nil || [lastInstVersion isVersionStringOlderThan:myVersion]) && installStatus != InstallStatusInstalled)
	{
		/*Check for temp after an update */
		NSString *tempPrefPane = [NSTemporaryDirectory() stringByAppendingPathComponent:@"PerianPane.prefPane"];
		
		[[NSFileManager defaultManager] removeItemAtPath:tempPrefPane error:NULL];
		
		[self installUninstall:nil];
		[self setKey:LastInstalledVersionKey forAppID:_perianAppID fromString:myVersion];
		[self updateCheck:nil];
	} else {
		[self checkForInstallation:NO];
	}
	
	NSDate *updateDate = [self getDateFromKey:NEXT_RUN_KEY forAppID:_perianAppID];
	if([updateDate timeIntervalSinceNow] > 1000000000) //futureDate
		[button_autoUpdateCheck setIntValue:0];
	else
		[button_autoUpdateCheck setIntValue:1];
	
	/* A52 Prefs */
	unsigned int twoChannelMode = [self getUnsignedIntFromKey:AC3TwoChannelModeKey forAppID:_a52AppID withDefault:0xffffffff];
	if(twoChannelMode != 0xffffffff)
	{
		/* sanity checks */
		if(twoChannelMode & A52_CHANNEL_MASK & 0xf7 != 2)
		{
			/* matches 2 and 10, which is Stereo and Dolby */
			twoChannelMode = A52_DOLBY;
		}
		twoChannelMode &= ~A52_ADJUST_LEVEL & ~A52_LFE;
	}
	else
		twoChannelMode = [self upgradeA52Prefs];
	CFPreferencesSetAppValue((__bridge CFStringRef)AC3StereoOverDolbyKey, NULL, (__bridge CFStringRef)_a52AppID);
	CFPreferencesSetAppValue((__bridge CFStringRef)AC3ProLogicIIKey, NULL, (__bridge CFStringRef)_a52AppID);
	switch(twoChannelMode)
	{
		case A52_STEREO:
			[popup_outputMode selectItemAtIndex:0];
			break;
		case A52_DOLBY:
			[popup_outputMode selectItemAtIndex:1];
			break;
		case A52_DOLBY | A52_USE_DPLII:
			[popup_outputMode selectItemAtIndex:2];
			break;
		default:
			[popup_outputMode selectItemAtIndex:3];
			break;
	}
	
	[self setAC3DynamicRange:[self getFloatFromKey:AC3DynamicRangeKey forAppID:_a52AppID withDefault:1.0]];
	
	[button_loadExternalSubtitles setState:[self getBoolFromKey:ExternalSubtitlesKey forAppID:_perianAppID withDefault:YES]];
}

- (void)didUnselect
{
	[self synchronizePreferencesForApp:_perianAppID];
	[self synchronizePreferencesForApp:_a52AppID];
}

- (void)dealloc
{
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:UPDATE_STATUS_NOTIFICATION object:nil];
	if(auth != nil)
		AuthorizationFree(auth, 0);
}


#pragma mark Install/Uninstall

/* Shamelessly ripped from Sparkle (and now different) */
- (BOOL)_extractArchivePath:archivePath toDestination:(NSString *)destination finalPath:(NSString *)finalPath
{
	BOOL ret = NO, oldExist;
	NSFileManager *defaultFileManager = [NSFileManager defaultManager];
	char *cmd;
	
	oldExist = [defaultFileManager fileExistsAtPath:finalPath];
	
	if(oldExist)
		cmd = "rm -rf \"$DST_COMPONENT\" && "
		"ditto -x -k --rsrc \"$SRC_ARCHIVE\" \"$DST_PATH\"";
	else
		cmd = "mkdir -p \"$DST_PATH\" && "
		"ditto -x -k --rsrc \"$SRC_ARCHIVE\" \"$DST_PATH\"";
	
	setenv("SRC_ARCHIVE", [archivePath fileSystemRepresentation], 1);
	setenv("DST_PATH", [destination fileSystemRepresentation], 1);
	setenv("DST_COMPONENT", [finalPath fileSystemRepresentation], 1);
	
	int status = system(cmd);
	if(WIFEXITED(status) && WEXITSTATUS(status) == 0)
		ret = YES;
	else
		errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"extraction of %@ failed\n", self.bundle, @""), [finalPath lastPathComponent]];
	
	unsetenv("SRC_ARCHIVE");
	unsetenv("DST_COMPONENT");
	unsetenv("DST_PATH");
	return ret;
}

- (BOOL)_authenticatedExtractArchivePath:(NSString *)archivePath toDestination:(NSString *)destination finalPath:(NSString *)finalPath
{
	BOOL ret = NO, oldExist;
	NSFileManager *defaultFileManager = [NSFileManager defaultManager];
	char *cmd;
	
	oldExist = [defaultFileManager fileExistsAtPath:finalPath];
	
	if(oldExist)
		cmd = "rm -rf \"$DST_COMPONENT\" && "
		"ditto -x -k --rsrc \"$SRC_ARCHIVE\" \"$DST_PATH\" && "
		"chown -R root:admin \"$DST_COMPONENT\"";
	else
		cmd = "mkdir -p \"$DST_PATH\" && "
		"ditto -x -k --rsrc \"$SRC_ARCHIVE\" \"$DST_PATH\" && "
		"chown -R root:admin \"$DST_COMPONENT\"";
	
	setenv("SRC_ARCHIVE", [archivePath fileSystemRepresentation], 1);
	setenv("DST_COMPONENT", [finalPath fileSystemRepresentation], 1);
	setenv("DST_PATH", [destination fileSystemRepresentation], 1);
	
	char* const arguments[] = { "-c", cmd, NULL };
	if(auth && AuthorizationExecuteWithPrivileges(auth, "/bin/sh", kAuthorizationFlagDefaults, arguments, NULL) == errAuthorizationSuccess)
	{
		int status;
		int pid = wait(&status);
		if(pid != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0)
			ret = YES;
		else
			errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"extraction of %@ failed\n", self.bundle, @""), [finalPath lastPathComponent]];
	}
	else
		errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"authentication failed while extracting %@\n", self.bundle, @""), [finalPath lastPathComponent]];
	
	unsetenv("SRC_ARCHIVE");
	unsetenv("DST_COMPONENT");
	unsetenv("DST_PATH");
	return ret;
}

- (BOOL)_authenticatedRemove:(NSString *)componentPath
{
	BOOL ret = NO;
	NSFileManager *defaultFileManager = [NSFileManager defaultManager];
	
	if(![defaultFileManager fileExistsAtPath:componentPath])
		return YES; // No error, just forget it
	
	char *cmd = "rm -rf \"$COMP_PATH\"";
	
	setenv("COMP_PATH", [componentPath fileSystemRepresentation], 1);
	
	char* const arguments[] = { "-c", cmd, NULL };
	if(auth && AuthorizationExecuteWithPrivileges(auth, "/bin/sh", kAuthorizationFlagDefaults, arguments, NULL) == errAuthorizationSuccess)
	{
		int status;
		int pid = wait(&status);
		if(pid != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0)
			ret = YES;
		else
			errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"removal of %@ failed\n", self.bundle, @""), [componentPath lastPathComponent]];
	}
	else
		errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"authentication failed while removing %@\n", self.bundle, @""), [componentPath lastPathComponent]];
	
	unsetenv("COMP_PATH");
	return ret;
}

- (BOOL)installArchive:(NSString *)archivePath forPiece:(NSString *)component type:(ComponentType)type withMyVersion:(NSString *)myVersion
{
	NSString *containingDir = [self basePathForType:type user:userInstalled];
	BOOL ret = YES;
	
	InstallStatus pieceStatus = [self installStatusForComponent:component type:type withMyVersion:myVersion];
	if(!userInstalled && currentInstallStatus(pieceStatus) != InstallStatusInstalled)
	{
		BOOL result = [self _authenticatedExtractArchivePath:archivePath toDestination:containingDir finalPath:[containingDir stringByAppendingPathComponent:component]];
		if(result == NO)
			ret = NO;
	}
	else
	{
		//Not authenticated
		if(currentInstallStatus(pieceStatus) == InstallStatusOutdated)
		{
			//Remove the old one here
			BOOL result = [[NSFileManager defaultManager] removeItemAtPath:[containingDir stringByAppendingPathComponent:component] error:NULL];
			if(result == NO) {
				errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"removal of %@ failed\n", self.bundle, @""), component];
				ret = NO;
			}
		}
		if(currentInstallStatus(pieceStatus) != InstallStatusInstalled)
		{
			//Decompress and install new one
			BOOL result = [self _extractArchivePath:archivePath toDestination:containingDir finalPath:[containingDir stringByAppendingPathComponent:component]];
			if(result == NO)
				ret = NO;
		}
	}
	if(ret != NO && isWrongLocationInstalled(pieceStatus) != 0)
	{
		/* Let's try and remove the wrong one, if we can, but only if install succeeded */
		containingDir = [self basePathForType:type user:!userInstalled];
		
		if(userInstalled)
			ret = [self _authenticatedRemove:[containingDir stringByAppendingPathComponent:component]];
		else
		{
			ret = [[NSFileManager defaultManager] removeItemAtPath:[containingDir stringByAppendingPathComponent:component] error:NULL];
			if(ret == NO)
				errorString = [NSString stringWithFormat:CPFLocalizedStringFromBundle(@"removal of %@ failed\n", self.bundle, @""), component];
		}
	}
	return ret;
}

- (void)lsRegisterApps
{
	NSString *resourcePath = [[self bundle] resourcePath];
	NSDictionary *infoDict = [self myInfoDict];
	NSDictionary *apps = infoDict[AppsToRegisterDictionaryKey];
	
	for (NSString *app in apps) {
		NSURL *url = [NSURL fileURLWithPath:[resourcePath stringByAppendingPathComponent:app]];
		
		LSRegisterURL((__bridge CFURLRef)url, true);
	}
}

- (void)deletePluginCache
{
	// delete the QuickTime web plugin's component cache
	// unfortunately there is no way for this to work for "all users" installs
	
	NSString *path = [@"~/Library/Preferences/com.apple.quicktime.plugin.preferences.plist" stringByExpandingTildeInPath];
	[[NSFileManager defaultManager] removeItemAtPath:path error:NULL];
}

- (void)install:(id)sender
{
	@autoreleasepool {
		NSDictionary *infoDict = [self myInfoDict];
		NSDictionary *myComponentsInfo = infoDict[ComponentInfoDictionaryKey];
		NSString *componentPath = [[[self bundle] resourcePath] stringByAppendingPathComponent:@"Components"];
		NSString *coreAudioComponentPath = [componentPath stringByAppendingPathComponent:@"CoreAudio"];
		NSString *quickTimeComponentPath = [componentPath stringByAppendingPathComponent:@"QuickTime"];
		NSString *frameworkComponentPath = [componentPath stringByAppendingPathComponent:@"Frameworks"];
		
		errorString = nil;
		/* This doesn't ask the user, so create it anyway.  If we don't need it, no problem */
		if(AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &auth) != errAuthorizationSuccess)
		/* Oh well, hope we don't need it */
			auth = nil;
		
		[self installArchive:[componentPath stringByAppendingPathComponent:@"Perian.zip"] forPiece:@"Perian.component" type:ComponentTypeQuickTime withMyVersion:infoDict[BundleVersionKey]];
		
		for (NSDictionary *myComponent in myComponentsInfo) {
			NSString *archivePath = nil;
			ComponentType type = [myComponent[ComponentTypeKey] intValue];
			switch(type)
			{
				case ComponentTypeCoreAudio:
					archivePath = [coreAudioComponentPath stringByAppendingPathComponent:myComponent[ComponentArchiveNameKey]];
					break;
				case ComponentTypeQuickTime:
					archivePath = [quickTimeComponentPath stringByAppendingPathComponent:myComponent[ComponentArchiveNameKey]];
					break;
				case ComponentTypeFramework:
					archivePath = [frameworkComponentPath stringByAppendingPathComponent:myComponent[ComponentArchiveNameKey]];
					break;
			}
			if (![self installArchive:archivePath forPiece:myComponent[ComponentNameKey] type:type withMyVersion:myComponent[BundleVersionKey]]) {
				break;
			}
		}
		if(auth != nil)
		{
			AuthorizationFree(auth, 0);
			auth = nil;
		}
		
		[self lsRegisterApps];
		[self deletePluginCache];
		
		[self performSelectorOnMainThread:@selector(installComplete:) withObject:nil waitUntilDone:NO];
	}
}

- (void)uninstall:(id)sender
{
	@autoreleasepool {
		NSDictionary *infoDict = [self myInfoDict];
		NSDictionary *myComponentsInfo = infoDict[ComponentInfoDictionaryKey];
		NSFileManager *fileManager = [NSFileManager defaultManager];
		NSString *componentPath;
		
		errorString = nil;
		/* This doesn't ask the user, so create it anyway.  If we don't need it, no problem */
		if(AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &auth) != errAuthorizationSuccess)
		/* Oh well, hope we don't need it */
			auth = nil;
		
		componentPath = [[self quickTimeComponentDir:userInstalled] stringByAppendingPathComponent:@"Perian.component"];
		if(auth != nil && !userInstalled)
			[self _authenticatedRemove:componentPath];
		else
			[fileManager removeItemAtPath:componentPath error:NULL];
		
		for (NSDictionary *myComponent in myComponentsInfo) {
			ComponentType type = [myComponent[ComponentTypeKey] intValue];
			NSString *directory = [self basePathForType:type user:userInstalled];
			componentPath = [directory stringByAppendingPathComponent:myComponent[ComponentNameKey]];
			if(auth != nil && !userInstalled)
				[self _authenticatedRemove:componentPath];
			else
				[fileManager removeItemAtPath:componentPath error:NULL];
		}
		if(auth != nil)
		{
			AuthorizationFree(auth, 0);
			auth = nil;
		}
		
		[self performSelectorOnMainThread:@selector(installComplete:) withObject:nil waitUntilDone:NO];
	}
}

- (IBAction)installUninstall:(id)sender
{
	if(installStatus == InstallStatusInstalled)
		[NSThread detachNewThreadSelector:@selector(uninstall:) toTarget:self withObject:nil];
	else
		[NSThread detachNewThreadSelector:@selector(install:) toTarget:self withObject:nil];
}

- (void)installComplete:(id)sender
{
	[self checkForInstallation:YES];
}

#pragma mark Component Version List
- (NSArray *)installedComponentsForUser:(BOOL)user
{
	NSString *path = [self basePathForType:ComponentTypeQuickTime user:user];
	NSArray *installedComponents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:path error:NULL];
	NSMutableArray *retArray = [[NSMutableArray alloc] initWithCapacity:[installedComponents count]];
	NSString *component;
	for (component in installedComponents) {
		if ([[component pathExtension] isEqualToString:@"component"])
			[retArray addObject:component];
	}
	return retArray;
}

- (NSDictionary *)componentInfoForComponent:(NSString *)component userInstalled:(BOOL)user
{
	NSString *compName = component;
	if ([[component pathExtension] isEqualToString:@"component"])
		compName = [component stringByDeletingPathExtension];
	NSMutableDictionary *componentInfo = [[NSMutableDictionary alloc] initWithObjectsAndKeys:compName, @"name", NULL];
	NSBundle *componentBundle = [NSBundle bundleWithPath:[[self basePathForType:ComponentTypeQuickTime
																		   user:user] stringByAppendingPathComponent:component]];
	NSDictionary *infoDictionary = nil;
	if (componentBundle)
		infoDictionary = [componentBundle infoDictionary];
	if (infoDictionary && infoDictionary[BundleIdentifierKey]) {
		NSString *componentVersion = infoDictionary[BundleVersionKey];
		if (componentVersion)
			componentInfo[@"version"] = componentVersion;
		else
			componentInfo[@"version"] = @"Unknown";
		componentInfo[@"installType"] = (user ? @"User" : @"System");
		componentInfo[@"status"] = [self checkComponentStatusByBundleIdentifier:[componentBundle bundleIdentifier]];
		componentInfo[@"bundleID"] = [componentBundle bundleIdentifier];
	} else {
		componentInfo[@"version"] = @"Unknown";
		componentInfo[@"installType"] = (user ? @"User" : @"System");
		NSString *bundleIdent = [NSString stringWithFormat:PERIAN_NO_BUNDLE_ID_FORMAT,compName];
		componentInfo[@"status"] = [self checkComponentStatusByBundleIdentifier:bundleIdent];
		componentInfo[@"bundleID"] = bundleIdent;
	}
	return componentInfo;
}

- (NSArray *)installedComponents
{
	NSArray *userComponents = [self installedComponentsForUser:YES];
	NSArray *systemComponents = [self installedComponentsForUser:NO];
	NSUInteger numComponents = [userComponents count] + [systemComponents count];
	NSMutableArray *components = [[NSMutableArray alloc] initWithCapacity:numComponents];
	for (NSString *compName in userComponents) {
		[components addObject:[self componentInfoForComponent:compName userInstalled:YES]];
	}
	
	for (NSString *compName in systemComponents) {
		[components addObject:[self componentInfoForComponent:compName userInstalled:NO]];
	}
	return components;
}

- (NSString *)checkComponentStatusByBundleIdentifier:(NSString *)bundleID
{
	NSString *status = @"OK";
	for (NSDictionary *infoDict in componentReplacementInfo) {
		for (NSString *obsoletedID in infoDict[ObsoletesKey]) {
			if ([obsoletedID isEqualToString:bundleID]) {
				status = [NSString stringWithFormat:@"Obsoleted by %@", infoDict[HumanReadableNameKey]];
			}
		}
	}
	return status;
}

#pragma mark Check Updates
- (void)updateCheckStatusChanged:(NSNotification*)notification
{
	NSString *status = [notification object];
	
	//FIXME: localize these
	if ([status isEqualToString:@"Starting"]) {
		[textField_statusMessage setStringValue:@"Checking..."];
	} else if ([status isEqualToString:@"Error"]) {
		[textField_statusMessage setStringValue:@"Couldn't reach the update server."];
	} else if ([status isEqualToString:@"NoUpdates"]) {
		[textField_statusMessage setStringValue:@"There are no updates."];
	} else if ([status isEqualToString:@"YesUpdates"]) {
		[textField_statusMessage setStringValue:@"Updates found!"];
	}
}

- (IBAction)updateCheck:(id)sender
{
	FSRef updateCheckRef;
	
	[[NSDistributedNotificationCenter defaultCenter] removeObserver:self name:UPDATE_STATUS_NOTIFICATION object:nil];
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(updateCheckStatusChanged:) name:UPDATE_STATUS_NOTIFICATION object:nil];
	[self setKey:MANUAL_RUN_KEY forAppID:_perianAppID fromBool:YES];
	[self synchronizePreferencesForApp:_perianAppID];
	OSStatus status = FSPathMakeRef((UInt8 *)[[[[self bundle] bundlePath] stringByAppendingPathComponent:@"Contents/Resources/PerianUpdateChecker.app"] fileSystemRepresentation], &updateCheckRef, NULL);
	if(status != noErr)
		return;
	
	LSOpenFSRef(&updateCheckRef, NULL);
}

- (IBAction)setAutoUpdateCheck:(id)sender
{
	NSString *key = NEXT_RUN_KEY;
	if([button_autoUpdateCheck intValue])
		[self setKey:key forAppID:_perianAppID fromDate:[NSDate dateWithTimeIntervalSinceNow:TIME_INTERVAL_TIL_NEXT_RUN]];
	else
		[self setKey:key forAppID:_perianAppID fromDate:[NSDate distantFuture]];
	
	[self synchronizePreferencesForApp:_perianAppID];
}


#pragma mark AC3
- (IBAction)setAC3DynamicRangePopup:(id)sender
{
	int selected = [popup_ac3DynamicRangeType indexOfSelectedItem];
	switch(selected)
	{
		case 0:
			[self saveAC3DynamicRange:1.0];
			break;
		case 1:
			[self saveAC3DynamicRange:2.0];
			break;
		case 3:
			[NSApp beginSheet:window_dynRangeSheet modalForWindow:[[self mainView] window] modalDelegate:nil didEndSelector:nil contextInfo:NULL];
			break;
		default:
			break;
	}
}

- (void)displayMultiChannelWarning
{
	NSString *multiChannelWarning = CPFLocalizedStringFromBundle(@"<p style=\"font: 13pt Lucida Grande;\">Multi-Channel Output is not Dolby Digital Passthrough!  It is designed for those with multiple discrete speakers connected to their mac.  If you selected this expecting passthrough, you are following the wrong instructions.  Follow <a href=\"http://www.cod3r.com/2008/02/the-correct-way-to-enable-ac3-passthrough-with-quicktime/\">these</a> instead.</p>", self.bundle, @"");
	NSAttributedString *multiChannelWarningAttr = [[NSAttributedString alloc] initWithHTML:[multiChannelWarning dataUsingEncoding:NSUTF8StringEncoding] documentAttributes:nil];
	[textField_multiChannelText setAttributedStringValue:multiChannelWarningAttr];
	[NSApp beginSheet:window_multiChannelSheet modalForWindow:[[self mainView] window] modalDelegate:nil didEndSelector:nil contextInfo:NULL];
}

- (IBAction)set2ChannelModePopup:(id)sender;
{
	int selected = [popup_outputMode indexOfSelectedItem];
	switch(selected)
	{
		case 0:
			[self setKey:AC3TwoChannelModeKey forAppID:_a52AppID fromInt:A52_STEREO];
			break;
		case 1:
			[self setKey:AC3TwoChannelModeKey forAppID:_a52AppID fromInt:A52_DOLBY];
			break;
		case 2:
			[self setKey:AC3TwoChannelModeKey forAppID:_a52AppID fromInt:A52_DOLBY | A52_USE_DPLII];
			break;
		case 3:
			[self setKey:AC3TwoChannelModeKey forAppID:_a52AppID fromInt:0];
			if(![self getBoolFromKey:DontShowMultiChannelWarning forAppID:_perianAppID withDefault:NO])
				[self displayMultiChannelWarning];
			break;
		default:
			break;
	}
}

- (void)setAC3DynamicRange:(float)newVal
{
	if(newVal > 4.0)
		newVal = 4.0;
	if(newVal < 0.0)
		newVal = 0.0;
	
	nextDynValue = newVal;
	[textField_ac3DynamicRangeValue setFloatValue:newVal];
	[slider_ac3DynamicRangeSlider setFloatValue:newVal];
	if(newVal == 1.0)
		[popup_ac3DynamicRangeType selectItemAtIndex:0];
	else if(newVal == 2.0)
		[popup_ac3DynamicRangeType selectItemAtIndex:1];
	else
		[popup_ac3DynamicRangeType selectItemAtIndex:3];
}

- (void)saveAC3DynamicRange:(float)newVal
{
	[self setKey:AC3DynamicRangeKey forAppID:_a52AppID fromFloat:newVal];
	[self setAC3DynamicRange:newVal];
}

- (IBAction)setAC3DynamicRangeValue:(id)sender
{
	float newVal = [textField_ac3DynamicRangeValue floatValue];
	
	[self setAC3DynamicRange:newVal];
}

- (IBAction)setAC3DynamicRangeSlider:(id)sender
{
	float newVal = [slider_ac3DynamicRangeSlider floatValue];
	
	[self setAC3DynamicRange:newVal];
}

- (IBAction)cancelDynRangeSheet:(id)sender
{
	[self setAC3DynamicRange:[self getFloatFromKey:AC3DynamicRangeKey forAppID:_a52AppID withDefault:1.0]];
	[NSApp endSheet:window_dynRangeSheet];
	[window_dynRangeSheet orderOut:self];
}

- (IBAction)saveDynRangeSheet:(id)sender;
{
	[NSApp endSheet:window_dynRangeSheet];
	[self saveAC3DynamicRange:nextDynValue];
	[window_dynRangeSheet orderOut:self];
}

- (IBAction)dismissMultiChannelSheet:(id)sender
{
	[NSApp endSheet:window_multiChannelSheet];
	[self setKey:DontShowMultiChannelWarning forAppID:_perianAppID fromBool:[button_multiChannelNeverShow state]];
	[window_multiChannelSheet orderOut:self];
	[self synchronizePreferencesForApp:_perianAppID];
}

#pragma mark Subtitles
- (IBAction)setLoadExternalSubtitles:(id)sender
{	
	[self setKey:ExternalSubtitlesKey forAppID:_perianAppID fromBool:(BOOL)[sender state]];
	[self synchronizePreferencesForApp:_perianAppID];
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
