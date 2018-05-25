//
//  SUUpdateAlert.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <WebKit/WebView.h>
#import <WebKit/WebPolicyDelegate.h>
#import <WebKit/WebFrameLoadDelegate.h>

typedef NS_ENUM(int, SUUpdateAlertChoice) {
	SUInstallUpdateChoice,
	SURemindMeLaterChoice,
	SUSkipThisVersionChoice
};

@class SUAppcastItem;
@protocol SUUpdateAlertDelegate;

@interface SUUpdateAlert : NSWindowController<WebPolicyDelegate, WebFrameLoadDelegate> {
	SUAppcastItem *updateItem;
	
	NSProgressIndicator *releaseNotesSpinner;
	BOOL webViewFinishedLoading;
}
@property (weak) NSObject<SUUpdateAlertDelegate> *delegate;
@property (weak) IBOutlet WebView *releaseNotesView;
@property (weak) IBOutlet NSTextField *itemDescription;

- (instancetype)initWithAppcastItem:(SUAppcastItem *)item;

- (IBAction)installUpdate:(id)sender;
- (IBAction)skipThisVersion:(id)sender;
- (IBAction)remindMeLater:(id)sender;

@end

@protocol SUUpdateAlertDelegate <NSObject>
- (void)updateAlert:(SUUpdateAlert *)updateAlert finishedWithChoice:(SUUpdateAlertChoice)updateChoice;
@end
