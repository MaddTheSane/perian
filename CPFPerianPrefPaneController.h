/* CPFPerianPrefPaneController */

#import <Cocoa/Cocoa.h>
#import <PreferencePanes/NSPreferencePane.h>
#import <Security/Security.h>

#define ComponentInfoDictionaryKey	@"Components"
#define BundleVersionKey @"CFBundleVersion"
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

InstallStatus inline currentInstallStatus(InstallStatus status)
{
	return (status | 1);
}

BOOL inline isWrongLocationInstalled(InstallStatus status)
{
	return ((status & 1) == 0);
}

InstallStatus inline setWrongLocationInstalled(InstallStatus status)
{
	return (status & ~1);
}

typedef enum
{
	ComponentTypeQuickTime,
	ComponentTypeCoreAudio,
	ComponentTypeFramework
} ComponentType;

@interface CPFPerianPrefPaneController : NSPreferencePane
{
	//General Pane
	IBOutlet NSButton					*button_install;
	IBOutlet NSTextField				*textField_installStatus;
	IBOutlet NSProgressIndicator		*progress_install;
	
	IBOutlet NSTextField				*textField_currentVersion;
	IBOutlet NSButton					*button_updateCheck;
	IBOutlet NSProgressIndicator		*progress_updateCheck;
	IBOutlet NSButton					*button_autoUpdateCheck;
	
	//AC3 Settings in General Pane
	IBOutlet NSPopUpButton				*popup_ac3DynamicRangeType;
	IBOutlet NSPopUpButton				*popup_2ChannelMode;
	
	IBOutlet NSWindow					*window_dynRangeSheet;
    IBOutlet NSTextField                *textField_ac3DynamicRangeValue;
    IBOutlet NSSlider                   *slider_ac3DynamicRangeSlider;
	
	//About
	IBOutlet NSTextView					*textView_about;
	IBOutlet NSButton					*button_website;
	IBOutlet NSButton					*button_donate;
	IBOutlet NSButton					*button_forum;
	
	InstallStatus						installStatus; //This is only marked as installed if everything is installed
	BOOL								userInstalled;
	AuthorizationRef					auth;
	NSMutableString						*errorString;
	
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

//AC3 Settings
- (IBAction)setAC3DynamicRangePopup:(id)sender;
- (IBAction)set2ChannelModePopup:(id)sender;

- (IBAction)setAC3DynamicRangeValue:(id)sender;
- (IBAction)setAC3DynamicRangeSlider:(id)sender;
- (IBAction)cancelDynRangeSheet:(id)sender;
- (IBAction)saveDynRangeSheet:(id)sender;

//About
- (IBAction)launchWebsite:(id)sender;
- (IBAction)launchDonate:(id)sender;
- (IBAction)launchForum:(id)sender;

@end