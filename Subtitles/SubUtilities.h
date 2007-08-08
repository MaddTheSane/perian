//
//  SubUtilities.h
//  SSARender2
//
//  Created by Alexander Strange on 7/28/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
extern "C"
{
#endif
	
extern NSArray *STSplitStringIgnoringWhitespace(NSString *str, NSString *split);
extern NSArray *STSplitStringWithCount(NSString *str, NSString *split, size_t count);
extern NSMutableString *STStandardizeStringNewlines(NSString *str);
extern NSString *STLoadFileWithUnknownEncoding(NSString *path);

#ifdef __cplusplus
}
#endif