//
//  SUStatusController.m
//  Sparkle
//
//  Created by Andy Matuschak on 3/14/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import "SUStatusController.h"
#import "SUUtilities.h"
#import "ARCBridge.h"

@interface SUStatusController ()
@property (copy) NSString *title, *buttonTitle;
@end

@implementation SUStatusController
@synthesize progressValue;
@synthesize maxProgressValue;
@synthesize statusText;
@synthesize title;
@synthesize buttonTitle;

- (id)init
{
	NSString *path = [[NSBundle bundleForClass:[self class]] pathForResource:@"SUStatus" ofType:@"nib"];
	if (!path) // slight hack to resolve issues with running in debug configurations
	{
		NSBundle *current = [NSBundle bundleForClass:[self class]];
		NSString *frameworkPath = [NSString pathWithComponents:[[[[NSBundle mainBundle] sharedFrameworksPath] pathComponents] arrayByAddingObjectsFromArray:@[[current bundleIdentifier], @"Sparkle.framework"]]];
		NSBundle *framework = [NSBundle bundleWithPath:frameworkPath];
		path = [framework pathForResource:@"SUStatus" ofType:@"nib"];
	}
	if (self = [super initWithWindowNibPath:path owner:self])
	{
		[self setShouldCascadeWindows:NO];
	}
	return self;
}

#if !__has_feature(objc_arc)
- (void)dealloc
{
	self.title = nil;
	self.statusText = nil;
	self.buttonTitle = nil;
	[super dealloc];
}
#endif

- (void)awakeFromNib
{
	[[self window] center];
	[[self window] setFrameAutosaveName:@"SUStatusFrame"];
}

- (NSString *)windowTitle
{
	return [NSString stringWithFormat:SULocalizedString(@"Updating %@", nil), SUHostAppDisplayName()];
}

- (NSImage *)applicationIcon
{
	return [NSApp applicationIconImage];
}

- (void)beginActionWithTitle:(NSString *)aTitle maxProgressValue:(double)aMaxProgressValue statusText:(NSString *)aStatusText
{
	self.title = aTitle;
	
	[self setMaxProgressValue:aMaxProgressValue];
	[self setStatusText:aStatusText];
}

- (void)setButtonTitle:(NSString *)aButtonTitle target:target action:(SEL)action isDefault:(BOOL)isDefault
{
	self.buttonTitle = aButtonTitle;
	
	[actionButton sizeToFit];
	// Except we're going to add 15 px for padding.
	[actionButton setFrameSize:NSMakeSize([actionButton frame].size.width + 15, [actionButton frame].size.height)];
	// Now we have to move it over so that it's always 15px from the side of the window.
	[actionButton setFrameOrigin:NSMakePoint([[self window] frame].size.width - 15 - [actionButton frame].size.width, [actionButton frame].origin.y)];	
	// Redisplay superview to clean up artifacts
	[[actionButton superview] display];
	
	[actionButton setTarget:target];
	[actionButton setAction:action];
	[actionButton setKeyEquivalent:isDefault ? @"\r" : @""];
}

- (void)setButtonEnabled:(BOOL)enabled
{
	[actionButton setEnabled:enabled];
}

- (BOOL)isButtonEnabled
{
	return [actionButton isEnabled];
}

- (void)setMaxProgressValue:(double)value
{
	[self willChangeValueForKey:@"maxProgressValue"];
	maxProgressValue = value;
	[self didChangeValueForKey:@"maxProgressValue"];
	self.progressValue = 0;
}

@end
