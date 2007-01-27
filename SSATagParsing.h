//
//  SSATagParsing.h
//  SSAView
//
//  Created by Alexander Strange on 1/21/07.
//  Copyright 2007 Perian Project. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SSADocument.h"

@interface SSAStyleSpan : NSObject
{
	@public
	float outline, shadow;
	ATSUStyle astyle;
	NSRange range;
	BOOL outlineblur;
	ssacolors color;
}
@end

@interface SSARenderEntity : NSObject
{
	@public
	ATSUTextLayout layout;
	int layer;
	ssastyleline *style;
	int marginl, marginr, marginv, usablewidth;
	int posx, posy, halign, valign;
	NSString *nstext;
	unichar *text;
	SSAStyleSpan **styles;
	size_t style_count;
	BOOL disposelayout, multipleruns;
}
@end

extern NSArray *ParseSubPacket(NSString *str, SSADocument *ssa);