//
//  Categories.h
//  SSAView
//
//  Created by Alexander Strange on 1/18/07.
//  Copyright 2007 Perian Project. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface NSCharacterSet (STAdditions)
+ (NSCharacterSet *)newlineCharacterSet;
@end

@interface NSScanner (STAdditions)
- (int)scanInt;
@end

@interface NSString (STAdditions)
- (NSString *)stringByStandardizingNewlines;
- (NSArray *)pairSeparatedByString:(NSString *)str;
- (NSArray *)componentsSeparatedByString:(NSString *)str count:(int)count;
+ (NSString *)stringFromUnknownEncodingFile:(NSString *)file;
@end
