//
//  SUAppcast.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "ARCBridge.h"

@class RSS, SUAppcastItem;
@protocol SUAppcastDelegate;

@interface SUAppcast : NSObject {
	NSArray *items;
	__arcweak NSObject<SUAppcastDelegate> *delegate;
}

- (void)fetchAppcastFromURL:(NSURL *)url;
@property (readonly) NSArray *items;
@property (arcweak) NSObject<SUAppcastDelegate> *delegate;

- (SUAppcastItem *)newestItem;

@end

@protocol SUAppcastDelegate <NSObject>
@optional
- (void)appcastDidFinishLoading:(SUAppcast *)appcast;
- (void)appcastDidFailToLoad:(SUAppcast *)appcast;
@end
