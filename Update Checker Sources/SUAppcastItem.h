//
//  SUAppcastItem.h
//  Sparkle
//
//  Created by Andy Matuschak on 3/12/06.
//  Copyright 2006 Andy Matuschak. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface SUAppcastItem : NSObject {
	NSString *title;
	NSDate *date;
	NSString *description;
	
	NSURL *releaseNotesURL;
	
	NSString *DSASignature;
	NSString *MD5Sum;
	
	NSString *minimumSystemVersion;
	
	NSURL *fileURL;
	NSString *fileVersion;
	NSString *versionString;
}

// Initializes with data from a dictionary provided by the RSS class.
- (id)initWithDictionary:(NSDictionary *)dict;

@property (copy) NSString *title;
@property (copy) NSDate *date;
@property (copy) NSString *description;

@property (retain) NSURL *releaseNotesURL;

@property (copy) NSString *DSASignature;
@property (copy) NSString *MD5Sum;

@property (retain) NSURL *fileURL;
@property (copy) NSString *fileVersion;
@property (copy) NSString *versionString;

@property (copy) NSString *minimumSystemVersion;

@end
