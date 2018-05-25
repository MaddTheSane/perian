/*
 * UpdateCheckerAppDelegate.m
 * Created by Augie Fackler on 1/6/07.
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

// This is really just a heavily-customized version of SUUpdate designed
// so that the updates are done using an app and not a framework. We do
// this so that things like little snitch don't give us problems, and so
// we can be more sure that the update won't get run twice accidentaly.

#import "UpdateCheckerAppDelegate.h"
#include <stdlib.h>

//define the following to use the beta appcast URL, but DON'T commit that change
//#define betaAppcastUrl @"whatever"

@interface UpdateCheckerAppDelegate ()
- (void)showUpdateErrorAlertWithInfo:(NSString *)info;
@property (strong) NSDate *lastRunDate;
@property (strong) SUAppcastItem *latest;
@property (strong) NSString *downloadPath;
@property (strong) SUAppcast *appcast;
@end

@implementation UpdateCheckerAppDelegate
{
	SUUpdateAlert *updateAlert;
	SUAppcastItem *latest;
	SUStatusController *statusController;
	SUAppcast *appcast;
	NSURLDownload *downloader;
	NSString *downloadPath;
	NSDate *lastRunDate;
	BOOL	manualRun;
}
@synthesize lastRunDate;
@synthesize latest;
@synthesize downloadPath;
@synthesize appcast;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	self.lastRunDate = [defaults objectForKey:NEXT_RUN_KEY];
	
	if (![lastRunDate isEqualToDate:[NSDate distantFuture]]) {
		[defaults setObject:[NSDate dateWithTimeIntervalSinceNow:TIME_INTERVAL_TIL_NEXT_RUN] forKey:NEXT_RUN_KEY];
	}
	
	manualRun = [defaults boolForKey:MANUAL_RUN_KEY];
	[defaults removeObjectForKey:MANUAL_RUN_KEY];
	[defaults synchronize];
	[self doUpdateCheck];
}

- (void)doUpdateCheck
{
	NSString *updateUrlString = SUInfoValueForKey(UPDATE_URL_KEY);
	
	if (!updateUrlString) { [NSException raise:@"NoFeedURL" format:@"No feed URL is specified in the Info.plist!"]; }
	
#ifdef betaAppcastUrl
	updateUrlString = [[updateUrlString substringToIndex:[updateUrlString length]-4] stringByAppendingFormat:@"-%@.xml", betaAppcastUrl];
#endif
	if(manualRun)
		[[NSDistributedNotificationCenter defaultCenter] postNotificationName:UPDATE_STATUS_NOTIFICATION object:@"Starting"];
	
	self.appcast = [[SUAppcast alloc] init];
	[appcast setDelegate:self];
	[appcast fetchAppcastFromURL:[NSURL URLWithString:updateUrlString]];
}

- (void)appcastDidFinishLoading:(SUAppcast *)anappcast
{
	self.latest = [anappcast items][0];
	
	if (![latest versionString])
	{
		[self updateFailed];
		[NSException raise:@"SUAppcastException" format:@"Can't extract a version string from the appcast feed. The filenames should look like YourApp_1.5.tgz, where 1.5 is the version number."];
	}
	
	// OS version (Apple recommends using SystemVersion.plist instead of Gestalt() here, don't ask me why).
	// This code *should* use NSSearchPathForDirectoriesInDomains(NSCoreServiceDirectory, NSSystemDomainMask, YES)
	// but that returns /Library/CoreServices for some reason
	NSString *versionPlistPath = @"/System/Library/CoreServices/SystemVersion.plist";
	NSString *currentSystemVersion = [NSDictionary dictionaryWithContentsOfFile:versionPlistPath][@"ProductVersion"];
	
	BOOL updateAvailable = SUStandardVersionComparison(latest.minimumSystemVersion, currentSystemVersion);
	NSString *panePath = [[[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
	updateAvailable = (updateAvailable && (SUStandardVersionComparison([latest versionString], [[NSBundle bundleWithPath:panePath] objectForInfoDictionaryKey:@"CFBundleVersion"]) == NSOrderedAscending));
	
	if (![[panePath lastPathComponent] isEqualToString:@"Perian.prefPane"]) {
		NSLog(@"The update checker needs to be run from inside the preference pane, quitting...");
		updateAvailable = NO;
	}
	
	NSString *skippedVersion = [[NSUserDefaults standardUserDefaults] objectForKey:SKIPPED_VERSION_KEY];
	
	if (updateAvailable && (!skippedVersion ||
							(skippedVersion && ![skippedVersion isEqualToString:[latest versionString]]))) {
		if(manualRun)
			[[NSDistributedNotificationCenter defaultCenter] postNotificationName:UPDATE_STATUS_NOTIFICATION object:@"YesUpdates"];
		[self showUpdatePanelForItem:latest];
	} else {
		if(manualRun)
			[[NSDistributedNotificationCenter defaultCenter] postNotificationName:UPDATE_STATUS_NOTIFICATION object:@"NoUpdates"];
		[[NSApplication sharedApplication] terminate:self];
	}
	
	//RELEASEOBJ(appcast);
	self.appcast = nil;
}

- (void)appcastDidFailToLoad:(SUAppcast *)anappcast
{
	[self updateFailed];
	if(manualRun)
		[[NSDistributedNotificationCenter defaultCenter] postNotificationName:UPDATE_STATUS_NOTIFICATION object:@"Error"];
	//RELEASEOBJ(anappcast);
	self.appcast = nil;
	[[NSApplication sharedApplication] terminate:self];
}

- (void)appcast:(SUAppcast *)aappcast failedToLoadWithError:(NSError *)error
{
	[self appcastDidFailToLoad:aappcast];
}

- (void)showUpdatePanelForItem:(SUAppcastItem *)updateItem
{
	updateAlert = [[SUUpdateAlert alloc] initWithAppcastItem:updateItem];
	updateAlert.delegate = self;
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
	NSRunAlertPanel(SULocalizedString(@"Update Error!", nil), @"%@", SULocalizedString(@"Cancel", nil), nil, nil, info);
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
#if 0
	NSString *tempDir = [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSProcessInfo processInfo] globallyUniqueString]];
	BOOL success = [[NSFileManager defaultManager] createDirectoryAtPath:tempDir withIntermediateDirectories:YES attributes:nil error:NULL];
#else
	NSURL *tempURL = [[[NSFileManager defaultManager] URLForDirectory:NSItemReplacementDirectory inDomain:NSUserDomainMask appropriateForURL:[NSURL fileURLWithPath:@"/tmp"] create:YES error:nil] URLByAppendingPathComponent:[[NSProcessInfo processInfo] globallyUniqueString]];
	NSString *tempDir = [tempURL path];
	BOOL success = [[NSFileManager defaultManager] createDirectoryAtPath:tempDir withIntermediateDirectories:YES attributes:nil error:NULL];
#endif
	
	if (!success)
	{
		[NSException raise:@"SUFailTmpWrite" format:@"Couldn't create temporary directory in /tmp"];
		[download cancel];
	}
	
	self.downloadPath = [tempDir stringByAppendingPathComponent:name];
	[download setDestination:downloadPath allowOverwrite:YES];
}

- (void)download:(NSURLDownload *)download didReceiveDataOfLength:(NSUInteger)length
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
	NSTask *hdiTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/env" arguments:@[@"hdiutil", @"internet-enable", @"-quiet", archivePath]];
	[hdiTask waitUntilExit];
	if ([hdiTask terminationStatus] != 0) { return NO; }
	
	// Now, open the volume; it'll extract into its own directory.
	hdiTask = [NSTask launchedTaskWithLaunchPath:@"/usr/bin/env" arguments:@[@"hdiutil", @"attach", @"-idme", @"-noidmereveal", @"-noidmetrash", @"-noverify", @"-nobrowse", @"-noautoopen", @"-quiet", archivePath]];
	[hdiTask waitUntilExit];
	if ([hdiTask terminationStatus] != 0) { return NO; }
	
	return YES;
}

extern char **environ;

- (void)downloadDidFinish:(NSURLDownload *)download
{
	downloader = nil;
	
	//Indeterminate progress bar
	[statusController setMaxProgressValue:0];
	[statusController setStatusText:SULocalizedString(@"Extracting...", nil)];
	
	if(![self extractDMG:downloadPath])
	{
		[self updateFailed];
		[self showUpdateErrorAlertWithInfo:NSLocalizedString(@"Could not Extract Downloaded File",@"")];
	}
	
	NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:[downloadPath stringByDeletingLastPathComponent]];
	NSString *file = nil;
	NSString *prefpanelocation = nil;
	NSError *moveErr = nil;
	while((file = [dirEnum nextObject]) != nil)
	{
		if([[dirEnum fileAttributes][NSFileTypeSymbolicLink] boolValue])
			[dirEnum skipDescendents];
		if([[file pathExtension] isEqualToString:@"prefPane"])
		{
			NSString *containingLocation = [downloadPath stringByDeletingLastPathComponent];
			NSString *oldLocation = [containingLocation stringByAppendingPathComponent:file];
			prefpanelocation = [[containingLocation stringByDeletingLastPathComponent] stringByAppendingPathComponent:[file lastPathComponent]];
			[[NSFileManager defaultManager] moveItemAtPath:oldLocation toPath:prefpanelocation error:&moveErr];
		}
	}
	
	const char *buf = "open \"$PREFPANE_LOCATION\"; rm -rf \"$TEMP_FOLDER\"";
	if(!buf || moveErr)
	{
		[self updateFailed];
		[self showUpdateErrorAlertWithInfo:NSLocalizedString(@"Could not Create Extraction Script",@"")];
	}
	
	NSAppleScript *quitSysPrefsScript = [[NSAppleScript alloc] initWithSource:@"tell application \"System Preferences\" to quit"];
	[quitSysPrefsScript executeAndReturnError:nil];
	
	const char * args[] = {"/bin/sh", "-c", buf, NULL};
	setenv("PREFPANE_LOCATION", [prefpanelocation fileSystemRepresentation], 1);
	setenv("TEMP_FOLDER", [[downloadPath stringByDeletingLastPathComponent] fileSystemRepresentation], 1);
	int forkVal = fork();
	if(forkVal == -1)
	{
		[self updateFailed];
		[self showUpdateErrorAlertWithInfo:NSLocalizedString(@"Could not Run Update",@"")];
	}
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
