/*
 * CPFPerianPrefPaneController.h
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
#import <PreferencePanes/NSPreferencePane.h>
#import <Security/Security.h>
#import "CommonUtils.h"

#define ComponentInfoPlist @"ComponentInfo.plist"
#define ObsoletesKey @"obsoletes"
#define HumanReadableNameKey @"HumanReadableName"
#define PERIAN_NO_BUNDLE_ID_FORMAT @"org.perian.No.Bundle.ID.%@"

#define ComponentInfoDictionaryKey	@"Components"
#define AppsToRegisterDictionaryKey @"ApplicationsToRegister"

#define BundleVersionKey @"CFBundleVersion"
#define BundleIdentifierKey @"CFBundleIdentifier"

#define ComponentNameKey @"Name"
#define ComponentArchiveNameKey @"ArchiveName"
#define ComponentTypeKey @"Type"

typedef enum
{
	InstallStatusInstalledInWrongLocation = 0,
	InstallStatusNotInstalled = 1,
	InstallStatusOutdatedWithAnotherInWrongLocation = 2,
	InstallStatusOutdated = 3,
	InstallStatusInstalledInBothLocations = 4,
	InstallStatusInstalled = 5
} InstallStatus;

static inline InstallStatus currentInstallStatus(InstallStatus status)
{
	return (status | 1);
}

static inline BOOL isWrongLocationInstalled(InstallStatus status)
{
	return ((status & 1) == 0);
}

static inline InstallStatus setWrongLocationInstalled(InstallStatus status)
{
	return (status & ~1);
}

typedef enum
{
	ComponentTypeQuickTime,
	ComponentTypeCoreAudio,
	ComponentTypeFramework
} ComponentType;

PERIAN_EXPORTED
@interface CPFPerianPrefPaneController : NSPreferencePane
{
	//General Pane
	IBOutlet NSButton					*button_install;
	IBOutlet NSTextField				*textField_installStatus;
	
	IBOutlet NSTextField				*textField_currentVersion;
	IBOutlet NSTextField				*textField_statusMessage;
	IBOutlet NSButton					*button_updateCheck;
	IBOutlet NSButton					*button_autoUpdateCheck;
	
	//AC3 Settings in General Pane
	IBOutlet NSPopUpButton				*popup_ac3DynamicRangeType;
	IBOutlet NSPopUpButton				*popup_outputMode;
	
	IBOutlet NSWindow					*window_dynRangeSheet;
    IBOutlet NSTextField                *textField_ac3DynamicRangeValue;
    IBOutlet NSSlider                   *slider_ac3DynamicRangeSlider;
	
	IBOutlet NSButton					*button_loadExternalSubtitles;
	
	IBOutlet NSWindow					*window_multiChannelSheet;
	IBOutlet NSTextField				*textField_multiChannelText;
	IBOutlet NSButton					*button_multiChannelNeverShow;
	
	//About
	IBOutlet NSTextView					*textView_about;
	IBOutlet NSButton					*button_website;
	IBOutlet NSButton					*button_donate;
	IBOutlet NSButton					*button_forum;
	
	InstallStatus						installStatus; //This is only marked as installed if everything is installed
	BOOL								userInstalled;
	AuthorizationRef					auth;
	NSString							*errorString;
	
	NSArray								*componentReplacementInfo;
	
	NSURL								*perianForumURL;
	NSURL								*perianDonateURL;
	NSURL								*perianWebSiteURL;
	
	CFStringRef							perianAppID;
	CFStringRef							a52AppID;
	
	float								nextDynValue;
}
//General Pane
- (IBAction)installUninstall:(id)sender;
- (IBAction)updateCheck:(id)sender;
- (IBAction)setAutoUpdateCheck:(id)sender;
- (IBAction)setLoadExternalSubtitles:(id)sender;

//AC3 Settings
- (IBAction)setAC3DynamicRangePopup:(id)sender;
- (IBAction)set2ChannelModePopup:(id)sender;

- (IBAction)setAC3DynamicRangeValue:(id)sender;
- (IBAction)setAC3DynamicRangeSlider:(id)sender;
- (IBAction)cancelDynRangeSheet:(id)sender;
- (IBAction)saveDynRangeSheet:(id)sender;

- (IBAction)dismissMultiChannelSheet:(id)sender;

//Component List
- (NSString *)checkComponentStatusByBundleIdentifier:(NSString *)bundleID;

//About
- (IBAction)launchWebsite:(id)sender;
- (IBAction)launchDonate:(id)sender;
- (IBAction)launchForum:(id)sender;

@end