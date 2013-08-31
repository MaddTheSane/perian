//
//  SUAppcast.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class RSS, SUAppcastItem;
@protocol SUAppcastDelegate;

@interface SUAppcast : NSObject {
	NSArray *items;
	NSObject<SUAppcastDelegate> *delegate;
}

- (void)fetchAppcastFromURL:(NSURL *)url;
@property (assign) NSObject<SUAppcastDelegate> *delegate;

- (SUAppcastItem *)newestItem;
- (NSArray *)items;

@end

@protocol SUAppcastDelegate <NSObject>
@optional
- (void)appcastDidFinishLoading:(SUAppcast *)appcast;
- (void)appcastDidFailToLoad:(SUAppcast *)appcast;
@end
