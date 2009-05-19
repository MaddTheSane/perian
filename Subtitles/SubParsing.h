/*
 * SubParsing.h
 * Created by Alexander Strange on 7/25/07.
 *
 * This file is part of Perian.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
	BOOL positioned;
	
	unsigned layer;
}
-(SubRenderDiv*)nextDivWithDelegate:(SubRenderer*)delegate;
@end

@interface SubRenderSpan : NSObject {
	@public;
	UniCharArrayOffset offset;
	id ex;
	SubRenderer *delegate;
}
+(SubRenderSpan*)startingSpanForDiv:(SubRenderDiv*)div delegate:(SubRenderer*)delegate;
-(SubRenderSpan*)cloneWithDelegate:(SubRenderer*)delegate;
@end

extern void SubParseSSAFile(NSString *ssa, NSDictionary **headers, NSArray **styles, NSArray **subs);
extern NSArray *SubParsePacket(NSString *packet, SubContext *context, SubRenderer *delegate);

#ifdef __cplusplus
}
#endif