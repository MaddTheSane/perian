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
#include <stdlib.h>

@interface UpdateCheckerAppDelegate (private)
- (void)showUpdateErrorAlertWithInfo:(NSString *)info;
@end

@implementation UpdateCheckerAppDelegate

- (void)dealloc
{
    [downloader release];
    [downloadPath release];
    [lastRunDate release];
    [super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	lastRunDate = [[[NSUserDefaults standardUserDefaults] objectForKey:NEXT_RUN_KEY] retain];
	if (lastRunDate == nil || [lastRunDate laterDate:[NSDate date]] != lastRunDate) {
		//this means we should in fact run
		[[NSUserDefaults standardUserDefaults] setObject:[NSDate dateWithTimeIntervalSinceNow:TIME_INTERVAL_TIL_NEXT_RUN] forKey:NEXT_RUN_KEY];
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
        [self updateFailed];
		[NSException raise:@"SUAppcastException" format:@"Can't extract a version string from the appcast feed. The filenames should look like YourApp_1.5.tgz, where 1.5 is the version number."];
	}
	
	// OS version (Apple recommends using SystemVersion.plist instead of Gestalt() here, don't ask me why).
	// This code *should* use NSSearchPathForDirectoriesInDomains(NSCoreServiceDirectory, NSSystemDomainMask, YES)
	// but that returns /Library/CoreServices for some reason
	NSString *versionPlistPath = @"/System/Library/CoreServices/SystemVersion.plist";
	NSString *currentSystemVersion = [[[NSDictionary dictionaryWithContentsOfFile:versionPlistPath] objectForKey:@"ProductVersion"] retain];

	BOOL updateAvailable = SUStandardVersionComparison([latest minimumSystemVersion], currentSystemVersion);
    NSString *panePath = [[[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
	updateAvailable = (updateAvailable && (SUStandardVersionComparison([latest fileVersion], [[NSBundle bundleWithPath:panePath] objectForInfoDictionaryKey:@"CFBundleVersion"]) == NSOrderedAscending));
	NSString *skippedVersion = [[NSUserDefaults standardUserDefaults] objectForKey:SKIPPED_VERSION_KEY];
	if (updateAvailable && (!skippedVersion || 
		(skippedVersion && ![skippedVersion isEqualToString:[latest versionString]]))) {
		[self showUpdatePanelForItem:latest];
	} else //nothing to see here, so we move along
		[[NSApplication sharedApplication] terminate:self];
}

- (void)appcastDidFailToLoad:(SUAppcast *)appcast
{
	[self updateFailed];
	[self showUpdateErrorAlertWithInfo:SULocalizedString(@"An error occurred while trying to load Perian's version info. Please try again later.", nil)];
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
    [self updateFailed];
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
	[self updateFailed];
	NSLog(@"Download error: %@", [error localizedDescription]);
	[self showUpdateErrorAlertWithInfo:SULocalizedString(@"An error occurred while trying to download the newest version of Perian. Please try again later.", nil)];
	[[NSApplication sharedApplication] terminate:self];
}

//Stolen from sprakle
- (BOOL)extractDMG:(NSString *)archivePath
{
	// First, we internet-enable the volume.
	NSTask *hdiTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/env" arguments:[NSArray arrayWithObjects:@"hdiutil", @"internet-enable", @"-quiet", archivePath, nil]];
	[hdiTask waitUntilExit];
	if ([hdiTask terminationStatus] != 0) { return NO; }
	
	// Now, open the volume; it'll extract into its own directory.
	hdiTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/env" arguments:[NSArray arrayWithObjects:@"hdiutil", @"attach", @"-idme", @"-noidmereveal", @"-noidmetrash", @"-noverify", @"-nobrowse", @"-noautoopen", @"-quiet", archivePath, nil]];
	[hdiTask waitUntilExit];
	if ([hdiTask terminationStatus] != 0) { return NO; }
	
	return YES;
}

extern char **environ;

- (void)downloadDidFinish:(NSURLDownload *)download
{
	[download release];
	downloader = nil;
	
	//Indeterminate progress bar
	[statusController setMaxProgressValue:0];
	[statusController setStatusText:SULocalizedString(@"Extracting...", nil)];
	
	if(![self extractDMG:downloadPath])
		[self showUpdateErrorAlertWithInfo:NSLocalizedString(@"Could not Extract Downloaded File",@"")];
	
	NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:[downloadPath stringByDeletingLastPathComponent]];
	NSString *file = nil;
	NSString *prefpanelocation = nil;
	while((file = [dirEnum nextObject]) != nil)
	{
		if([[[dirEnum fileAttributes] objectForKey:NSFileTypeSymbolicLink] boolValue])
			[dirEnum skipDescendents];
		if([[file pathExtension] isEqualToString:@"prefPane"])
		{
			NSString *containingLocation = [downloadPath stringByDeletingLastPathComponent];
			NSString *oldLocation = [containingLocation stringByAppendingPathComponent:file];
			prefpanelocation = [[containingLocation stringByDeletingLastPathComponent] stringByAppendingPathComponent:[file lastPathComponent]];
			[[NSFileManager defaultManager] movePath:oldLocation toPath:prefpanelocation handler:nil];
		}
	}
	
	char *buf = NULL;
	asprintf(&buf, "open \"$PREFPANE_LOCATION\"; rm -rf \"$TEMP_FOLDER\"");
	if(!buf)
		[self showUpdateErrorAlertWithInfo:NSLocalizedString(@"Could not Create Extraction Script",@"")];
		
	char *args[] = {"/bin/sh", "-c", buf, NULL};
	setenv("PREFPANE_LOCATION", [prefpanelocation fileSystemRepresentation], 1);
	setenv("TEMP_FOLDER", [[downloadPath stringByDeletingLastPathComponent] fileSystemRepresentation], 1);
    int forkVal = fork();
    if(forkVal == -1)
        [self showUpdateErrorAlertWithInfo:NSLocalizedString(@"Could not Run Update",@"")];
	if(forkVal == 0)
		execve("/bin/sh", args, environ);
	[NSApp terminate:self];
	//And, we are out of here!!!
}	

- (BOOL)showsReleaseNotes
{
	return YES;
}

- (void)updateFailed
{
    if(lastRunDate == nil)
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:NEXT_RUN_KEY];
    else
        [[NSUserDefaults standardUserDefaults] setObject:lastRunDate forKey:NEXT_RUN_KEY];
}


@end
