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

@interface SUAppcast : NSObject
@property (readonly) NSArray *items;
@property (weak) NSObject<SUAppcastDelegate> *delegate;

- (void)fetchAppcastFromURL:(NSURL *)url;

- (SUAppcastItem *)newestItem;

@end

@protocol SUAppcastDelegate <NSObject>
- (void)appcastDidFinishLoading:(SUAppcast *)appcast;
- (void)appcastDidFailToLoad:(SUAppcast *)appcast;
@end
