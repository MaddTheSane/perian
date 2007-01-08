//
//  UpdateCheckerAppDelegate.m
//  Perian
//
//  Created by Augie Fackler on 1/6/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//
// This is really just a heavily-customized version of SUUpdate designed
// so that the updates are done using an app and not a framework. We do
// this so that things like little snitch don't give us problems, and so
// we can be more sure that the update won't get run twice accidentaly.

#import "UpdateCheckerAppDelegate.h"

@implementation UpdateCheckerAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	NSDate *nextRunDate = [[NSUserDefaults standardUserDefaults] objectForKey:NEXT_RUN_KEY];
	if (nextRunDate == nil || [nextRunDate laterDate:[NSDate date]] != nextRunDate) {
		//this means we should in fact run
		[[NSUserDefaults standardUserDefaults] setObject:[NSDate date] forKey:NEXT_RUN_KEY];
		[self doUpdateCheck];
	} else {
		//another instance was already started and therefore we don't need to do this again
		[[NSApplication sharedApplication] terminate:self];
	}
}

- (void)doUpdateCheck
{
	NSString *updateUrlString = SUInfoValueForKey(UPDATE_URL_KEY);
	
	if (!updateUrlString) { [NSException raise:@"NoFeedURL" format:@"No feed URL is specified in the Info.plist!"]; }

	SUAppcast *appcast = [[SUAppcast alloc] init];
	[appcast setDelegate:self];
	[appcast fetchAppcastFromURL:[NSURL URLWithString:updateUrlString]];
}

- (void)appcastDidFinishLoading:(SUAppcast *)appcast
{
	latest = [[appcast newestItem] retain];
	
	if (![latest fileVersion])
	{
		[NSException raise:@"SUAppcastException" format:@"Can't extract a version string from the appcast feed. The filenames should look like YourApp_1.5.tgz, where 1.5 is the version number."];
	}
	
	// OS version (Apple recommends using SystemVersion.plist instead of Gestalt() here, don't ask me why).
	// This code *should* use NSSearchPathForDirectoriesInDomains(NSCoreServiceDirectory, NSSystemDomainMask, YES)
	// but that returns /Library/CoreServices for some reason
	NSString *versionPlistPath = @"/System/Library/CoreServices/SystemVersion.plist";
	NSString *currentSystemVersion = [[[NSDictionary dictionaryWithContentsOfFile:versionPlistPath] objectForKey:@"ProductVersion"] retain];

	BOOL updateAvailable = SUStandardVersionComparison([latest minimumSystemVersion], currentSystemVersion);
#warning This should compare against the Perian.prefpane version, not the SUHostAppVersion, but I didn't do that yet - RAF
	updateAvailable = (updateAvailable && (SUStandardVersionComparison([latest fileVersion], SUHostAppVersion()) == NSOrderedAscending));
	NSString *skippedVersion = [[NSUserDefaults standardUserDefaults] objectForKey:SKIPPED_VERSION_KEY];
	if (updateAvailable && (!skippedVersion || 
		(skippedVersion && ![skippedVersion isEqualToString:[latest versionString]]))) {
		[self showUpdatePanelForItem:latest];
	} else //nothing to see here, so we move along
		[[NSApplication sharedApplication] terminate:self];
}

- (void)showUpdatePanelForItem:(SUAppcastItem *)updateItem
{
	updateAlert = [[SUUpdateAlert alloc] initWithAppcastItem:updateItem];
 	[updateAlert setDelegate:self];
	[updateAlert showWindow:self];
}

- (void)updateAlert:(SUUpdateAlert *)alert finishedWithChoice:(SUUpdateAlertChoice)choice
{
	if (choice == SUInstallUpdateChoice) {
		[self beginDownload];
	} else {
		if (choice == SUSkipThisVersionChoice)
			[[NSUserDefaults standardUserDefaults] setObject:[latest versionString] forKey:SKIPPED_VERSION_KEY];
		[[NSApplication sharedApplication] terminate:self];
	}
}

- (void)showUpdateErrorAlertWithInfo:(NSString *)info
{
	NSRunAlertPanel(SULocalizedString(@"Update Error!", nil), info, SULocalizedString(@"Cancel", nil), nil, nil);
}

- (void)beginDownload
{
	statusController = [[SUStatusController alloc] init];
	[statusController beginActionWithTitle:SULocalizedString(@"Downloading update...", nil) maxProgressValue:0 statusText:nil];
	[statusController setButtonTitle:SULocalizedString(@"Cancel", nil) target:self action:@selector(cancelDownload:) isDefault:NO];
	[statusController showWindow:self];
	
	downloader = [[NSURLDownload alloc] initWithRequest:[NSURLRequest requestWithURL:[latest fileURL]] delegate:self];	
}

- (void)cancelDownload:(id)sender
{
	[downloader cancel];
	[statusController close];
	[[NSApplication sharedApplication] terminate:self];
}

#pragma mark NSURLDownload delegate methods
- (void)download:(NSURLDownload *)download didReceiveResponse:(NSURLResponse *)response
{
	[statusController setMaxProgressValue:[response expectedContentLength]];
}

- (void)download:(NSURLDownload *)download decideDestinationWithSuggestedFilename:(NSString *)name
{
	// If name ends in .txt, the server probably has a stupid MIME configuration. We'll give
	// the developer the benefit of the doubt and chop that off.
	if ([[name pathExtension] isEqualToString:@"txt"])
		name = [name stringByDeletingPathExtension];
	
	// We create a temporary directory in /tmp and stick the file there.
	NSString *tempDir = [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSProcessInfo processInfo] globallyUniqueString]];
	BOOL success = [[NSFileManager defaultManager] createDirectoryAtPath:tempDir attributes:nil];
	if (!success)
	{
		[NSException raise:@"SUFailTmpWrite" format:@"Couldn't create temporary directory in /tmp"];
		[download cancel];
		[download release];
	}
	
	downloadPath = [[tempDir stringByAppendingPathComponent:name] retain];
	[download setDestination:downloadPath allowOverwrite:YES];
}

- (void)download:(NSURLDownload *)download didReceiveDataOfLength:(unsigned)length
{
	[statusController setProgressValue:[statusController progressValue] + length];
	[statusController setStatusText:[NSString stringWithFormat:SULocalizedString(@"%.0lfk of %.0lfk", nil), [statusController progressValue] / 1024.0, [statusController maxProgressValue] / 1024.0]];
}

- (void)download:(NSURLDownload *)download didFailWithError:(NSError *)error
{
	
	NSLog(@"Download error: %@", [error localizedDescription]);
	[self showUpdateErrorAlertWithInfo:SULocalizedString(@"An error occurred while trying to download the file. Please try again later.", nil)];
	[[NSApplication sharedApplication] terminate:self];
}

- (void)downloadDidFinish:(NSURLDownload *)download
{
	[download release];
	downloader = nil;
	NSLog(@"EXTRACT!");
	NSBeep();
#warning we should actually extract and not do this bogosity.
}	

- (BOOL)showsReleaseNotes
{
	return YES;
}


@end
