//
//  UpdateCheckerAppDelegate.h
//  Perian
//
//  Created by Augie Fackler on 1/6/07.
//  Copyright 2007 Augie Fackler. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SUAppcast.h"
#import "SUAppcastItem.h"
#import "SUUtilities.h"
#import "SUUpdateAlert.h"
#import "SUStatusController.h"

#define NEXT_RUN_KEY @"NextRunDate"
#define MANUAL_RUN_KEY @"ManualUpdateCheck"
#define UPDATE_URL_KEY @"UpdateFeedURL"
#define SKIPPED_VERSION_KEY @"SkippedVersion"
#define TIME_INTERVAL_TIL_NEXT_RUN 7*24*60*60

@interface UpdateCheckerAppDelegate : NSObject {
	SUUpdateAlert *updateAlert;
	SUAppcastItem *latest;
	SUStatusController *statusController;
	NSURLDownload *downloader;
	NSString *downloadPath;
    NSDate *lastRunDate;
	BOOL	manualRun;
}

- (void)doUpdateCheck;
- (void)showUpdatePanelForItem:(SUAppcastItem *)updateItem;
- (void)beginDownload;
- (void)updateFailed;

@end
