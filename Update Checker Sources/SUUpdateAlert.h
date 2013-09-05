//
//  SUUpdateAlert.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ARCBridge.h"

typedef enum
{
	SUInstallUpdateChoice,
	SURemindMeLaterChoice,
	SUSkipThisVersionChoice
} SUUpdateAlertChoice;

@class WebView, SUAppcastItem;
@protocol SUUpdateAlertDelegate;

@interface SUUpdateAlert : NSWindowController {
	SUAppcastItem *updateItem;
	__arcweak NSObject<SUUpdateAlertDelegate> *delegate;
	
	IBOutlet WebView *releaseNotesView;
	IBOutlet NSTextField *description;
	NSProgressIndicator *releaseNotesSpinner;
	BOOL webViewFinishedLoading;
}

@property (arcweak) NSObject<SUUpdateAlertDelegate> *delegate;

- (id)initWithAppcastItem:(SUAppcastItem *)item;

- (IBAction)installUpdate:sender;
- (IBAction)skipThisVersion:sender;
- (IBAction)remindMeLater:sender;

@end

@protocol SUUpdateAlertDelegate <NSObject>
@optional
- (void)updateAlert:(SUUpdateAlert *)updateAlert finishedWithChoice:(SUUpdateAlertChoice)updateChoice;
@end
