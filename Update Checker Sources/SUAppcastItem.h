//
//  SUAppcastItem.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface SUAppcastItem : NSObject
@property (copy) NSString *title;
@property (copy) NSDate *date;
@property (copy) NSString *description;

@property (strong) NSURL *releaseNotesURL;

@property (copy) NSString *DSASignature;
@property (copy) NSString *MD5Sum;

@property (strong) NSURL *fileURL;
@property (copy) NSString *fileVersion;
@property (copy) NSString *versionString;

@property (copy) NSString *minimumSystemVersion;

// Initializes with data from a dictionary provided by the RSS class.
- (instancetype)initWithDictionary:(NSDictionary *)dict;
@end
