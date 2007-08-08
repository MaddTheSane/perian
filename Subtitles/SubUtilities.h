//
//  Categories.h
//  SSAView
//
//  Created by Alexander Strange on 1/18/07.
//  Copyright 2007 Perian Project. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
extern "C" {
#endif
	
extern NSCharacterSet *STWhitespaceAndBomCharacterSet();

extern NSString *STStringByStandardizingNewlines(NSString *st);
extern NSArray *STPairSeparatedByString(NSString *st, NSString *split);
extern NSArray *STSplitStringWithCount(NSString *st, NSString *split, int count);
extern NSString *STLoadFileWithUnknownEncoding(NSString *path);

#ifdef __cplusplus
}
#endif
