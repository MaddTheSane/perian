/*
 * UpdateCheckerAppDelegate.h
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
#define UPDATE_STATUS_NOTIFICATION @"org.perian.UpdateCheckStatus"
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
