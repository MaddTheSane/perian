/*
 *  SubParsing.h
 *  SSARender2
 *
 *  Created by Alexander Strange on 7/25/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import "SubContext.h"

#ifdef __cplusplus
extern "C"
{
#endif

@class SubSerializer, SubRenderer;

@interface SubRenderDiv : NSObject {
	@public;
	NSMutableString *text;
	SubStyle *styleLine;
	unsigned marginL, marginR, marginV;
	NSMutableArray *spans;
	
	int posX, posY;
	UInt8 alignH, alignV, wrapStyle, render_complexity;
	BOOL is_shape;
	
	unsigned layer;
}
-(SubRenderDiv*)nextDivWithDelegate:(SubRenderer*)delegate;
@end

@interface SubRenderSpan : NSObject {
	@public;
	UniCharArrayOffset offset;
	__strong void *ex;
	SubRenderer *delegate;
}
+(SubRenderSpan*)startingSpanForDiv:(SubRenderDiv*)div delegate:(SubRenderer*)delegate;
-(SubRenderSpan*)cloneWithDelegate:(SubRenderer*)delegate;
@end

extern void SubParseSSAFile(const unichar *ssa, size_t len, NSDictionary **headers, NSArray **styles, NSArray **subs);
extern NSArray *SubParsePacket(NSString *packet, SubContext *context, SubRenderer *delegate, unichar *linebuf);

#ifdef __cplusplus
}
#endif